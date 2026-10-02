#pragma once
#include <cstdint>
#include <string>
#include <cmath>

// ABI prefixes from xNVSE PluginAPI.h, tag 6.3.11. No executable patching.
struct BrowserPlayerControls {
 void (__fastcall* disable)(uint32_t,const char*);
 void (__fastcall* enable)(uint32_t,const char*);
 bool (__cdecl* disabled)(uint8_t,uint32_t,const char*);
};
struct BrowserScriptResult {double number=0;uint8_t type=0;};
struct BrowserScriptAPI {
 bool (*call)(void*,void*,void*,BrowserScriptResult*,uint8_t,...);
 void* reserved[3]; // GetFunctionParams, ExtractArgsEx, ExtractFormatStringArgs
 bool (*callAlt)(void*,void*,uint8_t,...);
 void* (*compile)(const char*);
 void* (*compileExpression)(const char*);
};
namespace controls {
constexpr uint32_t GameplayMask=0xAFDF; // movement/look/combat/POV/sneak/VATS/jump/run/wait/reload
constexpr uint32_t ButtonMask=0xF3FF; // defined gamepad buttons, excluding Guide/reserved bits
constexpr uint32_t InputMask=ButtonMask|0x30000; // triggers in bits 16/17
struct InputAPI {
 virtual bool capture(uint32_t& disabled)=0;
 virtual bool change(uint32_t mask,bool disable)=0;
 virtual ~InputAPI()=default;
};
class ScriptInputAPI:public InputAPI {
 struct Entry {uint32_t bit=0;void* read=nullptr;void* disable=nullptr;void* enable=nullptr;};
 BrowserScriptAPI* api=nullptr;Entry entries[16]{};std::string error;
public:
 const std::string& lastError()const{return error;}
 bool initialize(BrowserScriptAPI* value){
  api=value;error.clear();
  if(!api||!api->call||!api->compile||!api->compileExpression){error="xNVSE script interface unavailable";return false;}
  // The native API is a partial-script compiler. Use short query expressions
  // and parameterless constant commands; no local declarations or parameter binding.
  // All source is internal and bounded; no user strings are compiled.
  unsigned n=0;
  for(unsigned bit=1;bit<=0x20000;bit<<=1)if(bit&InputMask){
   auto& e=entries[n++];e.bit=bit;
   std::string type=bit<=32768?"Button":"Trigger";
   std::string code=std::to_string(bit<=32768?bit:(bit==65536?0:1));
   std::string query="Is"+type+"Disabled "+code;
   if(!e.read)e.read=api->compileExpression(query.c_str());
   if(!e.read){error="Cannot compile query: "+query;return false;}
   for(int action=0;action<2;++action){
    auto& script=action?e.enable:e.disable;
    std::string command=std::string(action?"Enable":"Disable")+type+" "+code;
    std::string source="begin Function {}\n"+command+"\nSetFunctionValue 1\nend\n";
    if(!script)script=api->compile(source.c_str());
    if(!script){error="Cannot compile command: "+command;return false;}
   }
  }
  return true;
 }
 bool capture(uint32_t& mask) override {
  mask=0;
  for(auto& e:entries){
   BrowserScriptResult r{};
   if(!e.read||!api->call(e.read,nullptr,nullptr,&r,0)||r.type!=1||(r.number!=0&&r.number!=1))return false;
   if(r.number==1)mask|=e.bit;
  }
  return true;
 }
 bool change(uint32_t mask,bool disable) override {
  if(mask&~InputMask)return false;
  for(auto& e:entries)if(e.bit&mask){
   BrowserScriptResult r{};auto script=disable?e.disable:e.enable;
   if(!script||!api->call(script,nullptr,nullptr,&r,0)||r.type!=1||r.number!=1)return false;
  }
  return true;
 }
};
class Lease {
 BrowserPlayerControls* api=nullptr;InputAPI* input=nullptr;const char* owner=nullptr;
 uint32_t acquired=0;bool held=false;
public:
 void configure(BrowserPlayerControls* value,InputAPI* buttons,const char* name){api=value;input=buttons;owner=name;}
 bool active()const{return held;}
 bool acquire(){
  if(held)return true;
  if(!api||!api->disable||!api->enable||!input||!owner)return false;
  uint32_t existing=0;if(!input->capture(existing))return false;
  acquired=InputMask&~existing;
  held=true; // Retain ownership until rollback succeeds, including partial failures.
  api->disable(GameplayMask,owner);
  if(input->change(acquired,true))return true;
  release();return false;
 }
 bool release(){
  if(!held)return true;
  // JIP's flags have no per-mod owners. Restore only flags we changed, never
  // flags already disabled at acquisition. Keep the browser gate until restored.
  if(!input->change(acquired,false))return false;
  api->enable(GameplayMask,owner);acquired=0;held=false;return true;
 }
};
}

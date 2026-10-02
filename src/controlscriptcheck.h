#pragma once
#include <deque>
namespace scriptcheck {
static std::deque<std::string> programs;
static uint32_t buttons=0;
static bool failCompile=false;
static void* compile(const char* text){
 // Exercise the real reentrant callback boundary used by xNVSE compilation.
 BrowserNVSEMessage event{"NVSE",21,0,nullptr};browserNVSEMessage(&event);
 if(failCompile)return nullptr;
 programs.emplace_back(text);return &programs.back();
}
static bool call(void* script,void*,void*,BrowserScriptResult* result,uint8_t count,...){
 if(count||!script||!result)return false;
 std::string text=*(std::string*)script;
 bool query=text.rfind("Is",0)==0;
 if(!query){auto start=text.find('\n');auto end=text.find('\n',start+1);if(start==std::string::npos||end==std::string::npos)return false;text=text.substr(start+1,end-start-1);}
 auto space=text.find(' ');if(space==std::string::npos)return false;
 auto name=text.substr(0,space);unsigned code=(unsigned)std::stoul(text.substr(space+1));
 bool trigger=name.find("Trigger")!=std::string::npos;
 if(trigger&&code>1)return false;
 uint32_t bit=trigger?(1u<<(16+code)):code;
 if(!(bit&controls::InputMask)||(bit&(bit-1)))return false;
 result->type=1;result->number=1;
 if(query)result->number=(buttons&bit)?1:0;
 else if(name.rfind("Disable",0)==0)buttons|=bit;
 else if(name.rfind("Enable",0)==0)buttons&=~bit;
 else return false;
 return true;
}
static int run(){
 BrowserScriptAPI api{};api.call=call;api.compile=compile;api.compileExpression=compile;
 controls::ScriptInputAPI input;
 {std::unique_lock<std::mutex> lock(catalogMutex);if(!input.initialize(&api))return 60;}
 if(programs.size()!=48||!input.initialize(&api)||programs.size()!=48)return 61;
 uint32_t read=0;buttons=0x10001;
 if(!input.capture(read)||read!=buttons)return 62;
 if(!input.change(0x20010,true)||buttons!=0x30011)return 63;
 if(!input.change(0x20010,false)||buttons!=0x10001)return 64;
 if(input.change(0x400000,true))return 65;
 controls::ScriptInputAPI retry;failCompile=true;
 if(retry.initialize(&api)||retry.lastError().find("IsButtonDisabled 1")==std::string::npos)return 66;
 failCompile=false;if(!retry.initialize(&api))return 67;
 for(const auto& text:programs)if(text.find("int ")!=std::string::npos||text.find("Function {mask}")!=std::string::npos)return 68;
 return 0;
}
}

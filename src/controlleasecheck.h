#pragma once
static std::map<std::string,uint32_t> restrictions;
static void __fastcall fakeDisable(uint32_t flags,const char* name){restrictions[name]|=flags;}
static void __fastcall fakeEnable(uint32_t flags,const char* name){restrictions[name]&=~flags;}
struct FakeButtons:controls::InputAPI {
 uint32_t disabled=0;bool captureOK=true,disableOK=true,enableOK=true;int writes=0;
 bool capture(uint32_t& value) override {value=disabled;return captureOK;}
 bool change(uint32_t value,bool disable) override {
  ++writes;if(disable){disabled|=value;return disableOK;}
  if(!enableOK)return false;disabled&=~value;return true;
 }
};
static DWORD WINAPI fakePad(DWORD user,XINPUT_STATE* state){
 *state={};if(user==0)state->Gamepad.wButtons=XINPUT_GAMEPAD_B;return ERROR_SUCCESS;
}
static DWORD WINAPI idlePad(DWORD,XINPUT_STATE* state){*state={};return ERROR_SUCCESS;}
static DWORD WINAPI missingPad(DWORD,XINPUT_STATE*){return ERROR_DEVICE_NOT_CONNECTED;}
static int checkControlLease(){
 // xNVSE sends these notifications synchronously while compiling/calling UDFs.
 // The real callback must ignore them even with the browser mutex already held.
 {
  std::unique_lock<std::mutex> held(catalogMutex);
  for(uint32_t type:{5u,10u,19u,21u,22u}){
   BrowserNVSEMessage nested{"NVSE",type,0,nullptr};browserNVSEMessage(&nested);
  }
 }
 BrowserPlayerControls api{fakeDisable,fakeEnable,nullptr};FakeButtons buttons;
 controls::Lease item,actor;item.configure(&api,&buttons,"Item");actor.configure(&api,&buttons,"Actor");
 const uint32_t prior=0x10001;buttons.disabled=prior;
 restrictions["Quest"]=1;
 if(!item.acquire()||buttons.disabled!=controls::InputMask||restrictions["Item"]!=controls::GameplayMask)return 40;
 int writes=buttons.writes;if(!item.acquire()||writes!=buttons.writes)return 41;
 // A quest can add another restriction during browsing; releasing our owner
 // never enables controls owned by that quest.
 restrictions["Quest"]|=2;
 if(!item.release()||restrictions["Item"]||restrictions["Quest"]!=3||buttons.disabled!=prior)return 42;
 if(!actor.acquire()||!actor.release()||buttons.disabled!=prior)return 43;
 controls::Lease unavailable;if(unavailable.acquire())return 44;
 buttons.captureOK=false;if(item.acquire()||item.active()||buttons.disabled!=prior)return 45;buttons.captureOK=true;
 buttons.disableOK=false;if(item.acquire()||item.active()||restrictions["Item"]||buttons.disabled!=prior)return 46;
 buttons.disableOK=true;if(!item.acquire())return 47;
 buttons.enableOK=false;if(item.release()||!item.active()||!restrictions["Item"])return 48;
 buttons.enableOK=true;if(!item.release()||buttons.disabled!=prior)return 49;
 auto saved=pad::raw;pad::raw=fakePad;
 if(pad::allReleased()||pad::sample().wButtons!=XINPUT_GAMEPAD_B)return 50;
 pad::raw=idlePad;if(!pad::allReleased())return 51;
 pad::raw=missingPad;if(!pad::allReleased())return 52;pad::raw=saved;
 // Actual browser lifecycle callback releases its lease on main-menu return.
 controlLease.configure(&api,&buttons,"Lifecycle");if(!controlLease.acquire()||!acquireBrowser())return 53;
 opened=true;BrowserNVSEMessage leave{"NVSE",2,0,nullptr};browserNVSEMessage(&leave);
 if(opened||controlLease.active()||ownsBrowserGate||buttons.disabled!=prior||restrictions["Lifecycle"])return 54;
 controlLease.configure(nullptr,nullptr,nullptr);
 return 0;
}

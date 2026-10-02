#include "browser.cpp"
#include <iostream>
static HookRegistry rival;
struct Mock {void** table;};
static int stateA=0,stateB=0,rivalCalls=0,baseCalls=0;
static HRESULT STDMETHODCALLTYPE baseA(IDirectInputDevice8W*,DWORD,LPVOID){++stateA;return S_OK;}
static HRESULT STDMETHODCALLTYPE baseB(IDirectInputDevice8W*,DWORD,LPVOID){++stateB;return S_OK;}
static HRESULT STDMETHODCALLTYPE rivalState(IDirectInputDevice8W* d,DWORD n,LPVOID p){++rivalCalls;return rival.next<StateFn>(d,9)(d,n,p);}
using Generic=HRESULT(STDMETHODCALLTYPE*)(void*);
static HRESULT STDMETHODCALLTYPE baseGeneric(void*){++baseCalls;return S_OK;}
static int activeSlot;
static HookRegistry firstGeneric,secondGeneric;
static HRESULT STDMETHODCALLTYPE firstHook(void* p){return firstGeneric.next<Generic>(p,activeSlot)(p);}
static HRESULT STDMETHODCALLTYPE secondHook(void* p){return secondGeneric.next<Generic>(p,activeSlot)(p);}
int main(){
 void* a[11]{};void* b[11]{};Mock da{a},db{b};a[9]=(void*)baseA;b[9]=(void*)baseB;
 void* ignored=nullptr;
 for(int n=0;n<100;++n){
  for(auto obj:{&da,&db}){
   if(!replaceSlot(obj->table,9,(void*)stateHook,(void**)&originalState))return 1;
   if(!rival.install(obj->table,9,(void*)rivalState,&ignored))return 2;
   if(FAILED(((StateFn)obj->table[9])((IDirectInputDevice8W*)obj,0,nullptr)))return 3;
  }
 }
 if(stateA!=100||stateB!=100||rivalCalls!=200)return 4;
 // Exercise all factory/input slots with both module installation orders.
 void* tables[2][21]{};Mock objects[]={{tables[0]},{tables[1]}};
 for(int slot: {3,9,10,16,20}){
  activeSlot=slot;
  for(int j=0;j<2;++j){
   auto obj=&objects[j];obj->table[slot]=(void*)baseGeneric;
   for(int n=0;n<100;++n){
    if(j==0){
     if(!firstGeneric.install(obj->table,slot,(void*)firstHook,&ignored)||!secondGeneric.install(obj->table,slot,(void*)secondHook,&ignored))return 5;
    }else{
     if(!secondGeneric.install(obj->table,slot,(void*)secondHook,&ignored)||!firstGeneric.install(obj->table,slot,(void*)firstHook,&ignored))return 6;
    }
    if(FAILED(((Generic)obj->table[slot])(obj)))return 7;
   }
  }
 }
 if(baseCalls!=1000)return 8;
 std::cout<<"PASS: two interposers, both load orders, repeated input/factory captures, separate interface forwarding (1200 calls).\n";
 return 0;
}


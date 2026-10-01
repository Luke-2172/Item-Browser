#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <iostream>
int main() {
 IDirectInput8W* di=nullptr;IDirectInputDevice8W* mouse=nullptr;IDirectInputDevice8W* keyboard=nullptr;
 if(FAILED(DirectInput8Create(GetModuleHandle(nullptr),0x800,IID_IDirectInput8W,(void**)&di,nullptr)))return 1;
 if(FAILED(di->CreateDevice(GUID_SysMouse,&mouse,nullptr))||FAILED(di->CreateDevice(GUID_SysKeyboard,&keyboard,nullptr)))return 2;
 auto m=*(void***)mouse,k=*(void***)keyboard;std::cout<<"Shared input vtable="<<(m==k)<<" state="<<(m[9]==k[9])<<" data="<<(m[10]==k[10])<<"\n";
 keyboard->Release();mouse->Release();di->Release();return m==k?0:3;
}


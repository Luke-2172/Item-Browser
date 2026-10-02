#include "browser.cpp"
#include <iostream>
int main() {
 moduleHandle=GetModuleHandleW(nullptr);
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=moduleHandle;wc.lpszClassName=L"LukesItemBrowserRenderTest";RegisterClassW(&wc);
 auto hwnd=CreateWindowW(wc.lpszClassName,L"LukesItemBrowser test",WS_POPUP,-32000,-32000,Width,Height,nullptr,nullptr,moduleHandle,nullptr);
 if(initialize(nullptr)!=0)return 1;
 auto d3d=Direct3DCreate9(D3D_SDK_VERSION);if(!d3d)return 1;
 D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=hwnd;pp.BackBufferWidth=Width;pp.BackBufferHeight=Height;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
 IDirect3DDevice9* dev=nullptr;if(FAILED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,hwnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&dev)))return 2;
 bool hooked=(*(void***)dev)[42]==(void*)endHook;std::cout<<"Created device EndScene hooked="<<hooked<<"\n";if(!hooked)return 3;
 catalog.items={ib::Item{0x123,"WEAP","Render test pistol","LukesItemBrowserTest",false}};plugins={L"Test.esm"};pluginIndex=0;filterPlugins();filterItems();opened=true;
 dev->Clear(0,nullptr,D3DCLEAR_TARGET,0xFF000000,1,0);if(FAILED(dev->BeginScene()))return 4;
 dev->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);draw(dev);DWORD state=0;dev->GetRenderState(D3DRS_FILLMODE,&state);if(state!=D3DFILL_WIREFRAME)return 5;
 if(FAILED(dev->EndScene()))return 6;
 IDirect3DSurface9 *back=nullptr,*copy=nullptr;dev->GetRenderTarget(0,&back);
 if(FAILED(dev->CreateOffscreenPlainSurface(Width,Height,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&copy,nullptr)))return 7;
 if(FAILED(dev->GetRenderTargetData(back,copy)))return 8;
 D3DLOCKED_RECT lr{};copy->LockRect(&lr,nullptr,D3DLOCK_READONLY);size_t bright=0;
 for(int y=0;y<Height;++y){auto p=(DWORD*)((char*)lr.pBits+y*lr.Pitch);for(int x=0;x<Width;++x)if(((p[x]>>16)&255)>100)++bright;}
 copy->UnlockRect();copy->Release();back->Release();std::cout<<"Rendered bright pixels="<<bright<<"; state restored="<<(state==D3DFILL_WIREFRAME)<<"\n";
 if(bright<1000)return 9;
 if(FAILED(dev->Reset(&pp)))return 10;if(texture)return 11;
 dev->BeginScene();draw(dev);dev->EndScene();if(!texture)return 12;
 std::cout<<"PASS: native D3D9 overlay draw, vtable hook, state restoration, Reset texture recreation. Not a New Vegas test.\n";
 return 0;
}


#include "browser.cpp"
#include <iostream>
static void* fakeD3DTable[21]{};
static void* fakeDeviceTable[134]{};
struct FakeCOM {void** table;};
static FakeCOM fakeD3D{fakeD3DTable},fakeDevice{fakeDeviceTable};
static int endCalls=0,resetCalls=0;
static bool visibleParameters=false;
static HRESULT STDMETHODCALLTYPE fakeEnd(IDirect3DDevice9*) {++endCalls;return S_OK;}
static HRESULT STDMETHODCALLTYPE fakeReset(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*) {++resetCalls;return S_OK;}
static HRESULT STDMETHODCALLTYPE fakeParameters(IDirect3DDevice9*,D3DDEVICE_CREATION_PARAMETERS* cp) {*cp={};if(visibleParameters)cp->hFocusWindow=GetDesktopWindow();return S_OK;}
static HRESULT STDMETHODCALLTYPE fakeCreate(IDirect3D9*,UINT,D3DDEVTYPE,HWND,DWORD,D3DPRESENT_PARAMETERS*,IDirect3DDevice9** out) {*out=(IDirect3DDevice9*)&fakeDevice;return S_OK;}
static IDirect3D9* WINAPI fakeFactory(UINT) {return (IDirect3D9*)&fakeD3D;}
static int presentCalls=0,presentExCalls=0,resetExCalls=0,textureCalls=0;
static HRESULT STDMETHODCALLTYPE fakePresent(IDirect3DDevice9*,const RECT*,const RECT*,HWND,const RGNDATA*){++presentCalls;return S_OK;}
static HRESULT STDMETHODCALLTYPE fakePresentEx(IDirect3DDevice9Ex*,const RECT*,const RECT*,HWND,const RGNDATA*,DWORD){++presentExCalls;return S_OK;}
static HRESULT STDMETHODCALLTYPE fakeResetEx(IDirect3DDevice9Ex*,D3DPRESENT_PARAMETERS*,D3DDISPLAYMODEEX*){++resetExCalls;return S_OK;}
static ULONG STDMETHODCALLTYPE fakeRelease(void*){return 1;}
static void* fakeTextureTable[22]{};
static FakeCOM fakeTexture{fakeTextureTable};
static bool rejectEveryTexture=false;
static HRESULT STDMETHODCALLTYPE fakeTextureCreate(IDirect3DDevice9*,UINT width,UINT,UINT,DWORD,D3DFORMAT,D3DPOOL,IDirect3DTexture9** out,HANDLE*) {
 ++textureCalls;*out=nullptr;if(rejectEveryTexture||width>1120)return D3DERR_OUTOFVIDEOMEMORY;
 *out=(IDirect3DTexture9*)&fakeTexture;return S_OK;
}
int main() {
 moduleHandle=GetModuleHandleW(nullptr);
 if(initialize(nullptr)!=0){std::cerr<<"Initialise failed\n";return 1;}
 auto kernel=GetModuleHandleW(L"kernel32.dll");
 auto proc=GetProcAddress(kernel,"GetCurrentProcessId");
 if(!proc || ((DWORD(WINAPI*)())proc)()!=GetCurrentProcessId())return 2;
 std::cout<<"PASS: executable GetProcAddress import hooked; unrelated calls preserved\n";
 fakeD3DTable[16]=(void*)fakeCreate;fakeDeviceTable[9]=(void*)fakeParameters;fakeDeviceTable[16]=(void*)fakeReset;fakeDeviceTable[42]=(void*)fakeEnd;
 originalFactory=fakeFactory;auto factory=factoryHook(D3D_SDK_VERSION);IDirect3DDevice9* device=nullptr;D3DPRESENT_PARAMETERS pp{};
 if(FAILED(factory->CreateDevice(0,D3DDEVTYPE_HAL,nullptr,0,&pp,&device)))return 3;
 if(fakeDeviceTable[42]!=(void*)endHook || fakeDeviceTable[16]!=(void*)resetHook)return 4;
 device->EndScene();device->Reset(&pp);
 // Recapturing a shared vtable must not replace an original with our own hook.
 attachRenderer(device);device->EndScene();
 if(endCalls!=2||resetCalls!=1)return 5;
 void* exTable[134]{};FakeCOM exDevice{exTable};
 exTable[9]=(void*)fakeParameters;exTable[16]=(void*)fakeReset;exTable[17]=(void*)fakePresent;
 exTable[42]=(void*)fakeEnd;exTable[121]=(void*)fakePresentEx;exTable[132]=(void*)fakeResetEx;
 auto ex=(IDirect3DDevice9Ex*)&exDevice;attachRenderer(ex,true);
 ex->Present(nullptr,nullptr,nullptr,nullptr);ex->PresentEx(nullptr,nullptr,nullptr,nullptr,0);ex->ResetEx(&pp,nullptr);
 if(presentCalls!=1||presentExCalls!=1||resetExCalls!=1)return 40;
 visibleParameters=true;frameSeen=false;sceneDevice=nullptr;ex->Present(nullptr,nullptr,nullptr,nullptr);visibleParameters=false;
 if(!frameSeen||!lastFrameTick)return 45;
 toggleTick=100;toggleQueued=true;if(!consumeToggle(150)||consumeToggle(151))return 46;
 toggleQueued=true;if(consumeToggle(2100))return 47;
 device->EndScene();if(endCalls!=3)return 41;
 fakeTextureTable[2]=(void*)fakeRelease;exTable[23]=(void*)fakeTextureCreate;
 renderScale=3;if(!createCanvas()||!ensureTexture(ex)||renderScale!=1||textureCalls!=3)return 42;
 releaseTexture();rejectEveryTexture=true;textureCalls=0;
 if(ensureTexture(ex)||textureCalls!=1)return 43;
 rejectEveryTexture=false;renderScale=2;if(!createCanvas())return 44;
 std::cout<<"PASS: separate device chains, Present/PresentEx/ResetEx, texture downsizing and terminal allocation failure\n";
 std::cout<<"PASS: game factory -> CreateDevice -> EndScene/Reset chain, including duplicate capture\n";
 IDirectInput8W* di=nullptr;IDirectInputDevice8W* mouse=nullptr;IDirectInputDevice8W* keyboard=nullptr;
 if(FAILED(DirectInput8Create(moduleHandle,0x800,IID_IDirectInput8W,(void**)&di,nullptr)))return 6;
 if(FAILED(di->CreateDevice(GUID_SysMouse,&mouse,nullptr))||FAILED(di->CreateDevice(GUID_SysKeyboard,&keyboard,nullptr)))return 7;
 if(!inputReady || (*(void***)mouse)[9]!=(void*)stateHook || (*(void***)keyboard)[9]!=(void*)stateHook)return 8;
 std::cout<<"PASS: real DirectInput factory and both keyboard/mouse device hooks\n";
 sessionToken=123456;
 for(const auto* format:{L"123456",L"123456.000000",L"1.23456e+005"}) {
  WritePrivateProfileStringW(L"Bridge",L"Ready",format,bridgePath.c_str());if(bridgeNumber(L"Bridge",L"Ready")!=sessionToken)return 9;
 }
 plugins={L"Test Weapons.esp"};pluginIndex=0;catalog.items={ib::Item{0x01003026,"WEAP","USP-16","WeapRedFactionPistol",false}};visible={0};selected=0;quantity=10;
 requestItem();wchar_t form[32]{},plugin[128]{};GetPrivateProfileStringW(L"Request",L"Form",L"",form,32,bridgePath.c_str());GetPrivateProfileStringW(L"Request",L"Plugin",L"",plugin,128,bridgePath.c_str());
 if(wcscmp(form,L"003026")||wcscmp(plugin,L"Test Weapons.esp")||bridgeNumber(L"Request",L"Count")!=10||bridgeNumber(L"Request",L"Pending")!=1)return 10;
 std::cout<<"PASS: decimal/scientific INI readiness and Test Weapons item-request serialization\n";
 if(rasterWidth()!=2240||rasterHeight()!=1400)return 11;
 drawCanvas();if(((uint32_t*)pixels)[0]>>24!=0)return 12;
  std::cout<<"PASS: 2240x1400 menu canvas with complete alpha initialization\n";
 auto alpha=((uint32_t*)pixels)[(600*renderScale)*rasterWidth()+750*renderScale]>>24;
 if(alpha<180||alpha>210)return 13;
 WritePrivateProfileStringW(L"Request",L"Pending",L"0",bridgePath.c_str());
 cursorX=400;cursorY=ListTop+10;lastClickItem=-1;click();
 if(bridgeNumber(L"Request",L"Pending")!=0)return 14;
 click();if(bridgeNumber(L"Request",L"Pending")!=1)return 15;
 WritePrivateProfileStringW(L"Request",L"Pending",L"0",bridgePath.c_str());
 click();if(bridgeNumber(L"Request",L"Pending")!=0)return 16;
 filterItems();click();if(bridgeNumber(L"Request",L"Pending")!=0)return 17;
 lastClickTime=GetTickCount()-GetDoubleClickTime()-1;click();if(bridgeNumber(L"Request",L"Pending")!=0)return 18;
 filteredPlugins.resize(100);dragScroll=1;dragOffset=scrollThumb(100)/2.f;cursorY=ListTop+ListHeight;dragScrollbar();if(pluginScroll!=100-ListRows)return 19;
 cursorY=ListTop;dragScrollbar();if(pluginScroll!=0)return 20;
 menuSounds=true;menuSound(1);if(bridgeNumber(L"Audio",L"Menu")!=1)return 21;
 closeBrowser();if(bridgeNumber(L"Audio",L"Menu")!=2)return 22;
 std::cout<<"PASS: translucent background, double-click single request, filter/timeout safety, scrollbar endpoints, open/close audio events\n";
  cursorX=980;cursorY=85;click();if(focus!=1||searchScope!=1)return 23;
 cursorX=800;cursorY=120;click();if(focus!=1)return 24;
 cursorX=720;cursorY=400;click();if(focus!=0||searchScope!=1)return 25;
 cursorX=880;cursorY=85;click();if(focus!=2||searchScope!=2)return 26;
 for(int c=0;c<10;++c){cursorX=(float)categoryX(c)+10;cursorY=115;click();if(category!=c)return 27;}
 std::cout<<"PASS: relocated search controls, persistent search scope, all ten category hit targets\n";
 for(const auto* bad:{"../bad.esp","C:\\bad.esp","x\n[Request].esp","test.esp:stream",".hidden.esp","a..b.esp"})
  if(ib::pluginName(bad))return 28;
 if(!ib::pluginName("Vegas Overhaul.esp")||!ib::pluginName("FalloutNV.esm"))return 29;
 WritePrivateProfileStringW(L"Controls",L"MouseSpeed",L"nan",iniPath.c_str());
 WritePrivateProfileStringW(L"Controls",L"FunctionKey",nullptr,iniPath.c_str());
 WritePrivateProfileStringW(L"Controls",L"Hotkey",L"1.230000e+002",iniPath.c_str());
 WritePrivateProfileStringW(L"Display",L"BackgroundOpacity",L"9.500000e+001",iniPath.c_str());
 WritePrivateProfileStringW(L"Browser",L"ShowOverrides",L"1.000000",iniPath.c_str());config();
 if(mouseSpeed!=1.6f||hotkey!=VK_F12||backgroundOpacity!=95||!showOverrides)return 30;
 WritePrivateProfileStringW(L"Controls",L"MouseSpeed",L"999999999999999999",iniPath.c_str());config();
 if(mouseSpeed!=5.f)return 31;
 WritePrivateProfileStringW(L"Controls",L"FunctionKey",L"2.400000e+001",iniPath.c_str());config();
 if(hotkey!=VK_F24)return 32;
 WritePrivateProfileStringW(L"Controls",L"FunctionKey",L"-9",iniPath.c_str());config();
 if(hotkey!=VK_F1)return 33;
 WritePrivateProfileStringW(L"Controls",L"FunctionKey",L"nan",iniPath.c_str());config();
 if(hotkey!=VK_F12)return 34;
 WritePrivateProfileStringW(L"Controls",L"FunctionKey",L"11",iniPath.c_str());
 WritePrivateProfileStringW(L"Controls",L"MouseSpeed",L"1.6",iniPath.c_str());
 WritePrivateProfileStringW(L"Controls",L"Hotkey",L"122",iniPath.c_str());
 WritePrivateProfileStringW(L"Display",L"BackgroundOpacity",L"72",iniPath.c_str());
 WritePrivateProfileStringW(L"Browser",L"ShowOverrides",L"0",iniPath.c_str());config();
 std::cout<<"PASS: path traversal rejection, scientific MCM values, NaN rejection and bounds\n";
 keyboard->Release();mouse->Release();di->Release();
 std::cout<<"Renderer test uses mock COM objects; does not certify in-game GPU rendering.\n";return 0;
}




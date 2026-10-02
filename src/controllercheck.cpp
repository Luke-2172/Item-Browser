#include "browser.cpp"
#include <iostream>
static int fail(int n){std::cerr<<"ControllerCheck failed: "<<n<<std::endl;return n;}
#include "controlleasecheck.h"
#include "controlscriptcheck.h"
int main(){
 if(auto error=checkControlLease())return fail(error);
 if(auto error=scriptcheck::run())return fail(error);
 wchar_t temporary[MAX_PATH]{};GetTempPathW(MAX_PATH,temporary);
 iniPath=std::wstring(temporary)+L"browser-controller-check-"+std::to_wstring(GetCurrentProcessId())+L".ini";
 bridgePath=iniPath+L".bridge";sessionToken=123456;
 pad::Decoder decoder;XINPUT_GAMEPAD g{};
 g.sThumbLX=7000;if(decoder.update(g,10,XINPUT_GAMEPAD_DPAD_LEFT).activity)return fail(1);
 g.sThumbLX=20000;if(decoder.update(g,20,XINPUT_GAMEPAD_DPAD_LEFT).dx!=1)return fail(2);
 if(decoder.update(g,100,XINPUT_GAMEPAD_DPAD_LEFT).dx)return fail(3);
 if(decoder.update(g,380,XINPUT_GAMEPAD_DPAD_LEFT).dx!=1)return fail(4);
 g={};decoder.update(g,400,XINPUT_GAMEPAD_DPAD_LEFT);g.wButtons=XINPUT_GAMEPAD_LEFT_SHOULDER|XINPUT_GAMEPAD_DPAD_LEFT;
 auto e=decoder.update(g,410,XINPUT_GAMEPAD_DPAD_LEFT);if(!e.chord||e.dx||e.down)return fail(5);
 if(decoder.update(g,420,XINPUT_GAMEPAD_DPAD_LEFT).chord)return fail(6);
  pad::Decoder shortcut;g={};g.wButtons=XINPUT_GAMEPAD_LEFT_SHOULDER;
 if(!(shortcut.update(g,1,XINPUT_GAMEPAD_DPAD_RIGHT).down&XINPUT_GAMEPAD_LEFT_SHOULDER))return fail(69);
 g.wButtons|=XINPUT_GAMEPAD_DPAD_RIGHT;auto chord=shortcut.update(g,2,XINPUT_GAMEPAD_DPAD_RIGHT);
 if(!chord.chord||chord.down||chord.dx)return fail(70);
 g={};shortcut.update(g,3,XINPUT_GAMEPAD_DPAD_RIGHT);g.wButtons=XINPUT_GAMEPAD_BACK|XINPUT_GAMEPAD_DPAD_RIGHT;
 if(shortcut.update(g,4,XINPUT_GAMEPAD_DPAD_RIGHT).chord)return fail(71);
 if(!acquireBrowser()||acquireBrowser())return fail(17);releaseBrowser();if(!acquireBrowser())return fail(18);releaseBrowser();
 opened=true;controllerPrompts=false;plugins={L"Test.esm"};pluginIndex=0;filteredPlugins={0};
 for(int i=0;i<50;++i){catalog.items.push_back({(uint32_t)i,
#ifdef ACTOR_BROWSER
 "NPC_",
#else
 "WEAP",
#endif
 "Fixture "+std::to_string(i),"Fixture",false});visible.push_back(i);}
 cursorX=500;cursorY=ListTop+14;selected=0;e={};e.dy=1;e.activity=true;controllerActions(e);
 if(!controllerPrompts||selected!=1)return fail(19);
 dirty=false;setPromptMode(false);if(controllerPrompts||!dirty)return fail(32);dirty=false;setPromptMode(false);if(dirty)return fail(33);setPromptMode(true);
 for(int i=0;i<20;++i)navMove(0,1);if(selected!=21||itemScroll!=8)return fail(20);
 navMove(0,0,1);if(selected!=35||itemScroll!=22)return fail(21);
 navColumn(-1);if(cursorX>=355)return fail(22);
 e={};e.down=XINPUT_GAMEPAD_Y;controllerActions(e);if(focus!=searchScope)return fail(23);
 focus=0;e.down=XINPUT_GAMEPAD_START;controllerActions(e);
#ifdef ACTOR_BROWSER
 if(!valuesPage)return fail(24);valueInput="100";avIndex=0;cursorX=900;cursorY=410;navMove(1,0);if(valueInput!="101")return fail(25);
 e.down=XINPUT_GAMEPAD_START;controllerActions(e);
#endif
 if(!settingsPage)return fail(26);cursorX=550;cursorY=330;bool sound=menuSounds;e.down=XINPUT_GAMEPAD_A;controllerActions(e);if(menuSounds==sound)return fail(27);
 e.down=XINPUT_GAMEPAD_B;controllerActions(e);if(settingsPage||!opened)return fail(28);
 themeRGB=0x20FF40;if(tintPixel(255)!=themeRGB||tintPixel(0)!=0)return fail(29);
 loadMenuFonts(L"package/NVSE/Plugins/LukesItemBrowser/Fonts/");renderScale=2;if(!createCanvas())return fail(30);
 cursorX=500;cursorY=ListTop+14;drawCanvas();auto p=(uint32_t*)pixels;
 // Full-colour text/frame uses the chosen tint, and external shadow remains black.
 bool found=false;for(size_t i=0;i<(size_t)rasterWidth()*rasterHeight();++i)if((p[i]&0xFFFFFF)==themeRGB)found=true;
 if(!found||(p[0]&0xFFFFFF))return fail(31);
 BITMAPFILEHEADER h{};h.bfType=0x4D42;h.bfOffBits=sizeof(h)+40;h.bfSize=h.bfOffBits+rasterWidth()*rasterHeight()*4;
 BITMAPINFOHEADER b{};b.biSize=40;b.biWidth=rasterWidth();b.biHeight=-rasterHeight();b.biPlanes=1;b.biBitCount=32;
 std::ofstream out("build/Controller-preview.bmp",std::ios::binary);out.write((char*)&h,sizeof(h));out.write((char*)&b,40);out.write((char*)pixels,rasterWidth()*rasterHeight()*4);
 std::cout<<"PASS: dead zones, repeat timing, opening chords, nested xNVSE notifications, script-API queries/commands/triggers/retry, supported control leases, independent quest ownership, baseline button preservation, rollback and lifecycle cleanup, mutual exclusion, all-page navigation, colour and footer render"<<std::endl;
 DeleteFileW(iniPath.c_str());DeleteFileW(bridgePath.c_str());return 0;
}

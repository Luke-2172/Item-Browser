#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <d3d9.h>
#include <dinput.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <sstream>
#include <cmath>
#include "catalog.h"
#include "importhook.h"

// Uses only the four-field version prefix of the public NVSE query ABI.
struct NVSEVersionPrefix { uint32_t nvseVersion,runtimeVersion,editorVersion,isEditor; };
struct PluginInfo { uint32_t infoVersion; const char* name; uint32_t version; };
static HMODULE moduleHandle;
static std::wstring root,iniPath;
static std::wstring bridgePath;
static UINT sessionToken=0;
static int quantity=1;
static bool livePlugins=false;
static std::atomic<bool> opened{false};
static std::atomic<DWORD> blockUntil{0};
static std::atomic<long> mouseDX{0},mouseDY{0},wheelDelta{0};
static std::mutex catalogMutex;
static ib::Catalog catalog;
static std::vector<std::wstring> plugins;
static std::vector<size_t> visible;
static std::string status="Select a plugin to browse items introduced by that file.";
static std::atomic<bool> loading{false};
static int pluginIndex=-1,pluginScroll=0,itemScroll=0,selected=-1,category=0,focus=0;
static bool showOverrides=false,settingsPage=false,dirty=true;
static int backgroundOpacity=72,dragScroll=0;
static float mouseSpeed=1.6f,dragOffset=0;
static bool menuSounds=true,pickupSounds=true;
static int lastClickItem=-1;static DWORD lastClickTime=0;static float lastClickX=0,lastClickY=0;
constexpr int ListTop=190,RowHeight=29,ListRows=14,ListHeight=ListRows*RowHeight;
static int searchScope=2; // 1=plugins, 2=items, independent of keyboard focus.
static std::string query,pluginQuery;
static std::vector<int> filteredPlugins;
static UINT hotkey=VK_F11;
static float cursorX=550,cursorY=350;
static bool previousKeys[256]{};
static HWND gameWindow=nullptr;
static HBRUSH background;
static HFONT font,titleFont,smallFont,detailFont;
static wchar_t fontFace[128]=L"Bahnschrift SemiCondensed";
static HDC canvas;
static HBITMAP bitmap;
static void* pixels;
static IDirect3DTexture9* texture=nullptr;
static IDirect3DDevice9* renderDevice=nullptr;
static ib::Inflate inflateFn=nullptr;
static std::atomic<bool> frameSeen{false},inputReady{false};
constexpr int Width=1120,Height=700;
static int renderScale=2;
static int rasterWidth() {return Width*renderScale;}
static int rasterHeight() {return Height*renderScale;}
static bool createCanvas() {
 BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=rasterWidth();bi.bmiHeader.biHeight=-rasterHeight();bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
 canvas=CreateCompatibleDC(nullptr);bitmap=CreateDIBSection(canvas,&bi,DIB_RGB_COLORS,&pixels,nullptr,0);
 if(!canvas||!bitmap||!pixels)return false;
 SelectObject(canvas,bitmap);SetMapMode(canvas,MM_ANISOTROPIC);
 SetWindowExtEx(canvas,Width,Height,nullptr);SetViewportExtEx(canvas,rasterWidth(),rasterHeight(),nullptr);
 GetPrivateProfileStringW(L"Display",L"FontFace",L"Bahnschrift SemiCondensed",fontFace,128,iniPath.c_str());
 // Text is measured directly in raster pixels, avoiding logical-unit advance rounding.
 font=CreateFontW(-22*renderScale,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,fontFace);
 titleFont=CreateFontW(-38*renderScale,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,fontFace);
 smallFont=CreateFontW(-16*renderScale,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,fontFace);
 detailFont=CreateFontW(-27*renderScale,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,fontFace);
 return font&&titleFont&&smallFont&&detailFont;
}
static const char* categories[]={"All","WEAP","ARMO","AMMO","ALCH","MISC","BOOK","KEYM","IMOD","NOTE"};
static const char* categoryLabels[]={"All","Weapons","Armour","Ammo","Aid","Misc","Books","Keys","Mods","Notes"};
static void logLine(const char* text) {
 std::ofstream f(root+L"Data\\NVSE\\Plugins\\LukesItemBrowser.log",std::ios::app); f<<text<<"\n";
}
static void nativeStatus(const wchar_t* key,const wchar_t* value) {
 if(!bridgePath.empty())WritePrivateProfileStringW(L"Native",key,value,bridgePath.c_str());
}
static void failNative(const char* reason) {
 logLine(reason);std::wstring message;for(auto c:std::string(reason))message.push_back((unsigned char)c);
 nativeStatus(L"Error",message.c_str());
}
static std::wstring wide(const std::string& s) {
 if(s.empty()) return L""; int n=MultiByteToWideChar(CP_ACP,0,s.data(),(int)s.size(),nullptr,0);
 std::wstring r(n,0); MultiByteToWideChar(CP_ACP,0,s.data(),(int)s.size(),&r[0],n); return r;
}
static std::string narrow(const std::wstring& s) {
 int n=WideCharToMultiByte(CP_ACP,0,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr);
 std::string r(n,0); if(n) WideCharToMultiByte(CP_ACP,0,s.data(),(int)s.size(),&r[0],n,nullptr,nullptr); return r;
}
static void filterItems() {
 dirty=true;lastClickItem=-1; visible.clear(); selected=-1; itemScroll=0;
 for(size_t i=0;i<catalog.items.size();++i) if((showOverrides||!catalog.items[i].overrideRecord) && ib::matches(catalog.items[i],query,category?categories[category]:"")) visible.push_back(i);
 if(!visible.empty()) selected=0;
}
static void filterPlugins() {
 dirty=true;lastClickItem=-1; filteredPlugins.clear(); pluginScroll=0;
 for(size_t i=0;i<plugins.size();++i) if(ib::lower(narrow(plugins[i])).find(ib::lower(pluginQuery))!=std::string::npos) filteredPlugins.push_back((int)i);
}
static UINT bridgeNumber(const wchar_t* section,const wchar_t* key) {
 wchar_t value[80]{};GetPrivateProfileStringW(section,key,L"0",value,80,bridgePath.c_str());
 // JIP writes floats. Win32's integer reader truncates scientific notation.
 double number=wcstod(value,nullptr);if(!(number>=0&&number<=16777215))return 0;return (UINT)(number+0.5);
}
static double setting(const wchar_t* section,const wchar_t* key,double fallback,double lo,double hi) {
 wchar_t value[80]{};GetPrivateProfileStringW(section,key,L"",value,80,iniPath.c_str());
 wchar_t* end=nullptr;double n=wcstod(value,&end);
 if(end==value||*end||!std::isfinite(n))return fallback;
 return std::clamp(n,lo,hi);
}
static void scanPlugins() {
 plugins.clear(); livePlugins=false;
 if(!bridgePath.empty() && sessionToken && bridgeNumber(L"Bridge",L"Ready")==sessionToken) {
  int count=(int)bridgeNumber(L"Bridge",L"Count");
  if(count>0&&count<256) {for(int i=0;i<count;++i) {wchar_t name[512]{};GetPrivateProfileStringW(L"Loaded",std::to_wstring(i).c_str(),L"",name,512,bridgePath.c_str()); if(ib::pluginName(narrow(name)))plugins.emplace_back(name);}livePlugins=true;}
 }
 if(livePlugins) {std::sort(plugins.begin(),plugins.end(),[](auto& a,auto& b){return _wcsicmp(a.c_str(),b.c_str())<0;});filterPlugins();return;}
 WIN32_FIND_DATAW data;
 HANDLE h=FindFirstFileW((root+L"Data\\*").c_str(),&data);
 if(h!=INVALID_HANDLE_VALUE) {
  do {std::wstring n=data.cFileName; auto p=n.find_last_of(L'.'); if(p==n.npos || data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue;
   auto ext=ib::lower(narrow(n.substr(p))); if(ext==".esp"||ext==".esm") plugins.push_back(n);
  }while(FindNextFileW(h,&data)); FindClose(h);
 }
 std::sort(plugins.begin(),plugins.end(),[](auto& a,auto& b){return _wcsicmp(a.c_str(),b.c_str())<0;}); filterPlugins();
}
static void choosePlugin(int i) {
 if(loading || i<0 || i>=(int)plugins.size()) return;
 if(!ib::pluginName(narrow(plugins[i]))) {status="Invalid plugin filename rejected.";dirty=true;return;}
 pluginIndex=i; catalog={}; dirty=true;lastClickItem=-1; visible.clear(); selected=-1; itemScroll=0; loading=true;
 status="Reading plugin..."; auto path=root+L"Data\\"+plugins[i];
 try { std::thread([path] {
  try { auto c=ib::readPlugin(path,inflateFn); std::lock_guard<std::mutex> guard(catalogMutex); catalog=std::move(c); filterItems(); status=std::to_string(visible.size())+" matching items. Record IDs are file-local, not console IDs."; }
  catch(const std::exception& e) {std::lock_guard<std::mutex> guard(catalogMutex); status=std::string("Could not read plugin: ")+e.what(); logLine(status.c_str());}
  {std::lock_guard<std::mutex> guard(catalogMutex);loading=false;dirty=true;}
 }).detach(); } catch(const std::exception&) {loading=false;status="Could not start plugin reader.";dirty=true;}
}
static void config() {
 float speed=(float)setting(L"Controls",L"MouseSpeed",1.6,.25,5);
 int opacity=(int)setting(L"Display",L"BackgroundOpacity",72,20,100);
 bool menu=setting(L"Audio",L"MenuSounds",1,0,1)>=.5,pickup=setting(L"Audio",L"PickupSounds",1,0,1)>=.5;
 bool overrides=setting(L"Browser",L"ShowOverrides",0,0,1)>=.5;
 // FunctionKey is shared directly with the ESP-free MCM slider (F1-F24).
 // Older INIs using Windows virtual-key Hotkey values remain readable.
 const double legacyKey=setting(L"Controls",L"Hotkey",VK_F11,VK_F1,VK_F24);
 UINT key=VK_F1-1+(UINT)setting(L"Controls",L"FunctionKey",legacyKey-VK_F1+1,1,24);
 if(speed!=mouseSpeed||opacity!=backgroundOpacity||menu!=menuSounds||pickup!=pickupSounds||key!=hotkey)dirty=true;
 mouseSpeed=speed;backgroundOpacity=opacity;menuSounds=menu;pickupSounds=pickup;hotkey=key;
 if(overrides!=showOverrides){showOverrides=overrides;filterItems();}
 nativeStatus(L"PickupSounds",pickupSounds?L"1":L"0");
 if(!bridgePath.empty())nativeStatus(L"Hotkey",(L"F"+std::to_wstring(hotkey-VK_F1+1)).c_str());
}
static void requestItem() { dirty=true;
 if(loading||selected<0||selected>=(int)visible.size()||pluginIndex<0||pluginIndex>=(int)plugins.size())return;
 if(visible[selected]>=catalog.items.size())return;
 if(bridgeNumber(L"Bridge",L"Ready")!=sessionToken) {status="Item bridge is not ready. Check the console for Luke's Item Browser script errors.";logLine("Item request blocked: script bridge session is not ready.");return;}
 if(GetPrivateProfileIntW(L"Request",L"Pending",0,bridgePath.c_str())) {status="Waiting for the previous request to finish.";return;}
 const auto& item=catalog.items[visible[selected]]; std::wstring origin=plugins[pluginIndex];
 if(item.overrideRecord){if((item.id>>24)>=catalog.masters.size())return;origin=wide(catalog.masters[item.id>>24]);}
 if(!ib::pluginName(narrow(origin))) {status="Invalid source filename rejected.";return;}
 quantity=std::clamp(quantity,1,100);
 wchar_t id[16];swprintf_s(id,L"%06X",item.id&0xFFFFFF);
 bool ok=WritePrivateProfileStringW(L"Request",L"Plugin",origin.c_str(),bridgePath.c_str())!=0;
 ok=ok && WritePrivateProfileStringW(L"Request",L"Form",id,bridgePath.c_str());
 ok=ok && WritePrivateProfileStringW(L"Request",L"Count",std::to_wstring(quantity).c_str(),bridgePath.c_str());
 ok=ok && WritePrivateProfileStringW(L"Request",L"Session",std::to_wstring(sessionToken).c_str(),bridgePath.c_str());
 ok=ok && WritePrivateProfileStringW(L"Request",L"Result",L"Waiting for game...",bridgePath.c_str());
 ok=ok && WritePrivateProfileStringW(L"Request",L"Pending",L"1",bridgePath.c_str());
 status=ok?"Item request queued; the game will verify the form and add it.":"Could not write the item request INI.";
}
static bool inside(int x,int y,int w,int h) {return cursorX>=x && cursorX<x+w && cursorY>=y && cursorY<y+h;}
static void fill(int x,int y,int w,int h,COLORREF color) { RECT r{x,y,x+w,y+h}; auto b=CreateSolidBrush(color); FillRect(canvas,&r,b); DeleteObject(b); }
static void text(int x,int y,int w,int h,const std::wstring& t,COLORREF color=RGB(235,182,86),HFONT f=nullptr) {
 int saved=SaveDC(canvas);if(!saved)return;
 SetMapMode(canvas,MM_TEXT);SetWindowOrgEx(canvas,0,0,nullptr);SetViewportOrgEx(canvas,0,0,nullptr);
 SelectObject(canvas,f?f:font);SetTextCharacterExtra(canvas,0);SetTextJustification(canvas,0,0);
 SetTextColor(canvas,color); SetBkMode(canvas,TRANSPARENT);
 RECT r{x*renderScale,y*renderScale,(x+w)*renderScale,(y+h)*renderScale};
 DrawTextW(canvas,t.c_str(),(int)t.size(),&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
 RestoreDC(canvas,saved);
}
static void label(int x,int y,int w,int h,const std::string& t,COLORREF color=RGB(235,182,86),HFONT f=nullptr) {text(x,y,w,h,wide(t),color,f);}
static void border(int x,int y,int w,int h,COLORREF c=RGB(121,92,45)) {fill(x,y,w,1,c);fill(x,y+h-1,w,1,c);fill(x,y,1,h,c);fill(x+w-1,y,1,h,c);}
static void button(int x,int y,int w,int h,const std::string& t,bool active=false) {
 if(active)fill(x,y,w,h,RGB(57,44,22));border(x,y,w,h,active?RGB(246,185,77):RGB(129,96,43));label(x+8,y,w-16,h,t);
}
static void tab(int x,int y,int w,const std::string& t,bool active) {
 if(active)button(x,y,w,30,t,true);else label(x+8,y,w-16,30,t,RGB(170,128,61));
}
static int scrollThumb(int count) {return count<=ListRows?ListHeight:std::max(24,ListHeight*ListRows/count);}
static int scrollY(int scroll,int count) {return ListTop+(count>ListRows?(ListHeight-scrollThumb(count))*scroll/(count-ListRows):0);}
static void scrollbar(int x,int scroll,int count) {
 fill(x+3,ListTop,1,ListHeight,RGB(120,90,41));if(count>ListRows)fill(x+1,scrollY(scroll,count),5,scrollThumb(count),RGB(247,185,76));
}
static void shortcut(int x,int w,const std::string& key,const std::string& title) {
 border(x,623,w,31);label(x+5,623,w-10,31,key,RGB(235,182,86),smallFont);label(x+w+8,623,135,31,title);
}
// Shared category geometry keeps visible tabs and mouse targets aligned.
static const int categoryWidths[]={44,92,88,68,48,57,65,55,63,67};
static int categoryX(int index) {int x=27;for(int n=0;n<index;++n)x+=categoryWidths[n]+5;return x;}
static void panel(int x,int w,const char* title) {
 border(x,153,w,449,RGB(103,78,39));
 fill(x+1,154,w-2,31,RGB(35,31,22));
 label(x+10,154,w-20,30,title,RGB(248,188,79));
 fill(x+1,185,w-2,1,RGB(173,126,48));
}
static void drawCanvas() {
 // Soft phosphor bands with irregular luminance, not a repeating square grid.
 fill(0,0,Width,Height,RGB(0,0,0));
 fill(10,10,Width-24,Height-24,RGB(9,10,9));
 GdiFlush();auto backgroundPixels=(uint32_t*)pixels;
 auto noise=[](int x,int y) {
  uint32_t h=(uint32_t)x*374761393u+(uint32_t)y*668265263u;
  h=(h^(h>>13))*1274126177u;h^=h>>16;
  return (h&65535)/65535.f;
 };
 for(int py=12*renderScale;py<(Height-16)*renderScale;++py) {
  float y=py/(float)renderScale;
  float wave=0.5f+0.5f*std::cos(y*6.2831853f/6.2f);
  float band=wave*wave;
  int ny=(int)(y/8.f);float fy=y/8.f-ny;fy=fy*fy*(3.f-2.f*fy);
  for(int px=12*renderScale;px<(Width-16)*renderScale;++px) {
   float x=px/(float)renderScale;
   int nx=(int)(x/5.f);float fx=x/5.f-nx;fx=fx*fx*(3.f-2.f*fx);
   float a=noise(nx,ny)*(1-fx)+noise(nx+1,ny)*fx;
   float b=noise(nx,ny+1)*(1-fx)+noise(nx+1,ny+1)*fx;
   float mottle=a*(1-fy)+b*fy;
   float edge=std::clamp(std::min({x-12.f,Width-16.f-x,y-12.f,Height-16.f-y})/26.f,0.f,1.f);
   float vignette=0.78f+0.22f*std::max(0.f,1.f-std::abs(x-Width*.5f)/(Width*.5f));
   float luminance=5.f+edge*vignette*(3.f+band*(9.f+17.f*mottle)+2.f*mottle);
   unsigned shade=(unsigned)std::clamp(luminance,0.f,255.f);
   backgroundPixels[py*rasterWidth()+px]=(shade<<16)|(shade<<8)|shade;
  }
 }
 // Inset amber frame leaves transparent room for a real external drop shadow.
 border(7,7,Width-20,Height-20,RGB(65,45,16));
 border(8,8,Width-22,Height-22,RGB(143,96,29));
 border(9,9,Width-24,Height-24,RGB(255,189,66));
 border(10,10,Width-26,Height-26,RGB(230,161,47));
 border(11,11,Width-28,Height-28,RGB(88,62,24));
 label(28,20,650,50,"LUKE'S ITEM BROWSER",RGB(250,194,90),titleFont);
 tab(858,27,108,"BROWSER",!settingsPage);tab(978,27,110,"SETTINGS",settingsPage);
 fill(26,144,1067,1,RGB(163,117,45));
 if(settingsPage) {
  label(32,150,600,36,"DISPLAY AND CONTROLS",RGB(250,194,90));
  label(32,210,470,32,"MOUSE SPEED");button(510,210,75,32,"-",false);button(598,210,135,32,std::to_string((int)(mouseSpeed*100))+"%",true);button(746,210,75,32,"+");
  label(32,267,470,32,"BACKGROUND OPACITY");button(510,267,75,32,"-");button(598,267,135,32,std::to_string(backgroundOpacity)+"%",true);button(746,267,75,32,"+");
  label(32,324,470,32,"MENU OPEN / CLOSE SOUNDS");button(510,324,180,32,menuSounds?"ON":"OFF",menuSounds);
  label(32,381,470,32,"ITEM PICKUP SOUNDS");button(510,381,180,32,pickupSounds?"ON":"OFF",pickupSounds);
  label(32,438,470,32,"INCLUDE OVERRIDE RECORDS");button(510,438,180,32,showOverrides?"ON":"OFF",showOverrides);
  label(32,510,980,30,"Settings save to LukesItemBrowser.ini. Hotkey and render resolution can also be edited there.",RGB(183,155,103),smallFont);
  button(32,563,190,32,"REFRESH PLUGINS");button(242,563,190,32,"CLEAR SEARCH");
 } else {
  for(int i=0;i<10;++i) {std::string t=categoryLabels[i];for(auto& c:t)c=(char)toupper(c);int x=categoryX(i),w=categoryWidths[i];if(category==i){fill(x,104,w,30,RGB(57,44,22));border(x,104,w,30,RGB(246,185,77));}label(x+6,104,w-12,30,t,category==i?RGB(250,194,90):RGB(190,147,73),smallFont);}
  label(788,72,62,26,"SEARCH",RGB(174,135,70),smallFont);
  button(855,72,99,26,"ITEMS",searchScope==2);button(961,72,128,26,"PLUGINS",searchScope==1);
  const auto& searchText=searchScope==1?pluginQuery:query;
  button(788,104,301,31,searchText.empty()?(searchScope==1?"Search plugins...":"Search items..."):searchText,focus!=0);
  panel(26,328,"PLUGINS");panel(364,339,"ITEMS");panel(713,378,"ITEM DETAILS");
  for(int row=0;row<ListRows;++row) {
   int p=pluginScroll+row,y=ListTop+row*RowHeight;
   if(p<(int)filteredPlugins.size()) {int idx=filteredPlugins[p];if(idx==pluginIndex){fill(27,y,310,28,RGB(57,44,22));border(27,y,310,28,RGB(241,183,80));}text(37,y,290,28,plugins[idx]);}
   int item=itemScroll+row;
   if(item<(int)visible.size()) {const auto& entry=catalog.items[visible[item]];if(item==selected){fill(369,y,313,28,RGB(57,44,22));border(369,y,313,28,RGB(241,183,80));}label(379,y,293,28,entry.name);}
  }
  scrollbar(340,pluginScroll,(int)filteredPlugins.size());scrollbar(686,itemScroll,(int)visible.size());
  if(loading)label(379,244,295,30,"Reading plugin...");
  else if(pluginIndex>=0&&visible.empty())label(379,244,295,30,"No matching items.");
  if(selected>=0&&selected<(int)visible.size()) {
   const auto& i=catalog.items[visible[selected]];char id[16];sprintf_s(id,"%08X",i.id);
   std::string name=i.name;for(auto& c:name)c=(char)toupper((unsigned char)c);label(726,198,350,44,name,RGB(250,194,90),detailFont);
   // Record metadata is real; no fake per-item weapon model is shown.
   label(727,263,350,30,"TYPE");label(727,295,350,30,i.type);
   label(727,348,350,28,"PLUGIN",RGB(171,132,67));
   if(i.overrideRecord&&(i.id>>24)<catalog.masters.size())label(727,375,350,30,catalog.masters[i.id>>24]);else text(727,375,350,30,plugins[pluginIndex]);
   label(727,421,350,28,"EDITOR ID",RGB(171,132,67));label(727,448,350,30,i.editor.empty()?"(none)":i.editor);
   label(727,490,350,25,std::string("LOCAL ID: ")+id,RGB(180,143,80),smallFont);
   fill(725,535,358,1,RGB(137,102,45));label(727,551,103,32,"Quantity:");
   const int counts[]={1,10,100};for(int n=0;n<3;++n)button(834+n*84,551,72,32,std::to_string(counts[n]),quantity==counts[n]);
  }
  label(28,601,560,22,std::to_string(visible.size())+" ITEMS  /  "+std::to_string(filteredPlugins.size())+(livePlugins?" LOADED PLUGINS":" INSTALLED PLUGINS"),RGB(170,132,70),smallFont);
 }
 if(!settingsPage)shortcut(609,60,"ENTER","ADD ITEM");shortcut(810,45,"F"+std::to_string(hotkey-VK_F1+1),"CLOSE");shortcut(973,45,"ESC","BACK");
 label(28,624,560,29,settingsPage?"Preferences apply immediately":"Double-click an item to add it",RGB(173,139,84),smallFont);
 label(28,658,1058,25,status,RGB(200,162,103),smallFont);
 GdiFlush();auto p=(uint32_t*)pixels;
 const unsigned baseAlpha=backgroundOpacity*255/100;
 for(int y=0;y<rasterHeight();++y)for(int x=0;x<rasterWidth();++x){
  int i=y*rasterWidth()+x;auto c=p[i];float lx=x/(float)renderScale,ly=y/(float)renderScale;
  bool inFrame=lx>=7&&lx<Width-13&&ly>=7&&ly<Height-13;
  unsigned brightness=std::max({c&255,(c>>8)&255,(c>>16)&255});
  unsigned alpha=baseAlpha+(255-baseAlpha)*std::min(brightness,180u)/180;
  if(!inFrame) {
   // Shadow offset down/right, fading to fully transparent at the outer edges.
   float dx=std::max({13.f-lx,0.f,lx-(Width-13.f)}),dy=std::max({13.f-ly,0.f,ly-(Height-13.f)});
   alpha=(unsigned)(115.f*std::max(0.f,1.f-std::max(dx,dy)/12.f));c=0;
  }
  p[i]=(c&0xFFFFFF)|(alpha<<24);
 }
}
static void menuSound(int event) {if(menuSounds&&!bridgePath.empty())WritePrivateProfileStringW(L"Audio",L"Menu",std::to_wstring(event).c_str(),bridgePath.c_str());}
static void closeBrowser() {opened=false;blockUntil=GetTickCount()+250;focus=0;dragScroll=0;lastClickItem=-1;menuSound(2);}
static void savePreferences() {
 WritePrivateProfileStringW(L"Browser",L"ShowOverrides",showOverrides?L"1":L"0",iniPath.c_str());
 WritePrivateProfileStringW(L"Controls",L"MouseSpeed",std::to_wstring(mouseSpeed).c_str(),iniPath.c_str());
 WritePrivateProfileStringW(L"Display",L"BackgroundOpacity",std::to_wstring(backgroundOpacity).c_str(),iniPath.c_str());
 WritePrivateProfileStringW(L"Audio",L"MenuSounds",menuSounds?L"1":L"0",iniPath.c_str());
 WritePrivateProfileStringW(L"Audio",L"PickupSounds",pickupSounds?L"1":L"0",iniPath.c_str());nativeStatus(L"PickupSounds",pickupSounds?L"1":L"0");
}
static void dragScrollbar() {
 int count=dragScroll==1?(int)filteredPlugins.size():(int)visible.size();auto& scroll=dragScroll==1?pluginScroll:itemScroll;
 int travel=ListHeight-scrollThumb(count);if(travel>0)scroll=std::clamp((int)((cursorY-ListTop-dragOffset)*(count-ListRows)/travel+0.5f),0,count-ListRows);dirty=true;
}
static void click() {
 dirty=true;
 if(inside(858,27,108,30)){settingsPage=false;lastClickItem=-1;return;}
 if(inside(978,27,110,30)){settingsPage=true;focus=0;lastClickItem=-1;return;}
 if(inside(810,623,148,31)){closeBrowser();return;} if(inside(973,623,123,31)){if(settingsPage){settingsPage=false;return;}closeBrowser();return;}
 if(settingsPage) {
  if(inside(510,210,75,32))mouseSpeed=std::max(.25f,mouseSpeed-.25f);
  if(inside(746,210,75,32))mouseSpeed=std::min(5.f,mouseSpeed+.25f);
  if(inside(510,267,75,32))backgroundOpacity=std::max(20,backgroundOpacity-5);
  if(inside(746,267,75,32))backgroundOpacity=std::min(100,backgroundOpacity+5);
  if(inside(510,324,180,32))menuSounds=!menuSounds;
  if(inside(510,381,180,32))pickupSounds=!pickupSounds;
  if(inside(510,438,180,32)){showOverrides=!showOverrides;filterItems();}
  if(inside(32,563,190,32)&&!loading){scanPlugins();pluginIndex=-1;catalog={};filterItems();status="Plugin list refreshed.";}
  if(inside(242,563,190,32)){query.clear();pluginQuery.clear();filterPlugins();filterItems();}
  savePreferences();return;
 }
 bool itemClick=inside(369,ListTop,313,ListHeight);if(!itemClick)lastClickItem=-1;
 for(int i=0;i<10;++i)if(inside(categoryX(i),104,categoryWidths[i],30)){category=i;filterItems();focus=0;return;}
 if(inside(855,72,99,26)){focus=searchScope=2;return;}if(inside(961,72,128,26)){focus=searchScope=1;return;}
 if(inside(788,104,301,31)){focus=searchScope;return;}focus=0;
 const int counts[]={1,10,100};for(int n=0;n<3;++n)if(inside(834+n*84,551,72,32)){quantity=counts[n];return;}
 if(inside(609,623,185,31)){requestItem();return;}
 for(int n=1;n<=2;++n)if(inside(n==1?338:684,ListTop,12,ListHeight)) {
  dragScroll=n;int scroll=n==1?pluginScroll:itemScroll,count=n==1?(int)filteredPlugins.size():(int)visible.size();
  int top=scrollY(scroll,count),thumb=scrollThumb(count);dragOffset=cursorY>=top&&cursorY<top+thumb?cursorY-top:thumb/2.f;dragScrollbar();lastClickItem=-1;return;
 }
 if(inside(27,ListTop,310,ListHeight)){int p=pluginScroll+((int)cursorY-ListTop)/RowHeight;if(p<(int)filteredPlugins.size())choosePlugin(filteredPlugins[p]);return;}
 if(itemClick&&!loading) {
  int p=itemScroll+((int)cursorY-ListTop)/RowHeight;if(p>=(int)visible.size()){lastClickItem=-1;return;}
  selected=p;DWORD now=GetTickCount();
  if(lastClickItem==p&&now-lastClickTime<=GetDoubleClickTime()&&abs(cursorX-lastClickX)<6&&abs(cursorY-lastClickY)<6){lastClickItem=-1;requestItem();}
  else {lastClickItem=p;lastClickTime=now;lastClickX=cursorX;lastClickY=cursorY;}
 }
}
static void inputs() {
 static DWORD lastConfigPoll=0;
 if(GetTickCount()-lastConfigPoll>=1000){lastConfigPoll=GetTickCount();config();}
 DWORD process=0; GetWindowThreadProcessId(GetForegroundWindow(),&process);
 if(process!=GetCurrentProcessId()) {if(opened)closeBrowser();return;}
 bool pressed[256]{}; for(int k=0;k<256;++k) {bool down=(GetAsyncKeyState(k)&0x8000)!=0;pressed[k]=down&&!previousKeys[k];previousKeys[k]=down;}
 if(pressed[hotkey]) {if(opened) closeBrowser();else {logLine("Hotkey detected; opening browser.");config();if(!loading){scanPlugins();pluginIndex=-1;catalog={};filterItems();}opened=true;dirty=true;settingsPage=false;lastClickItem=-1;menuSound(1);cursorX=560;cursorY=350;mouseDX=0;mouseDY=0;wheelDelta=0;}return;}
 if(!opened)return;
 if(pressed[VK_ESCAPE]) {if(settingsPage){settingsPage=false;dirty=true;}else closeBrowser();return;}
 cursorX=std::clamp(cursorX+mouseDX.exchange(0)*mouseSpeed,0.f,(float)Width-1); cursorY=std::clamp(cursorY+mouseDY.exchange(0)*mouseSpeed,0.f,(float)Height-1);
 if(pressed[VK_LBUTTON])click();if(!previousKeys[VK_LBUTTON])dragScroll=0;else if(dragScroll)dragScrollbar();
 auto wheel=wheelDelta.exchange(0); int direction=wheel>0?-3:wheel<0?3:0;
 if(direction){dirty=true;lastClickItem=-1;} if(!settingsPage&&inside(27,ListTop,323,ListHeight))pluginScroll=std::clamp(pluginScroll+direction,0,std::max(0,(int)filteredPlugins.size()-ListRows));
 else if(!settingsPage&&inside(369,ListTop,329,ListHeight))itemScroll=std::clamp(itemScroll+direction,0,std::max(0,(int)visible.size()-ListRows));
 if(pressed[VK_TAB]&&!settingsPage){searchScope=searchScope==1?2:1;focus=searchScope;dirty=true;}
 if(pressed[VK_RETURN]&&!focus&&!settingsPage)requestItem();
 static DWORD lastStatusPoll=0;
 if(GetTickCount()-lastStatusPoll>500) {
  lastStatusPoll=GetTickCount();wchar_t result[512]{};GetPrivateProfileStringW(L"Request",L"Result",L"",result,512,bridgePath.c_str());
  if(result[0] && !GetPrivateProfileIntW(L"Request",L"Pending",0,bridgePath.c_str())) {dirty=true;status=narrow(result);WritePrivateProfileStringW(L"Request",L"Result",L"",bridgePath.c_str());}
 }
 if(focus && !loading) {
  auto& target=focus==1?pluginQuery:query; bool changed=false;
  if(pressed[VK_BACK]&&!target.empty()) {target.pop_back();changed=true;}
  BYTE keyboard[256]{}; GetKeyboardState(keyboard); keyboard[VK_SHIFT]=previousKeys[VK_SHIFT]?0x80:0; keyboard[VK_CONTROL]=0;keyboard[VK_MENU]=0;
  for(UINT k=0x20;k<=0xFE;++k) if(pressed[k] && k!=hotkey && !(k>=VK_F1&&k<=VK_F24) && k!=VK_DELETE) {
   wchar_t chars[8]{}; int n=ToUnicode(k,MapVirtualKeyW(k,MAPVK_VK_TO_VSC),keyboard,chars,8,0);
   if(n>0 && chars[0]>=32 && target.size()<120) {auto s=narrow(std::wstring(chars,n)); target+=s;changed=true;}
  }
  if(changed) {if(focus==1)filterPlugins();else filterItems();}
 }
}
using StateFn=HRESULT (STDMETHODCALLTYPE*)(IDirectInputDevice8W*,DWORD,LPVOID);
using DataFn=HRESULT (STDMETHODCALLTYPE*)(IDirectInputDevice8W*,DWORD,LPDIDEVICEOBJECTDATA,LPDWORD,DWORD);
static StateFn originalState;
static DataFn originalData;
static bool blockInput() {return opened || (LONG)(blockUntil.load()-GetTickCount())>0;}
static HRESULT STDMETHODCALLTYPE stateHook(IDirectInputDevice8W* dev,DWORD size,LPVOID data) {
 auto hr=originalState(dev,size,data); if(SUCCEEDED(hr)&&data&&blockInput()) {
  if(opened && (size==sizeof(DIMOUSESTATE)||size==sizeof(DIMOUSESTATE2))) {auto m=(DIMOUSESTATE*)data;mouseDX+=m->lX;mouseDY+=m->lY;wheelDelta+=m->lZ;}
  memset(data,0,size);
 } return hr;
}
static HRESULT STDMETHODCALLTYPE dataHook(IDirectInputDevice8W* dev,DWORD size,LPDIDEVICEOBJECTDATA data,LPDWORD count,DWORD flags) {
 auto hr=originalData(dev,size,data,count,flags); if(SUCCEEDED(hr)&&blockInput()&&count) *count=0; return hr;
}
static bool replaceSlot(void** table,int slot,void* hook,void** original) {
 if(table[slot]==hook)return true;
 DWORD old=0;if(!VirtualProtect(table+slot,sizeof(void*),PAGE_READWRITE,&old))return false;
 *original=table[slot]; InterlockedExchangePointer((PVOID volatile*)(table+slot),hook); DWORD unused;VirtualProtect(table+slot,sizeof(void*),old,&unused);return true;
}
using EndFn=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*);
using ResetFn=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*);
static EndFn originalEnd;static ResetFn originalReset;
static void releaseTexture() {dirty=true;if(texture){texture->Release();texture=nullptr;}renderDevice=nullptr;}
static HRESULT STDMETHODCALLTYPE resetHook(IDirect3DDevice9* dev,D3DPRESENT_PARAMETERS* p) {releaseTexture();return originalReset(dev,p);}
struct Vertex {float x,y,z,rhw;DWORD color;float u,v;};
static void draw(IDirect3DDevice9* dev) {
 if(!opened)return;
 if(renderDevice!=dev){releaseTexture();renderDevice=dev;}
 if(!texture && FAILED(dev->CreateTexture(rasterWidth(),rasterHeight(),1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,nullptr))) {failNative("Could not create browser texture; menu closed to release input");closeBrowser();return;}
 if(dirty) {drawCanvas(); D3DLOCKED_RECT locked{}; if(FAILED(texture->LockRect(0,&locked,nullptr,D3DLOCK_DISCARD)))return;
 for(int y=0;y<rasterHeight();++y)memcpy((char*)locked.pBits+y*locked.Pitch,(char*)pixels+y*rasterWidth()*4,rasterWidth()*4);texture->UnlockRect(0);dirty=false;}
 IDirect3DStateBlock9* block=nullptr;if(FAILED(dev->CreateStateBlock(D3DSBT_ALL,&block)))return;block->Capture();
 D3DVIEWPORT9 viewport{}; dev->GetViewport(&viewport); float scale=std::min(viewport.Width/(float)(Width+30),viewport.Height/(float)(Height+30));
 float x=viewport.X+(viewport.Width-Width*scale)/2.f-0.5f,y=viewport.Y+(viewport.Height-Height*scale)/2.f-0.5f,w=Width*scale,h=Height*scale;
 Vertex v[]={{x,y,0,1,0xFFFFFFFF,0,0},{x+w,y,0,1,0xFFFFFFFF,1,0},{x,y+h,0,1,0xFFFFFFFF,0,1},{x+w,y+h,0,1,0xFFFFFFFF,1,1}};
 dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
 dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
 dev->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);dev->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);dev->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
 dev->SetRenderState(D3DRS_FOGENABLE,FALSE);dev->SetRenderState(D3DRS_LIGHTING,FALSE);dev->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);dev->SetRenderState(D3DRS_COLORWRITEENABLE,15);
 dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);dev->SetTexture(0,texture);dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);dev->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
 dev->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);dev->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);dev->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
 dev->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);dev->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);dev->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);
  dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(Vertex));
 // Pip-Boy-style solid amber arrow: independent geometry follows input every frame.
 dev->SetTexture(0,nullptr);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
 float cx=x+cursorX*scale,cy=y+cursorY*scale;
 Vertex arrow[]={{cx,cy,0,1,0xFFFFC350,0,0},{cx+18*scale,cy+18*scale,0,1,0xFFFFC350,0,0},{cx+7*scale,cy+18*scale,0,1,0xFFFFC350,0,0},{cx,cy+25*scale,0,1,0xFFFFC350,0,0}};
 dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,arrow,sizeof(Vertex));block->Apply();block->Release();
}
static HRESULT STDMETHODCALLTYPE endHook(IDirect3DDevice9* dev) {
 D3DDEVICE_CREATION_PARAMETERS cp{};dev->GetCreationParameters(&cp);
 if(cp.hFocusWindow && IsWindowVisible(cp.hFocusWindow)) {
  if(!frameSeen.exchange(true)){nativeStatus(L"Frames",L"1");logLine("Game EndScene observed; renderer initialised.");}
  gameWindow=cp.hFocusWindow;
  std::lock_guard<std::mutex> guard(catalogMutex);inputs();draw(dev);
 }
 return originalEnd(dev);
}
using CreateDeviceFn=HRESULT (STDMETHODCALLTYPE*)(IDirect3D9*,UINT,D3DDEVTYPE,HWND,DWORD,D3DPRESENT_PARAMETERS*,IDirect3DDevice9**);
using CreateDeviceExFn=HRESULT (STDMETHODCALLTYPE*)(IDirect3D9Ex*,UINT,D3DDEVTYPE,HWND,DWORD,D3DPRESENT_PARAMETERS*,D3DDISPLAYMODEEX*,IDirect3DDevice9Ex**);
using FactoryFn=IDirect3D9* (WINAPI*)(UINT);
using FactoryExFn=HRESULT (WINAPI*)(UINT,IDirect3D9Ex**);
using GetProcFn=FARPROC (WINAPI*)(HMODULE,LPCSTR);
static CreateDeviceFn originalCreateDevice;
static CreateDeviceExFn originalCreateDeviceEx;
static FactoryFn originalFactory;
static FactoryExFn originalFactoryEx;
static GetProcFn originalGetProc;
static void attachRenderer(IDirect3DDevice9* device) {
 if(!device)return;auto table=*(void***)device;
 if(!replaceSlot(table,16,(void*)resetHook,(void**)&originalReset)||!replaceSlot(table,42,(void*)endHook,(void**)&originalEnd)) {failNative("Could not attach game renderer hooks");return;}
 nativeStatus(L"Hooks",L"1");logLine("Captured the game's D3D9 device; EndScene and Reset hooks attached.");
}
static HRESULT STDMETHODCALLTYPE createDeviceHook(IDirect3D9* self,UINT adapter,D3DDEVTYPE type,HWND window,DWORD flags,D3DPRESENT_PARAMETERS* pp,IDirect3DDevice9** device) {
 auto hr=originalCreateDevice(self,adapter,type,window,flags,pp,device);if(SUCCEEDED(hr)&&device)attachRenderer(*device);return hr;
}
static HRESULT STDMETHODCALLTYPE createDeviceExHook(IDirect3D9Ex* self,UINT adapter,D3DDEVTYPE type,HWND window,DWORD flags,D3DPRESENT_PARAMETERS* pp,D3DDISPLAYMODEEX* mode,IDirect3DDevice9Ex** device) {
 auto hr=originalCreateDeviceEx(self,adapter,type,window,flags,pp,mode,device);if(SUCCEEDED(hr)&&device)attachRenderer(*device);return hr;
}
static IDirect3D9* WINAPI factoryHook(UINT version) {
 auto d3d=originalFactory(version);if(d3d)replaceSlot(*(void***)d3d,16,(void*)createDeviceHook,(void**)&originalCreateDevice);return d3d;
}
static HRESULT WINAPI factoryExHook(UINT version,IDirect3D9Ex** out) {
 auto hr=originalFactoryEx(version,out);if(SUCCEEDED(hr)&&out&&*out){auto table=*(void***)*out;replaceSlot(table,16,(void*)createDeviceHook,(void**)&originalCreateDevice);replaceSlot(table,20,(void*)createDeviceExHook,(void**)&originalCreateDeviceEx);}return hr;
}
static FARPROC WINAPI getProcHook(HMODULE mod,LPCSTR name) {
 auto proc=originalGetProc(mod,name);if(!proc || (uintptr_t)name<=0xFFFF)return proc;
 if(!strcmp(name,"Direct3DCreate9")) {originalFactory=(FactoryFn)proc;logLine("Intercepted game Direct3DCreate9 lookup.");return (FARPROC)factoryHook;}
 if(!strcmp(name,"Direct3DCreate9Ex")) {originalFactoryEx=(FactoryExFn)proc;logLine("Intercepted game Direct3DCreate9Ex lookup.");return (FARPROC)factoryExHook;}
 return proc;
}
using InputFactoryFn=HRESULT (WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
using InputDeviceFn=HRESULT (STDMETHODCALLTYPE*)(IDirectInput8W*,REFGUID,IDirectInputDevice8W**,LPUNKNOWN);
static InputFactoryFn originalInputFactory;
static InputDeviceFn originalInputDevice;
static HRESULT STDMETHODCALLTYPE inputDeviceHook(IDirectInput8W* self,REFGUID guid,IDirectInputDevice8W** out,LPUNKNOWN outer) {
 auto hr=originalInputDevice(self,guid,out,outer);
 if(SUCCEEDED(hr)&&out&&*out&&(guid==GUID_SysMouse||guid==GUID_SysKeyboard)) {
  auto table=*(void***)*out;
  if(replaceSlot(table,9,(void*)stateHook,(void**)&originalState)&&replaceSlot(table,10,(void*)dataHook,(void**)&originalData)) {
   inputReady=true;nativeStatus(L"Input",L"1");logLine(guid==GUID_SysMouse?"Captured game mouse input.":"Captured game keyboard input.");
  }else failNative("Could not attach game input hooks");
 }return hr;
}
static HRESULT WINAPI inputFactoryHook(HINSTANCE instance,DWORD version,REFIID iid,LPVOID* out,LPUNKNOWN outer) {
 auto hr=originalInputFactory(instance,version,iid,out,outer);
 if(SUCCEEDED(hr)&&out&&*out)replaceSlot(*(void***)*out,3,(void*)inputDeviceHook,(void**)&originalInputDevice);return hr;
}
static DWORD WINAPI initialize(void*) {
 wchar_t exe[MAX_PATH];GetModuleFileNameW(nullptr,exe,MAX_PATH);root=exe;root=root.substr(0,root.find_last_of(L"\\/")+1);
 iniPath=root+L"Data\\NVSE\\Plugins\\LukesItemBrowser.ini"; config();
 CreateDirectoryW((root+L"Data\\config").c_str(),nullptr);bridgePath=root+L"Data\\config\\LukesItemBrowserRuntime.ini";
 sessionToken=(GetTickCount()%900000)+100000;
 WritePrivateProfileStringW(L"Bridge",L"Session",std::to_wstring(sessionToken).c_str(),bridgePath.c_str());
 WritePrivateProfileStringW(L"Bridge",L"Ready",L"0",bridgePath.c_str());WritePrivateProfileStringW(L"Request",L"Pending",L"0",bridgePath.c_str());
 WritePrivateProfileStringW(L"Audio",L"Menu",L"0",bridgePath.c_str());nativeStatus(L"Frames",L"0");nativeStatus(L"Input",L"0");nativeStatus(L"Hooks",L"0");nativeStatus(L"Error",L"");config();
 auto z=LoadLibraryExW((root+L"Data\\NVSE\\Plugins\\LukesItemBrowser\\zlib1.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);if(z)inflateFn=(ib::Inflate)GetProcAddress(z,"uncompress");
 logLine("Luke's Item Browser 1.0: NVSEPlugin_Load entered; installing game factory hooks.");
 if(!inflateFn)failNative("zlib1.dll could not be loaded; compressed item records unavailable");
 renderScale=(int)setting(L"Display",L"RenderScale",2,1,3);
 if(!createCanvas()){failNative("GDI canvas creation failed");return 1;}
 auto exeModule=GetModuleHandleW(nullptr);
 bool imports=hookImport(exeModule,"GetProcAddress",(void*)getProcHook,(void**)&originalGetProc);
 // Also support hosts that import the factory directly.
 bool direct=hookImport(exeModule,"Direct3DCreate9",(void*)factoryHook,(void**)&originalFactory);
 bool input=hookImport(exeModule,"DirectInput8Create",(void*)inputFactoryHook,(void**)&originalInputFactory);
 if(!imports&&!direct){failNative("Game D3D factory import not found");return 1;}
 if(!input)failNative("Game DirectInput8Create import not found");
 logLine("Factory hooks installed; waiting for the game's renderer and input devices.");
 return 0;
}
extern "C" __declspec(dllexport) bool NVSEPlugin_Query(const NVSEVersionPrefix* nvse,PluginInfo* info) {
 info->infoVersion=1;info->name="Luke's Item Browser";info->version=100;
 return nvse&&!nvse->isEditor&&nvse->runtimeVersion==0x040020D0&&nvse->nvseVersion>=6;
}
extern "C" __declspec(dllexport) bool NVSEPlugin_Load(const void*) {return initialize(nullptr)==0;}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {if(reason==DLL_PROCESS_ATTACH){moduleHandle=instance;DisableThreadLibraryCalls(instance);}return TRUE;}










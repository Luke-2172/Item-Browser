#include "browser.cpp"
#include <chrono>
#include <iostream>
int main() {
 {
  wchar_t temp[MAX_PATH]{},file[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);
  if(!GetTempFileNameW(temp,L"LAB",0,file))return (40);
  if(!CopyFileW(L"package/NVSE/Plugins/LukesItemBrowser/Fonts/ShareTechMono-Regular.ttf",file,FALSE))return (41);
  VectorFont memoryFont;memoryFont.load(file,L"Share Tech Mono",FW_NORMAL);
  if(!DeleteFileW(file))return (42);
  std::vector<uint32_t> out(400*80,0);
  memoryFont.draw(out.data(),400,80,0,0,400,80,"Memory font",37,RGB(255,255,255));
  if(std::none_of(out.begin(),out.end(),[](uint32_t p){return p!=0;}))return (43);
  VectorFont missing;loadMenuFont(missing,file,L"Share Tech Mono",FW_NORMAL,L"Consolas");
  std::fill(out.begin(),out.end(),0);
  missing.draw(out.data(),400,80,0,0,400,80,"Fallback",37,RGB(255,255,255));
  if(std::none_of(out.begin(),out.end(),[](uint32_t p){return p!=0;}))return (44);
  std::cout<<"PASS: memory font renders after file deletion; missing font uses fallback"<<std::endl;
 }

 loadMenuFonts(L"package/NVSE/Plugins/LukesItemBrowser/Fonts/");
 renderScale=2;if(!createCanvas())return 1;
 plugins={L"Test.esm"};pluginIndex=0;filteredPlugins={0};selected=0;
 for(int n=0;n<100;++n){catalog.items.push_back(ib::Item{(uint32_t)n,"WEAP","Fixture item "+std::to_string(n),"Fixture",false});visible.push_back(n);}
 drawCanvas();auto builds=backdropBuilds;
 std::vector<uint32_t> first((uint32_t*)pixels,(uint32_t*)pixels+(size_t)rasterWidth()*rasterHeight());
 dirty=false;listDirty=0;cursorX=400;cursorY=220;
 scrollWheel(120);if(dirty||listDirty||itemScroll!=0)return 2;
 scrollWheel(-60);if(listDirty)return 3;scrollWheel(-60);if(itemScroll!=3||listDirty!=2)return 4;
 redrawLists(listDirty);std::vector<uint32_t> partial((uint32_t*)pixels,(uint32_t*)pixels+first.size());
 drawCanvas();if(memcmp(partial.data(),pixels,partial.size()*4))return 5;
 if(backdropBuilds!=builds)return 6;
 for(int y=0;y<rasterHeight();++y)for(int x=0;x<rasterWidth();++x){
  if(x>=365*renderScale&&x<702*renderScale&&y>=ListTop*renderScale&&y<(ListTop+ListHeight)*renderScale)continue;
  size_t i=(size_t)y*rasterWidth()+x;if(partial[i]!=first[i])return 7;
 }
 listDirty=0;cursorX=900;scrollWheel(-120);if(listDirty)return 8;
 auto time=[&](bool full){auto start=std::chrono::steady_clock::now();for(int n=0;n<20;++n){itemScroll=n;if(full)drawCanvas();else redrawLists(2);}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/20;};
 std::cout<<"Canvas mean ms: full cached="<<time(true)<<"; item-list-only="<<time(false)<<"\n";
 std::cout<<"PASS: partial redraw equals full redraw, static pixels unchanged, no-op wheel and high-resolution wheel accumulation\n";
 destroyCanvas();return 0;
}

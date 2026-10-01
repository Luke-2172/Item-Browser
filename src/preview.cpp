#include "browser.cpp"
int wmain(int argc,wchar_t** argv) {
 if(argc!=4&&argc!=5)return 2;settingsPage=argc==5;
 auto z=LoadLibraryW(argv[1]);inflateFn=(ib::Inflate)GetProcAddress(z,"uncompress");
 catalog=ib::readPlugin(argv[2],inflateFn);plugins={L"FalloutNV.esm",L"DeadMoney.esm",L"HonestHearts.esm",L"OldWorldBlues.esm",L"LonesomeRoad.esm",L"GunRunnersArsenal.esm"};
 pluginIndex=0;livePlugins=true;category=1;filterPlugins();filterItems();for(size_t n=0;n<visible.size();++n)if(catalog.items[visible[n]].editor=="WeapNV9mmPistol"){selected=(int)n;break;}status="Layout preview rendered from real FalloutNV.esm records. Not an in-game screenshot.";
 if(!createCanvas())return 3;
 drawCanvas();BITMAPFILEHEADER header{};header.bfType=0x4D42;header.bfOffBits=sizeof(header)+40;header.bfSize=header.bfOffBits+rasterWidth()*rasterHeight()*4;
 std::ofstream out(argv[3],std::ios::binary);out.write((char*)&header,sizeof(header));BITMAPINFOHEADER bi{};bi.biSize=40;bi.biWidth=rasterWidth();bi.biHeight=-rasterHeight();bi.biPlanes=1;bi.biBitCount=32;out.write((char*)&bi,40);out.write((char*)pixels,rasterWidth()*rasterHeight()*4);return out?0:1;
}





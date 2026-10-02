#include "browser.cpp"
#include <iostream>
struct Object {void** table;};
static void* deviceTable[134]{};
static void* surfaceTable[15]{};
static void* textureTable[22]{};
static Object device{deviceTable},back{surfaceTable},shadow{surfaceTable},other{surfaceTable},depth{surfaceTable},upload{surfaceTable},destination{surfaceTable},tex{textureTable};
static IDirect3DSurface9* rt[2]={(IDirect3DSurface9*)&shadow,(IDirect3DSurface9*)&other};
static IDirect3DSurface9* ds=(IDirect3DSurface9*)&depth;
static D3DVIEWPORT9 vp{3,4,128,128,0.f,1.f};
static bool rejectBack=false;static int releases=0,updates=0;static size_t copied=0;
static std::vector<uint32_t> staging,gpu;
static ULONG STDMETHODCALLTYPE release(void*){++releases;return 1;}
static HRESULT STDMETHODCALLTYPE caps(void*,D3DCAPS9* p){*p={};p->NumSimultaneousRTs=2;return S_OK;}
static HRESULT STDMETHODCALLTYPE getBack(void*,UINT,UINT,D3DBACKBUFFER_TYPE,IDirect3DSurface9** p){*p=(IDirect3DSurface9*)&back;return S_OK;}
static HRESULT STDMETHODCALLTYPE getRT(void*,DWORD i,IDirect3DSurface9** p){*p=rt[i];return *p?S_OK:D3DERR_NOTFOUND;}
static HRESULT STDMETHODCALLTYPE setRT(void*,DWORD i,IDirect3DSurface9* p){if(rejectBack&&p==(IDirect3DSurface9*)&back)return D3DERR_INVALIDCALL;rt[i]=p;return S_OK;}
static HRESULT STDMETHODCALLTYPE getDS(void*,IDirect3DSurface9** p){*p=ds;return ds?S_OK:D3DERR_NOTFOUND;}
static HRESULT STDMETHODCALLTYPE setDS(void*,IDirect3DSurface9* p){ds=p;return S_OK;}
static HRESULT STDMETHODCALLTYPE getVP(void*,D3DVIEWPORT9* p){*p=vp;return S_OK;}
static HRESULT STDMETHODCALLTYPE setVP(void*,const D3DVIEWPORT9* p){vp=*p;return S_OK;}
static HRESULT STDMETHODCALLTYPE desc(void*,D3DSURFACE_DESC* p){*p={};p->Width=1920;p->Height=1080;return S_OK;}
static HRESULT STDMETHODCALLTYPE createUpload(void*,UINT w,UINT h,D3DFORMAT,D3DPOOL pool,IDirect3DSurface9** p,HANDLE*){
 if(pool!=D3DPOOL_SYSTEMMEM)return D3DERR_INVALIDCALL;staging.resize((size_t)w*h);*p=(IDirect3DSurface9*)&upload;return S_OK;
}
static HRESULT STDMETHODCALLTYPE getSurface(void*,UINT,IDirect3DSurface9** p){*p=(IDirect3DSurface9*)&destination;return S_OK;}
static HRESULT STDMETHODCALLTYPE lockSurface(void*,D3DLOCKED_RECT* p,const RECT* r,DWORD){p->Pitch=rasterWidth()*4;p->pBits=staging.data()+(size_t)r->top*rasterWidth()+r->left;return S_OK;}
static HRESULT STDMETHODCALLTYPE unlockSurface(void*){return S_OK;}
static HRESULT STDMETHODCALLTYPE update(void*,IDirect3DSurface9*,const RECT* r,IDirect3DSurface9*,const POINT* p){
 if(p->x!=r->left||p->y!=r->top)return D3DERR_INVALIDCALL;
 ++updates;copied+=(size_t)(r->right-r->left)*(r->bottom-r->top)*4;
 for(int y=r->top;y<r->bottom;++y)memcpy(gpu.data()+(size_t)y*rasterWidth()+r->left,staging.data()+(size_t)y*rasterWidth()+r->left,(r->right-r->left)*4);
 return S_OK;
}
int main(){
 deviceTable[7]=(void*)caps;deviceTable[18]=(void*)getBack;deviceTable[37]=(void*)setRT;deviceTable[38]=(void*)getRT;
 deviceTable[39]=(void*)setDS;deviceTable[40]=(void*)getDS;deviceTable[47]=(void*)setVP;deviceTable[48]=(void*)getVP;
 surfaceTable[2]=(void*)release;surfaceTable[12]=(void*)desc;
 auto d=(IDirect3DDevice9*)&device;auto before=vp;
 {BackbufferScope s(d);if(FAILED(s.bind())||rt[0]!=(IDirect3DSurface9*)&back||rt[1]||ds||vp.Width!=1920)return 1;}
 if(rt[0]!=(IDirect3DSurface9*)&shadow||rt[1]!=(IDirect3DSurface9*)&other||ds!=(IDirect3DSurface9*)&depth||memcmp(&vp,&before,sizeof(vp))||releases!=4)return 2;
 rejectBack=true;{BackbufferScope s(d);if(SUCCEEDED(s.bind()))return 3;}
 if(rt[0]!=(IDirect3DSurface9*)&shadow||rt[1]!=(IDirect3DSurface9*)&other||ds!=(IDirect3DSurface9*)&depth||memcmp(&vp,&before,sizeof(vp))||releases!=8)return 4;
 renderScale=2;if(!createCanvas())return 5;
 deviceTable[36]=(void*)createUpload;deviceTable[30]=(void*)update;textureTable[18]=(void*)getSurface;
 surfaceTable[13]=(void*)lockSurface;surfaceTable[14]=(void*)unlockSurface;texture=(IDirect3DTexture9*)&tex;
 size_t size=(size_t)rasterWidth()*rasterHeight();gpu.resize(size);std::fill_n((uint32_t*)pixels,size,0xFF123456u);
 if(FAILED(uploadCanvas(d,true,0))||updates!=1||copied!=size*4||memcmp(pixels,gpu.data(),size*4))return 6;
 std::fill_n((uint32_t*)pixels,size,0xFFABCDEFu);copied=0;
 if(FAILED(uploadCanvas(d,false,2))||updates!=2||copied!=(size_t)337*ListHeight*renderScale*renderScale*4)return 7;
 for(int y=0;y<rasterHeight();++y)for(int x=0;x<rasterWidth();++x){bool region=x>=365*renderScale&&x<702*renderScale&&y>=ListTop*renderScale&&y<(ListTop+ListHeight)*renderScale;
  if(gpu[(size_t)y*rasterWidth()+x]!=(region?0xFFABCDEFu:0xFF123456u))return 8;
 }
 std::cout<<"PASS: backbuffer selection, MRT/depth/viewport restoration including failed bind; exact partial upload region and byte count\n";
 texture=nullptr;uploadSurface=nullptr;destroyCanvas();return 0;
}

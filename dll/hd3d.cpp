//#define D3D10_IGNORE_SDK_LAYERS 1
#include <windows.h>
#include <d3d9.h>
#include <D3D10_1.h>
#include <D3D10Misc.h>
#include <D3D11.h>
#include "dxwnd.h"
#include "dxwcore.hpp"

typedef void* (WINAPI *Direct3DCreate8_Type)(UINT);
typedef void* (WINAPI *Direct3DCreate9_Type)(UINT);
typedef UINT (WINAPI *GetAdapterCount_Type)(void *);
typedef HRESULT (WINAPI *CreateDevice_Type)(void *, UINT, D3DDEVTYPE, HWND, DWORD, void *, void **);
typedef HRESULT (WINAPI *EnumAdapterModes8_Type)(void *, UINT, UINT, D3DDISPLAYMODE *);
typedef HRESULT (WINAPI *EnumAdapterModes9_Type)(void *, UINT, D3DFORMAT ,UINT, D3DDISPLAYMODE *);
typedef HRESULT (WINAPI *GetAdapterDisplayMode_Type)(void *, UINT, D3DDISPLAYMODE *);
typedef HRESULT (WINAPI *GetDisplayMode_Type)(void *, D3DDISPLAYMODE *);
typedef HRESULT (WINAPI *Present_Type)(void *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *);
typedef HRESULT (WINAPI *SetRenderState_Type)(void *, D3DRENDERSTATETYPE, DWORD);
typedef HRESULT (WINAPI *GetRenderState_Type)(void *, D3DRENDERSTATETYPE, DWORD );

typedef HRESULT (WINAPI *D3D10CreateDevice_Type)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, ID3D10Device **);
typedef HRESULT (WINAPI *D3D10CreateDeviceAndSwapChain_Type)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D10Device **);
typedef HRESULT (WINAPI *D3D10CreateDevice1_Type)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, D3D10_FEATURE_LEVEL1, UINT, ID3D10Device **);
typedef HRESULT (WINAPI *D3D10CreateDeviceAndSwapChain1_Type)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D10Device **);
typedef HRESULT (WINAPI *D3D11CreateDevice_Type)(IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL *, UINT, UINT, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext **);
typedef HRESULT (WINAPI *D3D11CreateDeviceAndSwapChain_Type)(IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL *, UINT, UINT, const DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext **);

void* WINAPI extDirect3DCreate8(UINT);
void* WINAPI extDirect3DCreate9(UINT);
UINT WINAPI extGetAdapterCount(void *);
HRESULT WINAPI extCreateDevice(void *, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS *, void **);
HRESULT WINAPI extEnumAdapterModes8(void *, UINT, UINT , D3DDISPLAYMODE *);
HRESULT WINAPI extEnumAdapterModes9(void *, UINT, D3DFORMAT, UINT , D3DDISPLAYMODE *);
HRESULT WINAPI extGetAdapterDisplayMode(void *, UINT, D3DDISPLAYMODE *);
HRESULT WINAPI extGetDisplayMode(void *, D3DDISPLAYMODE *);
HRESULT WINAPI extPresent(void *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *);
HRESULT WINAPI extSetRenderState(void *, D3DRENDERSTATETYPE, DWORD);
HRESULT WINAPI extGetRenderState(void *, D3DRENDERSTATETYPE, DWORD);

HRESULT WINAPI extD3D10CreateDevice(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, ID3D10Device **);
HRESULT WINAPI extD3D10CreateDeviceAndSwapChain(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D10Device **);
HRESULT WINAPI extD3D10CreateDevice1(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, D3D10_FEATURE_LEVEL1, UINT, ID3D10Device **);
HRESULT WINAPI extD3D10CreateDeviceAndSwapChain1(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D10Device **);
HRESULT WINAPI extD3D11CreateDevice(IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL *, UINT, UINT, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext **);
HRESULT WINAPI extD3D11CreateDeviceAndSwapChain(IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL *, UINT, UINT, const DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext **);

extern char *ExplainDDError(DWORD);

Direct3DCreate8_Type pDirect3DCreate8 = 0;
Direct3DCreate9_Type pDirect3DCreate9 = 0;
GetAdapterCount_Type pGetAdapterCount;
CreateDevice_Type pCreateDevice;
EnumAdapterModes8_Type pEnumAdapterModes8;
EnumAdapterModes9_Type pEnumAdapterModes9;
GetAdapterDisplayMode_Type pGetAdapterDisplayMode;
GetDisplayMode_Type pGetDisplayMode;
Present_Type pPresent;
SetRenderState_Type pSetRenderState;
GetRenderState_Type pGetRenderState;

D3D10CreateDevice_Type pD3D10CreateDevice;
D3D10CreateDeviceAndSwapChain_Type pD3D10CreateDeviceAndSwapChain;
D3D10CreateDevice1_Type pD3D10CreateDevice1;
D3D10CreateDeviceAndSwapChain1_Type pD3D10CreateDeviceAndSwapChain1;
D3D11CreateDevice_Type pD3D11CreateDevice;
D3D11CreateDeviceAndSwapChain_Type pD3D11CreateDeviceAndSwapChain;

DWORD dwD3DVersion;

int HookDirect3D(HMODULE module, int version){
	HINSTANCE hinst;
	void *tmp;
	LPDIRECT3D9 lpd3d;
	ID3D10Device *lpd3d10;
	ID3D11Device *lpd3d11;
	HRESULT res;

	switch(version){
	case 0:
		// D3D8
		tmp = HookAPI(module, "d3d8.dll", NULL, "Direct3DCreate8", extDirect3DCreate8);
		if(tmp) pDirect3DCreate8 = (Direct3DCreate8_Type)tmp;
		// D3D9
		tmp = HookAPI(module, "d3d9.dll", NULL, "Direct3DCreate9", extDirect3DCreate9);
		if(tmp) pDirect3DCreate9 = (Direct3DCreate9_Type)tmp;
		// D3D10
		tmp = HookAPI(module, "d3d10.dll", NULL, "D3D10CreateDevice", extD3D10CreateDevice);
		if(tmp) pD3D10CreateDevice = (D3D10CreateDevice_Type)tmp;
		tmp = HookAPI(module, "d3d10.dll", NULL, "D3D10CreateDeviceAndSwapChain", extD3D10CreateDeviceAndSwapChain);
		if(tmp) pD3D10CreateDeviceAndSwapChain = (D3D10CreateDeviceAndSwapChain_Type)tmp;
		// D3D10.1
		tmp = HookAPI(module, "d3d10_1.dll", NULL, "D3D10CreateDevice1", extD3D10CreateDevice);
		if(tmp) pD3D10CreateDevice1 = (D3D10CreateDevice1_Type)tmp;
		tmp = HookAPI(module, "d3d10_1.dll", NULL, "D3D10CreateDeviceAndSwapChain1", extD3D10CreateDeviceAndSwapChain);
		if(tmp) pD3D10CreateDeviceAndSwapChain1 = (D3D10CreateDeviceAndSwapChain1_Type)tmp;
		// D3D11
		tmp = HookAPI(module, "d3d11.dll", NULL, "D3D11CreateDevice", extD3D11CreateDevice);
		if(tmp) pD3D11CreateDevice = (D3D11CreateDevice_Type)tmp;
		tmp = HookAPI(module, "d3d11.dll", NULL, "D3D11CreateDeviceAndSwapChain", extD3D11CreateDeviceAndSwapChain);
		if(tmp) pD3D11CreateDeviceAndSwapChain = (D3D11CreateDeviceAndSwapChain_Type)tmp;
		break;
	case 8:
		hinst = LoadLibrary("d3d8.dll");
		pDirect3DCreate8 =
			(Direct3DCreate8_Type)GetProcAddress(hinst, "Direct3DCreate8");
		if(pDirect3DCreate8){
			lpd3d = (LPDIRECT3D9)extDirect3DCreate8(220);
			if(lpd3d) lpd3d->Release();
		}
		break;
	case 9:
		hinst = LoadLibrary("d3d9.dll");
		pDirect3DCreate9 =
			(Direct3DCreate9_Type)GetProcAddress(hinst, "Direct3DCreate9");
		if(pDirect3DCreate9){
			lpd3d = (LPDIRECT3D9)extDirect3DCreate9(31);
			if(lpd3d) lpd3d->Release();
		}
		break;
	case 10:
		hinst = LoadLibrary("d3d10.dll");
		pD3D10CreateDevice =
			(D3D10CreateDevice_Type)GetProcAddress(hinst, "D3D10CreateDevice");
		if(pD3D10CreateDevice){
			res = extD3D10CreateDevice(
				NULL,
				D3D10_DRIVER_TYPE_HARDWARE,
				NULL,
				0,
				D3D10_SDK_VERSION,
				&lpd3d10);
			if(res==DD_OK) lpd3d10->Release();
		}
		hinst = LoadLibrary("d3d10_1.dll");
		pD3D10CreateDevice1 =
			(D3D10CreateDevice1_Type)GetProcAddress(hinst, "D3D10CreateDevice1");
		break;
		if(pD3D10CreateDevice1){
			res = extD3D10CreateDevice1(
				NULL,
				D3D10_DRIVER_TYPE_HARDWARE,
				NULL,
				0,
				D3D10_FEATURE_LEVEL_10_1,
				D3D10_SDK_VERSION,
				&lpd3d10);
			if(res==DD_OK) lpd3d10->Release();
		}
		pD3D10CreateDeviceAndSwapChain =
			(D3D10CreateDeviceAndSwapChain_Type)GetProcAddress(hinst, "D3D10CreateDeviceAndSwapChain");
		if(pD3D10CreateDeviceAndSwapChain){
			DXGI_SWAP_CHAIN_DESC swapChainDesc;
			IDXGISwapChain *pSwapChain;
			ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
			 
			//set buffer dimensions and format
			swapChainDesc.BufferCount = 2;
			swapChainDesc.BufferDesc.Width = dxw.GetScreenWidth();
			swapChainDesc.BufferDesc.Height = dxw.GetScreenHeight();
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;;
			 
			//set refresh rate
			swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
			swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
			 
			//sampling settings
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.SampleDesc.Count = 1;
			 
			//output window handle
			swapChainDesc.OutputWindow = dxw.GethWnd();
			swapChainDesc.Windowed = true;	
			res = extD3D10CreateDeviceAndSwapChain( 
				NULL,
                D3D10_DRIVER_TYPE_HARDWARE,
                NULL,
                0,
                D3D10_SDK_VERSION,
                &swapChainDesc,
                &pSwapChain,
                &lpd3d10);
			if(res==DD_OK) lpd3d10->Release();
		}
	break;
	case 11:
		hinst = LoadLibrary("d3d11.dll");
		pD3D11CreateDevice =
			(D3D11CreateDevice_Type)GetProcAddress(hinst, "D3D11CreateDevice");
		if(pD3D11CreateDevice){
			D3D_FEATURE_LEVEL FeatureLevel;
			ID3D11DeviceContext *pImmediateContext;
			res = extD3D11CreateDevice(
				NULL,
				D3D_DRIVER_TYPE_HARDWARE,
				NULL,
				0, // flags
				NULL, // FeatureLevels
				0,
				D3D11_SDK_VERSION,
				&lpd3d11,
				&FeatureLevel,
				&pImmediateContext);
			if(res==DD_OK) lpd3d11->Release();
		}
		pD3D11CreateDeviceAndSwapChain =
			(D3D11CreateDeviceAndSwapChain_Type)GetProcAddress(hinst, "D3D11CreateDeviceAndSwapChain");
		break;
	}
	if(pDirect3DCreate8 || pDirect3DCreate9) return 1;
	return 0;
}

void* WINAPI extDirect3DCreate8(UINT sdkversion)
{
	void *lpd3d;

	dwD3DVersion = 8;
	lpd3d = (*pDirect3DCreate8)(sdkversion);
	if(!lpd3d) return 0;
	//OutTrace("DEBUG: Hooking lpd3d=%x\n", lpd3d);
	//OutTrace("DEBUG: Hooking %x -> %x as CreateDevice(D8)\n", (void *)(*(DWORD *)lpd3d + 60), extCreateDevice);
	SetHook((void *)(*(DWORD *)lpd3d + 16), extGetAdapterCount, (void **)&pGetAdapterCount, "GetAdapterCount(D8)");
	SetHook((void *)(*(DWORD *)lpd3d + 28), extEnumAdapterModes8, (void **)&pEnumAdapterModes8, "EnumAdapterModes(D8)");
	SetHook((void *)(*(DWORD *)lpd3d + 32), extGetAdapterDisplayMode, (void **)&pGetAdapterDisplayMode, "GetAdapterDisplayMode(D8)");
	SetHook((void *)(*(DWORD *)lpd3d + 60), extCreateDevice, (void **)&pCreateDevice, "CreateDevice(D8)");

	OutTraceD("Direct3DCreate8: SDKVERSION=%x pCreateDevice=%x\n",
		sdkversion, pCreateDevice);
	return lpd3d;
}

void* WINAPI extDirect3DCreate9(UINT sdkversion)
{
	void *lpd3d;

	dwD3DVersion = 9;
	lpd3d = (*pDirect3DCreate9)(sdkversion);
	if(!lpd3d) return 0;
	SetHook((void *)(*(DWORD *)lpd3d + 16), extGetAdapterCount, (void **)&pGetAdapterCount, "GetAdapterCount(D9)");
	SetHook((void *)(*(DWORD *)lpd3d + 28), extEnumAdapterModes9, (void **)&pEnumAdapterModes9, "EnumAdapterModes(D9)");
	SetHook((void *)(*(DWORD *)lpd3d + 32), extGetAdapterDisplayMode, (void **)&pGetAdapterDisplayMode, "GetAdapterDisplayMode(D9)");
	SetHook((void *)(*(DWORD *)lpd3d + 64), extCreateDevice, (void **)&pCreateDevice, "CreateDevice(D9)");

	OutTraceD("Direct3DCreate9: SDKVERSION=%x pCreateDevice=%x\n",
		sdkversion, pCreateDevice);

	return lpd3d;
}

UINT WINAPI extGetAdapterCount(void *lpd3d)
{
	UINT res;
	res=(*pGetAdapterCount)(lpd3d);
	OutTraceD("GetAdapterCount: count=%d\n", res);
	if(dxw.dwFlags2 & HIDEMULTIMONITOR) {
		OutTraceD("GetAdapterCount: HIDEMULTIMONITOR count=1\n");
		res=1;
	}
	return res;
}

HRESULT WINAPI extReset(void *pd3dd, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	OutTraceD("DEBUG: neutralizing pd3dd->Reset()\n");
	return D3D_OK;
}

HRESULT WINAPI extPresent(void *pd3dd, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion)
{
	if (IsDebug) OutTrace("Present\n");
	// frame counter handling....
	if (dxw.HandleFPS()) return D3D_OK;
	// proxy ....
	return (*pPresent)(pd3dd, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT WINAPI extGetDisplayMode(void *lpd3d, D3DDISPLAYMODE *pMode)
{
	HRESULT res;
	res=(*pGetDisplayMode)(lpd3d, pMode);
	OutTraceD("DEBUG: GetDisplayMode: size=(%dx%d) RefreshRate=%d Format=%d\n",
		pMode->Width, pMode->Height, pMode->RefreshRate, pMode->Format);
	if(dxw.dwFlags2 & KEEPASPECTRATIO){
		pMode->Width=dxw.iSizX;
		pMode->Height=dxw.iSizY;
		OutTraceD("DEBUG: GetDisplayMode: fixed size=(%dx%d)\n", pMode->Width, pMode->Height);
	}
	return res;
}

HRESULT WINAPI extEnumAdapterModes8(void *lpd3d, UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode)
{
	HRESULT res;
	OutTraceD("EnumAdapterModes(8): adapter=%d mode=%d pMode=%x\n", Adapter, Mode, pMode);
	res=(*pEnumAdapterModes8)(lpd3d, Adapter, Mode, pMode);
	if(res) OutTraceE("EnumAdapterModes ERROR: err=%x(%s)\n", res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extEnumAdapterModes9(void *lpd3d, UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode)
{
	HRESULT res;
	OutTraceD("EnumAdapterModes(9): adapter=%d format=%x mode=%d pMode=%x\n", Adapter, Format, Mode, pMode);
	res=(*pEnumAdapterModes9)(lpd3d, Adapter, Format, Mode, pMode);
	if(res) OutTraceE("EnumAdapterModes ERROR: err=%x(%s)\n", res, ExplainDDError(res));
	return res;
}

HRESULT WINAPI extGetAdapterDisplayMode(void *lpd3d, UINT Adapter, D3DDISPLAYMODE *pMode)
{
	HRESULT res;
	res=(*pGetAdapterDisplayMode)(lpd3d, Adapter, pMode);
	OutTraceD("DEBUG: GetAdapterDisplayMode: size=(%dx%d) RefreshRate=%d Format=%d\n",
		pMode->Width, pMode->Height, pMode->RefreshRate, pMode->Format);
	if(dxw.dwFlags2 & KEEPASPECTRATIO){
		pMode->Width=dxw.iSizX;
		pMode->Height=dxw.iSizY;
		OutTraceD("DEBUG: GetDisplayMode: fixed size=(%dx%d)\n", pMode->Width, pMode->Height);
	}
	return res;
}

HRESULT WINAPI extCreateDevice(void *lpd3d, UINT adapter, D3DDEVTYPE devicetype,
	HWND hfocuswindow, DWORD behaviorflags, D3DPRESENT_PARAMETERS *ppresentparam, void **ppd3dd)
{
	HRESULT res;
	DWORD param[64], *tmp;
	D3DDISPLAYMODE mode;
	int Windowed;

	memcpy(param, ppresentparam, (dwD3DVersion == 9)?56:52);
	dxw.SethWnd(hfocuswindow);
	dxw.SetScreenSize(param[0], param[1]);
	AdjustWindowFrame(dxw.GethWnd(), dxw.GetScreenWidth(), dxw.GetScreenHeight());

	tmp = param;
	OutTraceD("D3D%d::CreateDevice\n", dwD3DVersion);
	OutTraceD("  Adapter = %i\n", adapter);
	OutTraceD("  DeviceType = %i\n", devicetype);
	OutTraceD("  hFocusWindow = 0x%x\n", hfocuswindow);
	OutTraceD("  BehaviorFlags = 0x%x\n", behaviorflags);
	OutTraceD("    BackBufferWidth = %i\n", *(tmp ++));
	OutTraceD("    BackBufferHeight = %i\n", *(tmp ++));
	OutTraceD("    BackBufferFormat = %i\n", *(tmp ++));
	OutTraceD("    BackBufferCount = %i\n", *(tmp ++));
	OutTraceD("    MultiSampleType = %i\n", *(tmp ++));
	if(dwD3DVersion == 9) OutTraceD("    MultiSampleQuality = %i\n", *(tmp ++));
	OutTraceD("    SwapEffect = 0x%x\n", *(tmp ++));
	OutTraceD("    hDeviceWindow = 0x%x\n", *(tmp ++));
	OutTraceD("    Windowed = %i\n", (Windowed=*(tmp ++)));
	OutTraceD("    EnableAutoDepthStencil = %i\n", *(tmp ++));
	OutTraceD("    AutoDepthStencilFormat = %i\n", *(tmp ++));
	OutTraceD("    Flags = 0x%x\n", *(tmp ++));
	OutTraceD("    FullScreen_RefreshRateInHz = %i\n", *(tmp ++));
	OutTraceD("    PresentationInterval = 0x%x\n", *(tmp ++));

	//((LPDIRECT3D9)lpd3d)->GetAdapterDisplayMode(0, &mode);
	(*pGetAdapterDisplayMode)(lpd3d, 0, &mode);
	param[2] = mode.Format;
	OutTraceD("    Current Format = 0x%x\n", mode.Format);

	// useless ...
	//param[0] = 0;			//defaulting to window width
	//param[1] = 0;			//defaulting to window height
	//param[2] = D3DFMT_UNKNOWN;	// try

	if(dwD3DVersion == 9){
		param[7] = 0;			//hDeviceWindow
		dxw.SetFullScreen(~param[8]?TRUE:FALSE); 
		param[8] = 1;			//Windowed
		param[12] = 0;			//FullScreen_RefreshRateInHz;
		param[13] = D3DPRESENT_INTERVAL_DEFAULT;	//PresentationInterval
	}
	else{
		param[6] = 0;			//hDeviceWindow
		dxw.SetFullScreen(~param[7]?TRUE:FALSE); 
		param[7] = 1;			//Windowed
		param[11] = 0;			//FullScreen_RefreshRateInHz;
		param[12] = D3DPRESENT_INTERVAL_DEFAULT;	//PresentationInterval
	}

	res = (*pCreateDevice)(lpd3d, 0, devicetype, hfocuswindow, behaviorflags, param, ppd3dd);
	if(res){
		OutTraceD("FAILED! %x\n", res);
		return res;
	}
	OutTraceD("SUCCESS!\n");

	if(dwD3DVersion == 8){
		void *pReset;
		pReset=NULL; // to avoid assert condition
		SetHook((void *)(*(DWORD *)lpd3d + 32), extGetDisplayMode, (void **)&pGetDisplayMode, "GetDisplayMode(D8)");
		SetHook((void *)(**(DWORD **)ppd3dd + 56), extReset, (void **)&pReset, "Reset(D8)");
		SetHook((void *)(**(DWORD **)ppd3dd + 60), extPresent, (void **)&pPresent, "Present(D8)");
		if(dxw.dwFlags2 & WIREFRAME){
			SetHook((void *)(**(DWORD **)ppd3dd + 200), extSetRenderState, (void **)&pSetRenderState, "SetRenderState(D9)");
			SetHook((void *)(**(DWORD **)ppd3dd + 204), extGetRenderState, (void **)&pGetRenderState, "GetRenderState(D9)");
			(*pSetRenderState)((void *)*ppd3dd, D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		}
	}
	else {
		void *pReset;
		pReset=NULL; // to avoid assert condition
		SetHook((void *)(*(DWORD *)lpd3d + 32), extGetDisplayMode, (void **)&pGetDisplayMode, "GetDisplayMode(D9)");
		SetHook((void *)(**(DWORD **)ppd3dd + 64), extReset, (void **)&pReset, "Reset(D9)");
		SetHook((void *)(**(DWORD **)ppd3dd + 68), extPresent, (void **)&pPresent, "Present(D9)");
		if(dxw.dwFlags2 & WIREFRAME){
			SetHook((void *)(**(DWORD **)ppd3dd + 228), extSetRenderState, (void **)&pSetRenderState, "SetRenderState(D9)");
			SetHook((void *)(**(DWORD **)ppd3dd + 232), extGetRenderState, (void **)&pGetRenderState, "GetRenderState(D9)");
			(*pSetRenderState)((void *)*ppd3dd, D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		}
	}

	GetHookInfo()->IsFullScreen = dxw.IsFullScreen();
	GetHookInfo()->DXVersion=(short)dwD3DVersion;
	GetHookInfo()->Height=(short)dxw.GetScreenHeight();
	GetHookInfo()->Width=(short)dxw.GetScreenWidth();
	GetHookInfo()->ColorDepth=(short)dxw.VirtualPixelFormat.dwRGBBitCount;
	
	return 0;
}

HRESULT WINAPI extSetRenderState(void *pd3dd, D3DRENDERSTATETYPE State, DWORD Value) 
{
	if (State == D3DRS_FILLMODE) Value=D3DFILL_WIREFRAME;
	return (*pSetRenderState)(pd3dd, State, Value);
}

HRESULT WINAPI extGetRenderState(void *pd3dd, D3DRENDERSTATETYPE State, DWORD Value) 
{
	return (*pGetRenderState)(pd3dd, State, Value);
}

// to do:
// hook IDirect3DDevice9::CreateAdditionalSwapChain to intercept Present method on further Swap Chains
// hook SetCursorPosition ShowCursor to handle cursor


HRESULT WINAPI extD3D10CreateDevice(
	IDXGIAdapter *pAdapter, 
	D3D10_DRIVER_TYPE DriverType, 
	HMODULE Software, 
	UINT Flags, 
	UINT SDKVersion, 
	ID3D10Device **ppDevice)
{
	HRESULT res;
	OutTraceD("D3D10CreateDevice\n");
	res=(*pD3D10CreateDevice)(pAdapter, DriverType, Software, Flags, SDKVersion, ppDevice);
	OutTraceD("D3D10CreateDevice ret=%x\n", res);
	return res;
}

HRESULT WINAPI extD3D10CreateDevice1(
	IDXGIAdapter *pAdapter, 
	D3D10_DRIVER_TYPE DriverType, 
	HMODULE Software, 
	UINT Flags, 
	D3D10_FEATURE_LEVEL1 HardwareLevel,
	UINT SDKVersion, 
	ID3D10Device **ppDevice)
{
	HRESULT res;
	OutTraceD("D3D10CreateDevice1\n");
	res=(*pD3D10CreateDevice1)(pAdapter, DriverType, Software, Flags, HardwareLevel, SDKVersion, ppDevice);
	OutTraceD("D3D10CreateDevice1 ret=%x\n", res);
	return res;
}

HRESULT WINAPI extD3D10CreateDeviceAndSwapChain(
	IDXGIAdapter *pAdapter, 
	D3D10_DRIVER_TYPE DriverType, 
	HMODULE Software, 
	UINT Flags, 
	UINT SDKVersion, 
	DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, 
	IDXGISwapChain **ppSwapChain, 
	ID3D10Device **ppDevice)
{
	HRESULT res;
	OutTraceD("D3D10CreateDeviceAndSwapChain\n");
	pSwapChainDesc->OutputWindow = dxw.GethWnd();
	pSwapChainDesc->Windowed = true;	
	res=(*pD3D10CreateDeviceAndSwapChain)(pAdapter, DriverType, Software, Flags, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice);
	OutTraceD("D3D10CreateDeviceAndSwapChain ret=%x\n", res);
	return res;
}

HRESULT WINAPI extD3D10CreateDeviceAndSwapChain1(
	IDXGIAdapter *pAdapter, 
	D3D10_DRIVER_TYPE DriverType, 
	HMODULE Software, 
	UINT Flags, 
	UINT SDKVersion, 
	DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, 
	IDXGISwapChain **ppSwapChain, 
	ID3D10Device **ppDevice)
{
	HRESULT res;
	OutTraceD("D3D10CreateDeviceAndSwapChain1\n");
	pSwapChainDesc->OutputWindow = dxw.GethWnd();
	pSwapChainDesc->Windowed = true;	
	res=(*pD3D10CreateDeviceAndSwapChain1)(pAdapter, DriverType, Software, Flags, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice);
	OutTraceD("D3D10CreateDeviceAndSwapChain1 ret=%x\n", res);
	return res;
}

HRESULT WINAPI extD3D11CreateDevice(
	IDXGIAdapter *pAdapter, 
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	const D3D_FEATURE_LEVEL *pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	ID3D11Device **ppDevice,
	D3D_FEATURE_LEVEL *pFeatureLevel,
	ID3D11DeviceContext **ppImmediateContext)
{
	HRESULT res;
	OutTraceD("D3D11CreateDevice\n");
	res=(*pD3D11CreateDevice)(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
	OutTraceD("D3D11CreateDevice ret=%x\n", res);
	return res;
}

HRESULT WINAPI extD3D11CreateDeviceAndSwapChain(
	IDXGIAdapter *pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	const D3D_FEATURE_LEVEL *pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
	IDXGISwapChain **ppSwapChain,
	ID3D11Device **ppDevice,
	D3D_FEATURE_LEVEL *pFeatureLevel,
	ID3D11DeviceContext **ppImmediateContext)
{
	HRESULT res;
	OutTraceD("D3D11CreateDeviceAndSwapChain\n");
	//res=(*pD3D11CreateDeviceAndSwapChain)(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, *ppDevice, pFeatureLevel, ppImmediateContext);
	res=DD_OK;
	OutTraceD("D3D11CreateDeviceAndSwapChain ret=%x\n", res);
	return res;
}
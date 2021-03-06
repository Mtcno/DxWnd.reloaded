#define _CRT_SECURE_NO_WARNINGS
#define SYSLIBNAMES_DEFINES

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhelper.h"
#include "resource.h"
#include "hddraw.h"
#include "d3d9.h"
extern GetDC_Type pGetDCMethod();
extern ReleaseDC_Type pReleaseDC1;
extern HandleDDThreadLock_Type pReleaseDDThreadLock;

/* ------------------------------------------------------------------ */
// Internal function pointers
/* ------------------------------------------------------------------ */

typedef DWORD (*TimeShifter_Type)(DWORD, int);
typedef LARGE_INTEGER (*TimeShifter64_Type)(LARGE_INTEGER, int);

TimeShifter_Type pTimeShifter;
TimeShifter64_Type pTimeShifter64;

static DWORD TimeShifterFine(DWORD, int);
static LARGE_INTEGER TimeShifter64Fine(LARGE_INTEGER, int);
static DWORD TimeShifterCoarse(DWORD, int);
static LARGE_INTEGER TimeShifter64Coarse(LARGE_INTEGER, int);

/* ------------------------------------------------------------------ */
// Constructor, destructor, initialization....
/* ------------------------------------------------------------------ */

dxwCore::dxwCore()
{
	// initialization stuff ....
	FullScreen=FALSE;
	SethWnd(NULL);
	SetScreenSize();
	dwMaxDDVersion=7;
	hParentWnd = 0;
	hChildWnd = 0;
	bActive = TRUE;
	bDInputAbs = 0;
	TimeShift = 0;
	ResetEmulatedDC();
	MustShowOverlay=FALSE;
	TimerEvent.dwTimerType = TIMER_TYPE_NONE;
	// initialization of default vsync emulation array
	iRefreshDelays[0]=16;
	iRefreshDelays[1]=17;
	iRefreshDelayCount=2;
	TimeFreeze = FALSE;
	dwScreenWidth = 0;
	dwScreenHeight = 0;
}

dxwCore::~dxwCore()
{
}

void dxwCore::SetFullScreen(BOOL fs, int line) 
{
	OutTraceDW("SetFullScreen: %s at %d\n", fs?"FULLSCREEN":"WINDOWED", line);
	FullScreen=fs;
}

void dxwCore::SetFullScreen(BOOL fs) 
{
	if(dxw.dwFlags3 & FULLSCREENONLY) fs=TRUE;
	OutTraceDW("SetFullScreen: %s\n", fs?"FULLSCREEN":"WINDOWED");
	FullScreen=fs;
}

BOOL dxwCore::IsFullScreen()
{
	return (Windowize && FullScreen);
}

BOOL dxwCore::IsToRemap(HDC hdc)
{
	if(!hdc) return TRUE;
	return (Windowize && FullScreen && (OBJ_DC == (*pGetObjectType)(hdc)));
}

void dxwCore::InitTarget(TARGETMAP *target)
{
	dwFlags1 = target->flags;
	dwFlags2 = target->flags2;
	dwFlags3 = target->flags3;
	dwFlags4 = target->flags4;
	dwFlags5 = target->flags5;
	dwFlags6 = target->flags6;
	dwFlags7 = target->flags7;
	dwFlags8 = target->flags8;
	dwFlags9 = target->flags9;
	dwFlags10= target->flags10;
	dwTFlags = target->tflags;
	Windowize = (dwFlags2 & WINDOWIZE) ? TRUE : FALSE;
	IsVisible = TRUE;
	if(dwFlags3 & FULLSCREENONLY) FullScreen=TRUE;
	gsModules = target->module;
	MaxFPS = target->MaxFPS;
	CustomOpenGLLib = target->OpenGLLib;
	if(!strlen(CustomOpenGLLib)) CustomOpenGLLib=NULL;
	// bounds control
	dwTargetDDVersion = target->dxversion;
	MaxDdrawInterface = target->MaxDdrawInterface;
	if(dwTargetDDVersion<0) dwTargetDDVersion=0;
	if(dwTargetDDVersion>12) dwTargetDDVersion=12;
	TimeShift = target->InitTS;
	if(TimeShift < -8) TimeShift = -8;
	if(TimeShift >  8) TimeShift =  8;
	FakeVersionId = target->FakeVersionId;
	MaxScreenRes = target->MaxScreenRes;
	Coordinates = target->coordinates;
	switch(target->SwapEffect){
		case 0: SwapEffect = D3DSWAPEFFECT_DISCARD; break;
		case 1: SwapEffect = D3DSWAPEFFECT_FLIP; break;
		case 2: SwapEffect = D3DSWAPEFFECT_COPY; break;
		case 3: SwapEffect = D3DSWAPEFFECT_OVERLAY; break;
		case 4: SwapEffect = D3DSWAPEFFECT_FLIPEX; break;
	}
	MustShowOverlay=((dwFlags2 & SHOWFPSOVERLAY) || (dwFlags4 & SHOWTIMESTRETCH));
	if(dwFlags4 & FINETIMING){
		pTimeShifter = TimeShifterFine;
		pTimeShifter64 = TimeShifter64Fine;
	}
	else{
		pTimeShifter = TimeShifterCoarse;
		pTimeShifter64 = TimeShifter64Coarse;
	}
	iSiz0X = iSizX = target->sizx;
	iSiz0Y = iSizY = target->sizy;
	iPos0X = iPosX = target->posx;
	iPos0Y = iPosY = target->posy;
	iMaxW = target->resw;
	iMaxH = target->resh;
	// Aspect Ratio from window size, or traditional 4:3 by default
	iRatioX = iSizX ? iSizX : 800;
	iRatioY = iSizY ? iSizY : 600;
	// AutoScale: when iSizX == iSizY == 0, size is set to current screen resolution
	bAutoScale = !(iSizX && iSizY);
	// guessed initial screen resolution
	// v2.04.01.fx4: set default value ONLY when zero, because some program may initialize
	// them before creating a window that triggers second initialization, like "Spearhead"
	// through the Smack32 SmackSetSystemRes call
	if(!dwScreenWidth) dwScreenWidth = 800;
	if(!dwScreenHeight) dwScreenHeight = 600;

	SlowRatio = target->SlowRatio;
	ScanLine = target->ScanLine;

	GDIEmulationMode = GDIMODE_NONE; // default
	if (dwFlags2 & GDISTRETCHED)	GDIEmulationMode = GDIMODE_STRETCHED;  
	if (dwFlags3 & GDIEMULATEDC)	GDIEmulationMode = GDIMODE_EMULATED; 
	if (dwFlags6 & SHAREDDC)		GDIEmulationMode = GDIMODE_SHAREDDC; 

	if(dwFlags5 & HYBRIDMODE) {
		// special mode settings ....
		dwFlags1 |= EMULATESURFACE;
		dwFlags2 |= SETCOMPATIBILITY;
		dwFlags5 &= ~(BILINEARFILTER | AEROBOOST); 
	}
	if(dwFlags5 & GDIMODE) dwFlags1 |= EMULATESURFACE;
	if(dwFlags5 & STRESSRESOURCES) dwFlags5 |= LIMITRESOURCES;
	IsEmulated = (dwFlags1 & (EMULATESURFACE|EMULATEBUFFER)) ? TRUE : FALSE; // includes also the HYBRIDMODE and GDIMODE cases ....

	extern GetWindowLong_Type pGetWindowLong;
	extern SetWindowLong_Type pSetWindowLong;
	// made before hooking !!!
	pGetWindowLong = (dwFlags5 & ANSIWIDE) ? GetWindowLongW : GetWindowLongA;
	pSetWindowLong = (dwFlags5 & ANSIWIDE) ? SetWindowLongW : SetWindowLongA;

	// hint system
	bHintActive = (dwFlags7 & SHOWHINTS) ? TRUE : FALSE;

	MonitorId = target->monitorid;

	// if specified, set the custom initial resolution
	if(dxw.dwFlags7 & INITIALRES) SetScreenSize(target->resw, target->resh);
}

void dxwCore::SetScreenSize(void) 
{
	// if specified, use values registered in InitTarget
	if(dxw.dwFlags7 & INITIALRES) return;

	if(dxw.Windowize){
		SetScreenSize(800, 600); // set to default screen resolution
	}
	else{
		int sizx, sizy;
		sizx = GetSystemMetrics(SM_CXSCREEN);
		sizy = GetSystemMetrics(SM_CYSCREEN);
		SetScreenSize(sizx, sizy);
	}
}

void dxwCore::SetScreenSize(int x, int y) 
{
	DXWNDSTATUS *p;
	OutTraceDW("DXWND: set screen size=(%d,%d)\n", x, y);
	if(x) dwScreenWidth=x; 
	if(y) dwScreenHeight=y;
	p = GetHookInfo();
	if(p) {
		p->Width = (short)dwScreenWidth;
		p->Height = (short)dwScreenHeight;
	}
	if(dwFlags4 & LIMITSCREENRES){
		#define HUGE 100000
		DWORD maxw, maxh;
		maxw=HUGE; maxh=HUGE;
		switch(dxw.MaxScreenRes){
			case DXW_LIMIT_320x200: maxw=320; maxh=200; break;
			case DXW_LIMIT_400x300: maxw=400; maxh=300; break;
			case DXW_LIMIT_640x480: maxw=640; maxh=480; break;
			case DXW_LIMIT_800x600: maxw=800; maxh=600; break;
			case DXW_LIMIT_1024x768: maxw=1024; maxh=768; break;
			case DXW_LIMIT_1280x960: maxw=1280; maxh=960; break;
		}
		if(((DWORD)p->Width > maxw) || ((DWORD)p->Height > maxh)){
			OutTraceDW("DXWND: limit device size=(%d,%d)\n", maxw, maxh);
			// v2.02.95 setting new virtual desktop size 
			dwScreenWidth = p->Width = (short)maxw;
			dwScreenHeight= p->Height = (short)maxh;
		}
	}
	if(dxw.dwFlags7 & MAXIMUMRES){
		if(((long)p->Width > dxw.iMaxW) || ((long)p->Height > dxw.iMaxH)){
			OutTraceDW("DXWND: limit device size=(%d,%d)\n", dxw.iMaxW, dxw.iMaxH);
			// v2.03.90 setting new virtual desktop size 
			dwScreenWidth = p->Width = (short)dxw.iMaxW;
			dwScreenHeight= p->Height = (short)dxw.iMaxH;
		}
	}
}

void dxwCore::DumpDesktopStatus()
{
	HDC hDC;
	HWND hDesktop;
	RECT desktop;
	PIXELFORMATDESCRIPTOR pfd;
	int  iPixelFormat, iBPP;
	char ColorMask[32+1]; 

	// get the current pixel format index
	hDesktop = GetDesktopWindow();
	hDC = GetDC(hDesktop);
	::GetWindowRect(hDesktop, &desktop);
	iBPP = GetDeviceCaps(hDC, BITSPIXEL);
	iPixelFormat = GetPixelFormat(hDC); 
	if(!iPixelFormat) iPixelFormat=1; // why returns 0???
	// obtain a detailed description of that pixel format  
	if(!DescribePixelFormat(hDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd)){
		OutTrace("DescribePixelFormat ERROR: err=%d\n", GetLastError());
		return;
	}

	memset(ColorMask, ' ', 32); // blank fill
	ColorMask[32] = 0; // terminate
	if ((pfd.cRedShift+pfd.cRedBits <= 32) &&
		(pfd.cGreenShift+pfd.cGreenBits <= 32) &&
		(pfd.cBlueShift+pfd.cBlueBits <= 32) &&
		(pfd.cAlphaShift+pfd.cAlphaBits <= 32)){ // everything within the 32 bits ...
		for (int i=pfd.cRedShift; i<pfd.cRedShift+pfd.cRedBits; i++) ColorMask[i]='R';
		for (int i=pfd.cGreenShift; i<pfd.cGreenShift+pfd.cGreenBits; i++) ColorMask[i]='G';
		for (int i=pfd.cBlueShift; i<pfd.cBlueShift+pfd.cBlueBits; i++) ColorMask[i]='B';
		for (int i=pfd.cAlphaShift; i<pfd.cAlphaShift+pfd.cAlphaBits; i++) ColorMask[i]='A';
	}
	else
		strcpy(ColorMask, "???");
	OutTrace( 
		"Desktop"
		"\tSize (W x H)=(%d x %d)\n" 
		"\tColor depth = %d (color bits = %d)\n"
		"\tPixel format = %d\n"
		"\tColor mask  (RGBA)= (%d,%d,%d,%d)\n" 
		"\tColor shift (RGBA)= (%d,%d,%d,%d)\n"
		"\tColor mask = \"%s\"\n"
		, 
		desktop.right, desktop.bottom, 
		iBPP, pfd.cColorBits,
		iPixelFormat,
		pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits,
		pfd.cRedShift, pfd.cGreenShift, pfd.cBlueShift, pfd.cAlphaShift,
		ColorMask
	);
}

void dxwCore::InitWindowPos(int x, int y, int w, int h)
{
	iPosX = x;
	iPosY = y; //v2.02.09
	iSizX = w;
	iSizY = h;
}

BOOL dxwCore::IsDesktop(HWND hwnd)
{
	return (
		(hwnd == 0)
		||
		(hwnd == (*pGetDesktopWindow)())
		||
		(hwnd == hWnd)
		);
}

BOOL dxwCore::IsRealDesktop(HWND hwnd)
{
	return (
		(hwnd == 0)
		||
		(hwnd == (*pGetDesktopWindow)())
		);
}

// v2.1.93: FixCursorPos completely revised to introduce a clipping tolerance in
// clipping regions as well as in normal operations

#define CLIP_TOLERANCE 4

POINT dxwCore::FixCursorPos(POINT prev)
{
	POINT curr;
	RECT rect;
	extern LPRECT lpClipRegion;
	static BOOL IsWithin = TRUE;
	static POINT LastPos;

	curr=prev;

	// scale mouse coordinates
	// remember: rect from GetClientRect always start at 0,0!
	if(dxw.dwFlags1 & MODIFYMOUSE){
		int w, h; // width, height and border

		if (!(*pGetClientRect)(hWnd, &rect)) { // v2.02.30: always use desktop win
			OutTraceDW("GetClientRect ERROR %d at %d\n", GetLastError(),__LINE__);
			curr.x = curr.y = 0;
		}
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;

		if(dxw.dwFlags4 & RELEASEMOUSE){
			if ((curr.x < 0) || (curr.y < 0) || (curr.x > w) || (curr.y > h)){
				if(IsWithin){
					int RestX, RestY;
					RestX = w ? ((CLIP_TOLERANCE * w) / dxw.GetScreenWidth()) + 2 : CLIP_TOLERANCE + 2;
					RestY = h ? ((CLIP_TOLERANCE * h) / dxw.GetScreenHeight()) + 2 : CLIP_TOLERANCE + 2;
					if (curr.x < 0) curr.x = RestX;
					if (curr.y < 0) curr.y = RestY;
					if (curr.x > w) curr.x = w - RestX;
					if (curr.y > h) curr.y = h - RestY;
					LastPos = curr;
					IsWithin = FALSE;
				}
				else{
					curr = LastPos;
				}
			}
			else{
				IsWithin = TRUE;
				LastPos = curr;
			}
		}
		else {
			if (curr.x < 0) curr.x = 0;
			if (curr.y < 0) curr.y = 0;
			if (curr.x > w) curr.x = w;
			if (curr.y > h) curr.y = h;
		}

		if (w) curr.x = (curr.x * dxw.GetScreenWidth()) / w;
		if (h) curr.y = (curr.y * dxw.GetScreenHeight()) / h;
	}

	if((dxw.dwFlags1 & DISABLECLIPPING) && lpClipRegion){
		// v2.1.93:
		// in clipping mode, avoid the cursor position to lay outside the valid rect
		// note 1: the rect follow the convention and valid coord lay between left to righ-1,
		// top to bottom-1
		// note 2: CLIP_TOLERANCE is meant to handle possible integer divide tolerance errors
		// that may prevent reaching the clip rect borders. The smaller you shrink the window, 
		// the bigger tolerance is required
		if (curr.x < lpClipRegion->left+CLIP_TOLERANCE) curr.x=lpClipRegion->left;
		if (curr.y < lpClipRegion->top+CLIP_TOLERANCE) curr.y=lpClipRegion->top;
		if (curr.x >= lpClipRegion->right-CLIP_TOLERANCE) curr.x=lpClipRegion->right-1;
		if (curr.y >= lpClipRegion->bottom-CLIP_TOLERANCE) curr.y=lpClipRegion->bottom-1;
	}
	else{
		if (curr.x < CLIP_TOLERANCE) curr.x=0;
		if (curr.y < CLIP_TOLERANCE) curr.y=0;
		if (curr.x >= (LONG)dxw.GetScreenWidth()-CLIP_TOLERANCE) curr.x=dxw.GetScreenWidth()-1;
		if (curr.y >= (LONG)dxw.GetScreenHeight()-CLIP_TOLERANCE) curr.y=dxw.GetScreenHeight()-1;
	}

	return curr;
}

POINT dxwCore::ScreenToClient(POINT point)
{
	// convert absolute screen coordinates to frame relative
	if (!(*pScreenToClient)(hWnd, &point)) {
		OutTraceE("ScreenToClient(%x) ERROR %d at %d\n", hWnd, GetLastError(), __LINE__);
		point.x =0; point.y=0;
	}

	return point;
}

void dxwCore::FixNCHITCursorPos(LPPOINT lppoint)
{
	RECT rect;
	POINT point;

	point=*lppoint;
	(*pGetClientRect)(dxw.GethWnd(), &rect);
	(*pScreenToClient)(dxw.GethWnd(), &point);

	if (point.x < 0) return;
	if (point.y < 0) return;
	if (point.x > rect.right) return;
	if (point.y > rect.bottom) return;

	*lppoint=point;
	lppoint->x = (lppoint->x * dxw.GetScreenWidth()) / rect.right;
	lppoint->y = (lppoint->y * dxw.GetScreenHeight()) / rect.bottom;
	if(lppoint->x < CLIP_TOLERANCE) lppoint->x=0;
	if(lppoint->y < CLIP_TOLERANCE) lppoint->y=0;
	if(lppoint->x > (LONG)dxw.GetScreenWidth()-CLIP_TOLERANCE) lppoint->x=dxw.GetScreenWidth()-1;
	if(lppoint->y > (LONG)dxw.GetScreenHeight()-CLIP_TOLERANCE) lppoint->y=dxw.GetScreenHeight()-1;
}

void dxwCore::InitializeClipCursorState(void)
{
	RECT cliprect;
	BOOL clipret;
	clipret = (*pGetClipCursor)(&cliprect);
	// v2.04.06: you always get a clipper area. To tell that the clipper is NOT active for your window 
	// you can compare the clipper area with the whole desktop. If they are equivalent, you have no 
	// clipper (or you are in fullscreen mode, but that is equivalent).
	ClipCursorToggleState = TRUE;
	if (((cliprect.right - cliprect.left) == (*pGetSystemMetrics)(SM_CXVIRTUALSCREEN)) && 
		((cliprect.bottom - cliprect.top) == (*pGetSystemMetrics)(SM_CYVIRTUALSCREEN))) 
		ClipCursorToggleState = FALSE;
	OutTraceDW("Initial clipper status=%x\n", ClipCursorToggleState);
}

BOOL dxwCore::IsClipCursorActive(void)
{
	static BOOL bDoOnce = TRUE;
	if (bDoOnce) InitializeClipCursorState();
	return ClipCursorToggleState;
}

void dxwCore::SetClipCursor()
{
	RECT Rect;
	POINT UpLeftCorner={0,0};

	OutTraceDW("SetClipCursor:\n");
	if (hWnd==NULL) {
		OutTraceDW("SetClipCursor: ASSERT hWnd==NULL\n");
		return;
	}

	// check for errors to avoid setting random clip regions
	//if((*pIsWindowVisible)(hWnd)){
	//	OutTraceE("SetClipCursor: not visible\n");
	//	return;
	//}
	if(!(*pGetClientRect)(hWnd, &Rect)){
		OutTraceE("SetClipCursor: GetClientRect ERROR err=%d at %d\n", GetLastError(), __LINE__);
		return;
	}
	if((Rect.right == 0) && (Rect.bottom == 0)){ 
		OutTraceE("SetClipCursor: GetClientRect returns zero sized rect at %d\n", __LINE__);
		return;
	}
	if(!(*pClientToScreen)(hWnd, &UpLeftCorner)){
		OutTraceE("SetClipCursor: ClientToScreen ERROR err=%d at %d\n", GetLastError(), __LINE__);
		return ;
	}
	Rect.left+=UpLeftCorner.x;
	Rect.right+=UpLeftCorner.x;
	Rect.top+=UpLeftCorner.y;
	Rect.bottom+=UpLeftCorner.y;

	if(dwFlags8 & CLIPMENU) {
		// v2.04.11:
		// if flag set and the window has a menu, extend the mouse clipper area to allow reaching the manu
		// implementation is partial: doesn't take in account multi-lines menues or menus positioned
		// not on the top of the window client area, but it seems good for the most cases.
		if(GetMenu(hWnd)) Rect.top -= (*pGetSystemMetrics)(SM_CYMENU);
	}

	(*pClipCursor)(NULL);
	if((*pClipCursor)(&Rect)){
		ClipCursorToggleState = TRUE;
	}
	else{
		OutTraceE("SetClipCursor: ERROR err=%d at %d\n", GetLastError(), __LINE__);
	}

	OutTraceDW("SetClipCursor: rect=(%d,%d)-(%d,%d)\n",
		Rect.left, Rect.top, Rect.right, Rect.bottom);
}

void dxwCore::EraseClipCursor()
{
	OutTraceDW("EraseClipCursor:\n");
	(*pClipCursor)(NULL);
	ClipCursorToggleState = FALSE;
}

void dxwCore::SethWnd(HWND hwnd) 
{
	RECT WinRect;
	if(!pGetWindowRect) pGetWindowRect=::GetWindowRect;
	if(!pGDIGetDC)		pGDIGetDC=::GetDC;

	hWnd=hwnd; 
	hWndFPS=hwnd;	

	if(hwnd){
		(*pGetWindowRect)(hwnd, &WinRect);
		OutTraceDW("SethWnd: setting main win=%x pos=(%d,%d)-(%d,%d)\n", 
			hwnd, WinRect.left, WinRect.top, WinRect.right, WinRect.bottom);
	}
	else{
		OutTraceDW("SethWnd: clearing main win\n");
	}
}

BOOL dxwCore::ishWndFPS(HWND hwnd) 
{
	return (hwnd == hWndFPS);
}

void dxwCore::FixWorkarea(LPRECT workarea)
{
	int w, h, b; // width, height and border

	w = workarea->right - workarea->left;
	h = workarea->bottom - workarea->top;
	if ((w * iRatioY) > (h * iRatioX)){
		b = (w - (h * iRatioX / iRatioY))/2;
		workarea->left += b;
		workarea->right -= b;
	}
	else {
		b = (h - (w * iRatioY / iRatioX))/2;
		workarea->top += b;
		workarea->bottom -= b;
	}
}

//#define DUMPALLPALETTECHANGES TRUE

void dxwCore::DumpPalette(DWORD dwcount, LPPALETTEENTRY lpentries)
{
	char sInfo[(14*256)+1];
	sInfo[0]=0;
	// "Spearhead" has a bug that sets 897 palette entries!
	if(dwcount > 256) dwcount=256;
	for(DWORD idx=0; idx<dwcount; idx++) 
		sprintf(sInfo, "%s(%02x.%02x.%02x:%02x)", sInfo, 
		lpentries[idx].peRed, lpentries[idx].peGreen, lpentries[idx].peBlue, lpentries[idx].peFlags);
	strcat(sInfo,"\n");
	OutTrace(sInfo);

#ifdef DUMPALLPALETTECHANGES
	if(DUMPALLPALETTECHANGES){
		extern DXWNDSTATUS *pStatus;
		for(DWORD idx=0; idx<dwcount; idx++)  
			pStatus->Palette[idx]= lpentries[idx];
		Sleep(2000);
	}
#endif
}

void dxwCore::ScreenRefresh(void)
{
	// optimization: don't blit too often!
	// 20mSec seems a good compromise.
	#define DXWREFRESHINTERVAL 20

	LPDIRECTDRAWSURFACE lpDDSPrim;
	extern HRESULT WINAPI extBlt(int, Blt_Type, LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);

	static int t = -1;
	if (t == -1)
		t = (*pGetTickCount)()-(DXWREFRESHINTERVAL+1); // V.2.1.69: trick - subtract 
	int tn = (*pGetTickCount)();

	if (tn-t < DXWREFRESHINTERVAL) return;
	t = tn;

	// if not too early, refresh primary surface ....
	lpDDSPrim=dxwss.GetPrimarySurface();
	extern Blt_Type pBltMethod();
	extern int lpddsHookedVersion();
	if (lpDDSPrim) extBlt(lpddsHookedVersion(), pBltMethod(), lpDDSPrim, NULL, lpDDSPrim, NULL, 0, NULL); 

	// v2.02.44 - used for what? Commenting out seems to fix the palette update glitches  
	// and make the "Palette updates don't blit" option useless....
	//(*pInvalidateRect)(hWnd, NULL, FALSE); 
}

void dxwCore::DoSlow(int delay)
{
	MSG uMsg;
	int t, tn;
	t = (*pGetTickCount)();

	uMsg.message=0; // initialize somehow...
	while((tn = (*pGetTickCount)())-t < delay){
		while (PeekMessage (&uMsg, NULL, 0, 0, PM_REMOVE) > 0){
			if(WM_QUIT == uMsg.message) break;
			TranslateMessage (&uMsg);
			DispatchMessage (&uMsg);
		}	
		(*pSleep)(1);
	}
}

// Remarks:
// If the target window is owned by the current process, GetWindowText causes a WM_GETTEXT message 
// to be sent to the specified window or control. If the target window is owned by another process 
// and has a caption, GetWindowText retrieves the window caption text. If the window does not have 
// a caption, the return value is a null string. This behavior is by design. It allows applications 
// to call GetWindowText without becoming unresponsive if the process that owns the target window 
// is not responding. However, if the target window is not responding and it belongs to the calling 
// application, GetWindowText will cause the calling application to become unresponsive. 

static void CountFPS(HWND hwnd)
{
	static DWORD time = 0xFFFFFFFF;
	static DWORD FPSCount = 0;
	static HWND LasthWnd = 0;
	extern void SetFPS(int);
	static char sBuf[80+15+1]; // title + fps string + terminator
	//DXWNDSTATUS Status;
	DWORD tmp;
	tmp = (*pGetTickCount)();
	if((tmp - time) > 1000) {
		char *fpss;
		// log fps count
		// OutTrace("FPS: Delta=%x FPSCount=%d\n", (tmp-time), FPSCount);
		// show fps count on status win
		GetHookInfo()->FPSCount = FPSCount; // for overlay display
		// show fps on win title bar
		if (dxw.dwFlags2 & SHOWFPS){
			if(hwnd != LasthWnd){
				GetWindowText(hwnd, sBuf, 80);
				LasthWnd = hwnd;
			}
			fpss=strstr(sBuf," ~ (");
			if(fpss==NULL) fpss=&sBuf[strlen(sBuf)];
			sprintf_s(fpss, 15, " ~ (%d FPS)", FPSCount);
			SetWindowText(hwnd, sBuf);
		}
		// reset
		FPSCount=0;
		time = tmp;
	}
	else {
		FPSCount++;
		OutTraceB("FPS: Delta=%x FPSCount++=%d\n", (tmp-time), FPSCount);
	}
}

static void LimitFrameCount(DWORD delay)
{
	static DWORD oldtime = (*pGetTickCount)();
	DWORD newtime;
	newtime = (*pGetTickCount)();
	if(IsDebug) OutTrace("FPS limit: old=%x new=%x delay=%d sleep=%d\n", 
		oldtime, newtime, delay, (oldtime+delay-newtime));
	// use '<' and not '<=' to avoid the risk of sleeping forever....
	if((newtime < oldtime+delay) && (newtime >= oldtime)) {
		//if(IsDebug) OutTrace("FPS limit: old=%x new=%x delay=%d sleep=%d\n", 
		//	oldtime, newtime, delay, (oldtime+delay-newtime));
		do{
			if(IsDebug) OutTrace("FPS limit: sleep=%d\n", oldtime+delay-newtime);
			(*pSleep)(oldtime+delay-newtime);
			newtime = (*pGetTickCount)();
			if(IsDebug) OutTrace("FPS limit: newtime=%d\n", newtime);
		} while(newtime < oldtime+delay);
	}
	oldtime += delay;
	if(oldtime < newtime-delay) oldtime = newtime-delay;
}

static BOOL SkipFrameCount(DWORD delay)
{
	static DWORD oldtime=(*pGetTickCount)();
	DWORD newtime;
	newtime = (*pGetTickCount)();
	if(newtime < oldtime+delay) return TRUE; // TRUE => skip the screen refresh
	oldtime = newtime;
	return FALSE; // don't skip, do the update

}

BOOL dxwCore::HandleFPS()
{
	if(dwFlags2 & (SHOWFPS|SHOWFPSOVERLAY)) CountFPS(hWndFPS);
	if(dwFlags2 & LIMITFPS) LimitFrameCount(dxw.MaxFPS);
	if(dwFlags2 & SKIPFPS) if(SkipFrameCount(dxw.MaxFPS)) return TRUE;
	return FALSE;
}

// auxiliary functions ...

void dxwCore::SetVSyncDelays(UINT RefreshRate)
{
	int Reminder;
	char sInfo[256];

	if(!(dxw.dwFlags1 & SAVELOAD)) return;
	if((RefreshRate < 10) || (RefreshRate > 100)) return; 

	gdwRefreshRate = RefreshRate;
	if(!gdwRefreshRate) return;
	iRefreshDelayCount=0;
	Reminder=0;
	do{
		iRefreshDelays[iRefreshDelayCount]=(1000+Reminder)/gdwRefreshRate;
		Reminder=(1000+Reminder)-(iRefreshDelays[iRefreshDelayCount]*gdwRefreshRate);
		iRefreshDelayCount++;
	} while(Reminder && (iRefreshDelayCount<MAXREFRESHDELAYCOUNT));
	if(IsTraceDW){
		strcpy(sInfo, "");
		for(int i=0; i<iRefreshDelayCount; i++) sprintf(sInfo, "%s%d ", sInfo, iRefreshDelays[i]);
		OutTraceDW("Refresh rate=%d: delay=%s\n", gdwRefreshRate, sInfo);
	}
}

void dxwCore::VSyncWait()
{
	static DWORD time = 0;
	static BOOL step = 0;
	DWORD tmp;
	tmp = (*pGetTickCount)();
	if((time - tmp) > 32) time = tmp;
	(*pSleep)(time - tmp);
	time += iRefreshDelays[step++];
	if(step >= iRefreshDelayCount) step=0;
}

void dxwCore::VSyncWaitLine(DWORD ScanLine)
{
	extern LPDIRECTDRAW lpPrimaryDD;
	static DWORD iLastScanLine = 0;
	DWORD iCurrentScanLine;
	if (!lpPrimaryDD) return;
	while(1){
		HRESULT res;
		if(res=lpPrimaryDD->GetScanLine(&iCurrentScanLine)) {
			OutTraceE("VSyncWaitLine: GetScanLine ERROR res=%x\n", res);
			iLastScanLine = 0;
			break; // error
		}
		if((iLastScanLine <= ScanLine) && (iCurrentScanLine > ScanLine)) {
			OutTraceB("VSyncWaitLine: line=%d last=%d\n", iCurrentScanLine, iLastScanLine);
			break;
		}
		iLastScanLine = iCurrentScanLine;
		(*pSleep)(1);
	}
}

static float fMul[17]={2.14F, 1.95F, 1.77F, 1.61F, 1.46F, 1.33F, 1.21F, 1.10F, 1.00F, 0.91F, 0.83F, 0.75F, 0.68F, 0.62F, 0.56F, 0.51F, 0.46F};

static DWORD TimeShifterFine(DWORD val, int shift)
{
	float fVal;
	fVal = (float)val * fMul[shift+8];
	return (DWORD)fVal;
}

static DWORD TimeShifterCoarse(DWORD val, int shift)
{
	int exp, reminder;
	if (shift > 0) {
		exp = shift >> 1;
		reminder = shift & 0x1;
		if (reminder) val -= (val >> 2); // val * (1-1/4) = val * 3/4
		val >>= exp; // val * 2^exp
	}
	if (shift < 0) {
		exp = (-shift) >> 1;
		reminder = (-shift) & 0x1;
		val <<= exp; // val / 2^exp
		if (reminder) val += (val >> 1); // val * (1+1/2) = val * 3/2
	}
	return val;
}

static LARGE_INTEGER TimeShifter64Fine(LARGE_INTEGER val, int shift)
{
	float fVal;
	fVal = (float)val.LowPart * fMul[shift+8];
	val.HighPart = 0;
	val.LowPart = (DWORD)fVal;
	return val;	
}

static LARGE_INTEGER TimeShifter64Coarse(LARGE_INTEGER val, int shift)
{
	int exp, reminder;
	if (shift > 0) {
		exp = shift >> 1;
		reminder = shift & 0x1;
		if (reminder) val.QuadPart -= (val.QuadPart >> 2); // val * (1-1/4) = val * 3/4
		val.QuadPart >>= exp; // val * 2^exp
	}
	if (shift < 0) {
		exp = (-shift) >> 1;
		reminder = (-shift) & 0x1;
		val.QuadPart <<= exp; // val / 2^exp
		if (reminder) val.QuadPart += (val.QuadPart >> 1); // val * (1+1/2) = val * 3/2
	}
	return val;
}

DWORD dxwCore::GetTickCount(void)
{
	DWORD dwTick;
	static DWORD dwLastRealTick=0;
	static DWORD dwLastFakeTick=0;
	DWORD dwNextRealTick;
	static BOOL FirstTime = TRUE;

	if(FirstTime){
		dwLastRealTick=(*pGetTickCount)();
		dwLastFakeTick=dwLastRealTick;
		FirstTime=FALSE;
	}
	dwNextRealTick=(*pGetTickCount)();
	dwTick=(dwNextRealTick-dwLastRealTick);
	TimeShift=GetHookInfo()->TimeShift;
	dwTick = (*pTimeShifter)(dwTick, TimeShift);
	if(TimeFreeze) dwTick=0;
	dwLastFakeTick += dwTick;
	dwLastRealTick = dwNextRealTick;
	return dwLastFakeTick;
}

DWORD dxwCore::StretchTime(DWORD dwTimer)
{
	TimeShift=GetHookInfo()->TimeShift;
	dwTimer = (*pTimeShifter)(dwTimer, -TimeShift);
	return dwTimer;
}

DWORD dxwCore::StretchCounter(DWORD dwTimer)
{
	TimeShift=GetHookInfo()->TimeShift;
	dwTimer = (*pTimeShifter)(dwTimer, TimeShift);
	return (dxw.TimeFreeze) ? 0 : dwTimer;
}

LARGE_INTEGER dxwCore::StretchCounter(LARGE_INTEGER dwTimer)
{
	static int Reminder = 0;
	LARGE_INTEGER ret;
	LARGE_INTEGER zero = {0,0};
	TimeShift=GetHookInfo()->TimeShift;
	dwTimer.QuadPart += Reminder;
	ret = (*pTimeShifter64)(dwTimer, TimeShift);
	Reminder = (ret.QuadPart==0) ? dwTimer.LowPart : 0;
	return (dxw.TimeFreeze) ? zero : ret;
}

void dxwCore::GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	DWORD dwTick;
	DWORD dwCurrentTick;
	FILETIME CurrFileTime;
	static DWORD dwStartTick=0;
	static FILETIME StartFileTime;

	if(dwStartTick==0) {
		SYSTEMTIME StartingTime;
		// first time through, initialize & return true time
		dwStartTick = (*pGetTickCount)();
		(*pGetSystemTime)(&StartingTime);
		SystemTimeToFileTime(&StartingTime, &StartFileTime);
		*lpSystemTimeAsFileTime = StartFileTime;
	}
	else {
		dwCurrentTick=(*pGetTickCount)();
		dwTick=(dwCurrentTick-dwStartTick);
		TimeShift=GetHookInfo()->TimeShift;
		dwTick = (*pTimeShifter)(dwTick, TimeShift);
		if(dxw.TimeFreeze) dwTick=0;
		// From MSDN: Contains a 64-bit value representing the number of 
		// 100-nanosecond intervals since January 1, 1601 (UTC).
		// So, since 1mSec = 10.000 * 100nSec, you still have to multiply by 10.000.
		CurrFileTime.dwHighDateTime = StartFileTime.dwHighDateTime; // wrong !!!!
		CurrFileTime.dwLowDateTime = StartFileTime.dwLowDateTime + (10000 * dwTick); // wrong !!!!
		*lpSystemTimeAsFileTime=CurrFileTime;
		// reset to avoid time jumps on TimeShift changes...
		StartFileTime = CurrFileTime;
		dwStartTick = dwCurrentTick;
	}
}

void dxwCore::GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
	DWORD dwTick;
	DWORD dwCurrentTick;
	FILETIME CurrFileTime;
	static DWORD dwStartTick=0;
	static FILETIME StartFileTime;

	if(dwStartTick==0) {
		SYSTEMTIME StartingTime;
		// first time through, initialize & return true time
		dwStartTick = (*pGetTickCount)();
		(*pGetSystemTime)(&StartingTime);
		SystemTimeToFileTime(&StartingTime, &StartFileTime);
		*lpSystemTime = StartingTime;
	}
	else {
		dwCurrentTick=(*pGetTickCount)();
		dwTick=(dwCurrentTick-dwStartTick);
		TimeShift=GetHookInfo()->TimeShift;
		dwTick = (*pTimeShifter)(dwTick, TimeShift);
		if(TimeFreeze) dwTick=0;
		// From MSDN: Contains a 64-bit value representing the number of 
		// 100-nanosecond intervals since January 1, 1601 (UTC).
		// So, since 1mSec = 10.000 * 100nSec, you still have to multiply by 10.000.
		CurrFileTime.dwHighDateTime = StartFileTime.dwHighDateTime; // wrong !!!!
		CurrFileTime.dwLowDateTime = StartFileTime.dwLowDateTime + (10000 * dwTick); // wrong !!!!
		FileTimeToSystemTime(&CurrFileTime, lpSystemTime);
		// reset to avoid time jumps on TimeShift changes...
		StartFileTime = CurrFileTime;
		dwStartTick = dwCurrentTick;
	}
}

void dxwCore::ShowOverlay()
{
	if (MustShowOverlay) {
		RECT rect;
		(*pGetClientRect)(hWnd, &rect);
		this->ShowOverlay(GetDC(hWnd), rect.right, rect.bottom);
	}
}

void dxwCore::ShowOverlay(LPDIRECTDRAWSURFACE lpdds)
{
	typedef HRESULT (WINAPI *GetDC_Type) (LPDIRECTDRAWSURFACE, HDC FAR *);
	typedef HRESULT (WINAPI *ReleaseDC_Type)(LPDIRECTDRAWSURFACE, HDC);
	extern GetDC_Type pGetDCMethod();
	extern ReleaseDC_Type pReleaseDCMethod();
	if (MustShowOverlay) {
		HDC hdc; // the working dc
		int h, w;
		if(!lpdds) return;
		if (FAILED((*pGetDCMethod())(lpdds, &hdc))) return; 
		w = this->GetScreenWidth();
		h = this->GetScreenHeight();
		if(this->dwFlags4 & BILINEAR2XFILTER) {
			w <<=1;
			h <<=1;
		}
		this->ShowOverlay(hdc, w, h);
		(*pReleaseDCMethod())(lpdds, hdc);
	}
}

void dxwCore::ShowOverlay(HDC hdc)
{
	if(!hdc) return;
	RECT rect;
	(*pGetClientRect)(hWnd, &rect);
	this->ShowOverlay(hdc, rect.right, rect.bottom);
}

void dxwCore::ShowOverlay(HDC hdc, int w, int h)
{
	if(!hdc) return;
	if (dwFlags2 & SHOWFPSOVERLAY) ShowFPS(hdc, w, h);
	if (dwFlags4 & SHOWTIMESTRETCH) ShowTimeStretching(hdc, w, h);
}

// nasty global to ensure that the corner is picked semi-random, but never overlapped
// between FPS and TimeStretch (and, as a side effect, never twice the same!)
static int LastCorner;

void dxwCore::ShowFPS(HDC xdc, int w, int h)
{
	char sBuf[81];
	static DWORD dwTimer = 0;
	static int corner = 0;
	static int x, y;
	static DWORD color;

	if((*pGetTickCount)()-dwTimer > 6000){
		dwTimer = (*pGetTickCount)();
		corner = dwTimer % 4;
		if(corner==LastCorner) corner = (corner+1) % 4;
		LastCorner = corner;
		color=0xFF0000; // blue
		switch (corner) {
		case 0: x=10; y=10; break;
		case 1: x=w-60; y=10; break;
		case 2: x=w-60; y=h-20; break;
		case 3: x=10; y=h-20; break;
		}
	} 

	SetTextColor(xdc,color);
	SetBkMode(xdc, OPAQUE);
	sprintf_s(sBuf, 80, "FPS: %d", GetHookInfo()->FPSCount);
	TextOut(xdc, x, y, sBuf, strlen(sBuf));
}

void dxwCore::ShowTimeStretching(HDC xdc, int w, int h)
{
	char sBuf[81];
	static DWORD dwTimer = 0;
	static int corner = 0;
	static int x, y;
	static DWORD color;
	static int LastTimeShift = 1000; // any initial number different from -8 .. +8
	static int LastTimeFreeze = 1000; // any initial number different from TRUE, FALSE

	if((*pGetTickCount)()-dwTimer > 4000){
		if((LastTimeShift==TimeShift) && (LastTimeFreeze==TimeFreeze)) return; // after a while, stop the show
		dwTimer = (*pGetTickCount)();
		LastTimeShift=TimeShift;
		corner = dwTimer % 4;
		if(corner==LastCorner) corner = (corner+1) % 4;
		LastCorner = corner;
		color=0x0000FF; // red
		switch (corner) {
		case 0: x=10; y=10; break;
		case 1: x=w-60; y=10; break;
		case 2: x=w-60; y=h-20; break;
		case 3: x=10; y=h-20; break;
		}
	} 

	SetTextColor(xdc,color);
	SetBkMode(xdc, OPAQUE);
	sprintf_s(sBuf, 80, "t%s", dxw.GetTSCaption());
	TextOut(xdc, x, y, sBuf, strlen(sBuf));
}

char *dxwCore::GetTSCaption(int shift)
{
	static char *sTSCaptionCoarse[17]={
		"x16","x12","x8","x6",
		"x4","x3","x2","x1.5",
		"x1",
		":1.5",":2",":3",":4",
		":6",":8",":12",":16"};
	static char *sTSCaptionFine[17]={
		"x2.14","x1.95","x1.77","x1.61",
		"x1.46","x1.33","x1.21","x1.10",
		"x1.00",
		":1.10",":1.21",":1.33",":1.46",
		":1.61",":1.77",":1.95",":2.14"};
	if(TimeFreeze) return "x0";
	if (shift<(-8) || shift>(+8)) return "???";
	shift += 8;
	return (dxw.dwFlags4 & FINETIMING) ? sTSCaptionFine[shift] : sTSCaptionCoarse[shift];
}

char *dxwCore::GetTSCaption(void)
{
	return GetTSCaption(TimeShift);
}

void dxwCore::ShowBanner(HWND hwnd)
{
	static BOOL JustOnce=FALSE;
	extern HMODULE hInst;
	BITMAP bm;
	HDC hClientDC;
	HBITMAP g_hbmBall;
	RECT client;
	RECT win;
	POINT PrevViewPort;
	int StretchMode;

	hClientDC=(*pGDIGetDC)(hwnd); 
	(*pGetClientRect)(hwnd, &client);
	//(*pInvalidateRect)((*pGetDesktopWindow)(), NULL, FALSE); // invalidate full desktop, no erase.
	(*pInvalidateRect)(0, NULL, FALSE); // invalidate full desktop, no erase.
	(*pGDIBitBlt)(hClientDC, 0, 0,  client.right, client.bottom, NULL, 0, 0, BLACKNESS);

	if(JustOnce || (dwFlags2 & NOBANNER)) return;
	JustOnce=TRUE;

    g_hbmBall = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BANNER));
    HDC hdcMem = CreateCompatibleDC(hClientDC);
    HBITMAP hbmOld = (HBITMAP)(*pSelectObject)(hdcMem, g_hbmBall);
    GetObject(g_hbmBall, sizeof(bm), &bm);

	(*pGetWindowRect)(hwnd, &win);
	OutTraceDW("ShowBanner: hwnd=%x win=(%d,%d)-(%d,%d) banner size=(%dx%d)\n", 
		hwnd, win.left, win.top, win.right, win.bottom, bm.bmWidth, bm.bmHeight);

	//if(!pSetViewportOrgEx) pSetViewportOrgEx=SetViewportOrgEx;
	(*pSetViewportOrgEx)(hClientDC, 0, 0, &PrevViewPort);
	StretchMode=GetStretchBltMode(hClientDC);
	SetStretchBltMode(hClientDC, HALFTONE);
	for (int i=1; i<=16; i++){
		int w, h;
		w=(bm.bmWidth*i)/8;
		h=(bm.bmHeight*i)/8;
		(*pGDIStretchBlt)(hClientDC, (client.right-w)/2, (client.bottom-h)/2, w, h, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
		Sleep(40);
	}
	for (int i=16; i>=8; i--){
		int w, h;
		w=(bm.bmWidth*i)/8;
		h=(bm.bmHeight*i)/8;
		(*pGDIBitBlt)(hClientDC, 0, 0,  client.right, client.bottom, NULL, 0, 0, BLACKNESS);
		(*pGDIStretchBlt)(hClientDC, (client.right-w)/2, (client.bottom-h)/2, w, h, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
		Sleep(40);
	}
	SetStretchBltMode(hClientDC, StretchMode);
	(*pSetViewportOrgEx)(hClientDC, PrevViewPort.x, PrevViewPort.y, NULL);
    (*pSelectObject)(hdcMem, hbmOld);
    DeleteDC(hdcMem);
	(*pGDIReleaseDC)(hwnd, hClientDC); 
	Sleep(200);
}

void dxwCore::PushDLL(char *lpName, int idx)
{
	SysNames[idx] = lpName; // add entry
}

int dxwCore::GetDLLIndex(char *lpFileName)
{
	int idx;
	char *lpName, *lpNext;

	lpName=lpFileName;
	while (lpNext=strchr(lpName,'\\')) lpName=lpNext+1;
	for(idx=0; SysNames[idx]; idx++){
		char SysNameExt[81];
		strcpy(SysNameExt, SysNames[idx]);
		strcat(SysNameExt, ".dll");
		if(
			(!lstrcmpi(lpName,SysNames[idx])) ||
			(!lstrcmpi(lpName,SysNameExt))
		){
			OutTraceDW("Registered DLL FileName=%s\n", lpFileName);
			break;
		}
	}
	if (!SysNames[idx]) return -1;
	return idx;
}

DWORD dxwCore::FixWinStyle(DWORD dwStyle)
{
	switch(dxw.Coordinates){
		case DXW_SET_COORDINATES:
		case DXW_DESKTOP_CENTER:
			if(dxw.dwFlags2 & MODALSTYLE){
				dwStyle &= ~(WS_BORDER | WS_CAPTION | WS_DLGFRAME | WS_SYSMENU | WS_THICKFRAME);
			}
			else {
				dwStyle = WS_OVERLAPPEDWINDOW;
			}
			break;
		case DXW_DESKTOP_WORKAREA:
		case DXW_DESKTOP_FULL:
			dwStyle = 0;
			break;
	}
	return dwStyle;
}

DWORD dxwCore::FixWinExStyle(DWORD dwExStyle)
{
	switch(dxw.Coordinates){
		case DXW_SET_COORDINATES:
		case DXW_DESKTOP_CENTER:
			if(dxw.dwFlags2 & MODALSTYLE){
				dwExStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
			}
			else {
				dwExStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
			}
			break;
		case DXW_DESKTOP_WORKAREA:
		case DXW_DESKTOP_FULL:
			dwExStyle = 0;
			break;
	}

	return dwExStyle;
}

void dxwCore::FixWindowFrame(HWND hwnd)
{
	LONG nStyle, nExStyle;

	OutTraceDW("FixWindowFrame: hwnd=%x foreground=%x\n", hwnd, GetForegroundWindow());

	nStyle=(*pGetWindowLong)(hwnd, GWL_STYLE);
	// beware: 0 is a valid return code!
	//if (!nStyle){
	//	OutTraceE("FixWindowFrame: GetWindowLong ERROR %d at %d\n",GetLastError(),__LINE__);
	//	return;
	//}

	nExStyle=(*pGetWindowLong)(hwnd, GWL_EXSTYLE);
	// beware: 0 is a valid return code!
	//if (!nExStyle){
	//	OutTraceE("FixWindowFrame: GetWindowLong ERROR %d at %d\n",GetLastError(),__LINE__);
	//	return;
	//}

	OutTraceDW("FixWindowFrame: style=%x(%s) exstyle=%x(%s)\n",
		nStyle, ExplainStyle(nStyle),
		nExStyle, ExplainExStyle(nExStyle));

	// fix style
	if (!(*pSetWindowLongA)(hwnd, GWL_STYLE, FixWinStyle(nStyle))){
		OutTraceE("FixWindowFrame: SetWindowLong ERROR %d at %d\n",GetLastError(),__LINE__);
		return;
	}
	// fix exstyle
	if (!(*pSetWindowLongA)(hwnd, GWL_EXSTYLE, FixWinExStyle(nExStyle))){
		OutTraceE("FixWindowFrame: SetWindowLong ERROR %d at %d\n",GetLastError(),__LINE__);
		return;
	}

	// ShowWindow retcode means in no way an error code! Better ignore it.
	(*pShowWindow)(hwnd, SW_RESTORE);
	return;
}

void dxwCore::FixStyle(char *ApiName, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPSTYLESTRUCT lpSS;
	lpSS = (LPSTYLESTRUCT) lParam;

	switch (wParam) {
	case GWL_STYLE:
		OutTraceDW("%s: new Style=%x(%s)\n", 
			ApiName, lpSS->styleNew, ExplainStyle(lpSS->styleNew));
		if (dxw.dwFlags1 & FIXWINFRAME){ // set canonical style
			lpSS->styleNew=  WS_OVERLAPPEDWINDOW;
		}
		if (dxw.dwFlags9 & FIXTHINFRAME){ // set canonical style with thin border
			lpSS->styleNew=  WS_OVERLAPPEDTHIN;
		}
		if (dxw.dwFlags1 & LOCKWINSTYLE){ // set to current value
			lpSS->styleNew= (*pGetWindowLong)(hwnd, GWL_STYLE);
		}
		if (dxw.dwFlags1 & PREVENTMAXIMIZE){ // disable maximize settings
			if (lpSS->styleNew & WS_MAXIMIZE){
				OutTraceDW("%s: prevent maximize style\n", ApiName);
				lpSS->styleNew &= ~WS_MAXIMIZE;
			}
		}
		break;
	case GWL_EXSTYLE:
		OutTraceDW("%s: new ExStyle=%x(%s)\n", 
			ApiName, lpSS->styleNew, ExplainExStyle(lpSS->styleNew));
		if (dxw.dwFlags1 & FIXWINFRAME){ // set canonical style
			lpSS->styleNew= 0;
		}
		if (dxw.dwFlags1 & LOCKWINSTYLE){ // set to current value
				lpSS->styleNew= (*pGetWindowLong)(hwnd, GWL_EXSTYLE);
		}
		if ((dxw.dwFlags5 & UNLOCKZORDER) && (hwnd==hWnd)){ // disable maximize settings
			if (lpSS->styleNew & WS_EX_TOPMOST){
				OutTraceDW("%s: prevent EXSTYLE topmost style\n", ApiName);
				lpSS->styleNew &= ~WS_EX_TOPMOST;
			}
		} 
		break;
	default:
		break;
	}
}

void dxwCore::PushTimer(UINT uTimerId, UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent)
{
		// save current timer
		TimerEvent.dwTimerType = TIMER_TYPE_WINMM;
		TimerEvent.t.uTimerId = uTimerId;
		TimerEvent.t.uDelay = uDelay;
		TimerEvent.t.uResolution = uResolution;
		TimerEvent.t.lpTimeProc = lpTimeProc;
		TimerEvent.t.dwUser = dwUser;
		TimerEvent.t.fuEvent = fuEvent;
}

void dxwCore::PushTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
{
		// save current timer
		TimerEvent.dwTimerType = TIMER_TYPE_USER32;
		TimerEvent.t.hWnd = hWnd;
		TimerEvent.t.nIDEvent = nIDEvent;
		TimerEvent.t.uElapse = uElapse;
		TimerEvent.t.lpTimerFunc = lpTimerFunc;
}

void dxwCore::PopTimer(UINT uTimerId)
{
	// clear current timer
	if(TimerEvent.dwTimerType != TIMER_TYPE_WINMM) {
		// this should never happen, unless there are more than 1 timer!
		char msg[256];
		sprintf(msg,"PopTimer: TimerType=%x last=%x\n", TIMER_TYPE_WINMM, TimerEvent.dwTimerType);
		//MessageBox(0, msg, "PopTimer", MB_OK | MB_ICONEXCLAMATION);
		OutTraceE(msg);
		return;
	}
	if(uTimerId != TimerEvent.t.uTimerId){
		// this should never happen, unless there are more than 1 timer!
		char msg[256];
		sprintf(msg,"PopTimer: TimerId=%x last=%x\n", uTimerId, TimerEvent.t.uTimerId);
		//MessageBox(0, msg, "PopTimer", MB_OK | MB_ICONEXCLAMATION);
		OutTraceE(msg);
		return;
	}
	TimerEvent.dwTimerType = TIMER_TYPE_NONE;
}

void dxwCore::PopTimer(HWND hWnd, UINT_PTR nIDEvent)
{
	// clear current timer
	if(TimerEvent.dwTimerType != TIMER_TYPE_USER32) {
		// this should never happen, unless there are more than 1 timer!
		char msg[256];
		sprintf(msg,"PopTimer: TimerType=%x last=%x\n", TIMER_TYPE_WINMM, TimerEvent.dwTimerType);
		//MessageBox(0, msg, "PopTimer", MB_OK | MB_ICONEXCLAMATION);
		OutTraceE(msg);
		return;
	}
	if(nIDEvent != TimerEvent.t.nIDEvent){
		// this should never happen, unless there are more than 1 timer!
		char msg[256];
		sprintf(msg,"PopTimer: TimerId=%x last=%x\n", nIDEvent, TimerEvent.t.nIDEvent);
		//MessageBox(0, msg, "PopTimer", MB_OK | MB_ICONEXCLAMATION);
		OutTraceE(msg);
		return;
	}
	TimerEvent.dwTimerType = TIMER_TYPE_NONE;
}

void dxwCore::RenewTimers()
{
	OutTraceE("DXWND: RenewTimers type=%x\n", TimerEvent.dwTimerType);
	switch(TimerEvent.dwTimerType){
	case TIMER_TYPE_NONE:
		OutTraceDW("DXWND: RenewTimers type=NONE\n");
		break;
	case TIMER_TYPE_USER32:
		if(pSetTimer && pKillTimer){
			UINT uElapse;
			UINT_PTR res;
			res=(*pKillTimer)(TimerEvent.t.hWnd, TimerEvent.t.nIDEvent);
			if(!res) OutTraceE("DXWND: KillTimer ERROR hWnd=%x IDEvent=%x err=%d\n", TimerEvent.t.hWnd, TimerEvent.t.nIDEvent, GetLastError());
			uElapse = dxw.StretchTime(TimerEvent.t.uElapse);
			res=(*pSetTimer)(TimerEvent.t.hWnd, TimerEvent.t.nIDEvent, uElapse, TimerEvent.t.lpTimerFunc);
			TimerEvent.t.nIDEvent = res;
			if(res)
			OutTraceDW("DXWND: RenewTimers type=USER32 Elsapse=%d IDEvent=%x\n", uElapse, res);
			else
				OutTraceE("DXWND: SetTimer ERROR hWnd=%x Elapse=%d TimerFunc=%x err=%d\n", TimerEvent.t.hWnd, uElapse, TimerEvent.t.lpTimerFunc, GetLastError());
		}
		break;
	case TIMER_TYPE_WINMM:
		if(ptimeKillEvent && ptimeSetEvent){
			UINT NewDelay;
			MMRESULT res;
			(*ptimeKillEvent)(TimerEvent.t.uTimerId);
			NewDelay = dxw.StretchTime(TimerEvent.t.uDelay);
			res=(*ptimeSetEvent)(NewDelay, TimerEvent.t.uResolution, TimerEvent.t.lpTimeProc, TimerEvent.t.dwUser, TimerEvent.t.fuEvent);
			TimerEvent.t.uTimerId = res;
			OutTraceDW("DXWND: RenewTimers type=WINMM Delay=%d TimerId=%x\n", NewDelay, res);
		}
		break;
	default:
		OutTraceE("DXWND: RenewTimers type=%x(UNKNOWN)\n", TimerEvent.dwTimerType);
		break;
	}
}

LARGE_INTEGER dxwCore::StretchLargeCounter(LARGE_INTEGER CurrentInCount)
{
	LARGE_INTEGER CurrentOutPerfCount;
	static LARGE_INTEGER LastInPerfCount;
	static LARGE_INTEGER LastOutPerfCount;
	static BOOL FirstTime = TRUE;
	LARGE_INTEGER ElapsedCount;

	if(FirstTime){
		// first time through, initialize both inner and output per counters with real values
		LastInPerfCount.QuadPart = CurrentInCount.QuadPart;
		LastOutPerfCount.QuadPart = CurrentInCount.QuadPart;
		FirstTime=FALSE;
	}

	ElapsedCount.QuadPart = CurrentInCount.QuadPart - LastInPerfCount.QuadPart;
	ElapsedCount = dxw.StretchCounter(ElapsedCount);
	CurrentOutPerfCount.QuadPart = LastOutPerfCount.QuadPart + ElapsedCount.QuadPart;
	LastInPerfCount = CurrentInCount;
	LastOutPerfCount = CurrentOutPerfCount;

	OutTraceB("dxw::StretchCounter: Count=[%x-%x]\n", CurrentOutPerfCount.HighPart, CurrentOutPerfCount.LowPart);
	return CurrentOutPerfCount;
}

BOOL dxwCore::CheckScreenResolution(unsigned int w, unsigned int h)
{
	#define HUGE 100000
	if(dxw.dwFlags4 & LIMITSCREENRES){
		DWORD maxw, maxh;
		maxw=HUGE; maxh=HUGE; // v2.02.96
		switch(MaxScreenRes){
			case DXW_NO_LIMIT: maxw=HUGE; maxh=HUGE; break;
			case DXW_LIMIT_320x200: maxw=320; maxh=200; break;
			case DXW_LIMIT_640x480: maxw=640; maxh=480; break;
			case DXW_LIMIT_800x600: maxw=800; maxh=600; break;
			case DXW_LIMIT_1024x768: maxw=1024; maxh=768; break;
			case DXW_LIMIT_1280x960: maxw=1280; maxh=960; break;
			case DXW_LIMIT_1280x1024: maxw=1280; maxh=1024; break;
		}
		if((w > maxw) || (h > maxh)) return FALSE;
	}

	if(dxw.dwFlags7 & MAXIMUMRES){
		if(((long)w > dxw.iMaxW) || ((long)h > dxw.iMaxH)) return FALSE;
	}

	return TRUE;
}

UINT VKeyConfig[DXVK_SIZE];

static char *VKeyLabels[DXVK_SIZE]={
	"none",
	"cliptoggle", 
	"refresh", 
	"logtoggle", 
	"plocktoggle",
	"fpstoggle", 
	"timefast", 
	"timeslow", 
	"timetoggle", 
	"altf4",
	"printscreen",
	"corner",
	"freezetime",
	"fullscreen",
	"workarea",
	"desktop",
};

void dxwCore::MapKeysInit()
{
	char InitPath[MAX_PATH];
	char *p;
	DWORD dwAttrib;	
	int KeyIdx;
	dwAttrib = GetFileAttributes("dxwnd.dll");
	if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) return;
	GetModuleFileName(GetModuleHandle("dxwnd"), InitPath, MAX_PATH);
	p=&InitPath[strlen(InitPath)-strlen("dxwnd.dll")];
	strcpy(p, "dxwnd.ini");
	VKeyConfig[DXVK_NONE]=DXVK_NONE;
	for(KeyIdx=1; KeyIdx<DXVK_SIZE; KeyIdx++){
		VKeyConfig[KeyIdx]=GetPrivateProfileInt("keymapping", VKeyLabels[KeyIdx], KeyIdx==DXVK_ALTF4 ? 0x73 : 0x00, InitPath);
		if(VKeyConfig[KeyIdx]) OutTrace("keymapping[%d](%s)=%x\n", KeyIdx, VKeyLabels[KeyIdx], VKeyConfig[KeyIdx]);
	}
}

UINT dxwCore::MapKeysConfig(UINT message, LPARAM lparam, WPARAM wparam)
{
	if(message!=WM_SYSKEYDOWN) return DXVK_NONE;
	for(int idx=1; idx<DXVK_SIZE; idx++)
		if(VKeyConfig[idx]==wparam) {
			OutTrace("keymapping GOT=0x%02X\n", idx);
			return idx;
		}
	return DXVK_NONE;
}

void dxwCore::ToggleFreezedTime()
{
	static DWORD dwLastTime = 0;
	if(((*pGetTickCount)() - dwLastTime) < 1000) return;
	TimeFreeze = !TimeFreeze;
	dwLastTime = (*pGetTickCount)();
	OutTraceDW("DxWnd: time is %s\n", dxw.TimeFreeze ? "freezed" : "unfreezed");
}

void dxwCore::MessagePump()
{
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
		OutTraceW("MESSAGEPUMP: msg=%x l-wParam=(%x,%x)\n", msg.message, msg.lParam, msg.wParam);
		if((msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST)) break; // do not consume keyboard inputs
		if((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST)) break; // do not consume mouse inputs
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void dxwCore::Mark(HDC hdc, BOOL scale, COLORREF color, int x, int y, int cx, int cy)
{
	RECT frame;
	HBRUSH brush = CreateSolidBrush(color);
	frame.left = x; frame.top = y;
	frame.right = x+cx; frame.bottom = y+cy;
	if(scale) dxw.MapClient(&frame);
	(*pFrameRect)(hdc, &frame, brush);
	DeleteObject(brush);
}

void dxwCore::Mark(HDC hdc, BOOL scale, COLORREF color, RECT frame)
{
	HBRUSH brush = CreateSolidBrush(color);
	if(scale) dxw.MapClient(&frame);
	(*pFrameRect)(hdc, &frame, brush);
	DeleteObject(brush);
}
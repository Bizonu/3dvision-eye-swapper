////////////////////////////////////////////////////////////////////////////////////////////////////
///
///  File:        3DVisionEyeSwapper.cpp
///  Description: Application main entry point.
///  Author:      Chiuta Adrian Marius
///  Created:     23-11-2013
///
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  http://www.apache.org/licenses/LICENSE-2.0
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
///
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "3DVisionEyeSwapper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Enable Visual Styles
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Variables:
HINSTANCE       hInst;								    // current instance
HWND            hWnd;                                   // the main window handle
HICON           hIcon;
Registry::Key*  regStereo3D     = nullptr;              // The registry used to control the eye swapper
volatile bool   eyesSwapped     = true;
volatile bool   trayInitialized = false;

static const TCHAR* szTitle             = _T("3DVisionEyeSwapper");				// The title bar text
static const TCHAR* szWindowClass       = _T("C3DVISIONEYESWAPPER");			// the main window class name
static const TCHAR* keyStereo3D_x86_64  = _T("SOFTWARE\\Wow6432Node\\NVIDIA Corporation\\Global\\Stereo3D");
static const TCHAR* keyStereo3D_x86_32  = _T("SOFTWARE\\NVIDIA Corporation\\Global\\Stereo3D");

////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations:
BOOL                InitWindows(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
BOOL                Is64BitWindows();
LRESULT CALLBACK    OnIconMessage(WPARAM wParam, LPARAM lParam);
void                UpdateEyes(Registry::Key &key);
void                UpdateTray(bool showInfo);
void                CloseTray();

////////////////////////////////////////////////////////////////////////////////////////////////////
int APIENTRY _tWinMain(_In_ HINSTANCE       hInstance,
                       _In_opt_ HINSTANCE   hPrevInstance,
                       _In_ LPTSTR          lpCmdLine,
                       _In_ int             nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    if (!InitWindows(hInstance, SW_HIDE))
		return FALSE;

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Initialize the windows needed by the application
///
/// @param hInstance
/// @param nCmdShow
///
/// @return
///     TRUE if everything is OK.
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL InitWindows(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable
    hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY3DVISIONEYESWAPPER));

    WNDCLASSEX     wcex = { 0 };
    wcex.cbSize			= sizeof(WNDCLASSEX);
    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon			= hIcon;
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName	= NULL;
    wcex.lpszClassName	= szWindowClass;
    wcex.hIconSm		= hIcon;

    if (RegisterClassEx(&wcex))
    {
	    hWnd = CreateWindow(szWindowClass, szTitle,
						    WS_OVERLAPPEDWINDOW,
		                    CW_USEDEFAULT, 0,
						    CW_USEDEFAULT, 0,
						    NULL, NULL,
						    hInstance, NULL);

	    if (hWnd)
	    {
            ShowWindow(hWnd, nCmdShow);
            UpdateWindow(hWnd);

		    return TRUE;
	    }
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
    case WM_CREATE:
        {
            ::hWnd = hWnd;

            regStereo3D = Registry::Key::Open( Registry::PredefinedKey::Local_Machine,
                                               Is64BitWindows() ? keyStereo3D_x86_64 : keyStereo3D_x86_32,
                                               Registry::AccessRights::Read | Registry::AccessRights::Write );

            if(regStereo3D != nullptr)
            {
                regStereo3D->AddNotify( [] (Registry::Key &key, void *userData) -> bool
                    {
                        UNREFERENCED_PARAMETER(key);
                        UNREFERENCED_PARAMETER(userData);

                        UpdateEyes(key);

                        return true;
                    }
                );

                UpdateEyes(*regStereo3D);
            }

            UpdateTray(true);
        }break;

    case WM_COMMAND:
        {
            int wmId    = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

            case IDM_EXIT:
                {
                    CloseTray();

                    if(regStereo3D != nullptr)
                        regStereo3D->Close();

                    DestroyWindow(hWnd);
                }break;

            case IDM_SWAP_EYES:
                {
                    eyesSwapped ^= true;
                    if(regStereo3D != nullptr)
                    {
                        UpdateTray(false);
                        UpdateEyes(*regStereo3D);
                    }

                }break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }break;

    case IDM_TRAY_MESSAGE:
        {
            OnIconMessage(wParam, lParam);
        }break;

	case WM_PAINT:
        {
	        PAINTSTRUCT ps;
	        HDC hdc = BeginPaint(hWnd, &ps);
		    EndPaint(hWnd, &ps);
        }break;

    case WM_DESTROY:
        {
            PostQuitMessage(0);
        }break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK OnIconMessage(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    if (lParam == WM_RBUTTONUP)
    {
        POINT	mouse;
        GetCursorPos(&mouse);

        HMENU hMenu;

        hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING | (eyesSwapped ? MF_CHECKED : 0), IDM_SWAP_EYES, _T("Swap eyes"));
        AppendMenu(hMenu, MF_STRING, 0, NULL);
        AppendMenu(hMenu, MF_STRING, IDM_ABOUT, _T("About"));
        AppendMenu(hMenu, MF_STRING, IDM_EXIT, _T("Exit"));

        SetForegroundWindow(hWnd);
        TrackPopupMenu(hMenu, TPM_LEFTBUTTON, mouse.x, mouse.y, 0, hWnd, NULL);
        PostMessage(hWnd, WM_NULL, 0, 0);
        DestroyMenu(hMenu);
    }
     else if(lParam == WM_LBUTTONUP)
     {
         UpdateTray(true);
     }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UpdateEyes(Registry::Key &key)
{
    if(eyesSwapped)
    {
        key.SetValueDWORD(_T("InterleavePattern0"), 0xFF00FF00);
        key.SetValueDWORD(_T("InterleavePattern1"), 0xFF00FF00);
    }
    else
    {
        key.SetValueDWORD(_T("InterleavePattern0"), 0x00FF00FF);
        key.SetValueDWORD(_T("InterleavePattern1"), 0x00FF00FF);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UpdateTray(bool showInfo)
{
    NOTIFYICONDATA      nid = { 0 };
    nid.cbSize				= sizeof(NOTIFYICONDATA);
    nid.hWnd				= hWnd;
    nid.uID					= IDD_3DSWP_DIALOG;
    nid.uFlags				= NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.hIcon				= hIcon;
    nid.uCallbackMessage    = IDM_TRAY_MESSAGE;
    nid.uVersion            = NOTIFYICON_VERSION;
    nid.dwInfoFlags			= NIIF_USER;

    _tcscpy_s(nid.szInfoTitle,  64, _T("3D Vision Eye Swapper"));

    if(showInfo)
        nid.uFlags |= NIF_INFO;

    if(regStereo3D == nullptr)
    {
        nid.dwInfoFlags = NIIF_ERROR;
        _tcscpy_s(nid.szTip,   64, _T("Can't swap eyes. 3D Vision not enabled?"));
        _tcscpy_s(nid.szInfo, 256, _T("Can't swap the eyes. Check if 3D Vision is enabled!"));
    }
    else if(eyesSwapped)
    {
        _tcscpy_s(nid.szTip,   64, _T("Eyes are swapped."));
        _tcscpy_s(nid.szInfo, 256, _T("Eyes are swapped!"));
    }
    else
    {
        _tcscpy_s(nid.szTip,   64, _T("Eyes are NOT swapped."));
        _tcscpy_s(nid.szInfo, 256, _T("Eyes are NOT swapped!"));
    }

    if(!trayInitialized)
    {
        trayInitialized = true;
        Shell_NotifyIcon(NIM_ADD, &nid);
        Shell_NotifyIcon(NIM_SETVERSION, &nid);
    }
    else
        Shell_NotifyIcon(NIM_MODIFY, &nid);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CloseTray()
{
    NOTIFYICONDATA  nid = { 0 };
    nid.cbSize          = sizeof(NOTIFYICONDATA);
    nid.hWnd            = hWnd;
    nid.uID             = IDD_3DSWP_DIALOG;
    Shell_NotifyIcon(NIM_DELETE, &nid);

    trayInitialized     = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Is64BitWindows()
{
#if defined(_WIN64)
    return TRUE;  // 64-bit programs run only on Win64
#elif defined(_WIN32)
    // 32-bit programs run on both 32-bit and 64-bit Windows
    // so must sniff
    BOOL f64 = FALSE;
    return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#else
    return FALSE; // Win64 does not support Win16
#endif
}

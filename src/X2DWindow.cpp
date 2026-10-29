#include "X2DWindow.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XCrtFileAccess.h"
#include "XMMouseManager.h"
#include "XSystemMouseCursor.h"

#include <new>

namespace NXSys2DApp
{
	// Remaster replacement for Windows 0047B524: Unicode, DPI-aware, nonexclusive HWND.
	CWindow::CWindow(HINSTANCE instance, int showCommand, std::int32_t width, std::int32_t height, std::int32_t depth, bool fullscreen) : mWindowHandle(nullptr), mApplicationActive(false), mWindowActive(false), mInitialized(false), mMouseCaptureSuspended(false), mDisplay(nullptr), mInstance(instance), mShowCommand(showCommand)
	{
		static_assert(offsetof(CWindow, mDisplay) == (sizeof(void*) == 8 ? 0x10 : 0x08));
		static_assert(offsetof(CWindow, mInstance) == (sizeof(void*) == 8 ? 0x18 : 0x0C));
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(CDisplay));

		if (allocation == nullptr)
			return;

		mDisplay = ::new (allocation) CDisplay(fullscreen);
		WNDCLASSEXW windowClass{ sizeof(windowClass) };
		windowClass.lpfnWndProc = &CWindow::WindowProcedure;
		windowClass.hInstance = mInstance;
		windowClass.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
		windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
		windowClass.lpszClassName = WindowClassName;

		if (RegisterClassExW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			return;
		// The virtual desktop origin belongs to the primary monitor.
		constexpr POINT primaryMonitorPoint{ 0, 0 };
		MONITORINFO monitor{ sizeof(monitor) };

		if (!GetMonitorInfoW(MonitorFromPoint(primaryMonitorPoint, MONITOR_DEFAULTTOPRIMARY), &monitor))
			return;
		// Remaster: keep the hidden main window eligible for the taskbar on first show.
		mWindowHandle = CreateWindowExW(WS_EX_APPWINDOW, WindowClassName, L"OpenVikings", WS_POPUP, monitor.rcMonitor.left, monitor.rcMonitor.top, monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top, nullptr, nullptr, mInstance, this);

		if (mWindowHandle == nullptr)
			return;

		if (!mDisplay->StartUp(mWindowHandle, width, height, depth))
		{
			MessageBoxW(mWindowHandle, L"Could not initialize Direct3D 11 / DXGI presentation.", L"OpenVikings", MB_OK | MB_ICONERROR);

			return;
		}

		mDisplay->UpdateWindowPlacement();
		ShowWindow(mWindowHandle, mShowCommand == SW_SHOWMINIMIZED ? SW_SHOWMINIMIZED : SW_SHOW);
		// Request activation once at startup; later placement updates preserve focus.

		if (!IsIconic(mWindowHandle) && GetForegroundWindow() != mWindowHandle && !SetForegroundWindow(mWindowHandle))
			OutputDebugStringA("OpenVikings: Windows declined startup foreground activation.\n");

		UpdateWindow(mWindowHandle);
		mApplicationActive = GetForegroundWindow() == mWindowHandle;
		mWindowActive = mApplicationActive && !IsIconic(mWindowHandle);
		mInitialized = true;
	}

	LRESULT CALLBACK CWindow::WindowProcedure(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		auto* window = reinterpret_cast<CWindow*>(GetWindowLongPtrW(windowHandle, GWLP_USERDATA));

		if (message == WM_NCCREATE)
		{
			window = static_cast<CWindow*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
			SetWindowLongPtrW(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
			window->mWindowHandle = windowHandle;
		}

		if (window == nullptr)
			return DefWindowProcW(windowHandle, message, wParam, lParam);

		auto* application = CApplication::sTheObjectPtr;
		LRESULT result;

		if (application != nullptr && application->GetWindowHandle() == windowHandle)
		{
			result = application->HandleWindowMessage(windowHandle, message, wParam, lParam);
		}
		else
		{
			result = window->HandleWindowMessage(windowHandle, message, wParam, lParam);

			if (result != 0)
				result = DefWindowProcW(windowHandle, message, wParam, lParam);
		}

		if (message == WM_NCDESTROY)
		{
			SetWindowLongPtrW(windowHandle, GWLP_USERDATA, 0);
			window->mWindowHandle = nullptr;
		}
		// Internal dispatch uses zero for handled messages. Win32 expects TRUE
		// for a handled client cursor, after default processing has been suppressed.

		if (message == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT && result == 0)
			return TRUE;

		return result;
	}

	LRESULT CWindow::HandleWindowMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr;

		switch (message)
		{
			case WM_CLOSE:
				ClipCursor(nullptr);
				PostQuitMessage(0);
				return 0;
			case WM_DESTROY:
				ClipCursor(nullptr);

				if (mInitialized)
					PostQuitMessage(0);

				return 0;
			case WM_ACTIVATE:
				mWindowActive = LOWORD(wParam) != WA_INACTIVE && HIWORD(wParam) == 0;

				if (!mWindowActive)
					ClipCursor(nullptr);

				if (mouse != nullptr)
					mouse->Callback_WindowActivationChanged(mWindowActive);
				// Native 0047B6E7 returns nonzero so DefWindowProc sets input focus.
				return -1;
			case WM_ACTIVATEAPP:
				mApplicationActive = wParam != FALSE;

				if (!mApplicationActive)
					ClipCursor(nullptr);

				if (mouse != nullptr)
					mouse->Callback_WindowActivationChanged(mApplicationActive && mWindowActive);

				if (auto* application = CApplication::sTheObjectPtr)
				{
					if (application->applicationActivationCallback != nullptr)
						application->applicationActivationCallback();
				}

				return 0;
			case WM_KILLFOCUS:
				ClipCursor(nullptr);

				if (mouse != nullptr)
					mouse->Callback_WindowActivationChanged(false);

				return 0;
			case WM_ENTERSIZEMOVE:
			case WM_ENTERMENULOOP:
				mMouseCaptureSuspended = true;
				ClipCursor(nullptr);
				NXSysMisc::XWS_SystemMouseCursor_Show();
				return 0;
			case WM_EXITSIZEMOVE:
			case WM_EXITMENULOOP:
				mMouseCaptureSuspended = false;

				if (mouse != nullptr)
					mouse->ResetWindowsMouseState();

				return 0;
			case WM_MOVE:
			case WM_SIZE:
				if (mouse != nullptr)
					mouse->ResetWindowsMouseState();

				return 0;
			case WM_DISPLAYCHANGE:
				if (mInitialized && mDisplay != nullptr)
					mDisplay->UpdateWindowPlacement();

				if (mouse != nullptr)
					mouse->ResetWindowsMouseState();

				return 0;
			case WM_DPICHANGED:
				if (mDisplay != nullptr && mDisplay->IsFullscreen())
				{
					mDisplay->UpdateWindowPlacement();
				}
				else
				{
					const auto& bounds = *reinterpret_cast<const RECT*>(lParam);
					SetWindowPos(windowHandle, nullptr, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top, SWP_NOZORDER | SWP_NOACTIVATE);
				}

				if (mouse != nullptr)
					mouse->ResetWindowsMouseState();

				return 0;
			case WM_ERASEBKGND:
				return 0;
			case WM_PAINT:
			{
				PAINTSTRUCT paint{ };
				BeginPaint(windowHandle, &paint);
				EndPaint(windowHandle, &paint);

				if (mInitialized && mDisplay != nullptr)
					mDisplay->Update();

				return 0;
			}
			case WM_SETCURSOR:
				if (LOWORD(lParam) != HTCLIENT)
					return -1;

				if (mouse != nullptr)
				{
					mouse->ResetWindowsMouseState();
				}
				else
				{
					NXSysMisc::XWS_SystemMouseCursor_Reset();
				}

				return 0;
			case WM_SYSKEYUP:
				if (wParam == VK_RETURN)
				{
					auto* application = CApplication::sTheObjectPtr;

					if (application != nullptr && application->fullscreenToggle)
					{
						application->ToggleFullscreen();
						return 0;
					}
				}

				return -1;
			default:

				return -1;
		}
	}

	CWindow::~CWindow()
	{
		ClipCursor(nullptr);
		mInitialized = false;
		l_DeleteDisplay();

		if (mWindowHandle != nullptr)
			DestroyWindow(mWindowHandle);

		UnregisterClassW(WindowClassName, mInstance);
	}

	void CWindow::l_DeleteDisplay()
	{
		if (mDisplay == nullptr)
		{

			return;
		}

		mDisplay->~CDisplay();
		NXBasics::Runtime_Free(mDisplay);
		mDisplay = nullptr;
	}

	void CWindow::Update()
	{
		if (mDisplay != nullptr)
			mDisplay->Update();
	}

	// Kept for callers that show modal dialogs; DXGI never owns the desktop.
	void CWindow::FlipToGDISurface()
	{
		ClipCursor(nullptr);
	}

	void CWindow::ToggleFullscreen()
	{
		if (mDisplay != nullptr)
			mDisplay->ToggleFullscreen();
	}
}
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XBasicTypes.h"
#include "XCrtFileAccess.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"
#include "XSystemMouseCursor.h"

#include <Windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace NXSysMouseManager
{
	namespace
	{
		// Modern input state lives outside the original 0x54-byte mouse-manager layout.
		struct SRawMouseState
		{
			bool registered = false;
			std::uint32_t deltaX = 0;
			std::uint32_t deltaY = 0;
			bool absolutePositionValid = false;
			HANDLE absoluteDevice = nullptr;
			POINT absolutePosition{ };
			bool cursorPositionValid = false;
			POINT cursorPosition{ };
			bool buttonMessageTimeValid = false;
			std::uint32_t buttonMessageTime = 0;
			bool pendingMessageTimeValid = false;
			std::uint32_t inputMessageTime = 0;
		};

		SRawMouseState gRawMouse;

		bool HasMouseFocus()
		{
			const auto* application = NXSys2DApp::CApplication::sTheObjectPtr;

			return application != nullptr && application->IsMouseCaptureAllowed() && !IsIconic(application->GetWindowHandle()) && GetForegroundWindow() == application->GetWindowHandle();
		}

		bool MayConstrainCursor()
		{
			// A suspended process cannot release a cursor clip. Absolute devices may
			// be confined to the image, but must never be warped to a single pixel.

			return HasMouseFocus() && !IsDebuggerPresent();
		}

		void ClearRawMovement()
		{
			gRawMouse.deltaX = 0;
			gRawMouse.deltaY = 0;
			gRawMouse.absolutePositionValid = false;
			gRawMouse.cursorPositionValid = false;
			gRawMouse.pendingMessageTimeValid = false;
		}
	}

	// Original function: FUN_00403014.
	bool CMouseManager::IsAnyButtonDown() const
	{

		return mLeftButton || mMiddleButton || mRightButton;
	}

	// Original function: FUN_004792ec
	CMouseManager::CMouseManager()
	{
		// The desktop copies only the two fixed-width coordinates at offset zero.
		static_assert(offsetof(CMouseManager, mMouseX) == 0);
		static_assert(offsetof(CMouseManager, mMouseY) == 4);
		static_assert(offsetof(CMouseManager, mLastStateChangeTime) == 0x24);
		static_assert(offsetof(CMouseManager, mPositionCallback) == (sizeof(void*) == 8 ? 0x50 : 0x4C));
		static_assert(offsetof(CMouseManager, mHideSystemMouseCursorWhenFullscreen) == (sizeof(void*) == 8 ? 0x58 : 0x50));
		sTheObjectPtr = this;
		std::memset(this, 0, sizeof(*this));

		gRawMouse = { };
		const RAWINPUTDEVICE device{ 1, 0x02, 0, NXSys2DApp::CApplication::sTheObjectPtr->GetWindowHandle() };
		// Foreground delivery only; keep ordinary button and wheel messages enabled.
		gRawMouse.registered = RegisterRawInputDevices(&device, 1, sizeof(device)) != FALSE;

		if (!gRawMouse.registered)
			OutputDebugStringA("OpenVikings: raw mouse registration failed; using cursor-position fallback.\n");

		mHideSystemMouseCursorWhenFullscreen = false;
		ResetWindowsMouseState();
	}

	// Original function: FUN_00479323
	CMouseManager::~CMouseManager()
	{
		if (gRawMouse.registered)
		{
			const RAWINPUTDEVICE device{ 1, 0x02, RIDEV_REMOVE, nullptr };
			RegisterRawInputDevices(&device, 1, sizeof(device));
		}

		gRawMouse = { };

		ClipCursor(nullptr);

		NXSysMisc::XWS_SystemMouseCursor_SetNormal();
		NXSysMisc::XWS_SystemMouseCursor_Show();

		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_00479357
	void CMouseManager::UpdateMouseState()
	{
		if (HasMouseFocus())
		{
			l_UpdateMouseState();
			l_UpdateMouseButtonState();

			return;
		}

		ClearRawMovement();
		ClipCursor(nullptr);
		mDirectInputCursorLocked = false;
		mAccumulatedDeltaX = mAccumulatedDeltaY = mAccumulatedWheelDelta = 0;
		l_SetState_Total(false, false, false, mMouseX, mMouseY, 0, 0, 0);
	}

	void CMouseManager::HandleRawMouseInput(HRAWINPUT input)
	{
		if (!gRawMouse.registered || !HasMouseFocus())
			return;

		RAWINPUT raw{ };
		UINT bytes = sizeof(raw);
		const UINT read = GetRawInputData(input, RID_INPUT, &raw, &bytes, sizeof(RAWINPUTHEADER));

		if (read == static_cast<UINT>(-1) || read < sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE) || raw.header.dwType != RIM_TYPEMOUSE)
			return;

		// Do not replay old scroll movement when continuing from a debugger pause.

		if (IsDebuggerPresent() && GetTickCount() - static_cast<DWORD>(GetMessageTime()) > 250u)
		{
			ClearRawMovement();

			return;
		}

		const bool stopped = mPositionCallback != nullptr && mPositionCallback();
		const auto& mouse = raw.data.mouse;
		LONG deltaX = mouse.lLastX, deltaY = mouse.lLastY;

		if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
		{
			// Remote/absolute devices report normalized desktop coordinates.
			const bool virtualDesktop = (mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
			const int width = GetSystemMetrics(virtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
			const int height = GetSystemMetrics(virtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
			const int originX = virtualDesktop ? GetSystemMetrics(SM_XVIRTUALSCREEN) : 0;
			const int originY = virtualDesktop ? GetSystemMetrics(SM_YVIRTUALSCREEN) : 0;
			const POINT position{ originX + static_cast<LONG>(static_cast<std::int64_t>(mouse.lLastX) * width / 65535), originY + static_cast<LONG>(static_cast<std::int64_t>(mouse.lLastY) * height / 65535) };
			deltaX = deltaY = 0;

			if (gRawMouse.absolutePositionValid && gRawMouse.absoluteDevice == raw.header.hDevice)
			{
				deltaX = position.x - gRawMouse.absolutePosition.x;
				deltaY = position.y - gRawMouse.absolutePosition.y;
			}

			gRawMouse.absolutePosition = position;
			gRawMouse.absoluteDevice = raw.header.hDevice;
			gRawMouse.absolutePositionValid = true;
		}
		else
		{
			gRawMouse.absolutePositionValid = false;
		}

		if (stopped)
		{
			gRawMouse.deltaX += static_cast<std::uint32_t>(deltaX);
			gRawMouse.deltaY += static_cast<std::uint32_t>(deltaY);
		}
	}

	// Original function: FUN_0047938d
	void CMouseManager::UpdateMouseWheelState(int wheelDelta)
	{
		l_SetState_Wheel(wheelDelta);
		l_UpdateMouseButtonState();
	}

	// Original function: FUN_004793a4
	void CMouseManager::UpdateMouseButtonState(bool leftButton, bool middleButton, bool rightButton)
	{
		// This adapter is called only while dispatching legacy mouse messages.
		// Queue latency must not change the native 400 ms double-click interval.
		gRawMouse.buttonMessageTime = static_cast<std::uint32_t>(GetMessageTime());
		gRawMouse.buttonMessageTimeValid = true;
		gRawMouse.pendingMessageTimeValid = true;
		l_SetState_Total(leftButton, middleButton, rightButton, mMouseX, mMouseY, 0, 0, 0);
		gRawMouse.buttonMessageTimeValid = false;
	}

	// Original function: FUN_004793c2
	void CMouseManager::l_UpdateMouseState()
	{
		POINT cursorPosition{ };

		if (!GetCursorPos(&cursorPosition))
		{
			return;
		}

		NXSys2DApp::CApplication* application = NXSys2DApp::CApplication::sTheObjectPtr;

		if (application == nullptr)
		{
			return;
		}

		const auto* display = application->GetDisplay();

		if (display == nullptr)
		{

			return;
		}

		const POINT gamePosition = display->ScreenToGame(cursorPosition);
		const int mouseX = gamePosition.x;
		const int mouseY = gamePosition.y;
		const int fallbackDeltaX = gRawMouse.cursorPositionValid ? cursorPosition.x - gRawMouse.cursorPosition.x : 0;
		const int fallbackDeltaY = gRawMouse.cursorPositionValid ? cursorPosition.y - gRawMouse.cursorPosition.y : 0;

		const bool stopped = mPositionCallback != nullptr && mPositionCallback();

		if (stopped)
		{
			const int deltaX = gRawMouse.registered ? NXBasics::Runtime_TimeSigned32(gRawMouse.deltaX) : fallbackDeltaX;
			const int deltaY = gRawMouse.registered ? NXBasics::Runtime_TimeSigned32(gRawMouse.deltaY) : fallbackDeltaY;
			l_SetState_Total(mLeftButton, mMiddleButton, mRightButton, mMouseX, mMouseY, deltaX, deltaY, 0);
			l_UpdateWindowsMouseState();
			mDirectInputCursorLocked = MayConstrainCursor() && !gRawMouse.absolutePositionValid;
		}
		else
		{
			l_SetState_Position(mouseX, mouseY);
			l_UpdateWindowsMouseState();
			mDirectInputCursorLocked = false;
		}

		gRawMouse.deltaX = 0;
		gRawMouse.deltaY = 0;
		// Use the post-warp position as the fallback baseline, not the previous sampled position.
		gRawMouse.cursorPositionValid = GetCursorPos(&gRawMouse.cursorPosition) != FALSE;
	}

	// Original function: FUN_004794eb
	void CMouseManager::l_UpdateMouseButtonState()
	{
		const bool leftButton = (GetKeyState(VK_LBUTTON) & NXBasics::BitMask::Bit15Signed) != 0;
		const bool middleButton = (GetKeyState(VK_MBUTTON) & NXBasics::BitMask::Bit15Signed) != 0;
		const bool rightButton = (GetKeyState(VK_RBUTTON) & NXBasics::BitMask::Bit15Signed) != 0;

		l_SetState_Total(leftButton, middleButton, rightButton, mMouseX, mMouseY, 0, 0, 0);
	}

	// Original function: FUN_0047953d
	void CMouseManager::ResetWindowsMouseState()
	{
		l_UpdateWindowsMouseState();

		NXSys2DApp::CApplication* application = NXSys2DApp::CApplication::sTheObjectPtr;

		if (MayConstrainCursor() && application->IsFullscreen() && mHideSystemMouseCursorWhenFullscreen)
		{
			NXSysMisc::XWS_SystemMouseCursor_Hide();
		}
		else
		{
			NXSysMisc::XWS_SystemMouseCursor_Show();
		}

		NXSysMisc::XWS_SystemMouseCursor_Reset();
	}

	// Original function: FUN_0047956d
	void CMouseManager::l_UpdateWindowsMouseState()
	{
		NXSys2DApp::CApplication* application = NXSys2DApp::CApplication::sTheObjectPtr;

		const auto* display = application != nullptr ? application->GetDisplay() : nullptr;

		if (!MayConstrainCursor() || display == nullptr)
		{
			ClipCursor(nullptr);
			return;
		}

		if (mPositionCallback != nullptr && mPositionCallback() && !gRawMouse.absolutePositionValid)
		{
			const POINT cursor = display->GameToScreen({ mMouseX, mMouseY });
			const int cursorX = cursor.x;
			const int cursorY = cursor.y;

			SetCursorPos(cursorX, cursorY);

			RECT cursorRectangle{ cursorX, cursorY, cursorX + 1, cursorY + 1 };
			ClipCursor(&cursorRectangle);
			return;
		}

		if (mMouseAreaEnabled)
		{
			const POINT first = display->GameToScreen({ mMouseAreaX, mMouseAreaY });
			const POINT last = display->GameToScreen({ mMouseAreaX + mMouseAreaWidth, mMouseAreaY + mMouseAreaHeight });
			RECT mouseArea{ first.x, first.y, last.x, last.y };
			RECT imageArea = display->GetPresentationRectangle();
			MapWindowPoints(application->GetWindowHandle(), nullptr, reinterpret_cast<POINT*>(&imageArea), 2);
			RECT clipped{ };

			if (IntersectRect(&clipped, &mouseArea, &imageArea))
			{
				ClipCursor(&clipped);
			}
			else
			{
				ClipCursor(nullptr);
			}

			return;
		}

		if (application->IsFullscreen())
		{
			RECT displayArea = display->GetPresentationRectangle();
			MapWindowPoints(application->GetWindowHandle(), nullptr, reinterpret_cast<POINT*>(&displayArea), 2);

			if (displayArea.right > displayArea.left && displayArea.bottom > displayArea.top)
			{
				ClipCursor(&displayArea);
			}
			else
			{
				ClipCursor(nullptr);
			}

			return;
		}

		ClipCursor(nullptr);
	}

	// Original function: FUN_0047969a
	void CMouseManager::Callback_WindowActivationChanged(bool active)
	{
		ClearRawMovement();

		if (!active)
		{
			ClipCursor(nullptr);
			mDirectInputCursorLocked = false;
			mAccumulatedDeltaX = mAccumulatedDeltaY = mAccumulatedWheelDelta = 0;
			mPreviousLeftButtonClickTime = 0;
			mLeftButtonPressed = mMiddleButtonPressed = mRightButtonPressed = mLeftButtonDoubleClicked = false;
			l_SetState_Total(false, false, false, mMouseX, mMouseY, 0, 0, 0);
			NXSysMisc::XWS_SystemMouseCursor_Show();

			return;
		}

		ResetWindowsMouseState();
	}

	// Original function: FUN_004796af
	void CMouseManager::Callback_FullscreenToggled(bool fullscreen)
	{
		(void)fullscreen;

		l_UpdateWindowsMouseState();
	}

	// Original function: FUN_004796b7
	void CMouseManager::l_SetState_Position(int mouseX, int mouseY)
	{
		l_SetState_Total(mLeftButton, mMiddleButton, mRightButton, mouseX, mouseY, mouseX - mMouseX, mouseY - mMouseY, 0);
	}

	// Original function: FUN_004796ea
	void CMouseManager::l_SetState_Wheel(int wheelDelta)
	{
		l_SetState_Total(mLeftButton, mMiddleButton, mRightButton, mMouseX, mMouseY, 0, 0, wheelDelta);
	}

	// Original function: FUN_00479711
	void CMouseManager::l_SetState_Total(bool leftButton, bool middleButton, bool rightButton, int mouseX, int mouseY, int deltaX, int deltaY, int wheelDelta)
	{
		mAccumulatedDeltaX += deltaX;
		mAccumulatedDeltaY += deltaY;
		mAccumulatedWheelDelta += wheelDelta;

		bool newLeftButtonPress = false;

		if (mLeftButton != leftButton)
		{
			if (leftButton)
			{
				if (!mLeftButton)
				{
					newLeftButtonPress = true;
				}

				mLeftButtonPressed = true;
			}
			else
			{
				mLeftButtonReleased = true;
			}

			mLeftButton = leftButton;
			mStateChanged = true;
			mLastStateChangeTime = timeGetTime();
		}

		if (mMiddleButton != middleButton)
		{
			if (middleButton)
			{
				mMiddleButtonPressed = true;
			}
			else
			{
				mMiddleButtonReleased = true;
			}

			mMiddleButton = middleButton;
			mStateChanged = true;
			mLastStateChangeTime = timeGetTime();
		}

		if (mRightButton != rightButton)
		{
			if (rightButton)
			{
				mRightButtonPressed = true;
			}
			else
			{
				mRightButtonReleased = true;
			}

			mRightButton = rightButton;
			mStateChanged = true;
			mLastStateChangeTime = timeGetTime();
		}

		if (mPositionCallback != nullptr && mPositionCallback())
		{
			if (deltaX != 0 || deltaY != 0)
			{
				mPositionChanged = true;
				mStateChanged = true;
				mLastStateChangeTime = timeGetTime();
			}
		}
		else if (mMouseX != mouseX || mMouseY != mouseY)
		{
			mPositionChanged = true;
			mMouseX = mouseX;
			mMouseY = mouseY;

			if (mMouseAreaEnabled)
			{
				NXBasics::SPoint position{ mMouseX, mMouseY };
				const NXBasics::SRectangle rectangle(mMouseAreaX, mMouseAreaY, mMouseAreaWidth, mMouseAreaHeight);
				position.PlaceInside(rectangle);
				mMouseX = position.x;
				mMouseY = position.y;
			}

			mStateChanged = true;
			mLastStateChangeTime = timeGetTime();
		}

		if (mPositionChanged && mPreviousLeftButtonClickTime != 0)
		{
			if (std::abs(mPreviousLeftButtonClickX - mMouseX) > 4 || std::abs(mPreviousLeftButtonClickY - mMouseY) > 4)
			{
				mPreviousLeftButtonClickTime = 0;
			}
		}

		if (newLeftButtonPress)
		{
			const std::uint32_t currentTime = gRawMouse.buttonMessageTimeValid ? gRawMouse.buttonMessageTime : timeGetTime();

			if (currentTime - mPreviousLeftButtonClickTime < 400)
			{
				mPreviousLeftButtonClickTime = 0;
				mLeftButtonDoubleClicked = true;
				mStateChanged = true;
				mLastStateChangeTime = timeGetTime();
			}
			else
			{
				mPreviousLeftButtonClickTime = currentTime;
				mPreviousLeftButtonClickX = mMouseX;
				mPreviousLeftButtonClickY = mMouseY;
			}
		}

		if (wheelDelta != 0)
		{
			mStateChanged = true;
			mLastStateChangeTime = timeGetTime();
		}
	}

	// Original function: FUN_00479880
	void CMouseManager::GetNewMouseMessage(SMouseMessage& mouseMessage)
	{
		mouseMessage = { };

		if (!mStateChanged)
		{

			return;
		}

		mStateChanged = false;
		gRawMouse.inputMessageTime = gRawMouse.pendingMessageTimeValid ? gRawMouse.buttonMessageTime : timeGetTime();
		gRawMouse.pendingMessageTimeValid = false;

		mouseMessage.mouseX = mMouseX;
		mouseMessage.mouseY = mMouseY;
		mouseMessage.deltaX = mAccumulatedDeltaX;
		mouseMessage.deltaY = mAccumulatedDeltaY;
		mouseMessage.wheelDelta = mAccumulatedWheelDelta;

		mAccumulatedDeltaX = 0;
		mAccumulatedDeltaY = 0;
		mAccumulatedWheelDelta = 0;

		if (mLeftButtonReleased)
		{
			mouseMessage.flags |= static_cast<std::uint32_t>(EMouseMessageFlags::LeftButtonReleased);

			mLeftButtonReleased = false;
		}

		if (mLeftButtonPressed)
		{
			mouseMessage.flags |= static_cast<std::uint32_t>(EMouseMessageFlags::LeftButtonPressed);

			mLeftButtonPressed = false;
		}

		if (mMiddleButtonReleased)
		{
			mouseMessage.flags |= static_cast<std::uint32_t>(EMouseMessageFlags::MiddleButtonReleased);

			mMiddleButtonReleased = false;
		}

		if (mMiddleButtonPressed)
		{
			mouseMessage.flags |= static_cast<std::uint32_t>(EMouseMessageFlags::MiddleButtonPressed);

			mMiddleButtonPressed = false;
		}

		if (mRightButtonReleased)
		{
			mouseMessage.flags |= static_cast<std::uint32_t>(EMouseMessageFlags::RightButtonReleased);

			mRightButtonReleased = false;
		}

		if (mRightButtonPressed)
		{
			mouseMessage.flags |= static_cast<std::uint32_t>(EMouseMessageFlags::RightButtonPressed);

			mRightButtonPressed = false;
		}

		if (mLeftButtonDoubleClicked)
		{
			mouseMessage.flags |= static_cast<std::uint32_t>(EMouseMessageFlags::LeftButtonDoubleClicked);

			mLeftButtonDoubleClicked = false;
		}

		if (mPositionChanged)
		{
			mouseMessage.flags |= static_cast<std::uint32_t>(EMouseMessageFlags::PositionChanged);

			mPositionChanged = false;
		}

		if (mouseMessage.wheelDelta != 0)
		{
			mouseMessage.flags |= static_cast<std::uint32_t>(EMouseMessageFlags::WheelChanged);
		}
	}

	std::uint32_t CMouseManager::GetInputMessageTime() const
	{

		return gRawMouse.inputMessageTime;
	}

	// Original function: FUN_00479923
	void CMouseManager::SetHideSystemMouseCursorWhenFullscreenState(bool hideSystemMouseCursor)
	{
		mHideSystemMouseCursorWhenFullscreen = hideSystemMouseCursor;

		if (hideSystemMouseCursor)
		{
			NXSysMisc::XWS_SystemMouseCursor_Hide();
		}
		else
		{
			NXSysMisc::XWS_SystemMouseCursor_Show();
		}
	}
}
#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>

namespace NXSysMouseManager
{
	enum class EMouseMessageFlags : std::uint32_t
	{
		None = 0,
		PositionChanged = 1,
		WheelChanged = 0x002,
		LeftButtonPressed = 0x004,
		LeftButtonReleased = 0x008,
		MiddleButtonPressed = 0x010,
		MiddleButtonReleased = 0x020,
		RightButtonPressed = 0x040,
		RightButtonReleased = 0x080,
		LeftButtonDoubleClicked = 0x100
	};

	struct SMouseMessage
	{
		std::uint32_t flags;     // +0x00
		std::int32_t mouseX;     // +0x04
		std::int32_t mouseY;     // +0x08
		std::int32_t deltaX;     // +0x0C
		std::int32_t deltaY;     // +0x10
		std::int32_t wheelDelta; // +0x14
	};

	static_assert(sizeof(SMouseMessage) == 24);

	class CMouseManager
	{
	  public:
		// Original function: FUN_004792ec
		CMouseManager();

		// Original function: FUN_00479323
		~CMouseManager();
		CMouseManager(const CMouseManager&) = delete;
		CMouseManager& operator=(const CMouseManager&) = delete;

		// Original function: FUN_00479357
		void UpdateMouseState();

		// Original function: FUN_0047938d
		void UpdateMouseWheelState(int wheelDelta);

		// Original function: FUN_004793a4
		void UpdateMouseButtonState(bool leftButton, bool middleButton, bool rightButton);

		// Raw Input adapter; the original object layout and GUI message format remain unchanged.
		void HandleRawMouseInput(HRAWINPUT input);
		// Timestamp of the GUI event most recently consumed by the desktop.
		std::uint32_t GetInputMessageTime() const;

		// Original function: FUN_0047953d
		void ResetWindowsMouseState();

		// Original function: FUN_0047969a
		void Callback_WindowActivationChanged(bool active);

		// Original function: FUN_004796af
		void Callback_FullscreenToggled(bool fullscreen);

		// Original function: FUN_00479880
		void GetNewMouseMessage(SMouseMessage& mouseMessage);

		// Original function: FUN_00479923
		void SetHideSystemMouseCursorWhenFullscreenState(bool hideSystemMouseCursor);

		inline static CMouseManager* sTheObjectPtr = nullptr;

		bool IsMiddleButtonDown() const noexcept
		{

			return mMiddleButton;
		}

		bool IsLeftButtonDown() const noexcept
		{

			return mLeftButton;
		}

		bool IsAnyButtonDown() const;

		void SetPositionOverrideCallback(bool(__cdecl* callback)()) noexcept
		{
			mPositionCallback = callback;
		}

	  private:
		using TMousePositionOverrideCallback = bool(__cdecl*)();

		// Original function: FUN_004793c2
		void l_UpdateMouseState();

		// Original function: FUN_004794eb
		void l_UpdateMouseButtonState();

		// Original function: FUN_0047956d
		void l_UpdateWindowsMouseState();

		// Original function: FUN_004796b7
		void l_SetState_Position(int mouseX, int mouseY);

		// Original function: FUN_004796ea
		void l_SetState_Wheel(int wheelDelta);

		// Original function: FUN_00479711
		void l_SetState_Total(bool leftButton, bool middleButton, bool rightButton, int mouseX, int mouseY, int deltaX, int deltaY, int wheelDelta);

		std::int32_t mMouseX;                             // +0x00
		std::int32_t mMouseY;                             // +0x04
		bool mLeftButton;                                 // +0x08
		bool mMiddleButton;                               // +0x09
		bool mRightButton;                                // +0x0A
		std::byte mUnknown0B;                             // +0x0B
		bool mMouseAreaEnabled;                           // +0x0C
		std::byte mPadding0D[3];                          // +0x0D
		std::int32_t mMouseAreaX;                         // +0x10
		std::int32_t mMouseAreaY;                         // +0x14
		std::int32_t mMouseAreaWidth;                     // +0x18
		std::int32_t mMouseAreaHeight;                    // +0x1C
		bool mStateChanged;                               // +0x20
		std::byte mPadding21[3];                          // +0x21
		std::uint32_t mLastStateChangeTime;               // +0x24
		std::int32_t mAccumulatedDeltaX;                  // +0x28
		std::int32_t mAccumulatedDeltaY;                  // +0x2C
		std::int32_t mAccumulatedWheelDelta;              // +0x30
		bool mPositionChanged;                            // +0x34
		bool mLeftButtonReleased;                         // +0x35
		bool mLeftButtonPressed;                          // +0x36
		bool mMiddleButtonReleased;                       // +0x37
		bool mMiddleButtonPressed;                        // +0x38
		bool mRightButtonReleased;                        // +0x39
		bool mRightButtonPressed;                         // +0x3A
		bool mLeftButtonDoubleClicked;                    // +0x3B
		std::uint32_t mPreviousLeftButtonClickTime;       // +0x3C
		std::int32_t mPreviousLeftButtonClickX;           // +0x40
		std::int32_t mPreviousLeftButtonClickY;           // +0x44
		bool mDirectInputCursorLocked;                    // +0x48
		std::byte mUnknown49[3];                          // +0x49
		TMousePositionOverrideCallback mPositionCallback; // +0x4C
		bool mHideSystemMouseCursorWhenFullscreen;        // +0x50
		std::byte mPadding51[3];                          // +0x51
	};

	static_assert(sizeof(CMouseManager) == (sizeof(void*) == 8 ? 96 : 84), "Unexpected native mouse manager layout");
}
#pragma once

#include <Windows.h>

#include "X2DWindow.h"
#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CBitmap;
}

namespace NXSys2DApp
{
	char FindCDDrive(const char* volumeName, const char* markerFileName);

	using TApplicationCallback = void(__cdecl*)();
	using TWindowMessageCallback = int(__cdecl*)(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

	struct SApplicationControl
	{
		// Original function: FUN_00479076
		SApplicationControl();

		HINSTANCE instance;
		HINSTANCE previousInstance;
		LPSTR commandLine;
		int showCommand;

		bool fullscreen;
		std::byte padding11[3];

		std::int32_t screenWidth;
		std::int32_t screenHeight;
		std::int32_t screenDepth;

		std::int32_t minimumScreenWidth;
		std::int32_t minimumScreenHeight;
		std::int32_t minimumScreenDepth;

		std::int32_t maximumScreenWidth;
		std::int32_t maximumScreenHeight;
		std::int32_t maximumScreenDepth;

		bool fullscreenToggle;  // +0x38
		std::byte padding39[3]; // +0x39

		TApplicationCallback fullscreenToggledCallback;     // +0x3C
		TApplicationCallback applicationActivationCallback; // +0x40
		TWindowMessageCallback windowMessageCallback;       // +0x44
	};

	static_assert(sizeof(SApplicationControl) == (sizeof(void*) == 8 ? 96 : 72), "Unexpected native application control layout");

	struct SDisplayMode
	{
		std::uint32_t width;
		std::uint32_t height;
		std::uint32_t depth;
	};

	static_assert(sizeof(SDisplayMode) == 12);

	struct SDisplayModeData
	{
		std::uint32_t desktopWidth;
		std::uint32_t desktopHeight;
		std::uint32_t desktopDepth;
		std::uint32_t fullscreenDisplayModeCount;
		SDisplayMode fullscreenDisplayModes[80];
	};

	static_assert(sizeof(SDisplayModeData) == 976);

	class CWindow;

	class CApplication : public SApplicationControl
	{
	  public:
		// Original function: FUN_00478c02
		bool SetDisplayMode(std::int32_t width, std::int32_t height, std::uint32_t depth, bool skipDisplayModeValidation);

		// Original function: FUN_00478c98
		bool ToggleFullscreen();

		// Original function: FUN_00478ce0
		void FlipToGDISurface();

		// Original function: FUN_00478ced
		HWND GetWindowHandle() const;
		CDisplay* GetDisplay() const;

		bool IsMouseCaptureAllowed() const
		{

			return mWindow != nullptr && mWindow->IsMouseCaptureAllowed();
		}

		// Original function: FUN_00478cfa
		bool IsWindowActive() const;

		// Original function: FUN_00478d08
		bool IsApplicationActive() const;

		// Original function: FUN_00478d16
		int GetDisplayWidth() const;

		// Original function: FUN_00478d27
		int GetDisplayHeight() const;

		// Original function: FUN_00478d38
		int GetWindowOffsetX() const;

		// Original function: FUN_00478d4b
		int GetWindowOffsetY() const;

		// Original function: FUN_00478d5e
		int GetDisplayDepth() const;

		// Original function: FUN_00478d6f
		bool IsFullscreen() const;

		// Original function: FUN_00478efa
		bool CanDisplayModeBeActivated(bool fullscreen, std::uint32_t width, std::uint32_t height, std::uint32_t depth) const;
		bool CheckFullscreenDisplayMode(std::uint32_t width, std::uint32_t height, std::uint32_t depth) const;

		std::uint32_t GetDesktopDepth() const;

		const SDisplayModeData& GetDisplayModeData() const
		{

			return mDisplayModeData;
		}

		void BitmapToScreen(NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y);

		// Original function: FUN_004790cd
		explicit CApplication(const SApplicationControl& applicationControl);
		CApplication(const CApplication&) = delete;
		CApplication& operator=(const CApplication&) = delete;

		// Original function: FUN_00479169
		~CApplication();

		// Original function: FUN_004791a6
		bool StartUp();

		// Original function: FUN_0047929c
		void ShutDown();

		// Original function: FUN_00479a54
		void UpdateDisplay();

		// Original function: FUN_00479a61
		LRESULT HandleWindowMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

		HINSTANCE GetInstance() const;

		// Original function: FUN_004799d8
		bool DoMessages();

		// Original data: 00554d74.
		inline static CApplication* sTheObjectPtr = nullptr;

	  private:
		// Original function: FUN_00478d8a
		void l_GetDesktopDisplayMode();

		// Original function: FUN_00478ddf
		void l_GetFullscreenDisplayModes();

		bool mOwnsSystemManager;           // +0x48
		std::byte mPadding49[3];           // +0x49
		CWindow* mWindow;                  // +0x4C
		UINT mMouseWheelMessage;           // +0x50
		DWORD mDirectXVersion;             // +0x54
		DWORD mDirectXPlatform;            // +0x58
		SDisplayModeData mDisplayModeData; // +0x5C
	};

	static_assert(sizeof(CApplication) == (sizeof(void*) == 8 ? 1104 : 1068), "Unexpected native application layout");
}
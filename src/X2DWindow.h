#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>

namespace NXSys2DApp
{
	class CDisplay;

	class CWindow
	{
	  public:
		// Original function: FUN_0047b524
		CWindow(HINSTANCE instance, int showCommand, std::int32_t width, std::int32_t height, std::int32_t depth, bool fullscreen);

		// Original function: FUN_0047b662
		LRESULT HandleWindowMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

		~CWindow();
		CWindow(const CWindow&) = delete;
		CWindow& operator=(const CWindow&) = delete;

		// Original function: FUN_0047b888
		void Update();

		// Original function: FUN_0047b895
		void FlipToGDISurface();

		// Original function: FUN_0047b8a2
		void ToggleFullscreen();

		HWND GetWindowHandle() const
		{

			return mWindowHandle;
		}

		bool IsApplicationActive() const
		{

			return mApplicationActive;
		}

		bool IsWindowActive() const
		{

			return mWindowActive;
		}

		bool IsMouseCaptureAllowed() const
		{

			return mWindowActive && mApplicationActive && !mMouseCaptureSuspended;
		}

		bool IsInitialized() const
		{

			return mInitialized;
		}

		CDisplay* GetDisplay() const
		{

			return mDisplay;
		}

	  private:
		void l_DeleteDisplay();

		static constexpr const wchar_t* WindowClassName = L"OpenVikings.Remaster.Window";

		// Original function: FUN_0047b638
		static LRESULT CALLBACK WindowProcedure(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

		HWND mWindowHandle;          // +0x00
		bool mApplicationActive;     // +0x04
		bool mWindowActive;          // +0x05
		bool mInitialized;           // +0x06
		bool mMouseCaptureSuspended; // Remaster state in the former padding byte.
		CDisplay* mDisplay;          // +0x08
		HINSTANCE mInstance;         // +0x0C
		int mShowCommand;            // +0x10
	};

	static_assert(sizeof(CWindow) == (sizeof(void*) == 8 ? 40 : 20), "Unexpected native window layout");
}
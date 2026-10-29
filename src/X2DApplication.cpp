#include "OriginalStrings.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XCrtFileAccess.h"
#include "XMKeyManager.h"
#include "XMMessageDialogs.h"
#include "XMMouseManager.h"
#include "XSystemManager.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <cstring>
#include <new>
#include <Windows.h>

#pragma comment(lib, "Ole32.lib")

namespace NXSys2DApp
{
	// Original function: FUN_00478f4e.
	std::uint32_t CApplication::GetDesktopDepth() const
	{

		return mDisplayModeData.desktopDepth;
	}

	// Original function: FUN_00479052.
	void CApplication::BitmapToScreen(NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y)
	{
		if (mWindow != nullptr)
		{
			if (auto* display = mWindow->GetDisplay())
				display->BitmapToScreen(bitmap, x, y);
		}
	}

	// Original function: FUN_00479076
	SApplicationControl::SApplicationControl()
	{
		static_assert(offsetof(SApplicationControl, screenWidth) == (sizeof(void*) == 8 ? 0x20 : 0x14));
		static_assert(offsetof(SApplicationControl, fullscreenToggle) == (sizeof(void*) == 8 ? 0x44 : 0x38));
		static_assert(offsetof(SApplicationControl, fullscreenToggledCallback) == (sizeof(void*) == 8 ? 0x48 : 0x3C));
		static_assert(offsetof(SApplicationControl, windowMessageCallback) == (sizeof(void*) == 8 ? 0x58 : 0x44));
		std::memset(this, 0, sizeof(*this));

		screenWidth = 800;
		screenHeight = 600;
		screenDepth = 32;

		minimumScreenWidth = 640;
		minimumScreenHeight = 400;
		minimumScreenDepth = 32;

		maximumScreenWidth = 1600;
		maximumScreenHeight = 1200;
		maximumScreenDepth = 32;

		fullscreenToggle = true;
	}

	// Original function: FUN_004790cd
	CApplication::CApplication(const SApplicationControl& applicationControl) : SApplicationControl(applicationControl),
	                                                                            mOwnsSystemManager(false),
	                                                                            mPadding49{ },
	                                                                            mWindow(nullptr),
	                                                                            mMouseWheelMessage(0),
	                                                                            mDirectXVersion(0),
	                                                                            mDirectXPlatform(0),
	                                                                            mDisplayModeData{ }
	{
		static_assert(offsetof(CApplication, mWindow) == (sizeof(void*) == 8 ? 0x68 : 0x4C));
		static_assert(offsetof(CApplication, mMouseWheelMessage) == (sizeof(void*) == 8 ? 0x70 : 0x50));
		static_assert(offsetof(CApplication, mDisplayModeData) == (sizeof(void*) == 8 ? 0x7C : 0x5C));
		sTheObjectPtr = this;

		if (NXBasics::CXBSystemManager::sTheObjectPtr == nullptr)
		{
			new (std::nothrow) NXBasics::CXBSystemManager();
			mOwnsSystemManager = true;
		}

		NXBasics::CXBSystemManager::sFatalErrorFunctionPtr = reinterpret_cast<void*>(&NXSysMisc::XWS_AbortBox);
		NXBasics::CXBSystemManager::sWarningErrorFunctionPtr = reinterpret_cast<void*>(&NXSysMisc::XWS_WarningBox);
		NXBasics::CXBSystemManager::sAssertFunctionPtr = reinterpret_cast<void*>(&NXSysMisc::XWS_AssertBox);

		mMouseWheelMessage = RegisterWindowMessageA("MSWHEEL_ROLLMSG");

		l_GetDesktopDisplayMode();
		l_GetFullscreenDisplayModes();
	}

	HINSTANCE CApplication::GetInstance() const
	{

		return instance;
	}

	// Original function: FUN_00478c02
	bool CApplication::SetDisplayMode(std::int32_t width, std::int32_t height, std::uint32_t depth, bool skipDisplayModeValidation)
	{
		CDisplay* display = GetDisplay();

		if (display == nullptr)
		{
			return false;
		}

		if (!skipDisplayModeValidation)
		{
			const bool targetFullscreen = IsFullscreen();

			if (!CanDisplayModeBeActivated(targetFullscreen, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), depth))
			{
				NXSysMisc::XWS_WarningBoxV("NXSys2DApp::CApplication::SetDisplayMode\n\n display mode not available (%d*%d-%d)", width, height, depth);
				return false;
			}
		}

		if (!display->SetDisplayMode(width, height, static_cast<std::int32_t>(depth)))
		{
			NXSysMisc::XWS_WarningBoxV("NXSys2DApp::CApplication::SetDisplayMode\n\n could not set display mode (%d*%d-%d)", width, height, depth);
			PostMessageA(mWindow->GetWindowHandle(), WM_CLOSE, 0, 0);
			return false;
		}

		if (auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr)
			mouse->ResetWindowsMouseState();

		return true;
	}

	// Original function: FUN_00478c98
	bool CApplication::ToggleFullscreen()
	{
		if (mWindow == nullptr)
		{
			return false;
		}

		const std::uint32_t depth = static_cast<std::uint32_t>(GetDisplayDepth());
		const std::uint32_t height = static_cast<std::uint32_t>(GetDisplayHeight());
		const std::uint32_t width = static_cast<std::uint32_t>(GetDisplayWidth());
		const bool targetFullscreen = !IsFullscreen();

		if (!CanDisplayModeBeActivated(targetFullscreen, width, height, depth))
		{
			return false;
		}

		mWindow->ToggleFullscreen();

		return true;
	}

	// Original function: FUN_00478ce0
	void CApplication::FlipToGDISurface()
	{
		if (mWindow != nullptr)
		{
			mWindow->FlipToGDISurface();
		}
	}

	// Original function: FUN_00478ced
	HWND CApplication::GetWindowHandle() const
	{
		if (mWindow == nullptr)
		{
			return nullptr;
		}

		return mWindow->GetWindowHandle();
	}

	// Original function: FUN_00478cfa
	bool CApplication::IsWindowActive() const
	{
		if (mWindow == nullptr)
		{
			return false;
		}

		return mWindow->IsWindowActive();
	}

	// Original function: FUN_00478d08
	bool CApplication::IsApplicationActive() const
	{
		if (mWindow == nullptr)
		{
			return false;
		}

		return mWindow->IsApplicationActive();
	}

	// Remaster accessors use the backend interface rather than DirectDraw offsets.
	CDisplay* CApplication::GetDisplay() const
	{

		return mWindow != nullptr ? mWindow->GetDisplay() : nullptr;
	}

	int CApplication::GetDisplayWidth() const
	{
		const auto* display = GetDisplay();

		return display != nullptr ? display->GetWidth() : 0;
	}

	int CApplication::GetDisplayHeight() const
	{
		const auto* display = GetDisplay();

		return display != nullptr ? display->GetHeight() : 0;
	}

	int CApplication::GetDisplayDepth() const
	{
		const auto* display = GetDisplay();

		return display != nullptr ? display->GetDepth() : 0;
	}

	bool CApplication::IsFullscreen() const
	{
		const auto* display = GetDisplay();

		return display != nullptr && display->IsFullscreen();
	}

	int CApplication::GetWindowOffsetX() const
	{
		const auto* display = GetDisplay();

		return display != nullptr ? display->GetWindowOffsetX() : 0;
	}

	int CApplication::GetWindowOffsetY() const
	{
		const auto* display = GetDisplay();

		return display != nullptr ? display->GetWindowOffsetY() : 0;
	}

	// Original function: FUN_00478d8a
	void CApplication::l_GetDesktopDisplayMode()
	{
		HDC deviceContext = GetDC(nullptr);

		if (deviceContext != nullptr)
		{
			mDisplayModeData.desktopWidth = static_cast<std::uint32_t>(GetDeviceCaps(deviceContext, HORZRES));
			mDisplayModeData.desktopHeight = static_cast<std::uint32_t>(GetDeviceCaps(deviceContext, VERTRES));
			mDisplayModeData.desktopDepth = static_cast<std::uint32_t>(GetDeviceCaps(deviceContext, BITSPIXEL));

			ReleaseDC(nullptr, deviceContext);

			return;
		}

		mDisplayModeData.desktopWidth = 1024;
		mDisplayModeData.desktopHeight = 768;
		mDisplayModeData.desktopDepth = 32;
	}

	// Remaster: these are internal render sizes, independent of monitor modes.
	void CApplication::l_GetFullscreenDisplayModes()
	{
		mDisplayModeData.fullscreenDisplayModeCount = 0;
		const auto add = [this](std::uint32_t width, std::uint32_t height)
		{
			if (!CDisplay::IsSupportedMode(width, height, 32))
				return;

			if (width < static_cast<std::uint32_t>(minimumScreenWidth) || height < static_cast<std::uint32_t>(minimumScreenHeight) || width > static_cast<std::uint32_t>(maximumScreenWidth) || height > static_cast<std::uint32_t>(maximumScreenHeight))
				return;

			for (std::uint32_t index = 0; index < mDisplayModeData.fullscreenDisplayModeCount; ++index)
			{
				const auto& mode = mDisplayModeData.fullscreenDisplayModes[index];

				if (mode.width == width && mode.height == height)
				{

					return;
				}
			}

			if (mDisplayModeData.fullscreenDisplayModeCount < 80)
				mDisplayModeData.fullscreenDisplayModes[mDisplayModeData.fullscreenDisplayModeCount++] = { width, height, 32 };
		};
		add(screenWidth, screenHeight);
		add(mDisplayModeData.desktopWidth, mDisplayModeData.desktopHeight);
		// Preserve usable game sizes even when a modern monitor does not advertise them.

		for (const auto& mode : { SDisplayMode{ 640, 480, 0 }, SDisplayMode{ 800, 600, 0 }, SDisplayMode{ 1024, 768, 0 }, SDisplayMode{ 1280, 720, 0 }, SDisplayMode{ 1280, 1024, 0 }, SDisplayMode{ 1600, 900, 0 }, SDisplayMode{ 1600, 1200, 0 } })
		{
			add(mode.width, mode.height);
		}

		DEVMODEW mode{ };
		mode.dmSize = sizeof(mode);

		for (DWORD index = 0; EnumDisplaySettingsW(nullptr, index, &mode); ++index)
		{
			add(mode.dmPelsWidth, mode.dmPelsHeight);
		}

		const auto bySize = [](const SDisplayMode& left, const SDisplayMode& right)
		{
			const std::uint64_t leftPixels = static_cast<std::uint64_t>(left.width) * left.height;
			const std::uint64_t rightPixels = static_cast<std::uint64_t>(right.width) * right.height;

			if (leftPixels != rightPixels)
				return leftPixels < rightPixels;

			if (left.width != right.width)
				return left.width < right.width;

			return left.height < right.height;
		};
		std::sort(mDisplayModeData.fullscreenDisplayModes, mDisplayModeData.fullscreenDisplayModes + mDisplayModeData.fullscreenDisplayModeCount, bySize);
	}

	bool CApplication::CheckFullscreenDisplayMode(std::uint32_t width, std::uint32_t height, std::uint32_t depth) const
	{

		return CDisplay::IsSupportedMode(width, height, depth);
	}

	bool CApplication::CanDisplayModeBeActivated(bool targetFullscreen, std::uint32_t width, std::uint32_t height, std::uint32_t depth) const
	{
		(void)targetFullscreen;

		return CDisplay::IsSupportedMode(width, height, depth);
	}

	// Original function: FUN_00479169
	CApplication::~CApplication()
	{
		ShutDown();

		NXBasics::CXBSystemManager::sWarningErrorFunctionPtr = nullptr;
		NXBasics::CXBSystemManager::sFatalErrorFunctionPtr = nullptr;

		if (mOwnsSystemManager && NXBasics::CXBSystemManager::sTheObjectPtr != nullptr)
		{
			delete NXBasics::CXBSystemManager::sTheObjectPtr;
		}

		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_004791a6
	bool CApplication::StartUp()
	{
		mWindow = new (std::nothrow) CWindow(instance, showCommand, screenWidth, screenHeight, screenDepth, fullscreen);

		if (mWindow == nullptr || !mWindow->IsInitialized())
		{
			delete mWindow;
			mWindow = nullptr;
			return false;
		}

		new (std::nothrow) NXSysKeyManager::CKeyManager();
		new (std::nothrow) NXSysMouseManager::CMouseManager();

		return true;
	}

	// Original function: FUN_0047929c
	void CApplication::ShutDown()
	{
		// Unregister foreground input before destroying its target HWND.
		delete NXSysMouseManager::CMouseManager::sTheObjectPtr;
		delete NXSysKeyManager::CKeyManager::sTheObjectPtr;
		delete mWindow;
		mWindow = nullptr;
	}

	// Original function: FUN_00479a54
	void CApplication::UpdateDisplay()
	{
		if (mWindow != nullptr)
		{
			mWindow->Update();
		}
	}

	// Original function: FUN_00479c09; Windows-only tool name inferred from its callers.
	char FindCDDrive(const char* volumeName, const char* markerFileName)
	{
		static_assert(sizeof(WIN32_FIND_DATAA) == 320);
		char path[260];
		char actualVolumeName[260]{ };
		WIN32_FIND_DATAA foundFile;

		for (char drive = 'c'; drive <= 'z'; ++drive)
		{
			NXBasics::Runtime_PrintString(path, OriginalStrings::CDDriveRootNameFormat, static_cast<int>(drive));

			if (::GetDriveTypeA(path) != DRIVE_CDROM)
				continue;

			NXBasics::Runtime_PrintString(path, OriginalStrings::CDDriveRootNameFormat, static_cast<int>(drive));

			if (!::GetVolumeInformationA(path, actualVolumeName, sizeof(actualVolumeName), nullptr, nullptr, nullptr, nullptr, 0))
				continue;

			if (NXBasics::Runtime_CompareStringsIgnoreCase(actualVolumeName, volumeName) != 0)
				continue;

			DWORD sectorsPerCluster = 0, bytesPerSector = 0, freeClusters = 0, totalClusters = 0;

			if (!::GetDiskFreeSpaceA(path, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters))
				continue;
			// A matching volume with free clusters terminates the entire original scan.

			if (freeClusters != 0)
				return 0;

			NXBasics::Runtime_PrintString(path, OriginalStrings::CDDriveFileNameFormat, static_cast<int>(drive), markerFileName);
			const HANDLE search = ::FindFirstFileA(path, &foundFile);

			if (search != INVALID_HANDLE_VALUE)
			{
				::FindClose(search);
				return drive;
			}
		}

		return 0;
	}
}
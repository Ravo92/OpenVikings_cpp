#include "RemasterAILog.h"
#include "ApplicationGlobals.h"
#include "C2EngineLifetimeAccess.h"
#include "C2LanguageTools.h"
#include "C2LogicLifetimeAccess.h"
#include "CDAudioManager.h"
#include "DMManager.h"
#include "DSManager.h"
#include "GameProfile.h"
#include "MasterControlProgram.h"
#include "MasterPropertyManager.h"
#include "NetworkShutdown.h"
#include "OriginalStrings.h"
#include "RemasterHumanSprites.h"
#include "resource.h"
#include "X2DApplication.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XFileAccess.h"
#include "XPicture.h"
#include "XSystemManager.h"
#include "XSystemMouseCursor.h"

#include <cstddef>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <new>
#include <windows.h>

namespace NC2LogicExternals
{
	// Original function: FUN_00401c12
	void __stdcall External_Misc_ProgressBar_Update()
	{
		NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->ProgressBar_Update();
	}
}

namespace NMasterControlProgram
{
	// Original function: FUN_0040272b; trivial progress-bar color cleanup callback.
	// The hosted onexit callback result is ignored; native code contains only RET.
	static int __cdecl ProgressBarBackgroundColor_Destruct()
	{
		return 0;
	}

	// Original function: FUN_00402761; CD identification wrapper.
	char FindGameCDDrive()
	{
		// Original: Weltwunder 004FE2F4 "weltwunder", Die Saga 004FE320 "saga";
		// both pair the volume name with the same "autorun.exe" marker.
		return NXSys2DApp::FindCDDrive(NRemaster::GetGameCDVolumeName(), OriginalStrings::GameCDMarkerFile);
	}

	// Original function: FUN_00401000
	CMasterControlProgram::CMasterControlProgram(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
	{
		// The embedded startup record is constructed before entering this body.
		sTheObjectPtr = this;

		std::memset(this, 0, 360);

		mProgramState = 3;
		mWantedProgramState = 0;

		new (std::nothrow) NXBasics::CXBSystemManager();

		new (std::nothrow) NMasterPropertyManager::CPropertyManager();

		NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Load();
		NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_WorkOnIniFile("game.ini");
		NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_WorkOnCommandLine(commandLine);
		NMasterPropertyManager::CPropertyManager::sTheObjectPtr->System_AddDefaultProperties();
		NC2Logic::RemasterAILog::Initialize();

		bool customDataFileLoaded = false;
		char fileName[28];

		for (int index = 9; index >= 0; --index)
		{
			NXBasics::Runtime_PrintString(fileName, "use_data_file_%d", index);

			const char* configuredFile = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetStringValuePtr(fileName);

			if (configuredFile == nullptr || std::strlen(configuredFile) == 0)
			{
				continue;
			}

			if (!NXBasics::CFile::FileSystem_Tool_FileExists(configuredFile, false))
			{
				continue;
			}

			void* fileLibrary = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::SArchiveState), 1);

			if (fileLibrary != nullptr)
			{
				NXBasics::Archive_Initialize(fileLibrary, configuredFile);
			}

			NXBasics::CFile::FileSystem_FileLibrary_Add(fileLibrary);
			customDataFileLoaded = true;
		}

		if (!customDataFileLoaded)
		{
			for (int index = 10; index >= 1; --index)
			{
				NXBasics::Runtime_PrintString(fileName, "datax\\libs\\data%4.4d.lib", index);

				if (!NXBasics::CFile::FileSystem_Tool_FileExists(fileName, false))
				{
					continue;
				}

				void* fileLibrary = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::SArchiveState), 1);

				if (fileLibrary != nullptr)
				{
					NXBasics::Archive_Initialize(fileLibrary, fileName);
				}

				NXBasics::CFile::FileSystem_FileLibrary_Add(fileLibrary);
			}
		}

		auto* properties = NMasterPropertyManager::CPropertyManager::sTheObjectPtr;
		const auto spriteMode = properties->Property_DoesExists("remaster_sprites") ? properties->Property_GetIntegerValue("remaster_sprites") : 1;
		NRemaster::HumanSprites::Initialize(spriteMode != 0, spriteMode > 1);

		NXSys2DApp::SApplicationControl applicationControl;

		applicationControl.instance = instance;
		applicationControl.previousInstance = previousInstance;
		applicationControl.commandLine = commandLine;
		applicationControl.showCommand = showCommand;
		applicationControl.fullscreen = true;
		applicationControl.fullscreenToggle = true;

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("gfx_fullscreen"))
		{
			applicationControl.fullscreen = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("gfx_fullscreen") != 0;
		}

		applicationControl.screenWidth = 800;
		applicationControl.screenHeight = 600;
		applicationControl.screenDepth = 32;

		applicationControl.minimumScreenWidth = 800;
		applicationControl.minimumScreenHeight = 600;
		applicationControl.minimumScreenDepth = 32;

		applicationControl.maximumScreenWidth = 10000;
		applicationControl.maximumScreenHeight = 10000;
		// Remaster: all selectable render sizes use 32-bit color.
		applicationControl.maximumScreenDepth = 32;

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("gfx_screen_width"))
		{
			applicationControl.screenWidth = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("gfx_screen_width");
		}

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("gfx_screen_height"))
		{
			applicationControl.screenHeight = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("gfx_screen_height");
		}

		new (std::nothrow) NXSys2DApp::CApplication(applicationControl);

		NXBasics::CFile::FileSystem_SystemPathName_Set("$gameroot$", ".");

		char resolvedPath[MAX_PATH];

		// Requested installation layout: loose cursors are stored in DataX\Mouse.
		if (NXBasics::CFile::FileSystem_Tool_GetRealFilename("datax\\mouse\\mousenormal.cur", resolvedPath))
		{
			mMouseCursors[0] = LoadCursorFromFileA(resolvedPath);
		}

		if (NXBasics::CFile::FileSystem_Tool_GetRealFilename("datax\\mouse\\mousepressed.cur", resolvedPath))
		{
			mMouseCursors[1] = LoadCursorFromFileA(resolvedPath);
		}

		if (NXBasics::CFile::FileSystem_Tool_GetRealFilename("datax\\mouse\\mouseright.cur", resolvedPath))
		{
			mMouseCursors[2] = LoadCursorFromFileA(resolvedPath);
		}

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("set_language"))
		{
			const int language = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("set_language");

			NC2Logic::LanguageTool_SetCurrentLanguage(language);
		}

		NNetwork::Network_InitializeCallsign();

		if (!NXSys2DApp::CApplication::sTheObjectPtr->StartUp())
		{
			return;
		}

		mDisplayWidth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayWidth();
		mDisplayHeight = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayHeight();
		mDisplayDepth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayDepth();

		HINSTANCE applicationInstance = NXSys2DApp::CApplication::sTheObjectPtr->GetInstance();
		HWND window = NXSys2DApp::CApplication::sTheObjectPtr->GetWindowHandle();

		HICON smallIcon = static_cast<HICON>(LoadImageA(applicationInstance, MAKEINTRESOURCEA(IDI_OPENVIKINGS_SMALL), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
		SendMessageA(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));

		HICON largeIcon = static_cast<HICON>(LoadImageA(applicationInstance, MAKEINTRESOURCEA(IDI_OPENVIKINGS_LARGE), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
		SendMessageA(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon));

		SendMessageA(window, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(NRemaster::GetGameWindowTitle()));

		HardwareMouseCursor_Enable();

		if (mHardwareMouseCursorEnabled)
		{
			NXSysMisc::XWS_SystemMouseCursor_SetCustom(mMouseCursors[0]);
		}

		HardwareMouseCursor_ReSet();

		if (!NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("fx_off"))
		{
			void* soundManagerStorage = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NDSManager::CManager), 1);

			if (soundManagerStorage != nullptr)
				::new (soundManagerStorage) NDSManager::CManager(window, true);

			const int quality = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("fx_quality") ? NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("fx_quality") : 1;

			NDSManager::CManager::sTheObjectPtr->Quality_Set(quality);

			if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("fx_volume"))
			{
				const int volume = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("fx_volume");

				NDSManager::CManager::sTheObjectPtr->MainVolume_Set(volume);
			}
		}

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("dm_off"))
		{
			const int musicMode = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("music_mode");

			if (musicMode == 2)
			{
				NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Add("music_mode", true, 1, nullptr);
			}
		}
		else
		{
			void* directMusicManagerMemory = NXBasics::Runtime_AllocateWithNewHandler(NDMManager::ManagerSize, 1);

			if (directMusicManagerMemory != nullptr)
			{
				NDMManager::Construct(directMusicManagerMemory);
			}

			char directMusicPath[MAX_PATH]{ };

			GetCurrentDirectoryA(MAX_PATH - 1, directMusicPath);
			// Original append routine 004e5c30 is strcat.
			// Requested installation layout replaces original 004FE0A4's data directory with DataX.
			std::strcat(directMusicPath, NRemaster::GetMusicDirectorySuffix());

			NDMManager::Init(gDirectMusicManager, 0, directMusicPath);

			if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("dm_volume"))
			{
				const int volume = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("dm_volume");

				NDMManager::MainVolume_Set(gDirectMusicManager, volume);
			}
		}

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("cda_off"))
		{
			const int musicMode = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("music_mode");

			if (musicMode == 3)
			{
				NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Add("music_mode", true, 1, nullptr);
			}
		}
		else
		{
			// Original FUN_004e5ddd allocates through FUN_004e860e(size, 1).
			void* cdAudioMemory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NCDAudioManager::CCDAudioManager), 1);
			if (cdAudioMemory != nullptr)
				::new (cdAudioMemory) NCDAudioManager::CCDAudioManager();

			const char initializationValue = FindGameCDDrive();
			NCDAudioManager::CCDAudioManager::sTheObjectPtr->Init(initializationValue);
		}

		mProgramState = 2;
	}

	// Original function: FUN_004015fb
	CMasterControlProgram::~CMasterControlProgram()
	{
		NC2Logic::RemasterAILog::Shutdown();
		System_ProgramState_EndCurrent();
		// Remaster: retire the music apartment before the main COM/window teardown.
		if (gDirectMusicManager != nullptr) NDMManager::Exit(gDirectMusicManager);
		NNetwork::Network_ShutDown();
		GameEngine_ShutDownEngine();
		NRemaster::HumanSprites::Shutdown();
		ProgressBar_Exit();

		mProgramState = 3;

		NXSys2DApp::CApplication::sTheObjectPtr->ShutDown();

		auto* cdAudioManager = NCDAudioManager::CCDAudioManager::sTheObjectPtr;

		if (cdAudioManager != nullptr)
		{
			cdAudioManager->~CCDAudioManager();
			NXBasics::Runtime_Free(cdAudioManager);
		}

		void* directMusicManager = gDirectMusicManager;

		if (directMusicManager != nullptr)
		{
			// Invoke vtable slot 0 with the original deletion flag.
			using DeletingDestructor = void* (__thiscall*)(void*, unsigned int);

			auto* vtable = *reinterpret_cast<DeletingDestructor**>(directMusicManager);
			vtable[0](directMusicManager, 1);
		}

		auto* soundManager = NDSManager::CManager::sTheObjectPtr;

		if (soundManager != nullptr)
		{
			soundManager->~CManager();
			NXBasics::Runtime_Free(soundManager);
		}

		auto* application = NXSys2DApp::CApplication::sTheObjectPtr;

		if (application != nullptr)
		{
			delete application;
		}

		auto* propertyManager = NMasterPropertyManager::CPropertyManager::sTheObjectPtr;

		if (propertyManager != nullptr)
		{
			delete propertyManager;
		}

		for (std::size_t index = 0; index < 5; ++index)
		{
			void* fileLibrary = NXBasics::CFile::mStaticVars.fileLibraries[index];

			if (fileLibrary != nullptr)
			{
				NXBasics::CFile::FileSystem_FileLibrary_Remove(fileLibrary);
				NXBasics::Archive_Destroy(fileLibrary);
				NXBasics::Runtime_Free(fileLibrary);
			}
		}

		auto* systemManager = NXBasics::CXBSystemManager::sTheObjectPtr;

		if (systemManager != nullptr)
		{
			systemManager->~CXBSystemManager();
			NXBasics::Runtime_Free(systemManager);
		}

		mProgramState = 0;
		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_00401b85
	void CMasterControlProgram::GameEngine_ShutDownEngine()
	{
		if (mEngineInitialized == 0)
		{
			return;
		}

		if (mGameInitialized != 0)
		{
			NC2Logic::Game_ShutDown();
			mGameInitialized = 0;
		}

		auto* engineManager = NC2E2::gE2Manager;

		if (engineManager != nullptr)
		{
			NC2E2::E2Manager_Destruct(engineManager);
			NXBasics::Runtime_Free(engineManager);
		}

		auto* game = NC2Logic::gGame;

		if (game != nullptr)
		{
			NC2Logic::Game_Destruct(game);
			NXBasics::Runtime_Free(game);
		}

		mEngineInitialized = 0;
	}

	// Original function: FUN_00401bd5.
	void CMasterControlProgram::GameEngine_ShutDownGame()
	{
		if (mGameInitialized != 0)
		{
			NC2Logic::Game_ShutDown();
			mGameInitialized = 0;
		}
	}

	// Original function: FUN_00402530
	void CMasterControlProgram::ProgressBar_Init(bool useTitleScreen)
	{
		ProgressBar_Exit();

		const char* fileNamePattern = useTitleScreen ? "data\\gui\\lang\\%s\\pictures\\titlescreen.bmp" : "data\\gui\\lang\\%s\\pictures\\loadscreen.bmp";

		char fileName[260];
		NC2Logic::LanguageTool_GetFilename(fileNamePattern, fileName, false);

		// Explicit lifetimes preserve destruction before ProgressBar_PrintToScreen().
		alignas(NXBasics::CPicture) std::byte pictureStorage[sizeof(NXBasics::CPicture)];
		alignas(NXBasics::CBitmap) std::byte bitmapStorage[sizeof(NXBasics::CBitmap)];

		auto* picture = ::new (static_cast<void*>(pictureStorage)) NXBasics::CPicture(fileName);
		NXBasics::CBitmap* sourceBitmap = picture->GetBitmap();

		const auto sourceDisplayDepth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayDepth();
		const auto sourceBitmapFormat = NXBasics::ConvertBitsToTDepth(sourceDisplayDepth);

		const std::int32_t sourceHeight = sourceBitmap->GetRectangle().height;
		const std::int32_t sourceWidth = sourceBitmap->GetRectangle().width;

		auto* temporaryBitmap = ::new (static_cast<void*>(bitmapStorage)) NXBasics::CBitmap(sourceWidth, sourceHeight, sourceBitmapFormat);

		// Reload the picture's bitmap pointer at the original call site.
		sourceBitmap = picture->GetBitmap();
		sourceBitmap->CopyIntoBitmap(temporaryBitmap, 0, 0);

		void* progressBarMemory = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		void* progressBar = nullptr;

		if (progressBarMemory != nullptr)
		{
			auto* application = NXSys2DApp::CApplication::sTheObjectPtr;

			const auto displayDepth = application->GetDisplayDepth();
			const auto bitmapFormat = NXBasics::ConvertBitsToTDepth(displayDepth);
			const auto displayHeight = application->GetDisplayHeight();
			const auto displayWidth = application->GetDisplayWidth();

			progressBar = ::new (progressBarMemory) NXBasics::CBitmap(displayWidth, displayHeight, bitmapFormat);
		}

		mProgressBar = progressBar;

		temporaryBitmap->CopyIntoBitmap_FitIn(static_cast<NXBasics::CBitmap*>(progressBar));
		temporaryBitmap->~CBitmap();
		picture->~CPicture();

		ProgressBar_PrintToScreen();

		mProgressBarValue = 0;
	}

	// Original function: FUN_00402620
	void CMasterControlProgram::ProgressBar_Exit()
	{
		void* progressBar = mProgressBar;

		if (progressBar == nullptr)
		{
			return;
		}

		delete static_cast<NXBasics::CBitmap*>(progressBar);
		mProgressBar = nullptr;
	}

	// Original function: FUN_0040263d
	void CMasterControlProgram::ProgressBar_Update()
	{
		void* progressBar = mProgressBar;

		if (progressBar == nullptr)
		{
			return;
		}

		++mProgressBarValue;

		const auto& area = static_cast<NXBasics::CBitmap*>(progressBar)->GetRectangle();
		// C2 004025EC: 90 x 8, centered, with twelve rows below the bar at 600p.
		// Remaster scales each game's indicator against its reference height.
		const bool cultures2 = NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2;
		const std::int32_t sourceWidth = cultures2 ? 90 : 84;
		const std::int32_t referenceHeight = cultures2 ? 600 : 768;
		const std::int32_t barWidth = (std::min)(area.width, (std::max)(1, MulDiv(sourceWidth, area.height, referenceHeight)));
		const std::int32_t barHeight = (std::max)(1, MulDiv(cultures2 ? 8 : 4, area.height, referenceHeight));
		const std::int32_t bottomSpacing = MulDiv(cultures2 ? 12 : 2, area.height, referenceHeight);
		NXBasics::SRectangle rectangle((area.width - barWidth) / 2, area.height - bottomSpacing - barHeight, barWidth, barHeight);

		// Preserve the original explicit destruction point.
		alignas(NXBasics::CBitmap) std::byte bitmapStorage[sizeof(NXBasics::CBitmap)];
		auto* temporaryBitmap = ::new (static_cast<void*>(bitmapStorage)) NXBasics::CBitmap(static_cast<NXBasics::CBitmap*>(mProgressBar), rectangle);

		// Original static storage: DAT_0050f628 and DAT_0050f62c.
		static std::uint8_t backgroundInitializationFlags = 0;
		static NXBasics::SColorRGB backgroundColor(0, 0, 0);

		if ((backgroundInitializationFlags & 1u) == 0)
		{
			backgroundInitializationFlags |= 1u;

			backgroundColor = NXBasics::SColorRGB(108, 100, 83);

			NXBasics::Runtime_AtExit(ProgressBarBackgroundColor_Destruct);
		}

		temporaryBitmap->Fill(cultures2 ? NXBasics::SColorRGB(40, 40, 40) : backgroundColor);

		const std::uint32_t doubledValue = mProgressBarValue << 1;
		const std::int32_t position = static_cast<std::int32_t>(doubledValue % static_cast<std::uint32_t>(sourceWidth));

		for (std::int32_t column = 0; column < barWidth; ++column)
		{
			const std::int32_t offset = column * sourceWidth / barWidth - position;
			const std::int32_t distance = offset < 0 ? -offset : offset;
			if (distance > (cultures2 ? 15 : 7)) continue;
			const std::uint8_t intensity = static_cast<std::uint8_t>(255 - distance * 14);

			NXBasics::SColorRGB lineColor(intensity, intensity, intensity);
			temporaryBitmap->Draw_VerticalLine(column, 0, barHeight, lineColor);
		}
		if (cultures2)
		{
			const std::int32_t border = (std::min)((std::min)(barWidth, barHeight) / 2, (std::max)(1, MulDiv(1, area.height, 600)));
			for (std::int32_t inset = 0; inset < border; ++inset)
			{
				temporaryBitmap->Draw_Rectangle(NXBasics::SRectangle(inset, inset, barWidth - inset * 2, barHeight - inset * 2), NXBasics::SColorRGB(20, 20, 20));
			}
		}

		temporaryBitmap->~CBitmap();
		ProgressBar_PrintToScreen();
	}

	// Original function: FUN_0040272c
	void CMasterControlProgram::ProgressBar_PrintToScreen()
	{
		for (int iteration = 0; iteration < 4; ++iteration)
		{
			NXSys2DApp::CApplication::sTheObjectPtr->BitmapToScreen(static_cast<NXBasics::CBitmap*>(mProgressBar), 0, 0);
			NXSys2DApp::CApplication::sTheObjectPtr->UpdateDisplay();
			Sleep(10);
		}
	}
}

namespace NC2LogicExternals
{
	// Original function: FUN_00401bfa.
	void External_Misc_PropertyString_AddString(const char* name, const char* value)
	{
		NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Add(name, false, 0, value);
	}
}

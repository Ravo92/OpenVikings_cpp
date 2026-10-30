#include "C2E2AmbientSoundManagerAccess.h"
#include "C2GameMapIO.h"
#include "C2GameTime.h"
#include "C2LifetimeGlobals.h"
#include "CDAudioManager.h"
#include "GameProfile.h"
#include "MainMenuGuiManager.h"
#include "MasterControlProgram.h"
#include "MasterPropertyManager.h"
#include "X2DApplication.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"

#include <cstddef>
#include <cstdint>
#include <new>
#include <Windows.h>

namespace NMasterControlProgram
{

	// Original function: FUN_0040212b
	bool CMasterControlProgram::System_ProgramState_MainMenu_StartUp()
	{
		mProgramState = 7;

		ProgressBar_Init(mEngineInitialized == 0);

		static_assert(sizeof(NC2MainMenuGuiManager::CGuiManager) == (sizeof(void*) == 8 ? 8752 : 8628));

		if (void* storage = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2MainMenuGuiManager::CGuiManager), 1))
			::new (storage) NC2MainMenuGuiManager::CGuiManager();

		GameEngine_StartUpEngine();

		auto* application = NXSys2DApp::CApplication::sTheObjectPtr;
		auto* mainMenuGuiManager = NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr;
		const int displayDepth = application->GetDisplayDepth();

		const int displayHeight = application->GetDisplayHeight();

		const int displayWidth = application->GetDisplayWidth();

		mainMenuGuiManager->Desktop_Open(displayWidth, displayHeight, static_cast<std::uint32_t>(displayDepth));

		l_MainMenu_Misc_GameMap_LoadRandomMap();
		ProgressBar_Exit();

		if (NCDAudioManager::CCDAudioManager::sTheObjectPtr != nullptr)
		{
			NCDAudioManager::CCDAudioManager::sTheObjectPtr->StartTrack(2);
		}

		mProgramState = static_cast<std::int32_t>(TProgramState::MainMenu);

		constexpr const char* propertyName = "start_campaign_3_screen";

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists(propertyName))
		{
			NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr->Screen_ChangeTo(5, false);
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove(propertyName);
		}

		return true;
	}

	// Original function: FUN_004021ea
	void CMasterControlProgram::System_ProgramState_MainMenu_ShutDown()
	{
		mProgramState = 9;

		if (NCDAudioManager::CCDAudioManager::sTheObjectPtr != nullptr)
		{
			if (NCDAudioManager::CCDAudioManager::sTheObjectPtr->IsStillPlaying())
			{
				NCDAudioManager::CCDAudioManager::sTheObjectPtr->StopTrack();
			}
		}

		NC2E2::AmbientSoundManager_StopAllAmbientSounds(NC2E2::gE2AmbientSoundManager);
		NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr->Desktop_Close();
		GameEngine_ShutDownGame();

		auto* mainMenuGuiManager = NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr;

		if (mainMenuGuiManager != nullptr)
		{
			mainMenuGuiManager->~CGuiManager();
			NXBasics::Runtime_Free(mainMenuGuiManager);
		}

		mProgramState = static_cast<std::int32_t>(TProgramState::None);
	}

	// Original function: FUN_00402250
	void CMasterControlProgram::System_ProgramState_MainMenu_Update()
	{
		const DWORD currentTime = timeGetTime();

		if (currentTime - mMainMenuLastMapChangeTick >= 300000U)
		{
			l_MainMenu_Misc_GameMap_LoadRandomMap();
		}

		NC2Logic::Logic_EntryPoint_GameTime_Update();
		NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr->System_UpdateGui();

		auto* displayBitmap = NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap();

		NXSys2DApp::CApplication::sTheObjectPtr->BitmapToScreen(static_cast<NXBasics::CBitmap*>(displayBitmap), 0, 0);
	}

	// Original function: FUN_00402298 (EightWonders); Cultures 2 uses 004022D9.
	void CMasterControlProgram::l_MainMenu_Misc_GameMap_LoadRandomMap()
	{
		mMainMenuLastMapChangeTick = 0;
		char mapRoot[200];

		if (NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2)
		{
			// Cultures2.exe 004022F6..0040230B selects 00..03 from the current tick.
			NXBasics::Runtime_PrintString(mapRoot, "data\\maps\\demo_mainmenu_%2.2d", static_cast<int>(timeGetTime() & 3u));
		}
		else
		{
			NXBasics::Runtime_PrintString(mapRoot, "data\\maps\\demo_mainmenu_10");
		}

		NC2Logic::GameIO_Cleanmap_Load(mapRoot, true, true, false);
		NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr->EngineWorldDisplayGuiElement_CenterDisplay();
		mMainMenuLastMapChangeTick = timeGetTime();
	}
}
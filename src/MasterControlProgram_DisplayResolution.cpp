#include "C2EngineDisplayAccess.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "InGameGuiManager.h"
#include "MainMenuGuiManager.h"
#include "MasterControlProgram.h"
#include "MasterPropertyManager.h"
#include "X2DApplication.h"

#include <cstdint>

namespace NMasterControlProgram
{
	// Original function: FUN_004017a7
	void CMasterControlProgram::DisplayResolution_SetDirectly(std::int32_t width, std::int32_t height, std::uint32_t depth, bool updateStoredResolution, bool saveOptions, bool applicationFlag)
	{
		// Remaster: old saved/intro depth settings no longer select a 16-bit render mode.
		depth = 32;

		if (updateStoredResolution)
		{
			mDisplayWidth = width;
			mDisplayHeight = height;
			mDisplayDepth = static_cast<std::int32_t>(depth);
		}

		const int currentWidth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayWidth();

		if (currentWidth == width)
		{
			const int currentHeight = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayHeight();

			if (currentHeight == height)
			{
				const std::uint32_t currentDepth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayDepth();

				if (currentDepth == depth)
				{

					return;
				}
			}
		}

		if (mProgramState == 11 || mProgramState == 12)
		{
			std::uint32_t displayedMapPosition = 0;

			auto* inGameGuiManager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
			auto* inGameWorldElement = inGameGuiManager->GetDesktop();
			void* inGameDisplay = NC2Lifetime::Read<void*>(inGameWorldElement, NC2Layout::WorldGuiElementOffset::Display);

			NC2E2::DE_Misc_GetDisplayedCenterMapPosition(inGameDisplay, &displayedMapPosition);
			inGameGuiManager->Desktop_Close();

			NXSys2DApp::CApplication::sTheObjectPtr->SetDisplayMode(width, height, depth, applicationFlag);

			const int actualWidth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayWidth();
			const int actualHeight = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayHeight();
			const std::uint32_t actualDepth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayDepth();

			inGameGuiManager->Desktop_Open(actualWidth, actualHeight, actualDepth);

			inGameWorldElement = inGameGuiManager->GetDesktop();
			inGameDisplay = NC2Lifetime::Read<void*>(inGameWorldElement, NC2Layout::WorldGuiElementOffset::Display);

			NC2E2::DE_SetWantedPositionToMapPoint(inGameDisplay, &displayedMapPosition);
		}
		else if (mProgramState == static_cast<std::int32_t>(TProgramState::MainMenu))
		{
			std::uint32_t displayedMapPosition = 0;

			auto* mainMenuGuiManager = NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr;

			void* mainMenuDisplay = NC2Lifetime::Read<void*>(mainMenuGuiManager->worldDisplayElement, NC2Layout::WorldGuiElementOffset::Display);

			NC2E2::DE_Misc_GetDisplayedCenterMapPosition(mainMenuDisplay, &displayedMapPosition);

			NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr->Desktop_Close();

			NXSys2DApp::CApplication::sTheObjectPtr->SetDisplayMode(width, height, depth, applicationFlag);

			const int actualWidth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayWidth();

			const int actualHeight = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayHeight();

			const std::uint32_t actualDepth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayDepth();

			NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr->Desktop_Open(actualWidth, actualHeight, actualDepth);

			mainMenuGuiManager = NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr;

			mainMenuDisplay = NC2Lifetime::Read<void*>(mainMenuGuiManager->worldDisplayElement, NC2Layout::WorldGuiElementOffset::Display);

			NC2E2::DE_SetWantedPositionToMapPoint(mainMenuDisplay, &displayedMapPosition);
		}
		else
		{
			NXSys2DApp::CApplication::sTheObjectPtr->SetDisplayMode(width, height, depth, applicationFlag);
		}

		if (saveOptions)
		{
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_Global();
		}

		HardwareMouseCursor_ReSet();
	}

	// Original function: FUN_00401936
	void CMasterControlProgram::DisplayResolution_SetDuringNextMainloop(std::int32_t width, std::int32_t height, std::uint32_t depth)
	{
		mRequestedDisplayWidth = width;
		mRequestedDisplayHeight = height;
		mRequestedDisplayDepth = depth;
		mDisplayResolutionChangeRequested = true;
	}

	// Original function: FUN_00401952
	void CMasterControlProgram::DisplayResolution_ResetToUserResolution()
	{
		DisplayResolution_SetDirectly(mDisplayWidth, mDisplayHeight, static_cast<std::uint32_t>(mDisplayDepth), true, false, false);
	}

	// Original function: FUN_00401967
	void CMasterControlProgram::DisplayResolution_CheckChangeRequest()
	{
		if (!mDisplayResolutionChangeRequested)
		{

			return;
		}

		mDisplayResolutionChangeRequested = false;

		DisplayResolution_SetDirectly(mRequestedDisplayWidth, mRequestedDisplayHeight, mRequestedDisplayDepth, true, true, false);
	}
}
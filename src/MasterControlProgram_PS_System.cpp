#include "ApplicationGlobals.h"
#include "CDAudioManager.h"
#include "DMManager.h"
#include "DSManager.h"
#include "MasterControlProgram.h"
#include "SwitchIdentifiers.h"
#include "X2DApplication.h"
#include "XCrtFileAccess.h"

#include <cstddef>
#include <cstdint>
#include <Windows.h>

namespace NMasterControlProgram
{

	// Original function: FUN_0040234f
	bool CMasterControlProgram::System_ProgramState_StartUpSystem()
	{

		return System_ProgramState_Start(TProgramState::TitleScreen);
	}

	// Original function: FUN_0040235c
	bool CMasterControlProgram::System_ProgramState_Start(TProgramState programState)
	{
		if (mProgramState != static_cast<std::int32_t>(TProgramState::None))
		{
			return false;
		}

		switch (programState)
		{
			case TProgramState::TitleScreen:
				return System_ProgramState_TitleScreen_StartUp();

			case TProgramState::MainMenu:
				return System_ProgramState_MainMenu_StartUp();

			case TProgramState::Game:
				return System_ProgramState_Game_StartUp();

			case TProgramState::PreIngameFmv:
				return System_ProgramState_PreIngameFmv_StartUp();

			default:

				return false;
		}
	}

	// Original function: FUN_004023ae
	void CMasterControlProgram::System_ProgramState_EndCurrent()
	{
		switch (mProgramState)
		{
			case SwitchIds::ProgramState::TitleScreen:
				System_ProgramState_TitleScreen_ShutDown();
				break;

			case SwitchIds::ProgramState::MainMenu:
				System_ProgramState_MainMenu_ShutDown();
				break;

			case SwitchIds::ProgramState::Game:
			case SwitchIds::ProgramState::GameAlternate:
				System_ProgramState_Game_ShutDown();
				break;

			case SwitchIds::ProgramState::PreIngameFmv:
				System_ProgramState_PreIngameFmv_ShutDown();
				break;
		}
	}

	// Original function: FUN_004023de
	void CMasterControlProgram::System_ProgramState_ChangeNextTimePossible(TProgramState programState, bool forceChange)
	{
		mWantedProgramState = static_cast<std::int32_t>(programState);
		mForceProgramStateChange = forceChange;
	}

	// Original function: FUN_004023ef
	void CMasterControlProgram::System_ProgramState_ExecuteWantedChange()
	{
		if (mWantedProgramState == 0)
		{

			return;
		}

		if (mWantedProgramState != mProgramState || mForceProgramStateChange)
		{
			System_ProgramState_EndCurrent();
			System_ProgramState_Start(static_cast<TProgramState>(mWantedProgramState));
		}

		mWantedProgramState = 0;
		mForceProgramStateChange = false;
	}

	// Original function: FUN_0040241e
	bool CMasterControlProgram::System_Update()
	{
		switch (mProgramState)
		{
			case SwitchIds::ProgramState::TitleScreen:
				System_ProgramState_TitleScreen_Update();
				break;

			case SwitchIds::ProgramState::MainMenu:
				System_ProgramState_MainMenu_Update();
				break;

			case SwitchIds::ProgramState::Game:
			case SwitchIds::ProgramState::GameAlternate:
				System_ProgramState_Game_Update();
				break;

			case SwitchIds::ProgramState::PreIngameFmv:
				System_ProgramState_PreIngameFmv_Update();
				break;
		}

		if (NDSManager::CManager::sTheObjectPtr != nullptr)
		{
			NDSManager::CManager::sTheObjectPtr->System_Update();
		}

		if (gDirectMusicManager != nullptr)
		{
			NDMManager::Update(gDirectMusicManager);
			// Remaster: the music thread reports and releases its own error strings.
		}

		if (NCDAudioManager::CCDAudioManager::sTheObjectPtr != nullptr)
		{
			NCDAudioManager::CCDAudioManager::sTheObjectPtr->CheckLoopTrack();
		}

		NXSys2DApp::CApplication::sTheObjectPtr->UpdateDisplay();

		DisplayResolution_CheckChangeRequest();
		System_ProgramState_ExecuteWantedChange();

		if (!NXSys2DApp::CApplication::sTheObjectPtr->IsWindowActive() && !NXSys2DApp::CApplication::sTheObjectPtr->IsApplicationActive())
		{
			Sleep(10);
		}

		return reinterpret_cast<const std::uint8_t*>(this)[9] != 0;
	}
}
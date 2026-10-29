#pragma once

#include "C2ControlStartup.h"

#include <Windows.h>

#include <cstddef>
#include <cstdint>

namespace NC2LogicExternals
{
	void External_Misc_PropertyString_AddString(const char* name, const char* value);
	// Original function: FUN_00401c12
	void __stdcall External_Misc_ProgressBar_Update();
}

namespace NTitleScreen
{
	// Physical Windows owner; raw allocation and destructor call ordering remain unchanged.
	struct STitleScreenManagerStorage
	{
		std::uint8_t fmvMode;
		char fmvStem[200];
		std::uint8_t fmvActive, cursorHideRequested, nextIntroPending, prePictureSequence, postPictureSequence;
		std::byte unclassifiedCE[2];
		std::uint32_t pictureIndex, pictureStartedAtMilliseconds;
		void* pictureBitmap;
		void* screenBitmap;
		void* font;
		void* grayscalePalette;
	};
}

namespace NMasterControlProgram
{
	char FindGameCDDrive();

	enum class TProgramState : std::int32_t
	{
		None = 2,
		TitleScreen = 5,
		MainMenu = 8,
		Game = 11,
		PreIngameFmv = 15
	};

	enum class TMouseCursorModes : std::int32_t;

	class CMasterControlProgram
	{
	  public:
		// Original function: FUN_00401000
		CMasterControlProgram(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand);

		// Original function: FUN_004015fb
		~CMasterControlProgram();

		// Original function: FUN_004016ee
		void HardwareMouseCursor_Enable();

		// Original function: FUN_0040170c
		void HardwareMouseCursor_Disable();

		// Original function: FUN_0040172a
		void HardwareMouseCursor_SetMode(TMouseCursorModes mouseCursorMode);

		// Original function: FUN_00401750
		void HardwareMouseCursor_ReSet();

		// Original function: FUN_004017a7
		void DisplayResolution_SetDirectly(std::int32_t width, std::int32_t height, std::uint32_t depth, bool updateStoredResolution, bool saveOptions, bool applicationFlag);

		// Original function: FUN_00401936
		void DisplayResolution_SetDuringNextMainloop(std::int32_t width, std::int32_t height, std::uint32_t depth);

		// Original function: FUN_00401952
		void DisplayResolution_ResetToUserResolution();

		// Original function: FUN_00401967
		void DisplayResolution_CheckChangeRequest();

		void GameEngine_StartUpEngine();
		void GameEngine_StartUpGame();

		// Original function: FUN_00401b85
		void GameEngine_ShutDownEngine();

		void GameEngine_ShutDownGame();

		// Original function: FUN_00401c1d
		bool System_ProgramState_Game_StartUp();

		// Original function: FUN_00401ccb
		void System_ProgramState_Game_ShutDown();

		// Original function: FUN_00401d38
		void System_ProgramState_Game_Update();

		// Original function: FUN_0040212b
		bool System_ProgramState_MainMenu_StartUp();

		// Original function: FUN_004021ea
		void System_ProgramState_MainMenu_ShutDown();

		// Original function: FUN_00402250
		void System_ProgramState_MainMenu_Update();

		// Original function: FUN_004022f0
		bool System_ProgramState_PreIngameFmv_StartUp();

		// Original function: FUN_00402319
		void System_ProgramState_PreIngameFmv_ShutDown();

		// Original function: FUN_00402344
		void System_ProgramState_PreIngameFmv_Update();

		// Original function: FUN_0040234f
		bool System_ProgramState_StartUpSystem();

		// Original function: FUN_0040235c
		bool System_ProgramState_Start(TProgramState programState);

		// Original function: FUN_004023ae
		void System_ProgramState_EndCurrent();

		// Original function: FUN_004023de
		void System_ProgramState_ChangeNextTimePossible(TProgramState programState, bool forceChange);

		// Original function: FUN_004023ef
		void System_ProgramState_ExecuteWantedChange();

		// Original function: FUN_0040241e
		bool System_Update();

		// Original function: FUN_00402530
		void ProgressBar_Init(bool useTitleScreen);

		// Original function: FUN_00402620
		void ProgressBar_Exit();

		// Original function: FUN_0040263d
		void ProgressBar_Update();

		// Original function: FUN_0040272c
		void ProgressBar_PrintToScreen();

		// Original function: FUN_00402773
		bool System_ProgramState_TitleScreen_StartUp();

		// Original function: FUN_0040279c
		void System_ProgramState_TitleScreen_ShutDown();

		// Original function: FUN_004027c7
		void System_ProgramState_TitleScreen_Update();

		inline static CMasterControlProgram* sTheObjectPtr = nullptr;

		bool Remaster_IsMainMenuActive() const
		{

			return mProgramState == static_cast<std::int32_t>(TProgramState::MainMenu);
		}

	  private:
		void l_MainMenu_Misc_GameMap_LoadRandomMap();

		std::int32_t mProgramState;       // +0x00
		std::int32_t mWantedProgramState; // +0x04
		bool mForceProgramStateChange;    // +0x08
		bool mExitRequested;              // +0x09
		std::byte mUnknown0A[2];          // +0x0A

		std::int32_t mDisplayWidth;
		std::int32_t mDisplayHeight;
		std::int32_t mDisplayDepth;

		bool mDisplayResolutionChangeRequested;                    // +0x18
		std::byte mPadding19[3];                                   // +0x19
		int mRequestedDisplayWidth;                                // +0x1C
		int mRequestedDisplayHeight;                               // +0x20
		unsigned int mRequestedDisplayDepth;                       // +0x24
		std::uint8_t mEngineInitialized;                           // +0x28
		std::uint8_t mGameInitialized;                             // +0x29
		std::byte mPadding2A[2];                                   // +0x2A
		NC2Logic::SGameControlStartupData mGameControlStartupData; // +0x2C

		bool mHardwareMouseCursorEnabled; // +0x14C
		std::byte mPadding14D[3];         // +0x14D

		HCURSOR mMouseCursors[3]; // +0x150

		void* mProgressBar;                       // +0x15C
		std::uint32_t mProgressBarValue;          // +0x160
		std::uint32_t mMainMenuLastMapChangeTick; // +0x164
	};

	static_assert(sizeof(CMasterControlProgram) == (sizeof(void*) == 8 ? 376 : 360));
}
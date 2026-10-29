#pragma once

#include "StringInputHandler.h"

#include <cstddef>
#include <cstdint>

namespace NC2LogicExternals
{
	void External_FX_Earthquake_Start(std::uint32_t durationMilliseconds);
}

namespace NC2Logic
{
	class CIoHelper;
}

namespace NInGameGuiManager
{
	// Documentary Windows view; list and calculator byte storage does not change object lifetimes.
	struct SGuiSettingsAndPauseStorage
	{
		std::int32_t localPlayerId;
		void* worldDisplayElement;
		void* missionOnscreenInfoWindow;
		void* mainPanelGraphics;
		void* mainModeButtons[9];
		void* speedButton;
		void* networkInfoButton;
		void* messagePanelGraphics;
		void* messagePriorityButton;
		void* overviewWindow;
		void* statisticsWindow;
		std::byte frameCallListStorage[20];
		std::byte worldDisplayListStorage[20];
		std::byte observationWindowListStorage[20];
		std::uint32_t selectionChangeCount;
		void* singleHumanWindow;
		void* multipleHumansWindow;
		void* singleAnimalWindow;
		void* singleVehicleWindow;
		void* multipleVehiclesWindow;
		void* houseWindow;
		void* houseTypeWindow;
		void* guideWindow;
		void* landscapeWindow;
		void* roadBuildWindow;
		void* wallBuildWindow;
		std::byte actionButtonsListStorage[20];
		std::uint32_t actionButtonsChecksum;
		std::int32_t actionButtonsOriginX, actionButtonsOriginY;
		std::uint32_t scrollSpeed, mainMode;
		std::uint8_t expertMode, tooltipsDisabled, thirdButtonScroll, borderScroll, softwareMouse;
		std::uint8_t cheatsAllowed;
		std::byte unclassifiedE6[2];
		std::int16_t guiMarkers[10][2];
		std::byte frameRateCalculatorStorage[132];
		std::byte unclassified194[4];
		std::uint8_t pausedByLargeWindow;
		std::byte unclassified199[7];
		double savedGameSpeed;
	};

	static_assert(sizeof(SGuiSettingsAndPauseStorage) == 424);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, missionOnscreenInfoWindow) == 0x08);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, mainModeButtons) == 0x10);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, frameCallListStorage) == 0x4C);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, worldDisplayListStorage) == 0x60);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, observationWindowListStorage) == 0x74);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, selectionChangeCount) == 0x88);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, actionButtonsListStorage) == 0xB8);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, scrollSpeed) == 0xD8);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, cheatsAllowed) == 0xE5);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, guiMarkers) == 0xE8);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, frameRateCalculatorStorage) == 0x110);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, pausedByLargeWindow) == 0x198);
	static_assert(offsetof(SGuiSettingsAndPauseStorage, savedGameSpeed) == 0x1A0);

	struct SInGameGuiStaticVars
	{
		bool desktopInitialized; // 005690B8
		std::byte padding01[3];
		std::int32_t displayWidth;  // 005690BC
		std::int32_t displayHeight; // 005690C0
		std::uint32_t displayDepth; // 005690C4
		bool redrawRequested;       // 005690C8
		std::byte padding11[3];
	};

	static_assert(sizeof(SInGameGuiStaticVars) == 20);
	static_assert(offsetof(SInGameGuiStaticVars, displayWidth) == 0x04);
	static_assert(offsetof(SInGameGuiStaticVars, displayHeight) == 0x08);
	static_assert(offsetof(SInGameGuiStaticVars, displayDepth) == 0x0C);
	static_assert(offsetof(SInGameGuiStaticVars, redrawRequested) == 0x10);

	class CInGameGuiManager
	{
	  public:
		// Original function: FUN_0049fd79
		CInGameGuiManager();

		// Original function: FUN_004a00c1
		~CInGameGuiManager();

		void* GetDesktop() const;
		std::int32_t GetLocalPlayerId() const;
		void ControllingPlayer_Set(std::int32_t playerId, bool force);
		bool AreCheatsAllowed() const;
		NC2InGameGuiBaseTextInputElement::CStringInputHandler* GetStringInputHandler();
		bool IsMiddleButtonScrollingEnabled() const;
		void Desktop_Open(std::int32_t width, std::int32_t height, std::uint32_t depth);
		void Desktop_Close();
		void MiscWindows_OverviewWindow_Open();
		void* MiscWindows_OverviewWindow_GetPtr() const;
		void ActivateDesktop();
		void ObservationWindow_CreateNewOne();
		void ObservationWindow_Register(void* window);
		void ObservationWindow_UnRegister(void* window);
		void ObservationWindow_CloseAll();
		void Exploration_Toggle();
		void MiscWindows_OverviewWindow_Toggle();
		void MiscWindows_OverviewWindow_Close();
		void MiscWindows_OverviewWindow_Closed();
		void MiscWindows_LargeOverviewWindow_Toggle();
		void MiscWindows_LargeOverviewWindow_Open(bool topRight);
		void EngineWorldDisplayGuiElement_Register(void* element);
		void EngineWorldDisplayGuiElement_UnRegister(void* element);
		bool Exploration_GetFlag() const;
		void Exploration_Set(bool enabled);
		void FrameCall_Register(void* element);
		void FrameCall_UnRegister(void* element);
		static void System_StartGame();
		static void System_EndGame();
		static void l_IO_Savegame_Save(NC2Logic::CIoHelper* io);
		static void l_IO_Savegame_Load(NC2Logic::CIoHelper* io);
		void System_UpdateGui();
		void FrameCall_Do();
		void Selection_UpdateWindows();
		void MiscButtons_SpeedButton_Update(bool update);
		void MiscButtons_MessagePriorityButton_Update(bool update);
		void MiscWindows_CloseAllLargeOnes(bool resetInputMode);
		void MiscWindows_OptionsWindow_Toggle(bool loadPage, bool savePage);
		void MiscWindows_OptionsWindow_Open(bool loadPage, bool savePage);
		void MiscWindows_GlobalGameSettingsWindow_Toggle(bool specialItemsPage, bool assistantPage);
		void MiscWindows_GlobalGameSettingsWindow_Open(bool specialItemsPage, bool assistantPage);
		void MiscWindows_DiplomacyWindow_Toggle();
		void MiscWindows_DiplomacyWindow_Open();
		void MiscWindows_TechTreeWindow_Toggle();
		void MiscWindows_TechTreeWindow_Open();
		void MiscWindows_HumanListWindow_Toggle(std::uint32_t job, std::uint32_t possibleJob);
		void MiscWindows_HumanListWindow_Open(std::uint32_t job, std::uint32_t possibleJob);
		void MiscWindows_StatisticsWindow_Toggle();
		void MiscWindows_StatisticsWindow_Open();
		void MiscWindows_NetworkInfoWindow_Open();
		void MiscWindows_NetworkInfoWindow_Toggle();
		void MiscWindows_ConstructionSelectionWindow_Open(std::uint32_t type, std::uint32_t item);
		void MiscWindows_ConstructionSelectionWindow_Toggle(std::uint32_t type, std::uint32_t item);
		void MiscWindows_ConstructionSelectionWindow_Close();
		bool MiscWindows_ConstructionSelectionWindow_SendKey(std::uint32_t key, char character);
		void MiscWindows_HelpWindow_Open();
		void MiscWindows_HelpWindow_Toggle();
		void MiscWindows_HelpWindow_Open_HouseInfo(std::uint32_t type, std::uint32_t developmentStage);
		void MiscWindows_HelpWindow_Open_GoodInfo(std::uint32_t good);
		void MiscWindows_GameSafetyBoxWindow_Open(std::uint32_t mode);
		void MiscWindows_MissionInfoWindow_Toggle(std::uint32_t briefingId, bool goalsPage);
		void MiscWindows_MissionInfoWindow_Open(std::uint32_t briefingId, bool goalsPage);
		void MiscWindows_MissionInfoWindow_ReloadStrings();
		void MiscWindows_HumanChangeEquippedGoodsWindow_Open(std::uint32_t type, std::int32_t slot);
		void MiscWindows_HumanChangeJobWindow_Open();
		void MiscWindows_HumanChangeNameWindow_Open();
		bool MiscWindows_HumanChangeNameWindow_SendKey(std::uint32_t key, std::uint32_t character);
		void MiscWindows_HumanLearnJobWindow_Open();
		void MiscWindows_HumanChangeProducedGoodWindow_Open();
		void MiscWindows_HouseRemoveWindow_Open();
		void l_Selection_RemoveAllGuiElements();
		void Selection_ActionButtons_BringUp();
		void Options_ScrollSpeed_Set(std::uint32_t speed, bool save);
		std::uint32_t Options_ScrollSpeed_Get() const;
		std::uint32_t Options_ScrollSpeed_GetInPixelPerSecond() const;
		void Options_GuiMisc_SetMainMode(std::uint32_t mode, bool save);
		void Options_GuiMisc_SetExpertFlag(bool enabled, bool save);
		void Options_GuiMisc_SetTooltipsOffFlag(bool disabled, bool save);
		void Options_Scrolling_SetThirdMouseButtonScrollFlag(bool enabled, bool save);
		void Options_Scrolling_SetBorderScrollFlag(bool enabled, bool save);
		void Options_Scrolling_SetSoftwareMouseFlag(bool enabled, bool save);
		void Misc_GameTime_Stop_LargeWindowOpened();
		void Misc_GameTime_Continue_LargeWindowClosed();
		void MiscWindows_StringOutputWindow_Open();
		void MiscWindows_StringOutputWindow_PrintString(const char* text);
		void __cdecl MiscWindows_StringOutputWindow_PrintStringV(const char* format, ...);
		bool Selection_CheckKey_Action(std::uint32_t key, char character);
		static bool __cdecl ls_Logic_CallbackFunction(std::uint32_t type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
		void MiscWindows_StringOutputWindow_ClearAll();
		void GuiMarker_Set(std::uint32_t index, const std::uint32_t* point);
		void GuiMarker_Get(std::uint32_t index, std::uint32_t* point) const;

		inline static CInGameGuiManager* sTheObjectPtr = nullptr;
		inline static SInGameGuiStaticVars mStaticVars{ };

	  private:
		void l_RemoveAllGuiElements();
		void l_Selection_ActionButtons_CheckUpdate();
		bool l_IsElement0038Required() const;
		void l_UpdateElement0038();
		std::byte mLegacyState[424];                                                                                                                                 // +0x000
		std::byte mDisplayObject[44];                                                                                                                                // +0x1A8
		alignas(NC2InGameGuiBaseTextInputElement::CStringInputHandler) std::byte mStringInputStorage[sizeof(NC2InGameGuiBaseTextInputElement::CStringInputHandler)]; // Native input slot; preceding GUI state remains unconverted.
	};

	static_assert(sizeof(CInGameGuiManager) == 496, "The enclosing legacy GUI state still requires x64 migration");
}
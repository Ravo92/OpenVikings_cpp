#pragma once

#include "XGuiStorage.h"
#include "C2EngineDisplayStorage.h"
#include <cstddef>
#include <cstdint>

namespace NInGameGuiManager
{
	// Native runtime storage; placement-constructed list ownership follows the Windows lifetime order.
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
		alignas(NXBasics::CListBase) std::byte frameCallListStorage[sizeof(NXBasics::CListBase)];
		alignas(NXBasics::CListBase) std::byte worldDisplayListStorage[sizeof(NXBasics::CListBase)];
		alignas(NXBasics::CListBase) std::byte observationWindowListStorage[sizeof(NXBasics::CListBase)];
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
		alignas(NXBasics::CListBase) std::byte actionButtonsListStorage[sizeof(NXBasics::CListBase)];
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

	static_assert(sizeof(SGuiSettingsAndPauseStorage) == (sizeof(void*) == 8 ? 632 : 424));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, missionOnscreenInfoWindow) == (sizeof(void*) == 8 ? 16 : 0x08));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, mainModeButtons) == (sizeof(void*) == 8 ? 32 : 0x10));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, frameCallListStorage) == (sizeof(void*) == 8 ? 152 : 0x4C));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, worldDisplayListStorage) == (sizeof(void*) == 8 ? 192 : 0x60));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, observationWindowListStorage) == (sizeof(void*) == 8 ? 232 : 0x74));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, selectionChangeCount) == (sizeof(void*) == 8 ? 272 : 0x88));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, actionButtonsListStorage) == (sizeof(void*) == 8 ? 368 : 0xB8));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, scrollSpeed) == (sizeof(void*) == 8 ? 420 : 0xD8));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, cheatsAllowed) == (sizeof(void*) == 8 ? 433 : 0xE5));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, guiMarkers) == (sizeof(void*) == 8 ? 436 : 0xE8));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, frameRateCalculatorStorage) == (sizeof(void*) == 8 ? 476 : 0x110));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, pausedByLargeWindow) == (sizeof(void*) == 8 ? 612 : 0x198));
	static_assert(offsetof(SGuiSettingsAndPauseStorage, savedGameSpeed) == (sizeof(void*) == 8 ? 624 : 0x1A0));

}

namespace NC2InGameGuiWorldElement
{
	// Windows widget storage. Embedded engine search data and command buffers retain their widths.
	struct SWorldGuiElementStorage
	{
		NXBaseGui::SBaseElementStorage baseElement;
		NXBasics::CBitmap* bitmap;
		void* display;
		std::uint8_t inputEnabled;
		std::byte unresolved35[3];
		NC2E2::SDisplaySearchObject objectUnderMouse;
		std::uint32_t humanInteractionAction;
		std::uint32_t vehicleInteractionAction;
		std::byte primaryCommand[20];
		std::byte secondaryCommand[20];
		std::uint8_t areaSelectionActive;
		std::byte unresolved1F5[3];
		std::int32_t areaStartX, areaStartY, areaEndX, areaEndY;
		std::uint32_t frameMode;
		void* leftClickHandler;
		std::uint32_t leftClickCommand;
		void* doubleClickHandler;
		std::uint32_t doubleClickCommand;
		std::uint32_t lastDoubleClickEventTime;
		std::uint8_t followEnabled, followHuman;
		std::byte unresolved222[2];
		std::int32_t followHumanId;
		std::uint32_t followHumanUniqueId;
		std::uint8_t followVehicle;
		std::byte unresolved22D[3];
		std::int32_t followVehicleId;
		std::uint32_t followVehicleUniqueId;
		std::uint32_t darkenVariant;
		void* darkenPalette;
		std::uint16_t* darkenHighColorTable;
	};

	static_assert(sizeof(SWorldGuiElementStorage) == (sizeof(void*) == 8 ? 800 : 580));
}

namespace NC2InGameGuiBase
{
	// Windows graphic-button suffix; tooltip ownership remains with the original runtime.
	struct SGuiGfxElementStorage
	{
		alignas(NXBaseGui::SBaseElementStorage) std::byte inheritedElement[sizeof(NXBaseGui::SBaseElementStorage)];
		std::uint32_t graphicsId;
		void* palette;
		std::uint8_t centered;
		std::byte unclassified35[3];
	};

	struct SGuiGfxButtonStorage
	{
		alignas(NXBaseGui::SBaseElementStorage) std::byte inheritedElement[sizeof(NXBaseGui::SBaseElementStorage)];
		std::uint32_t graphicsId;
		void* palette;
		std::uint8_t centered, rectangularStyle, selected;
		std::byte unclassified37;
		std::uint32_t command;
		void* receiver;
		char* tooltip;
		std::uint8_t immediateTooltip;
		std::byte unclassified45[3];
		std::uintptr_t messageParameter1, messageParameter2;
		std::uint8_t disabled;
		std::byte unclassified51[3];
	};

	struct SGuiWindowMoverStorage
	{
		NXBaseGui::SBaseElementStorage inheritedElement;
		std::uint8_t pressed;
		std::byte unclassified2D[3];
		char caption[200];
	};

	static_assert(sizeof(SGuiGfxElementStorage) == (sizeof(void*) == 8 ? 88 : 56));
	static_assert(sizeof(SGuiGfxButtonStorage) == (sizeof(void*) == 8 ? 136 : 84));
	static_assert(sizeof(SGuiWindowMoverStorage) == (sizeof(void*) == 8 ? 272 : 248));
}

namespace NC2InGameGuiMiscWindows
{
	struct SStringOutputWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseWindow;
		void* strings;
	};

	static_assert(sizeof(SStringOutputWindowStorage) == (sizeof(void*) == 8 ? 192 : 116));
}

namespace NC2InGameGuiBase
{
	// Native text-control records retain the unresolved base-tool state and fixed caption capacity.
	struct STextElementStorage
	{
		alignas(SBaseToolElementStorage) std::byte baseTool[sizeof(SBaseToolElementStorage)];
		char caption[200];
		std::uint8_t centered, fillBackground, drawFrame, smallFont, wrapText, darkFont;
		std::byte paddingFE[2];
	};

	struct STextButtonStorage
	{
		alignas(SBaseToolElementStorage) std::byte baseTool[sizeof(SBaseToolElementStorage)];
		char* caption;
		std::uint8_t multiline;
		std::byte padding35[3];
		std::uint32_t lineCount, command;
		void* receiver;
		char* tooltip;
		std::uint8_t immediateTooltip;
		std::byte padding49[3];
		std::uintptr_t messageParameter1, messageParameter2;
		std::uint8_t centered, selected, disabled, smallFont, skipOverwideCaption, fillSolidColor;
		std::byte padding5A[2];
		NXBasics::SColorRGB solidColor;
	};

	static_assert(sizeof(STextElementStorage) == (sizeof(void*) == 8 ? 280 : 256));
	static_assert(sizeof(STextButtonStorage) == (sizeof(void*) == 8 ? 152 : 96));
}

namespace NC2InGameGuiWorldWindow
{
	struct SObservationWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseWindow;
		void* worldElement;
		std::int32_t humanId;
		std::uint32_t humanRevision;
		std::int32_t vehicleId;
		std::uint32_t vehicleRevision;
		std::int32_t houseId;
		std::uint32_t houseRevision;
	};

	static_assert(sizeof(SObservationWindowStorage) == (sizeof(void*) == 8 ? 216 : 140));
}

namespace NC2InGameGuiOverviewDisplay
{
	// The fixed color cache and numeric flag/marker region contain no runtime pointers.
	struct SOverviewElementState
	{
		std::byte textureColorCache[0x8000];
		std::int32_t originX, originY, scaleX, scaleY;
		NXBasics::CBitmap* background;
		std::byte flagsAndMarkers[296];
	};

	struct SOverviewElementStorage
	{
		NXBaseGui::SBaseElementStorage element;
		NXBasics::CBitmap* bitmap;
		SOverviewElementState state;
	};

	struct SStaticOverviewWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseWindow;
		std::uint32_t mode;
		void* overview;
		void* toggleButton;
	};

	struct SLargeOverviewWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseWindow;
		NXBasics::SRectangle frame;
		void* overview;
		void* buttons[13];
	};

	static_assert(sizeof(SOverviewElementState) == (sizeof(void*) == 8 ? 33088 : 33084));
	static_assert(sizeof(SOverviewElementStorage) == (sizeof(void*) == 8 ? 33160 : 33132));
	static_assert(sizeof(SStaticOverviewWindowStorage) == (sizeof(void*) == 8 ? 208 : 124));
	static_assert(sizeof(SLargeOverviewWindowStorage) == (sizeof(void*) == 8 ? 312 : 184));
}

namespace NC2MainMenuGuiWorldElement
{
	struct SMenuWorldElementStorage
	{
		NXBaseGui::SBaseElementStorage element;
		NXBasics::CBitmap* bitmap;
		void* display;
		void* darkenPalette;
		std::uint32_t unclassified38;
		std::uint8_t followActive, followHuman;
		std::byte padding3E[2];
		std::int32_t followHumanId;
		std::uint32_t followHumanRevision;
		std::uint8_t followVehicle;
		std::byte padding49[3];
		std::int32_t followVehicleId;
		std::uint32_t followVehicleRevision;
		std::uint32_t unclassified54;
	};

	static_assert(sizeof(SMenuWorldElementStorage) == (sizeof(void*) == 8 ? 120 : 88));
}
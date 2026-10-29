#pragma once
#include "InGameGuiStorage.h"
#include <cstddef>
#include <cstdint>

namespace NC2InGameGuiMiscWindows
{
	// Documentary Windows storage; the constructor still operates on original bytes.
	struct SConstructionSelectionHouseEntryStorage
	{
		std::uint32_t engineType, developmentStage;
	};

	struct SConstructionSelectionWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseWindow;
		std::uint32_t page;
		void* houseList;
		std::uint32_t lastRefreshTime, specialItemType, specialItem, cachedHouseCount;
		SConstructionSelectionHouseEntryStorage cachedHouses[100];
	};

	static_assert(sizeof(SConstructionSelectionWindowStorage) == (sizeof(void*) == 8 ? 1016 : 936));

	void* ConstructionSelectionWindow_Construct(void* window, std::uint32_t specialItemType, std::uint32_t specialItem);
	bool __fastcall ConstructionSelectionWindow_XGui_BE_Message_Handle(void* window, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uintptr_t param1Payload, std::uintptr_t param2Payload, std::uintptr_t param3Payload);
	void __fastcall ConstructionSelectionWindow_l_GuiButtons_Add(void* window);
	void __fastcall ConstructionSelectionWindow_l_GuiButtons_UpdateListBox(void* window);
}
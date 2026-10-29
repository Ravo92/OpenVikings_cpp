#pragma once
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
		std::byte baseWindow[112];
		std::uint32_t page;
		void* houseList;
		std::uint32_t lastRefreshTime, specialItemType, specialItem, cachedHouseCount;
		SConstructionSelectionHouseEntryStorage cachedHouses[100];
	};

	void* ConstructionSelectionWindow_Construct(void* window, std::uint32_t specialItemType, std::uint32_t specialItem);
	bool __fastcall ConstructionSelectionWindow_XGui_BE_Message_Handle(void* window, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	void __fastcall ConstructionSelectionWindow_l_GuiButtons_Add(void* window);
	void __fastcall ConstructionSelectionWindow_l_GuiButtons_UpdateListBox(void* window);
}
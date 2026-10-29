#pragma once
#include "InGameGuiStorage.h"
#include "InGameGuiSelectionWindowAccess.h"
#include <cstdint>

namespace NXBasics
{
	class CStringArray;
}

namespace NXBasics
{
	class CBitmap;
	struct SRectangle;
}

namespace NC2InGameGuiMiscWindows
{
	// Physical Windows storage; string arrays retain their original ownership.
	struct SHelpHouseEntryStorage
	{
		std::uint32_t engineType, developmentStage;
	};

	struct SHelpWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseWindow;
		std::uint32_t page;
		NXBasics::SRectangle hyperTextFrame;
		std::uint32_t houseCount;
		SHelpHouseEntryStorage houses[100];
		NXBasics::CStringArray* houseHelpStrings;
		std::int32_t selectedHouseIndex;
		std::uint32_t goodCount;
		std::uint32_t goods[100];
		NXBasics::CStringArray* goodHelpStrings;
		std::int32_t selectedGoodIndex;
		std::uint32_t miscCount;
		NXBasics::CStringArray* miscHelpStrings;
		std::int32_t selectedMiscIndex;
		void* hyperTextElement;
		NC2InGameGuiBase::CDynamicSubButtonsHandler dynamicButtons;
	};

	static_assert(sizeof(SHelpWindowStorage) == (sizeof(void*) == 8 ? 5472 : 4176));

	void* HyperLinkTextWindow_Construct(void* window);
	bool __fastcall HyperLinkTextWindow_XGui_BE_Message_Handle(void* window, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
	void __fastcall HyperLinkTextWindow_l_GuiButtons_Add(void* window);
	void __fastcall HyperLinkTextWindow_l_GuiButtons_Update(void* window);
	void __fastcall HyperLinkTextWindow_XGui_BE_Element_Draw(void* window, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	std::int32_t __cdecl lsSortHouseTypeCompareFunction(const void* first, const void* second);
}
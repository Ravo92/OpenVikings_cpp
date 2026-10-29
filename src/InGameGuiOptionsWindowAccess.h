#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2InGameGuiMiscWindows
{
	// Physical Windows suffixes; independent owners retain distinct page/mode fields.
	struct SOptionsWindowStorage
	{
		std::byte inheritedToolWindow[112];
		std::uint32_t page, speedComparisonValue;
	};

	struct SGameSafetyBoxStorage
	{
		std::byte inheritedToolWindow[112];
		std::uint32_t mode;
	};

	void* OptionsWindow_Construct(void* window, bool loadPage, bool savePage);
	void __fastcall OptionsWindow_l_GuiButtons_Add(void* window);
	bool __fastcall OptionsWindow_XGui_BE_Message_Handle(void* window, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
}
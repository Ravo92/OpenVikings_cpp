#pragma once
#include "InGameGuiStorage.h"

#include <cstddef>
#include <cstdint>

namespace NC2InGameGuiMiscWindows
{
	// Physical Windows suffixes; independent owners retain distinct page/mode fields.
	struct SOptionsWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage inheritedToolWindow;
		std::uint32_t page, speedComparisonValue;
	};

	struct SGameSafetyBoxStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage inheritedToolWindow;
		std::uint32_t mode;
	};

	static_assert(sizeof(SOptionsWindowStorage) == (sizeof(void*) == 8 ? 192 : 120));
	static_assert(sizeof(SGameSafetyBoxStorage) == (sizeof(void*) == 8 ? 192 : 116));

	void* OptionsWindow_Construct(void* window, bool loadPage, bool savePage);
	void __fastcall OptionsWindow_l_GuiButtons_Add(void* window);
	bool __fastcall OptionsWindow_XGui_BE_Message_Handle(void* window, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
}
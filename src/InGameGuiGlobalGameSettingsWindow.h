#pragma once
#include "InGameGuiStorage.h"

#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	struct SRectangle;
}

namespace NC2InGameGuiMiscWindows
{

	struct SGlobalGameSettingsWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint32_t lastRefreshTime;
	};

	static_assert(sizeof(SGlobalGameSettingsWindowStorage) == (sizeof(void*) == 8 ? 192 : 116));

	void* GlobalGameSettingsWindow_Construct(void* window, bool specialItemsPage, bool assistantPage);
	bool __fastcall GlobalGameSettingsWindow_XGui_BE_Message_Handle(void* window, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uintptr_t param1Payload, std::uintptr_t param2Payload, std::uintptr_t param3Payload);
	void __fastcall GlobalGameSettingsWindow_l_GuiButtons_Add(void* window);
	void __fastcall GlobalGameSettingsWindow_XGui_BE_Element_Draw(void* window, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
}
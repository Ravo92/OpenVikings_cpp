#pragma once

#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	struct SRectangle;
}

namespace NC2InGameGuiMiscWindows
{
	void* GlobalGameSettingsWindow_Construct(void* window, bool specialItemsPage, bool assistantPage);
	bool __fastcall GlobalGameSettingsWindow_XGui_BE_Message_Handle(void* window, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	void __fastcall GlobalGameSettingsWindow_l_GuiButtons_Add(void* window);
	void __fastcall GlobalGameSettingsWindow_XGui_BE_Element_Draw(void* window, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
}
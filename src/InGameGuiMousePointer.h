#pragma once
#include <cstddef>
#include <cstdint>
namespace NXBasics
{
	class CBitmap;
}
namespace NC2InGameGuiManager
{
	// Windows cursor suffix shared by separate in-game and main-menu method tables.
	struct SGuiManagerMousePointerStorage
	{
		const std::uintptr_t* methodTable;
		std::uint32_t tooltipDeadlineTick;
		const char* tooltipText;
		std::int32_t tooltipMouseX, tooltipMouseY;
	};
	void gToolTip_PrintStringSingleLine(NXBasics::CBitmap* bitmap, std::int32_t* point, const char* text, bool bright);
	void ls_PrintString(NXBasics::CBitmap* bitmap, std::int32_t* point, const char* text, bool bright);
	void __fastcall GuiManagerMousePointer_DrawMouse(void* pointer, void* unusedEdx, NXBasics::CBitmap* bitmap, const std::int32_t* mousePoint);
}
void* __fastcall InGameGuiManagerMousePointer_Construct(void* pointer);
namespace NC2InGameGuiWorldGameCursor
{
	bool __cdecl IGG_DrawWorldGameCursor_LogicCallback(std::uint32_t type, std::uintptr_t first, std::uintptr_t second, std::uintptr_t third, std::uintptr_t fourth);
}

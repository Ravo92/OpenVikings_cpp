#pragma once

#include "XBasicTypes.h"

namespace NXBaseGui
{
	class CBaseElement;
	class CBaseWindow;
	class CDesktop;
}

namespace NXBasics
{
	class CPalette;
	class CBitmap;
}

namespace NC2InGameGuiBase
{
	extern const std::uintptr_t gBaseToolWindowVirtualTable[19];
	void* BaseToolWindow_Construct(void* window, const NXBasics::SRectangle* rectangle);
	void BaseToolDesktop_AddElement(NXBaseGui::CBaseElement* element);
	void BaseToolDesktop_AddOverlayElement(NXBaseGui::CBaseElement* element);
	void BaseToolDesktop_AddWindow(NXBaseGui::CBaseWindow* window, bool showWhileHidden);
	void BaseToolDesktop_SelectionActiveChanged();
	NXBaseGui::CDesktop* GetDesktopPtr();
	std::int32_t GetDisplayWidth();
	std::int32_t GetDisplayHeight();
	void ToolGraphics_SetPalettePtr(void* object, NXBasics::CPalette* palette);
	void* __fastcall BaseToolWindow_Delete(void* window, void* unusedEdx, std::uint32_t flags);
	void __fastcall BaseToolWindow_Draw(void* window, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
}
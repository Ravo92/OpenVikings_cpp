#pragma once
#include "XGuiElementAccess.h"

namespace NXBaseGui
{
	extern const std::uintptr_t gBaseWindowVirtualTable[19];
	void* __fastcall BaseWindow_Construct(void* window, void* unusedEdx, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t elementId);
	void __fastcall BaseWindow_Destruct(void* window);
	void BaseWindow_Element_Remove(void* window, void* element);
	void BaseWindow_Element_Delete(void* window, void* element);
	void BaseWindow_Element_DeleteAll(void* window);
	void* BaseWindow_Message_SendToChildElements(void* window, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	void BaseWindow_DrawAllChildElements(void* window, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle& rectangle);
	void* BaseWindow_FindElementOnPosition(void* window, const std::int32_t* point, bool includeDisabled);
	void __fastcall BaseWindow_Activate(void* window, void* unusedEdx, const std::int32_t* point);
	void __fastcall BaseWindow_DeActivate(void* window);
	bool __fastcall BaseWindow_ButtonPressed(void* window, void* unusedEdx, const std::int32_t* point);
	bool __fastcall BaseWindow_ButtonReleased(void* window);
	void BaseWindow_Element_Add(void* window, void* element);
	void BaseWindow_NotifyAddedToDesktop(void* window);
	void BaseWindow_InternalHide(void* window);
	void BaseWindow_InternalUnHide(void* window);
	void BaseWindow_AddToCloseList(void* window);
	void BaseElement_InternalHide(void* element);
	void BaseElement_InternalUnHide(void* element);
}
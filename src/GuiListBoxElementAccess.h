#pragma once

#include "XBasicTypes.h"

void* __fastcall ToolListBoxElement_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle);
void* __fastcall ToolListBoxControllerElement_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, void* list, bool horizontal, bool vertical);
void __fastcall BaseListBoxElement_Element_Add(void* list, void* unusedEdx, void* element);
void __cdecl Tool_Options_FillSavegameListboxForLoading(void* list, void* handler, std::uint32_t command, bool network, std::uint32_t selectedId, bool specialHeadings);

namespace NC2InGameGuiBaseListBox
{
	// Physical Windows storage. Child lists borrow elements; deletion is explicit.
	struct SListBoxStorage
	{
		std::byte baseToolElement[48];
		std::int32_t scrollX, scrollY;
		NXBasics::SRectangle childBounds;
		void* activeChild;
		std::byte drawChildren[20], inputChildren[20];
	};

	struct SListBoxControllerStorage
	{
		std::byte baseToolElement[48];
		void* list;
		std::uint8_t horizontalEnabled, verticalEnabled;
		std::byte unresolved36[2];
		std::int32_t inset;
	};

	void* BaseListBoxElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t messageTypes);
	void BaseListBoxElement_Destruct(void* element);
	void BaseListBoxElement_Element_Remove(void* list, void* element);
	void BaseListBoxElement_Element_Delete(void* list, void* element);
	void BaseListBoxElement_Element_DeleteAll(void* list);
	void BaseListBoxElement_ScrollOffset_Set(void* list, std::int32_t x, std::int32_t y);
	void BaseListBoxElement_ScrollOffset_SetPercentage(void* list, std::int32_t x, std::int32_t y);
	bool BaseListBoxElement_ScrollOffset_CenterOnEntry_StartingWithLetter(void* list, char letter);
}
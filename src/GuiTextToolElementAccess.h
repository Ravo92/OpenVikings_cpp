#pragma once
#include "InGameGuiStorage.h"

namespace NC2InGameGuiBase
{

	extern const std::uintptr_t gTextButtonVirtualTable[18];
	void BaseToolTextButton_Destruct(void* element);
	void* __fastcall BaseToolTextElement_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, bool fillBackground, bool wrapText);
	void BaseToolTextElement_SetCaptionString(void* element, const char* caption);
	void* __fastcall BaseToolTextButton_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, std::uint32_t command, void* handler);
	void* __fastcall BaseToolTextButton_ConstructWithToolTip(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, const char* tooltip, std::uint32_t command, void* handler);
	void* __fastcall BaseToolTextButton_ConstructToggle(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, std::uint32_t command, void* handler, bool selected);
	void* __fastcall BaseToolTextButton_ConstructToggleWithToolTip(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, const char* tooltip, std::uint32_t command, void* handler, bool selected);
}
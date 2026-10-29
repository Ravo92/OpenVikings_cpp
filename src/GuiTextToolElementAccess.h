#pragma once
#include "XBasicTypes.h"

namespace NC2InGameGuiBase
{
	// Windows physical records; the inherited prefix retains its unresolved byte.
	struct STextElementStorage
	{
		std::byte baseTool[48];
		char caption[200];
		std::uint8_t centered, fillBackground, drawFrame, smallFont, wrapText, darkFont;
		std::byte paddingFE[2];
	};

	struct STextButtonStorage
	{
		std::byte baseTool[48];
		char* caption;
		std::uint8_t multiline;
		std::byte padding35[3];
		std::uint32_t lineCount, command;
		void* receiver;
		char* tooltip;
		std::uint8_t immediateTooltip;
		std::byte padding49[3];
		std::uint32_t messageParameter1, messageParameter2;
		std::uint8_t centered, selected, disabled, smallFont, skipOverwideCaption, fillSolidColor;
		std::byte padding5A[2];
		NXBasics::SColorRGB solidColor;
	};

	extern const std::uintptr_t gTextButtonVirtualTable[18];
	void BaseToolTextButton_Destruct(void* element);
	void* __fastcall BaseToolTextElement_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, bool fillBackground, bool wrapText);
	void BaseToolTextElement_SetCaptionString(void* element, const char* caption);
	void* __fastcall BaseToolTextButton_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, std::uint32_t command, void* handler);
	void* __fastcall BaseToolTextButton_ConstructWithToolTip(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, const char* tooltip, std::uint32_t command, void* handler);
	void* __fastcall BaseToolTextButton_ConstructToggle(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, std::uint32_t command, void* handler, bool selected);
	void* __fastcall BaseToolTextButton_ConstructToggleWithToolTip(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, const char* caption, const char* tooltip, std::uint32_t command, void* handler, bool selected);
}
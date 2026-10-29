#pragma once

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	struct SRectangle;
}

namespace NC2InGameGuiBase
{
	// Windows graphic-button suffix; tooltip ownership remains with the original runtime.
	struct SGuiGfxElementStorage
	{
		std::byte inheritedElement[44];
		std::uint32_t graphicsId;
		void* palette;
		std::uint8_t centered;
		std::byte unclassified35[3];
	};

	struct SGuiGfxButtonStorage
	{
		std::byte inheritedElement[44];
		std::uint32_t graphicsId;
		void* palette;
		std::uint8_t centered, rectangularStyle, selected;
		std::byte unclassified37;
		std::uint32_t command;
		void* receiver;
		char* tooltip;
		std::uint8_t immediateTooltip;
		std::byte unclassified45[3];
		std::uint32_t messageParameter1, messageParameter2;
		std::uint8_t disabled;
		std::byte unclassified51[3];
	};

	void* BaseBitmapToolElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t elementId, std::int32_t depth);
	void BaseBitmapToolElement_Destruct(void* element);
	void BaseBitmapToolElement_Draw(const void* element, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	void BaseElement_GlobalToLocalCoordinates(const void* element, std::int32_t* point);
	void* __fastcall BaseToolGfxButton_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, std::uint32_t graphicsId, std::uint32_t command, void* handler);
	void* __fastcall BaseToolGfxButton_ConstructWithToolTip(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, std::uint32_t graphicsId, const char* tooltip, std::uint32_t command, void* handler);
	void* __fastcall WindowCloserButton_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle);
	void* WindowMoverButton_Construct(void* element, const NXBasics::SRectangle* rectangle, const char* caption);
	void* __fastcall BaseToolGfxElement_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, std::uint32_t graphicsId);
	void BaseToolGfxButton_SetToolTipString(void* element, const char* tooltip);
	void __fastcall BaseToolGfxButton_SetGraphicsId(void* element, void* unusedEdx, std::uint32_t graphicsId);
}
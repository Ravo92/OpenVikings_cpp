#pragma once

#include "InGameGuiStorage.h"
#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	struct SRectangle;
}

namespace NC2InGameGuiBase
{

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
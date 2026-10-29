#pragma once

#include "XGuiStorage.h"

namespace NC2InGameGuiBase
{
	// Constructor clears the mode and thirty text bytes; the last allocated byte stays unclassified.
	struct SClockElementStorage
	{
		alignas(SBaseToolElementStorage) std::byte inheritedToolElement[sizeof(SBaseToolElementStorage)];
		std::uint8_t analogMode;
		char timeText[30];
		std::byte unclassified4F;
	};

	extern const std::uintptr_t gBaseToolElementVirtualTable[18];
	void* BaseToolElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t elementId);
	void* __fastcall BaseToolElement_Delete(void* element, void* unusedEdx, std::uint32_t flags);
	void* ToolClockElement_Construct(void* element, const NXBasics::SRectangle* rectangle);
}
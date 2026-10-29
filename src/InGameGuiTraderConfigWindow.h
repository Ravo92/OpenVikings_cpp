#pragma once
#include "InGameGuiStorage.h"
#include "XBasicTypes.h"
#include <cstddef>
#include <cstdint>

namespace NC2InGameGuiSelectedAction
{
	// Only good IDs 1..55 are observed; the tail of each 264-byte control region stays unclassified.
	struct STraderImportControlsStorage
	{
		void* buttons[56];
		std::byte unclassifiedTail[40];
	};

	struct STraderHousePreviewStorage
	{
		NXBasics::SRectangle rectangle;
		void* worldElement;
	};

	struct STraderConfigWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage inheritedToolWindow;
		void* owner;
		std::uint32_t closeCommand;
		STraderImportControlsStorage importControls[2];
		void* clearImportsButton;
		STraderHousePreviewStorage previews[2];
	};

	static_assert(sizeof(STraderConfigWindowStorage) == (sizeof(void*) == 8 ? 1232 : 692));
	static_assert(sizeof(STraderImportControlsStorage) == (sizeof(void*) == 8 ? 488 : 264));
	static_assert(sizeof(STraderHousePreviewStorage) == (sizeof(void*) == 8 ? 24 : 20));

	void* SelectedTraderConfigWindow_Construct(void* window, void* owner, std::uint32_t closeCommand);
}
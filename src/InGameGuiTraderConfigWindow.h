#pragma once
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
		std::byte inheritedToolWindow[112];
		void* owner;
		std::uint32_t closeCommand;
		STraderImportControlsStorage importControls[2];
		void* clearImportsButton;
		STraderHousePreviewStorage previews[2];
	};

	void* SelectedTraderConfigWindow_Construct(void* window, void* owner, std::uint32_t closeCommand);
}
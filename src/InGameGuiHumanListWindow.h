#pragma once
#include "InGameGuiStorage.h"
#include "XBasicTypes.h"
#include <cstdint>

namespace NC2InGameGuiMiscWindows
{
	// Physical Windows storage; callbacks, child ownership and raw access remain unchanged.
	struct SHumanListReferenceStorage
	{
		std::uint32_t humanId, uniqueId;
	};

	struct SHumanListWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseToolWindow;
		std::uint32_t humanCount;
		SHumanListReferenceStorage humans[500];
		void* jobSelectionWindow;
	};

	struct SHumanListJobSelectionWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseToolWindow;
		void* ownerWindow;
		std::uint8_t currentJob;
		std::byte unresolved75[3];
	};

	struct SHumanListButtonStorage
	{
		NC2InGameGuiBase::SBaseToolElementStorage baseToolElement;
		std::uint32_t humanId, uniqueId, pressedCommand, doubleClickCommand;
		void* handler;
		std::uint8_t debug;
		std::byte unresolved45[3];
	};

	static_assert(sizeof(SHumanListReferenceStorage) == 8);
	static_assert(sizeof(SHumanListWindowStorage) == (sizeof(void*) == 8 ? 4200 : 4120));
	static_assert(sizeof(SHumanListJobSelectionWindowStorage) == (sizeof(void*) == 8 ? 200 : 120));
	static_assert(sizeof(SHumanListButtonStorage) == (sizeof(void*) == 8 ? 104 : 72));

	void* HumanListWindow_Construct(void* window, std::uint32_t job, std::uint32_t possibleJob);
	bool HumanListWindow_l_UpdateHumanDataArray(void* window);
	void HumanListWindow_l_UpdateHumanListElements(void* window, bool force);
	void HumanListWindow_l_GuiButtons_Add(void* window);
	void HumanListWindow_l_JobSelectionWindow_Open(void* window, bool currentJob);
}
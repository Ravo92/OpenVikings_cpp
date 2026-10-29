#pragma once
#include "InGameGuiStorage.h"

#include "StringInputHandler.h"
#include <cstddef>
#include <cstdint>

namespace NC2InGameGuiSelectedAction
{
	// Numeric selection state stays fixed-width; enclosing windows use native GUI and input owners.
	struct SSelectedHumanActionWindowStateStorage
	{
		std::uint8_t validSelection;
		std::byte unclassified01[3];
		std::uint32_t selectionChangeCount;
	};

	struct SChangeJobWindowStateStorage
	{
		SSelectedHumanActionWindowStateStorage selection;
		std::uint32_t lastRefreshTime;
		void* jobList;
	};

	struct SChangeNameWindowStateStorage
	{
		SSelectedHumanActionWindowStateStorage selection;
		NC2InGameGuiBaseTextInputElement::CStringInputHandler inputHandler;
	};

	static_assert(offsetof(SChangeNameWindowStateStorage, inputHandler) == 8);
	static_assert(sizeof(SChangeNameWindowStateStorage) == (sizeof(void*) == 8 ? 40 : 36));

	struct SSelectedHumanActionWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		SSelectedHumanActionWindowStateStorage state;
	};

	struct SChangeJobWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		SChangeJobWindowStateStorage state;
	};

	struct SChangeNameWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		SChangeNameWindowStateStorage state;
	};

	static_assert(sizeof(SSelectedHumanActionWindowStateStorage) == 8);
	static_assert(sizeof(SChangeJobWindowStateStorage) == (sizeof(void*) == 8 ? 24 : 16));
	static_assert(sizeof(SSelectedHumanActionWindowStorage) == (sizeof(void*) == 8 ? 192 : 120));
	static_assert(sizeof(SChangeJobWindowStorage) == (sizeof(void*) == 8 ? 208 : 128));
	static_assert(sizeof(SChangeNameWindowStorage) == (sizeof(void*) == 8 ? 224 : 148));

	void* SelectedSingleHumanChangeJobWindow_Construct(void* window);
	void* SelectedSingleHumanChangeNameWindow_Construct(void* window);
	void* SelectedSingleHumanLearnJobWindow_Construct(void* window);
	void* SelectedSingleHumanChangeProducedGoodWindow_Construct(void* window);
}
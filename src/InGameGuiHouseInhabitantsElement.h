#pragma once
#include "InGameGuiStorage.h"
#include "XBasicTypes.h"

namespace NC2InGameGuiSelection
{
	// Physical Windows widget and row storage; callbacks retain borrowed ownership.
	struct SHouseInhabitantsSelectionSlotStorage
	{
		std::int32_t humanId;
		std::uint32_t roleMask;
		std::int32_t x, sortPriority;
		std::uint8_t trader;
		std::byte padding11[3];
	};

	struct SHouseInhabitantsSelectionStorage
	{
		NXBaseGui::SBaseElementStorage baseElement;
		std::int32_t houseId;
		std::uint32_t houseRevision;
		std::int32_t count;
		SHouseInhabitantsSelectionSlotStorage rows[100];
		std::int32_t baselineY, hitIndex;
		char tooltip[200];
		std::uint32_t selectCommand, actionCommand;
		void* receiver;
	};

	static_assert(sizeof(SHouseInhabitantsSelectionStorage) == (sizeof(void*) == 8 ? 2304 : 2276));
	static_assert(sizeof(SHouseInhabitantsSelectionSlotStorage) == (sizeof(void*) == 8 ? 20 : 20));

	void* SelectionHouseInhabitantsElement_Construct(void* element, const NXBasics::SRectangle* rectangle, void* receiver, std::uint32_t selectCommand, std::uint32_t actionCommand, std::int32_t houseId);
}
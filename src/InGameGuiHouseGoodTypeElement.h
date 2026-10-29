#pragma once
#include "InGameGuiStorage.h"
#include "XBasicTypes.h"

namespace NC2InGameGuiSelection
{
	struct SHouseGoodTypeElementStorage
	{
		NXBaseGui::SBaseElementStorage baseElement;
		std::int32_t houseId;
		std::uint32_t houseRevision, goodType;
		char tooltip[200];
		std::uint32_t pressedCommand, hoverCommand;
		void* receiver;
	};

	static_assert(sizeof(SHouseGoodTypeElementStorage) == (sizeof(void*) == 8 ? 296 : 268));

	void* SelectionHouseGoodTypeElement_Construct(void* element, const NXBasics::SRectangle* rectangle, void* receiver, std::uint32_t selectCommand, std::uint32_t actionCommand, std::int32_t houseId, std::uint32_t good);
}
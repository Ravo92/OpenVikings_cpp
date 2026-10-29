#pragma once
#include "XBasicTypes.h"

namespace NC2InGameGuiSelection
{
	void* SelectionHouseGoodTypeElement_Construct(void* element, const NXBasics::SRectangle* rectangle, void* receiver, std::uint32_t selectCommand, std::uint32_t actionCommand, std::int32_t houseId, std::uint32_t good);
}
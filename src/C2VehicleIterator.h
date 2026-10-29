#pragma once

#include "C2AnimalIterator.h"

namespace NC2Logic
{
	class CVehicleIterator : private CAnimalIterator
	{
	  public:
		CVehicleIterator() = default;
		std::int32_t GetFirstVehicleId();
		std::int32_t GetFirstVehicleId(std::uint32_t player);
		std::int32_t GetFirstVehicleId(std::uint32_t player, const void* point, std::int32_t radius);
		std::int32_t GetNextVehicleId();
		std::int32_t GetFirstSectorVehicleId(std::uint32_t player, std::uint32_t point, std::int32_t radius, bool useSecondList);
		std::int32_t GetNextSectorVehicleId();

	  private:
		std::int32_t l_GetNext(std::int32_t previous);
		std::int32_t l_GetNextSectorVehicleId(std::int32_t previous);
	};

	static_assert(sizeof(CVehicleIterator) == 44);
}
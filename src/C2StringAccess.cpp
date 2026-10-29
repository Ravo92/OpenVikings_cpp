#include "C2EngineDisplayAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2StringAccess.h"
#include "XStringArray.h"

namespace NC2Logic
{
	// Original function: FUN_0042f6e1.
	const char* StringAccess_GetGoodNamePtr(std::uint32_t type, std::uint32_t amount)
	{
		auto* strings = gStaticStrings[1];

		if (strings == nullptr)
			return nullptr;

		return strings->GetStringPtr(type * 2u + (amount > 1u ? 1u : 0u));
	}

	// Original function: FUN_0042f703.
	const char* StringAccess_GetTribeNamePtr(std::uint32_t type, std::uint8_t variant)
	{
		auto* strings = gStaticStrings[2];

		if (strings == nullptr)
			return nullptr;

		return strings->GetStringPtr(type * 2u + (variant == 0 ? 1u : 0u));
	}

	// Original function: FUN_0042f725.
	const char* StringAccess_GetJobNamePtr(std::uint32_t type, std::uint32_t amount)
	{
		auto* strings = gStaticStrings[3];

		if (strings == nullptr)
			return nullptr;

		return strings->GetStringPtr(type * 2u + (amount > 1u ? 1u : 0u));
	}

	// Original function: FUN_0042f747.
	const char* StringAccess_GetHouseNamePtr(std::uint32_t type, std::uint32_t amount)
	{
		auto* strings = gStaticStrings[4];

		if (strings == nullptr)
			return nullptr;

		return strings->GetStringPtr(type * 2u + (amount > 1u ? 1u : 0u));
	}

	// Original function: FUN_0042f792.
	const char* StringAccess_GetJobExperienceNamePtr(std::uint32_t type)
	{
		auto* strings = gStaticStrings[5];

		if (strings == nullptr)
			return nullptr;

		return strings->GetStringPtr(type);
	}

	// Original function: FUN_0042f7a9.
	const char* StringAccess_GetVehicleTypeNamePtr(std::uint32_t type)
	{
		auto* strings = gStaticStrings[6];

		if (strings == nullptr)
			return nullptr;

		return strings->GetStringPtr(type);
	}

	// Original function: FUN_0042f7c0.
	const char* StringAccess_GetVehicleTaskNamePtr(std::uint32_t type)
	{
		auto* strings = gStaticStrings[7];

		if (strings == nullptr)
			return nullptr;

		return strings->GetStringPtr(type);
	}

	// Original function: FUN_0042f7d7.
	const char* StringAccess_GetMilitaryModeNamePtr(std::uint32_t type)
	{
		auto* strings = gStaticStrings[8];

		if (strings == nullptr)
			return nullptr;

		return strings->GetStringPtr(type);
	}

	// Original function: FUN_0042f7ee.
	const char* StringAccess_GetMiscStringPtr(std::uint32_t type)
	{
		auto* strings = gStaticStrings[9];

		if (strings == nullptr)
			return nullptr;

		return strings->GetStringPtr(type);
	}

	// Original function: FUN_0042f769.
	const char* StringAccess_GetHouseNamePtr(std::uint32_t type, std::uint32_t developmentStage, std::uint32_t amount)
	{
		const void* names = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(type);

		if (names == nullptr || developmentStage >= NC2Lifetime::Read<std::uint32_t>(names, NC2Layout::EngineHouseLogicViewOffset::DevelopmentStageCount))
			return nullptr;

		return StringAccess_GetHouseNamePtr(NC2Lifetime::Read<std::uint32_t>(names, developmentStage * NC2Layout::RecordSize::HouseStageValueUnsigned), amount);
	}
}
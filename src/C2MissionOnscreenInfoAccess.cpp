#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionOnscreenInfoAccess.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include "XStringArray.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_004171e7; broadcast targets only allocated players.
	void MissionOnscreenInfoManager_Info_Set(void* manager, std::uint32_t player, std::uint32_t index, std::uint32_t type, std::uint32_t string, std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t e, std::uint32_t f, std::uint32_t g, std::uint32_t h)
	{
		if (NXBasics::Runtime_TimeSigned32(index) < 0 || NXBasics::Runtime_TimeSigned32(index) >= 5)
		{

			return;
		}

		const auto set = [&](std::uint32_t owner)
		{
			void* entry = static_cast<std::byte*>(manager) + (owner * 5u + index) * 40u;
			std::memset(entry, 0, 40);
			const std::uint32_t values[10] = { type, string, a, b, c, d, e, f, g, h };

			for (std::uint32_t field = 0; field < 10; ++field)
			{
				Write(entry, field * NC2Layout::RecordSize::MissionInfoArgumentUnsigned, values[field]);
			}
		};

		if (player == NXBasics::IntegerRange::UInt32Maximum || player == 20)
		{
			for (std::uint32_t owner = 0; owner < 20; ++owner)
			{
				if (Read<std::uint8_t>(gGameControl, owner * NC2Layout::RecordSize::GameControlPlayer) != 0)
					set(owner);
			}
		}
		else if (NXBasics::Runtime_TimeSigned32(player) >= 0 && player < 20 && Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer) != 0)
		{
			set(player);
		}
	}

	// Original function: FUN_00417323.
	const void* MissionOnscreenInfoManager_Info_GetPtr(const void* manager, std::int32_t player, std::int32_t index)
	{
		if (player < 0 || player >= 20 || Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer) == 0 || index < 0 || index >= 5)
			return nullptr;

		const auto* info = static_cast<const std::byte*>(manager) + (static_cast<std::uint32_t>(player) * 5u + static_cast<std::uint32_t>(index)) * 40u;

		return Read<std::uint32_t>(info, NC2Layout::MissionOnscreenInfoOffset::Type) == 0 ? nullptr : info;
	}

	// Original function: FUN_0043b42b.
	std::uint32_t Tool_CountGoodsOnMap(std::uint32_t good, const void* point)
	{
		if (!MapMigPoint_IsInsideMap(point))
			return 0;

		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1)
			return 0;

		const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);

		return landscape != 0 && static_cast<std::uint32_t>(gStaticDependencyGoodByLandscape[landscape]) == good ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0;
	}

	// Original function: FUN_0043b46b; unwrapped hexagonal rings preserve the native border behavior.
	std::uint32_t Tool_CountGoodsOnMap(std::uint32_t good, std::uint32_t point, std::uint32_t radius)
	{
		auto result = Tool_CountGoodsOnMap(good, &point);

		for (std::uint32_t ring = 1; ring <= radius && radius != 0; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(point, 4);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::uint32_t step = 0; step < ring; ++step)
				{
					result += Tool_CountGoodsOnMap(good, &point);
					MapMigPoint_MoveCursorUnwrapped(point, direction);
				}
			}
		}

		return result;
	}

	static std::uint32_t Distance(const void* house, std::uint32_t point)
	{

		return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPositionX), Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPositionY), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y)));
	}

	// Original function: FUN_0043b4d4.
	std::uint32_t Tool_CountGoodsInHouses(std::uint32_t player, std::uint32_t good, std::uint32_t point, std::uint32_t radius)
	{
		std::uint32_t count = 0;
		CHouseIterator iterator;

		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && Distance(house, point) <= radius && House_Stock_CanStoreGoodType(house, good, true))
				count += House_Stock_GetAmountOfGoods(house, good, true);
		}

		return count;
	}

	// Original function: FUN_0043b55c.
	std::uint32_t Tool_CountHousesOnMap(std::uint32_t player, std::uint32_t type, std::uint32_t point, std::uint32_t radius)
	{
		std::uint32_t count = 0;
		CHouseIterator iterator;

		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == type && Distance(house, point) <= radius)
				++count;
		}

		return count;
	}

	// Original function: FUN_0041736d.
	bool MissionOnscreenInfoManager_Info_StringCreate(const void* manager, std::int32_t player, std::int32_t index, NXBasics::CStringArray* strings, char* output, std::uint32_t capacity)
	{
		if (output == nullptr || capacity == 0)
			return false;

		*output = 0;
		const auto* info = MissionOnscreenInfoManager_Info_GetPtr(manager, player, index);

		if (info == nullptr)
			return false;

		const auto* format = strings->GetStringPtr(Read<std::uint32_t>(info, NC2Layout::MissionOnscreenInfoOffset::StringId));

		if (format == nullptr)
			return false;

		const auto type = Read<std::uint32_t>(info, NC2Layout::MissionOnscreenInfoOffset::Type);
		std::uint32_t count = 0, target = 0;

		if (type == 2 || type == 3)
		{
			const auto goodOrType = Read<std::uint32_t>(info, NC2Layout::MissionTypedAreaInfoOffset::ObjectType);
			const auto point = static_cast<std::uint32_t>(Read<std::uint16_t>(info, NC2Layout::MissionTypedAreaInfoOffset::CenterX)) | static_cast<std::uint32_t>(Read<std::uint16_t>(info, NC2Layout::MissionTypedAreaInfoOffset::CenterY)) << 16;
			const auto radius = Read<std::uint32_t>(info, NC2Layout::MissionTypedAreaInfoOffset::Radius);

			if (type == 2)
			{
				count = Tool_CountGoodsInHouses(player, goodOrType, point, radius) + Tool_CountGoodsOnMap(goodOrType, point, radius);
			}
			else
			{
				count = Tool_CountHousesOnMap(player, goodOrType, point, radius);
			}

			target = Read<std::uint32_t>(info, NC2Layout::MissionTypedAreaInfoOffset::TargetCount);
		}
		else if (type == 4 || type == 5)
		{
			const auto point = static_cast<std::uint32_t>(Read<std::uint16_t>(info, NC2Layout::MissionHumanAreaInfoOffset::CenterX)) | static_cast<std::uint32_t>(Read<std::uint16_t>(info, NC2Layout::MissionHumanAreaInfoOffset::CenterY)) << 16;
			const auto radius = Read<std::int32_t>(info, NC2Layout::MissionHumanAreaInfoOffset::Radius);
			CHumanIterator iterator;

			for (auto id = iterator.GetFirstSectorHumanId(player, point, radius, type == 5); id != -1; id = iterator.GetNextSectorHumanId())
			{
				++count;
			}

			if (type == 4)
			{
				CHumanIterator second;

				for (auto id = second.GetFirstSectorHumanId(player, point, radius, true); id != -1; id = second.GetNextSectorHumanId())
				{
					++count;
				}
			}

			target = Read<std::uint32_t>(info, NC2Layout::MissionHumanAreaInfoOffset::TargetCount);
		}
		else if (type == 6)
		{
			const auto animalType = Read<std::uint32_t>(info, NC2Layout::MissionTypedAreaInfoOffset::ObjectType);
			const auto point = static_cast<std::uint32_t>(Read<std::uint16_t>(info, NC2Layout::MissionTypedAreaInfoOffset::CenterX)) | static_cast<std::uint32_t>(Read<std::uint16_t>(info, NC2Layout::MissionTypedAreaInfoOffset::CenterY)) << 16;
			CAnimalIterator iterator;

			for (auto id = iterator.GetFirstSectorAnimalId(player, point, Read<std::int32_t>(info, NC2Layout::MissionTypedAreaInfoOffset::Radius), false, false); id != -1; id = iterator.GetNextSectorAnimalId())
			{
				if (Read<std::uint32_t>(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::ElementSize, NC2Layout::AnimalOffset::TribeType) == animalType)
					++count;
			}

			target = Read<std::uint32_t>(info, NC2Layout::MissionTypedAreaInfoOffset::TargetCount);
		}

		NXBasics::Runtime_PrintStringN(output, capacity, format, count, target);
		output[capacity - 1] = 0;

		return true;
	}
}
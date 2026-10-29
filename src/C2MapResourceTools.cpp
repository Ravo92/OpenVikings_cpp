#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanGoods.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapResourceTools.h"
#include "C2ObjectLayout.h"
#include "C2StaticTribeTypeData.h"
#include "C2VehicleAccess.h"
#include "C2VehicleIterator.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_0043B5C6; radius-filtered iterator order and DWORD sum wrap are native.
	std::uint32_t Tool_CountGoodsInVehicles(std::uint32_t player, std::uint32_t vehicleType, std::uint32_t good, std::uint32_t point, std::uint32_t radius)
	{
		CVehicleIterator iterator;
		std::uint32_t total = 0;
		for (auto id = iterator.GetFirstVehicleId(player, &point, NXBasics::Runtime_TimeSigned32(radius)); id != -1; id = iterator.GetNextVehicleId())
		{
			const auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;
			if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType) == vehicleType) total += Vehicle_Stock_GetAmountOfGoods(vehicle, good);
		}
		return total;
	}

	// Original function: FUN_0043B620; the return value is the remaining amount.
	std::int32_t Tool_AddGoodsToMapPoint(std::uint32_t good, std::int32_t amount, const void* point, bool useHouses, std::uint32_t player)
	{
		if (!MapMigPoint_IsInsideMap(point)) return amount;
		if (useHouses && Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ObjectType) == 2)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			{
				micro = GameMap_GetConstMapMicroPointPtr(point);
				const auto id = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 ? static_cast<std::uint32_t>(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId)) : NXBasics::IntegerRange::UInt32Maximum;
				auto* house = CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
				if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) == player && (House_Stock_CanStoreGoodType(house, good, false) || House_Stock_CanStoreGoodType(house, good, true)))
				{
					do
					{
						if (House_Stock_IsFull(house, good) || !House_Stock_ModifyAmount(house, good, 1, 0, false)) break;
						amount = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(amount) - 1u);
					} while (amount != 0);
					if (amount == 0) return 0;
				}
			}
		}
		do
		{
			if (!Map_GoodTool_PutOnMap(good, point, false)) return amount;
			amount = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(amount) - 1u);
		} while (amount != 0);
		return 0;
	}

	// Original function: FUN_0043B6FE; process before moving on each ring edge.
	std::int32_t Tool_AddGoodsToMapArea(std::uint32_t good, std::int32_t amount, std::uint32_t point, std::uint32_t radius, bool useHouses, std::uint32_t player)
	{
		amount = Tool_AddGoodsToMapPoint(good, amount, &point, useHouses, player);
		if (amount == 0) return 0;
		for (std::uint32_t ring = 1; ring <= radius; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(point, 4);
			for (std::uint32_t direction = 0; direction < 6; ++direction)
				for (std::uint32_t step = 0; step < ring; ++step)
				{
					amount = Tool_AddGoodsToMapPoint(good, amount, &point, useHouses, player);
					if (amount == 0) return 0;
					MapMigPoint_MoveCursorUnwrapped(point, direction);
				}
		}
		return amount;
	}

	// Original function: FUN_0043B78A; houses precede landscape transition 3.
	std::int32_t Tool_RemoveGoodsFromMapPoint(std::uint32_t good, std::int32_t amount, const void* point, bool useHouses, std::uint32_t player)
	{
		if (!MapMigPoint_IsInsideMap(point)) return amount;
		if (useHouses && Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ObjectType) == 2)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			{
				micro = GameMap_GetConstMapMicroPointPtr(point);
				const auto id = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 ? static_cast<std::uint32_t>(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId)) : NXBasics::IntegerRange::UInt32Maximum;
				auto* house = CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
				if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) == player && House_Stock_CanStoreGoodType(house, good, true))
				{
					do
					{
						if (!House_Stock_ModifyAmount(house, good, -1, 0, false)) break;
						amount = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(amount) - 1u);
					} while (amount != 0);
					if (amount == 0) return 0;
				}
			}
		}
		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
		do
		{
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1) return amount;
			const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);
			if (landscape == 0 || gStaticDependencyGoodByLandscape[landscape] != good || GameMap_MapMicroPoint_Landscape_ApplyTransition(point, 3) == -1) return amount;
			amount = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(amount) - 1u);
		} while (amount != 0);
		return 0;
	}

	// Original function: FUN_0043B86D; distinct original area-removal boundary.
	std::int32_t Tool_RemoveGoodsFromMapArea(std::uint32_t good, std::int32_t amount, std::uint32_t point, std::uint32_t radius, bool useHouses, std::uint32_t player)
	{
		amount = Tool_RemoveGoodsFromMapPoint(good, amount, &point, useHouses, player);
		if (amount == 0) return 0;
		for (std::uint32_t ring = 1; ring <= radius; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(point, 4);
			for (std::uint32_t direction = 0; direction < 6; ++direction)
				for (std::uint32_t step = 0; step < ring; ++step)
				{
					amount = Tool_RemoveGoodsFromMapPoint(good, amount, &point, useHouses, player);
					if (amount == 0) return 0;
					MapMigPoint_MoveCursorUnwrapped(point, direction);
				}
		}
		return amount;
	}
}

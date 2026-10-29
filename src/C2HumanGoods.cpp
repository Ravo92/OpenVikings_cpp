#include "C2Debug.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanTaskAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticGoodTypeData.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* House(std::uint32_t id)
		{
			return CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
		}
		void* Guide(std::int32_t id)
		{
			return CGuideArray::mStaticVars + static_cast<std::intptr_t>(id) * CGuideArray::ElementSize;
		}

		struct SHumanSearchGoodData
		{
			// Original constructor: FUN_00460ea8; only points and counters are initialized.
			SHumanSearchGoodData(std::uint32_t wanted, std::uint32_t landscape, std::uint32_t owner) : good(wanted), landscapeType(landscape), player(owner), firstDistance(-1), unknown10(0), landscapeCount(0), points{ }, houseCount(0), guideCount(0)
			{}
			std::uint32_t good, landscapeType, player;
			std::int32_t firstDistance;
			std::uint8_t unknown10;
			std::byte padding11[3];
			std::uint32_t landscapeCount, points[10], houseCount;
			void* houses[2];
			std::uint32_t guideCount, guides[3];
		};
		static_assert(sizeof(SHumanSearchGoodData) == (sizeof(void*) == 8 ? 104 : 92));

		// Original function: FUN_0045f5fd; stock and map candidates retain discovery order.
		bool __cdecl slHumanSearchGoodFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SHumanSearchGoodData*>(callback->userData);
			if (data.firstDistance >= 0 && callback->distance - static_cast<std::uint32_t>(data.firstDistance) > NC2Runtime::GoodSearchLimits::DistanceSlackUnsigned) return true;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			const auto kind = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);
			if (kind == SwitchIds::MapMicroObjectType::House && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			{
				auto* house = House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId));
				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || House_GetHouseMainType(house) == 2 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != data.player) return false;
				if (House_Stock_CanStoreGoodType(house, data.good, true) && House_Stock_RequestAmountOfGoods(house, data.good, true) != 0)
				{
					data.houses[data.houseCount++] = house;
					if (data.firstDistance < 0) data.firstDistance = NXBasics::Runtime_TimeSigned32(callback->distance);
				}
			}
			else if (kind == SwitchIds::MapMicroObjectType::Guide && Read<std::uint32_t>(Guide(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId)), NC2Layout::GuideOffset::PlayerId) == data.player)
			{
				if (data.guideCount < NC2Runtime::GoodSearchLimits::GuideCandidates) data.guides[data.guideCount++] = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId);
				return false;
			}
			else if ((kind == SwitchIds::MapMicroObjectType::Landscape ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) : 0u) == data.landscapeType)
			{
				data.points[data.landscapeCount++] = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
				if (data.firstDistance < 0) data.firstDistance = NXBasics::Runtime_TimeSigned32(callback->distance);
			}
			return data.landscapeCount == NC2Runtime::GoodSearchLimits::LandscapeCandidates || data.houseCount == NC2Runtime::GoodSearchLimits::HouseCandidates;
		}

		// Original function: FUN_0041b438; the Windows stock branch does not require an entry flag.
		bool __cdecl slGoodSearchAround(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SGoodSearch*>(callback->userData);
			if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapY), Read<std::int16_t>(&data.point, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(&data.point, NC2Layout::MapMigPointOffset::MapY))) >= NC2Runtime::GoodSearchLimits::SearchRadiusUnsigned) return false;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0)
			{
				if (gStaticDependencyGoodByLandscape[Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData)] != data.good) return false;
			}
			else
			{
				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::House) return false;
				auto* house = House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId));
				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || House_GetHouseMainType(house) == 2 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != data.player) return false;
				if (!House_Stock_CanStoreGoodType(house, data.good, true) || House_Stock_RequestAmountOfGoods(house, data.good, true) == 0) return false;
			}
			data.point = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
			return true;
		}

		struct SConsumerSearch
		{
			std::uint32_t good; std::uint32_t player; std::uint32_t count; void* houses[2];
		};
		static_assert(sizeof(SConsumerSearch) == (sizeof(void*) == 8 ? 32 : 20));
		static_assert(offsetof(SConsumerSearch, houses) == (sizeof(void*) == 8 ? 16 : 12));
		bool SearchConsumer(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback, bool freeStock)
		{
			auto& data = *static_cast<SConsumerSearch*>(callback->userData);
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::House || (freeStock && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) == 0)) return false;
			auto* house = House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId));
			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || (House_GetHouseMainType(house) == 1) != freeStock || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != data.player) return false;
			if (!House_Stock_CanStoreGoodType(house, data.good, false) || House_Stock_IsFull(house, data.good) || !House_Stock_HasDemandForGood(house, data.good)) return false;
			data.houses[data.count++] = house;
			return data.count == (freeStock ? 2u : 1u);
		}
		// Original functions: FUN_0045f29f and FUN_0045f331.
		bool __cdecl slHumanSearchConsumerFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			return SearchConsumer(point, callback, false);
		}
		bool __cdecl slHumanSearchFreeStockFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			return SearchConsumer(point, callback, true);
		}

		struct SPilePointSearch
		{
			std::uint32_t good; std::uint32_t count; std::uint32_t points[2];
		};
		// Original function: FUN_0045d724.
		bool __cdecl slHumanSearchPilePointFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SPilePointSearch*>(callback->userData);
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & 1u) != 0 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0) return false;
			if (Map_GoodTool_CanBePutOnMap(data.good, point)) data.points[data.count++] = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
			return data.count == 1;
		}

		// Original function: FUN_0045d6a3; no separate Mac name recovered.
		void AdjustPileSearchPosition(void* point)
		{
			auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::House && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0) return;
			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) & NXBasics::BitMask::Bit4) == 0)
			{
				if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) & NXBasics::BitMask::Bit5) == 0) return;
				MapMigPoint_MoveCursorUnwrapped(point, 5, 1);
				micro = GameMap_GetConstMapMicroPointPtr(point);
				if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & 1u) != 0 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
				{
					MapMigPoint_MoveCursorUnwrapped(point, 2, 1); return;
				}
				if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) & NXBasics::BitMask::Bit4) == 0) return;
			}
			MapMigPoint_MoveCursorUnwrapped(point, 4, 1);
			micro = GameMap_GetConstMapMicroPointPtr(point);
			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & 1u) != 0 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0) MapMigPoint_MoveCursorUnwrapped(point, 1, 1);
		}
		bool GuideHasAnyGood(const void* guide, const SHumanSearchGoodsComplexData& data)
		{
			for (std::uint32_t i = 0; i < data.goodCount; ++i) if (Read<std::uint32_t>(guide, NC2Layout::GuideOffset::GoodsCountsUnsigned + data.goods[i] * NC2Layout::RecordSize::GuideGoodCountUnsigned) != 0) return true;
			return false;
		}

		// Original function: FUN_0045d622; no separate Mac name recovered.
		void AdjustPickupWorkPosition(void* point)
		{
			auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::House && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0) return;
			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) & 2u) == 0)
			{
				if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) & 4u) == 0) return;
				MapMigPoint_MoveCursorUnwrapped(point, 2, 1);
				micro = GameMap_GetConstMapMicroPointPtr(point);
				if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & 1u) != 0 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
				{
					MapMigPoint_MoveCursorUnwrapped(point, 5, 1); return;
				}
				micro = GameMap_GetConstMapMicroPointPtr(point);
				if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) & 2u) == 0) return;
			}
			MapMigPoint_MoveCursorUnwrapped(point, 1, 1);
			micro = GameMap_GetConstMapMicroPointPtr(point);
			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & 1u) != 0 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0) MapMigPoint_MoveCursorUnwrapped(point, 4, 1);
		}

		// Original function: FUN_0045f9f9; requesting an empty stock records demand.
		bool __cdecl slHumanSearchGoodsComplexIgnoringHomesFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SHumanSearchGoodsComplexData*>(callback->userData);
			if (data.firstDistance >= 0 && callback->distance - static_cast<std::uint32_t>(data.firstDistance) > NC2Runtime::GoodSearchLimits::DistanceSlackUnsigned) return true;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);
			if (type == SwitchIds::MapMicroObjectType::House && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			{
				auto* house = House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId));
				if (House_GetHouseMainType(house) == 2 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != data.player || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0) return false;
				for (std::uint32_t i = 0; i < data.goodCount; ++i)
				{
					if (!House_Stock_CanStoreGoodType(house, data.goods[i], true) || House_Stock_RequestAmountOfGoods(house, data.goods[i], true) == 0) continue;
					data.houses[data.houseCount] = house;
					data.houseGoods[data.houseCount++] = data.goods[i];
					if (data.firstDistance < 0) data.firstDistance = NXBasics::Runtime_TimeSigned32(callback->distance);
					break;
				}
			}
			else if (type == SwitchIds::MapMicroObjectType::Guide && Read<std::uint32_t>(Guide(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId)), NC2Layout::GuideOffset::PlayerId) == data.player)
			{
				if (data.guideCount < NC2Runtime::GoodSearchLimits::GuideCandidates) data.guides[data.guideCount++] = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId);
				return false;
			}
			else if (type == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0)
			{
				for (std::uint32_t i = 0; i < data.goodCount; ++i)
				{
					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != data.landscapeTypes[i]) continue;
					data.landscapePoints[data.landscapeCount] = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
					data.landscapeGoods[data.landscapeCount++] = data.goods[i];
					if (data.firstDistance < 0)
					{
						data.firstDistance = NXBasics::Runtime_TimeSigned32(callback->distance); data.firstWasLandscape = 1;
					}
					break;
				}
			}
			return data.landscapeCount == NC2Runtime::GoodSearchLimits::LandscapeCandidates || data.houseCount == NC2Runtime::GoodSearchLimits::HouseCandidates;
		}

		// Original function: FUN_004604d1; home-house refill only fetches from work houses.
		bool __cdecl slHumanSearchGoodsComplexInWorkHousesOnlyFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SHumanSearchGoodsComplexData*>(callback->userData);
			if (data.firstDistance >= 0 && callback->distance - static_cast<std::uint32_t>(data.firstDistance) > NC2Runtime::GoodSearchLimits::DistanceSlackUnsigned) return true;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);
			if (type == SwitchIds::MapMicroObjectType::House && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			{
				auto* house = House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId));
				if (House_GetHouseMainType(house) != 3 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != data.player || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0) return false;
				for (std::uint32_t index = 0; index < data.goodCount; ++index)
				{
					if (!House_Stock_CanStoreGoodType(house, data.goods[index], true) || House_Stock_RequestAmountOfGoods(house, data.goods[index], true) == 0) continue;
					data.houses[data.houseCount] = house;
					data.houseGoods[data.houseCount++] = data.goods[index];
					if (data.firstDistance < 0) data.firstDistance = NXBasics::Runtime_TimeSigned32(callback->distance);
					break;
				}
			}
			else if (type == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0)
			{
				for (std::uint32_t index = 0; index < data.goodCount; ++index)
				{
					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != data.landscapeTypes[index]) continue;
					data.landscapePoints[data.landscapeCount] = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
					data.landscapeGoods[data.landscapeCount++] = data.goods[index];
					if (data.firstDistance < 0)
					{
						data.firstDistance = NXBasics::Runtime_TimeSigned32(callback->distance); data.firstWasLandscape = 1;
					}
					break;
				}
			}
			return data.landscapeCount == NC2Runtime::GoodSearchLimits::LandscapeCandidates || data.houseCount == NC2Runtime::GoodSearchLimits::HouseCandidates;
		}

		// Original function: FUN_00460676; both work houses and stocks qualify.
		bool __cdecl slHumanSearchGoodsComplexInWorkHousesAndStocksOnlyFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SHumanSearchGoodsComplexData*>(callback->userData);
			if (data.firstDistance >= 0 && callback->distance - static_cast<std::uint32_t>(data.firstDistance) > NC2Runtime::GoodSearchLimits::DistanceSlackUnsigned) return true;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);
			if (type == SwitchIds::MapMicroObjectType::House && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			{
				auto* house = House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId));
				const auto mainType = House_GetHouseMainType(house);
				if ((mainType != 3 && mainType != 1) || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != data.player || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0) return false;
				for (std::uint32_t index = 0; index < data.goodCount; ++index)
				{
					if (!House_Stock_CanStoreGoodType(house, data.goods[index], true) || House_Stock_RequestAmountOfGoods(house, data.goods[index], true) == 0) continue;
					data.houses[data.houseCount] = house;
					data.houseGoods[data.houseCount++] = data.goods[index];
					if (data.firstDistance < 0) data.firstDistance = NXBasics::Runtime_TimeSigned32(callback->distance);
					break;
				}
			}
			else if (type == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0)
			{
				for (std::uint32_t index = 0; index < data.goodCount; ++index)
				{
					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != data.landscapeTypes[index]) continue;
					data.landscapePoints[data.landscapeCount] = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
					data.landscapeGoods[data.landscapeCount++] = data.goods[index];
					if (data.firstDistance < 0)
					{
						data.firstDistance = NXBasics::Runtime_TimeSigned32(callback->distance); data.firstWasLandscape = 1;
					}
					break;
				}
			}
			return data.landscapeCount == NC2Runtime::GoodSearchLimits::LandscapeCandidates || data.houseCount == NC2Runtime::GoodSearchLimits::HouseCandidates;
		}

		// Original function: FUN_0041bc95; the first matching stock or landscape stops the search.
		bool __cdecl slGuideSearchGoodsComplexIgnoringHomesFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SHumanSearchGoodsComplexData*>(callback->userData);
			if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapY), Read<std::int16_t>(&data.guidePosition, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(&data.guidePosition, NC2Layout::MapMigPointOffset::MapY))) >= NC2Runtime::GoodSearchLimits::SearchRadiusUnsigned) return false;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::House && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			{
				auto* house = House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId));
				if (House_GetHouseMainType(house) == 2 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != data.player || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0) return false;
				for (std::uint32_t i = 0; i < data.goodCount; ++i)
				{
					if (!House_Stock_CanStoreGoodType(house, data.goods[i], true) || House_Stock_RequestAmountOfGoods(house, data.goods[i], true) == 0) continue;
					data.houses[data.houseCount] = house;
					data.houseGoods[data.houseCount++] = data.goods[i];
					return true;
				}
			}
			else if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0)
			{
				for (std::uint32_t i = 0; i < data.goodCount; ++i)
				{
					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != data.landscapeTypes[i]) continue;
					data.landscapePoints[data.landscapeCount] = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
					data.landscapeGoods[data.landscapeCount++] = data.goods[i];
					return true;
				}
			}
			return false;
		}
	}

	// Original function: FUN_00460ee8; unused array entries remain uninitialized.
	SHumanSearchGoodsComplexData::SHumanSearchGoodsComplexData(std::uint32_t playerId) : goodCount(0), player(playerId), firstDistance(-1), firstWasLandscape(0), landscapeCount(0), landscapePoints{ }, houseCount(0), guideCount(0), guidePosition(0)
	{}

	// Original function: FUN_0041bf40; distinct from the multi-good guide search.
	std::int32_t GuideManager_l_GetGuideIdWithGood(void* manager, std::int32_t guideId, std::uint32_t good)
	{
		auto* first = Guide(guideId);
		const auto goodOffset = NC2Layout::GuideOffset::GoodsCountsUnsigned + good * NC2Layout::RecordSize::GuideGoodCountUnsigned;
		if (Read<std::int32_t>(first, goodOffset) > 0) return guideId;
		auto* visited = Read<std::uint8_t*>(manager, NC2Layout::GuideManagerOffset::VisitedGuides);
		std::memset(visited, 0, static_cast<std::uint32_t>(CGuideArray::sTheObjectPtr->mCapacity));
		for (std::uint32_t bucket = 0; bucket < NC2Runtime::GuideRoutingLimits::DistanceBuckets; ++bucket) Write<std::uint32_t>(manager, NC2Layout::GuideManagerOffset::DistanceBucketsUnsigned + bucket * NC2Layout::RecordSize::GuideDistanceBucketUnsigned, 0);
		Write(manager, NC2Layout::GuideManagerOffset::FirstBucketGuides, first);
		Write<std::uint32_t>(manager, NC2Layout::GuideManagerOffset::DistanceBuckets, 1);
		visited[Read<std::uint32_t>(first, NC2Layout::GuideOffset::GuideId)] = 1;
		std::uint32_t pending = 1, bucket = 0;
		for (;;)
		{
			const auto offset = NC2Layout::GuideManagerOffset::DistanceBucketsUnsigned + bucket * NC2Layout::RecordSize::GuideDistanceBucketUnsigned;
			for (std::int32_t slot = 0; slot < Read<std::int32_t>(manager, offset); ++slot)
			{
				const auto* guide = Read<void*>(manager, offset + NC2Layout::GuideDistanceBucketOffset::GuidesUnsigned + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::GuidePointerUnsigned);
				--pending;
				for (std::int32_t link = 0; link < Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount); ++link)
				{
					const auto linkOffset = NC2Layout::GuideOffset::ConnectionsUnsigned + static_cast<std::uint32_t>(link) * NC2Layout::RecordSize::GuideConnectionUnsigned;
					const auto id = Read<std::int32_t>(guide, linkOffset + NC2Layout::GuideConnectionOffset::GuideId);
					if (visited[id] == 1) continue;
					auto* candidate = Guide(id);
					if (Read<std::int32_t>(candidate, goodOffset) > 0) return Read<std::int32_t>(candidate, NC2Layout::GuideOffset::GuideId);
					const auto targetBucket = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(guide, linkOffset) + bucket) % NC2Runtime::GuideRoutingLimits::DistanceBuckets;
					const auto targetOffset = NC2Layout::GuideManagerOffset::DistanceBucketsUnsigned + static_cast<std::uint32_t>(targetBucket) * NC2Layout::RecordSize::GuideDistanceBucketUnsigned;
					const auto count = Read<std::uint32_t>(manager, targetOffset);
					Write(manager, targetOffset + NC2Layout::GuideDistanceBucketOffset::GuidesUnsigned + count * NC2Layout::RecordSize::GuidePointerUnsigned, candidate);
					Write(manager, targetOffset, count + 1u);
					++pending;
					visited[id] = 1;
				}
			}
			if (pending == 0) return -1;
			Write<std::uint32_t>(manager, offset, 0);
			if (++bucket == NC2Runtime::GuideRoutingLimits::DistanceBuckets) bucket = 0;
		}
	}

	// Original function: FUN_0041bb51.
	bool GuideManager_IsGoodInLinkSystem(void* manager, std::int32_t guideId, std::uint32_t good)
	{
		return GuideManager_l_GetGuideIdWithGood(manager, guideId, good) != -1;
	}

	// Original function: FUN_0041bb85; a failed owned search invalidates the cached good count.
	bool GuideManager_GetMapPositionWithGoods(void* manager, std::uint32_t radius, std::int32_t guideId, SGoodSearch& data, bool anyPlayer)
	{
		const auto found = GuideManager_l_GetGuideIdWithGood(manager, guideId, data.good);
		if (found == -1) return false;
		auto* guide = Guide(found);
		data.point = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::MapPosition);
		if (CMapPathfinderElement::SearchOverMap(static_cast<std::byte*>(guide) + NC2Layout::GuideOffset::MapPosition, radius, true, slGoodSearchAround, &data, true, anyPlayer ? -1 : static_cast<std::int32_t>(data.player))) return true;
		if (!anyPlayer) Write<std::uint32_t>(guide, NC2Layout::GuideOffset::GoodsCountsUnsigned + data.good * NC2Layout::RecordSize::GuideGoodCountUnsigned, 0);
		return false;
	}

	// Original function: FUN_0045f721; availability search does not pick a random candidate.
	bool Human_FindEquipment_Complex_Nearby(const void* human, std::uint32_t good, bool anyPlayer)
	{
		SHumanSearchGoodData data(good, static_cast<std::uint32_t>(gStaticGoodTypeStorage.records[good].landscapeType), Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		CMapPathfinderElement::SearchOverMap(Human_GetMapPositionRef(human), NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchGoodFunction, &data, true, anyPlayer ? -1 : static_cast<std::int32_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
		if (data.landscapeCount != 0 || data.houseCount != 0) return true;
		for (std::uint32_t index = 0; index < data.guideCount; ++index)
		{
			const auto id = static_cast<std::int32_t>(data.guides[index]);
			if (!GuideManager_IsGoodInLinkSystem(gGuideManager, id, good)) continue;
			SGoodSearch search{ Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), good, 0 };
			if (GuideManager_GetMapPositionWithGoods(gGuideManager, NC2Runtime::GoodSearchLimits::SearchRadius, id, search, anyPlayer)) return true;
		}
		return false;
	}

	// Original function: FUN_0045f4ac; map goods precede direct stocks and guide fallback.
	bool Human_FindGood_Complex(const void* human, std::uint32_t good, void* realPosition, void* workPosition, const void* source)
	{
		SHumanSearchGoodData data(good, static_cast<std::uint32_t>(gStaticGoodTypeStorage.records[good].landscapeType), Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		CMapPathfinderElement::SearchOverMap(source, NC2Runtime::GoodSearchLimits::SearchRadius, false, slHumanSearchGoodFunction, &data, true, static_cast<std::int32_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
		if (data.landscapeCount != 0)
		{
			const auto point = data.points[Game_Random_GetNumber() % data.landscapeCount];
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, point);
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, point);
			AdjustPickupWorkPosition(workPosition);
			return true;
		}
		if (data.houseCount != 0)
		{
			const auto* house = data.houses[Game_Random_GetNumber() % data.houseCount];
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition));
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));
			return true;
		}
		for (std::uint32_t index = 0; index < data.guideCount; ++index)
		{
			const auto id = static_cast<std::int32_t>(data.guides[index]);
			if (!GuideManager_IsGoodInLinkSystem(gGuideManager, id, good)) continue;
			SGoodSearch search{ Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), good, 0 };
			if (!GuideManager_GetMapPositionWithGoods(gGuideManager, NC2Runtime::GoodSearchLimits::SearchRadius, id, search, false)) continue;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(&search.point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::House)
			{
				const auto* house = House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId));
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition));
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));
			}
			else
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, search.point);
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, search.point);
				AdjustPickupWorkPosition(workPosition);
			}
			return true;
		}
		return false;
	}

	// Original function: FUN_0041bde1; use the manager's original forty distance buckets.
	std::int32_t GuideManager_l_GetGuideIdWithAnyGood(void* manager, std::int32_t guideId, const SHumanSearchGoodsComplexData& data)
	{
		auto* first = Guide(guideId);
		if (GuideHasAnyGood(first, data)) return guideId;
		// Original outlined function: FUN_0041b90d.
		auto* visited = Read<std::uint8_t*>(manager, NC2Layout::GuideManagerOffset::VisitedGuides);
		std::memset(visited, 0, static_cast<std::uint32_t>(CGuideArray::sTheObjectPtr->mCapacity));
		for (std::uint32_t bucket = 0; bucket < NC2Runtime::GuideRoutingLimits::DistanceBuckets; ++bucket) Write<std::uint32_t>(manager, NC2Layout::GuideManagerOffset::DistanceBucketsUnsigned + bucket * NC2Layout::RecordSize::GuideDistanceBucketUnsigned, 0);
		Write(manager, NC2Layout::GuideManagerOffset::FirstBucketGuides, first);
		Write<std::uint32_t>(manager, NC2Layout::GuideManagerOffset::DistanceBuckets, 1);
		visited[Read<std::uint32_t>(first, NC2Layout::GuideOffset::GuideId)] = 1;
		std::uint32_t pending = 1;
		std::uint32_t bucket = 0;
		for (;;)
		{
			const auto offset = NC2Layout::GuideManagerOffset::DistanceBucketsUnsigned + bucket * NC2Layout::RecordSize::GuideDistanceBucketUnsigned;
			for (std::int32_t slot = 0; slot < Read<std::int32_t>(manager, offset); ++slot)
			{
				const auto* guide = Read<void*>(manager, offset + NC2Layout::GuideDistanceBucketOffset::GuidesUnsigned + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::GuidePointerUnsigned);
				--pending;
				for (std::int32_t link = 0; link < Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount); ++link)
				{
					const auto linkOffset = NC2Layout::GuideOffset::ConnectionsUnsigned + static_cast<std::uint32_t>(link) * NC2Layout::RecordSize::GuideConnectionUnsigned;
					const auto id = Read<std::int32_t>(guide, linkOffset + NC2Layout::GuideConnectionOffset::GuideId);
					if (visited[id] == 1) continue;
					auto* candidate = Guide(id);
					if (GuideHasAnyGood(candidate, data)) return Read<std::int32_t>(candidate, NC2Layout::GuideOffset::GuideId);
					const auto targetBucket = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(guide, linkOffset) + bucket) % NC2Runtime::GuideRoutingLimits::DistanceBuckets;
					const auto targetOffset = NC2Layout::GuideManagerOffset::DistanceBucketsUnsigned + static_cast<std::uint32_t>(targetBucket) * NC2Layout::RecordSize::GuideDistanceBucketUnsigned;
					const auto count = Read<std::uint32_t>(manager, targetOffset);
					Write(manager, targetOffset + NC2Layout::GuideDistanceBucketOffset::GuidesUnsigned + count * NC2Layout::RecordSize::GuidePointerUnsigned, candidate);
					Write(manager, targetOffset, count + 1u);
					++pending;
					visited[id] = 1;
				}
			}
			if (pending == 0) return -1;
			Write<std::uint32_t>(manager, offset, 0);
			if (++bucket == NC2Runtime::GuideRoutingLimits::DistanceBuckets) bucket = 0;
		}
	}

	// Original function: FUN_0041bb6b.
	bool GuideManager_IsAnyGoodInLinkSystem(void* manager, std::int32_t guideId, const SHumanSearchGoodsComplexData& data)
	{
		return GuideManager_l_GetGuideIdWithAnyGood(manager, guideId, data) != -1;
	}

	// Original function: FUN_0041bc03; preserve the position read before the -1 check.
	bool GuideManager_GetMapPositionWithGoodsComplex(void* manager, std::uint32_t radius, std::int32_t guideId, SHumanSearchGoodsComplexData& data)
	{
		const auto found = GuideManager_l_GetGuideIdWithAnyGood(manager, guideId, data);
		auto* guide = Guide(found);
		data.guidePosition = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::MapPosition);
		if (found == -1) return false;
		CMapPathfinderElement::SearchOverMap(static_cast<std::byte*>(guide) + NC2Layout::GuideOffset::MapPosition, radius, true, slGuideSearchGoodsComplexIgnoringHomesFunction, &data, true, -1);
		if (data.landscapeCount != 0 || data.houseCount != 0) return true;
		for (std::uint32_t i = 0; i < data.goodCount; ++i) Write<std::uint32_t>(guide, NC2Layout::GuideOffset::GoodsCountsUnsigned + data.goods[i] * NC2Layout::RecordSize::GuideGoodCountUnsigned, 0);
		return false;
	}

	// Original function: FUN_0045f452.
	bool Human_FindFoodInHomeHouse(const void* human, void* realPosition, void* workPosition)
	{
		if (!Human_IsAttachedToHomeHouse(human)) return false;
		const auto* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));
		if (House_Stock_GetAmountOfGoods(house, 16, false) == 0 && House_Stock_GetAmountOfGoods(house, 17, false) == 0) return false;
		Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition));
		Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));
		return true;
	}

	// Original function: FUN_0045f7fd; Windows food preference and radius 004F4308.
	std::uint32_t Human_FindFood_Complex(const void* human, void* realPosition, void* workPosition, const void* source)
	{
		SHumanSearchGoodsComplexData data(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		const std::uint32_t goods[] = { 16, 17, 18, 19, 22, 21, 20 };
		const std::uint32_t landscapes[] = { 39, 40, 11, 42, 45, 44, 43 };
		for (std::uint32_t i = 0; i < 7; ++i)
		{
			data.goods[i] = goods[i]; data.landscapeTypes[i] = landscapes[i]; ++data.goodCount;
		}
		CMapPathfinderElement::SearchOverMap(source, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchGoodsComplexIgnoringHomesFunction, &data, true, static_cast<std::int32_t>(data.player));
		bool landscape = data.firstWasLandscape != 0;
		if (data.firstDistance < 0)
		{
			bool found = false;
			for (std::uint32_t i = 0; i < data.guideCount; ++i)
			{
				if (!GuideManager_IsAnyGoodInLinkSystem(gGuideManager, static_cast<std::int32_t>(data.guides[i]), data) || !GuideManager_GetMapPositionWithGoodsComplex(gGuideManager, NC2Runtime::GoodSearchLimits::SearchRadius, static_cast<std::int32_t>(data.guides[i]), data)) continue;
				if (data.landscapeCount != 0)
				{
					found = true; landscape = true; break;
				}
				if (data.houseCount != 0)
				{
					found = true; landscape = false; break;
				}
			}
			if (!found) return 0;
		}
		if (landscape)
		{
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
			AdjustPickupWorkPosition(workPosition);
			return data.landscapeGoods[0];
		}
		Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::MapPosition));
		Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::DoorEntryPoint));
		return data.houseGoods[0];
	}

	// Original function: FUN_0045fbf6; home demand selects the exact Windows food/enhancer groups.
	bool Human_FindGoodsDemandedForHomeHouse(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source)
	{
		SHumanSearchGoodsComplexData data(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		auto* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));
		auto append = [&data] (std::uint32_t wanted, std::uint32_t landscape)
		{
			data.goods[data.goodCount] = wanted; data.landscapeTypes[data.goodCount++] = landscape;
		};
		if (House_Stock_HasDemandForGood(house, 16))
		{
			append(16, 39); append(19, 42); append(22, 45); append(21, 44);
		}
		if (House_Stock_HasDemandForGood(house, 17))
		{
			append(17, 40); append(20, 43);
		}
		if (House_Stock_HasDemandForGood(house, 28)) append(28, 54);
		if (House_Stock_HasDemandForGood(house, 29)) append(29, 55);
		if (House_Stock_HasDemandForGood(house, 15)) append(15, 38);
		if (data.goodCount == 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3); Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 2); return false;
		}
		CMapPathfinderElement::SearchOverMap(source, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchGoodsComplexIgnoringHomesFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (data.landscapeCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.landscapeCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			AdjustPickupWorkPosition(workPosition);
			*good = data.landscapeGoods[index];
			return true;
		}
		if (data.houseCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.houseCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::MapPosition));
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::DoorEntryPoint));
			*good = data.houseGoods[index];
			return true;
		}
		for (std::uint32_t index = 0; index < data.guideCount; ++index)
		{
			const auto id = static_cast<std::int32_t>(data.guides[index]);
			if (!GuideManager_IsAnyGoodInLinkSystem(gGuideManager, id, data) || !GuideManager_GetMapPositionWithGoodsComplex(gGuideManager, NC2Runtime::GoodSearchLimits::SearchRadius, id, data)) continue;
			if (data.landscapeCount != 0)
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
				AdjustPickupWorkPosition(workPosition);
				*good = data.landscapeGoods[0];
				return true;
			}
			if (data.houseCount != 0)
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::MapPosition));
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::DoorEntryPoint));
				*good = data.houseGoods[0];
				return true;
			}
		}
		Write(human, NC2Layout::HumanOffset::RequestedGood, data.goods[0]);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 5);
		return false;
	}

	// Original function: FUN_0045ff34; preserve build-material order and distinct failure side effects.
	bool Human_FindGoodsDemandedForBuildHouse(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source)
	{
		SHumanSearchGoodsComplexData data(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		const void* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));
		for (std::int32_t slot = 0; slot < Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsCount); ++slot)
		{
			const auto offset = NC2Layout::HouseOffset::ConstructionGoodEntries + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HouseConstructionGood;
			if (Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::RequiredAmount) <= Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::DeliveredAmount) + Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::IncomingAmount)) continue;
			const auto wanted = Read<std::uint32_t>(house, offset);
			data.goods[data.goodCount] = wanted;
			data.landscapeTypes[data.goodCount] = static_cast<std::uint32_t>(gStaticGoodTypeStorage.records[wanted].landscapeType);
			++data.goodCount;
		}
		if (data.goodCount == 0) return false;
		CMapPathfinderElement::SearchOverMap(source, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchGoodsComplexIgnoringHomesFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (data.landscapeCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.landscapeCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			AdjustPickupWorkPosition(workPosition);
			*good = data.landscapeGoods[index];
			return true;
		}
		if (data.houseCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.houseCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::MapPosition));
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::DoorEntryPoint));
			*good = data.houseGoods[index];
			return true;
		}
		for (std::uint32_t index = 0; index < data.guideCount; ++index)
		{
			const auto id = static_cast<std::int32_t>(data.guides[index]);
			if (!GuideManager_IsAnyGoodInLinkSystem(gGuideManager, id, data) || !GuideManager_GetMapPositionWithGoodsComplex(gGuideManager, NC2Runtime::GoodSearchLimits::SearchRadius, id, data)) continue;
			if (data.landscapeCount != 0)
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
				AdjustPickupWorkPosition(workPosition);
				*good = data.landscapeGoods[0];
				return true;
			}
			if (data.houseCount != 0)
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::MapPosition));
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::DoorEntryPoint));
				*good = data.houseGoods[0];
				return true;
			}
		}
		Write(human, NC2Layout::HumanOffset::RequestedGood, data.goods[0]);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 4);
		return false;
	}

	// Original function: FUN_00460827; the attached workshop is checked before landscapes and other houses.
	bool Human_FindGoodsDemandedForBuildVehicle(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source)
	{
		SHumanSearchGoodsComplexData data(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		const void* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId));
		for (std::int32_t slot = 0; slot < Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsCount); ++slot)
		{
			const auto offset = NC2Layout::HouseOffset::ConstructionGoodEntries + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HouseConstructionGood;
			if (Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::RequiredAmount) <= Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::DeliveredAmount) + Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::IncomingAmount)) continue;
			const auto wanted = Read<std::uint32_t>(house, offset);
			data.goods[data.goodCount] = wanted;
			data.landscapeTypes[data.goodCount] = static_cast<std::uint32_t>(gStaticGoodTypeStorage.records[wanted].landscapeType);
			++data.goodCount;
		}
		if (data.goodCount == 0) return false;
		if (Human_IsAttachedToWorkHouse(human))
		{
			void* workshop = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));
			for (std::uint32_t index = 0; index < data.goodCount; ++index)
			{
				const auto wanted = data.goods[index];
				if (!House_Stock_CanStoreGoodType(workshop, wanted, false) || House_Stock_RequestAmountOfGoods(workshop, wanted, false) == 0) continue;
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(workshop, NC2Layout::HouseOffset::MapPosition));
				House_Door_GetEntryPoint(workshop, workPosition);
				*good = wanted;
				return true;
			}
		}
		CMapPathfinderElement::SearchOverMap(source, NC2Runtime::GoodSearchLimits::SearchRadius, false, slHumanSearchGoodsComplexIgnoringHomesFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (data.landscapeCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.landscapeCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			AdjustPickupWorkPosition(workPosition);
			*good = data.landscapeGoods[index];
			return true;
		}
		if (data.houseCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.houseCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::MapPosition));
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::DoorEntryPoint));
			*good = data.houseGoods[index];
			return true;
		}
		for (std::uint32_t index = 0; index < data.guideCount; ++index)
		{
			const auto id = static_cast<std::int32_t>(data.guides[index]);
			if (!GuideManager_IsAnyGoodInLinkSystem(gGuideManager, id, data) || !GuideManager_GetMapPositionWithGoodsComplex(gGuideManager, NC2Runtime::GoodSearchLimits::SearchRadius, id, data)) continue;
			if (data.landscapeCount != 0)
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
				AdjustPickupWorkPosition(workPosition);
				*good = data.landscapeGoods[0];
				return true;
			}
			if (data.houseCount != 0)
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::MapPosition));
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::DoorEntryPoint));
				*good = data.houseGoods[0];
				return true;
			}
		}
		Write(human, NC2Layout::HumanOffset::RequestedGood, data.goods[0]);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 4);
		return false;
	}

	// Original function: FUN_0046011d; vehicle demand uses future stock.
	bool Human_FindGoodsDemandedForVehicle(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source)
	{
		SHumanSearchGoodsComplexData data(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		const auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
		if (Vehicle_Stock_IsFull(vehicle)) return false;
		for (std::uint32_t wanted = 0; wanted < 66; ++wanted)
		{
			if (!Vehicle_Stock_CanStoreGoodType(vehicle, wanted)) continue;
			const auto amount = Vehicle_Stock_GetAmountOfWantedGoods(vehicle, wanted);
			if (amount <= Vehicle_Stock_GetFutureAmount(vehicle, wanted)) continue;
			data.goods[data.goodCount] = wanted;
			data.landscapeTypes[data.goodCount++] = static_cast<std::uint32_t>(gStaticGoodTypeStorage.records[wanted].landscapeType);
		}
		if (data.goodCount == 0) return false;
		CMapPathfinderElement::SearchOverMap(source, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchGoodsComplexIgnoringHomesFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (data.landscapeCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.landscapeCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			AdjustPickupWorkPosition(workPosition);
			*good = data.landscapeGoods[index];
			return true;
		}
		if (data.houseCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.houseCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::MapPosition));
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::DoorEntryPoint));
			*good = data.houseGoods[index];
			return true;
		}
		for (std::uint32_t index = 0; index < data.guideCount; ++index)
		{
			const auto guide = static_cast<std::int32_t>(data.guides[index]);
			if (!GuideManager_IsAnyGoodInLinkSystem(gGuideManager, guide, data) || !GuideManager_GetMapPositionWithGoodsComplex(gGuideManager, NC2Runtime::GoodSearchLimits::SearchRadius, guide, data)) continue;
			if (data.landscapeCount != 0)
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[0]);
				AdjustPickupWorkPosition(workPosition);
				*good = data.landscapeGoods[0];
				return true;
			}
			if (data.houseCount != 0)
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::MapPosition));
				Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[0], NC2Layout::HouseOffset::DoorEntryPoint));
				*good = data.houseGoods[0];
				return true;
			}
		}
		Write(human, NC2Layout::HumanOffset::RequestedGood, data.goods[0]);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 3);
		return false;
	}

	// Original function: FUN_00460327; completed houses search from the work center.
	bool Human_FindGoodsDemandedForStockHouse(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source)
	{
		auto* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));
		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3) return Human_FindGoodsDemandedForBuildHouse(human, realPosition, workPosition, good, source);
		SHumanSearchGoodsComplexData data(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		for (std::uint32_t wanted = 1; wanted <= 55; ++wanted)
		{
			if (!House_Stock_CanStoreGoodType(house, wanted, false) || !House_Stock_HasDemandForGood(house, wanted)) continue;
			data.goods[data.goodCount] = wanted;
			data.landscapeTypes[data.goodCount++] = static_cast<std::uint32_t>(gStaticGoodTypeStorage.records[wanted].landscapeType);
			if (data.goodCount == 66) break;
		}
		if (data.goodCount == 0) return false;
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		const auto callback = House_GetHouseMainType(house) == 1 ? slHumanSearchGoodsComplexInWorkHousesOnlyFunction : slHumanSearchGoodsComplexInWorkHousesAndStocksOnlyFunction;
		CMapPathfinderElement::SearchOverMap(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter, job == 18 || job == 29 ? 10u : 20u, true, callback, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (data.landscapeCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.landscapeCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, data.landscapePoints[index]);
			AdjustPickupWorkPosition(workPosition);
			*good = data.landscapeGoods[index];
			return true;
		}
		if (data.houseCount != 0)
		{
			const auto index = Game_Random_GetNumber() % data.houseCount;
			Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::MapPosition));
			Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(data.houses[index], NC2Layout::HouseOffset::DoorEntryPoint));
			*good = data.houseGoods[index];
			return true;
		}
		return false;
	}

	// Original function: FUN_0042d73e; deleted houses are excluded.
	void* Human_GetHousePtrAtTargetPosition(const void* human)
	{
		const auto id = Logic_EntryPoint_House_GetOnPosition(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition);
		if (id == -1) return nullptr;
		auto* house = House(static_cast<std::uint32_t>(id));
		return Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0 ? house : nullptr;
	}

	// Original function: FUN_0042d768; validate only the explicit house target.
	void* Human_GetTargetHousePtr(const void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) != 5) return nullptr;
		auto* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectId));
		return Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectRevision) ? house : nullptr;
	}

	// Original function: FUN_0045ca9a; destination reservation follows walk startup.
	bool Human_l_StartAtomicWalkAndFetch(void* human, const void* realPosition, const void* workPosition, bool forSelf)
	{
		if (!Human_l_StartAtomicWalk(human, realPosition, workPosition)) return false;
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		if (forSelf) Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, SwitchIds::GoodCirculationMode::FetchForSelf);
		else if (job == 5 && Human_IsAttachedToHomeHouse(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, SwitchIds::GoodCirculationMode::FetchForHome);
			House_Stock_ModifyFutureAmount(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, true);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, SwitchIds::GoodCirculationMode::FillStockFetch);
			if (Human_IsAttachedToWorkHouse(human)) House_Stock_ModifyFutureAmount(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, true);
		}
		if (auto* house = Human_GetTargetHousePtr(human))
		{
			House_Stock_ModifyFutureAmount(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), -1, false); return true;
		}
		const auto landscape = Human_GetLandscapeTypeAtTargetPosition(human);
		Write(human, NC2Layout::HumanOffset::TargetLandscapeType, landscape);
		if (landscape != 0 && landscape != 1)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 2); return true;
		}
		if (job == 18 || job == 29 || job == 22 || job == 23) return true;
		Human_Target_Reset(human);
		Human_StopCurrentGoodCirculationMode(human);
		return false;
	}

	// Original function: FUN_0045cb93; reserve only explicit house targets.
	bool Human_l_StartAtomicWalkAndBring(void* human, const void* realPosition, const void* workPosition)
	{
		if (!Human_l_StartAtomicWalk(human, realPosition, workPosition)) return false;
		auto* house = Human_GetTargetHousePtr(human);
		if (house == nullptr) return true;
		const auto primary = Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 5 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, primary ? SwitchIds::GoodCirculationMode::FillStockBringAway : SwitchIds::GoodCirculationMode::FlushStock);
		if (House_Stock_ModifyFutureAmount(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), 1, primary)) return true;
		Human_Target_Reset(human);
		return false;
	}

	// Original function: FUN_0044515f.
	bool Human_l_StartTask_Tool_PickupWantedGood_FromTargetPosition(void* human)
	{
		auto* house = Human_GetHousePtrAtTargetPosition(human);
		std::uint32_t action = 22;
		if (house != nullptr)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetWorkPosition) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint)) C2Debug_WarningBox("standing at invalid position for interaction with house stock!");
			const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);
			auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood);
			if (type == 10 || type == 11)
			{
				if (good != (type == 10 ? 1u : 12u) || House_Stock_RequestAmountOfGoods(house, good, true) == 0) return false;
				action = type == 10 ? 44u : 45u;
			}
			else if (!House_Stock_CanStoreGoodType(house, good, true) || House_Stock_RequestAmountOfGoods(house, good, true) == 0)
			{
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 7)
				{
					const auto alternative = good == 16 ? 17u : good == 17 ? 16u : 0u;
					if (alternative == 0 || !House_Stock_CanStoreGoodType(house, alternative, true) || House_Stock_RequestAmountOfGoods(house, alternative, true) == 0) return false;
					Write(human, NC2Layout::HumanOffset::RequestedGood, alternative);
				}
				else
				{
					const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
					if ((job != 9 && job != 10) || Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId) == NXBasics::IntegerRange::UInt32Maximum || House_Stock_RequestAmountOfGoods(house, good, false) < 1u) return false;
				}
			}
		}
		else if (Human_GetLandscapeTypeAtTargetPosition(human) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetLandscapeType)) return false;
		Human_l_StartAtomicAnimation(human, action, 6);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		return true;
	}

	// Original function: FUN_0045f3d2.
	bool Human_FindConsumerNearWorkCenter(const void* human, std::uint32_t good)
	{
		SConsumerSearch data{ good, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 0,{ } };
		const auto* center = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
		CMapPathfinderElement::SearchOverMap(center, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchConsumerFunction, &data, true, static_cast<std::int32_t>(data.player));
		if (data.count != 0) return true;
		SConsumerSearch stock{ good, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 0,{ } };
		CMapPathfinderElement::SearchOverMap(center, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchFreeStockFunction, &stock, true, static_cast<std::int32_t>(stock.player));
		return stock.count != 0;
	}

	// Original function: FUN_0045f17d; preserve the native true return for a zero-demand consumer.
	bool Human_FindConsumerNearWorkCenter(const void* human, std::uint32_t good, void* realPosition, void* workPosition)
	{
		SConsumerSearch data{ good, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 0,{ } };
		const auto* center = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
		CMapPathfinderElement::SearchOverMap(center, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchConsumerFunction, &data, true, static_cast<std::int32_t>(data.player));
		if (data.count != 0)
		{
			auto* selected = data.houses[0];
			auto demand = House_Stock_GetDemandForGood(selected, good);
			for (std::uint32_t index = 1; index < data.count; ++index)
			{
				const auto next = House_Stock_GetDemandForGood(data.houses[index], good);
				if (next > demand)
				{
					demand = next; selected = data.houses[index];
				}
			}
			if (demand != 0)
			{
				Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(selected, NC2Layout::HouseOffset::MapPosition)); Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(selected, NC2Layout::HouseOffset::DoorEntryPoint));
			}
			return true;
		}
		SConsumerSearch stock{ good, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 0,{ } };
		CMapPathfinderElement::SearchOverMap(center, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchFreeStockFunction, &stock, true, static_cast<std::int32_t>(stock.player));
		if (stock.count == 0) return false;
		auto* selected = stock.houses[0];
		auto demand = House_Stock_GetDemandForGood(selected, good);
		for (std::uint32_t index = 1; index < stock.count; ++index)
		{
			const auto next = House_Stock_GetDemandForGood(stock.houses[index], good);
			if (next > demand)
			{
				demand = next; selected = stock.houses[index];
			}
		}
		Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(selected, NC2Layout::HouseOffset::MapPosition));
		Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, Read<std::uint32_t>(selected, NC2Layout::HouseOffset::DoorEntryPoint));
		return true;
	}

	// Original function: FUN_00462176; retain partial placement on failure.
	bool Map_GoodTool_PutOnMap(std::uint32_t good, std::uint32_t amount, std::uint32_t point, std::uint32_t radius, bool preserveEmptyLandscape)
	{
		while (amount != 0 && Map_GoodTool_PutOnMap(good, &point, preserveEmptyLandscape)) --amount;
		if (amount == 0) return true;
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId);
		for (std::uint32_t ring = 1; ring < radius; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(point, 4);
			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::uint32_t step = 0; step < ring; ++step)
				{
					MapMigPoint_MoveCursorUnwrapped(point, direction);
					if (!MapMigPoint_IsInsideMap(&point) || MapMigPoint_IsBorderPoint(&point) || Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId) != continent) continue;
					while (amount != 0 && Map_GoodTool_PutOnMap(good, &point, preserveEmptyLandscape)) --amount;
					if (amount == 0) return true;
				}
			}
		}
		return false;
	}

	// Original function: FUN_00461f6f.
	bool Map_GoodTool_PutOnMap(std::uint32_t good, const void* point, bool preserveEmptyLandscape)
	{
		using namespace NC2LogicExternals;
		const auto& data = gStaticGoodTypeStorage.records[good];
		if (data.engineLandscapeId == -1) return false;
		if (External_Landscape_GetEngineIdOfMapLandscape(point) == data.engineLandscapeId) return GameMap_MapMicroPoint_Landscape_ApplyTransition(point, 4) != -1;
		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
		if (micro == nullptr || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0) return false;
		const auto kind = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);
		const auto landscape = kind == SwitchIds::MapMicroObjectType::Landscape ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) : 0;
		if ((kind == SwitchIds::MapMicroObjectType::Landscape && landscape != 0 && landscape != 1) || kind == SwitchIds::MapMicroObjectType::House || kind == SwitchIds::MapMicroObjectType::Guide) return false;
		if (preserveEmptyLandscape && kind == SwitchIds::MapMicroObjectType::Landscape && landscape != 0)
		{
			const auto engine = External_Landscape_GetEngineIdOfMapLandscape(point);
			const auto valency = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency);
			const auto* walk = External_Landscape_GetAreaPtr(engine, valency, 0);
			const auto* build = External_Landscape_GetAreaPtr(engine, valency, 1);
			const auto* extra = External_Landscape_GetAreaPtr(engine, valency, 3);
			if (walk != nullptr || build != nullptr || extra != nullptr) return false;
		}
		GameMapModifyManager_StartGameMapModify();
		const auto result = GameMap_MapMicroPoint_Landscape_SetType(point, static_cast<std::uint32_t>(data.landscapeType), 1, data.engineLandscapeId, true, -1);
		GameMapModifyManager_StopGameMapModify();
		return result;
	}

	// Original function: FUN_004620a9; equal valency is rejected, rather than a greater-than check.
	bool Map_GoodTool_CanBePutOnMap(std::uint32_t good, const void* point)
	{
		using namespace NC2LogicExternals;
		if (!MapMigPoint_IsInsideMap(point) || MapMigPoint_IsBorderPoint(point)) return false;
		if (GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId)) != 1) return false;
		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
		if (micro == nullptr || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0) return false;
		const auto kind = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);
		if (kind == SwitchIds::MapMicroObjectType::House || kind == SwitchIds::MapMicroObjectType::Guide) return false;
		const auto& data = gStaticGoodTypeStorage.records[good];
		if (data.engineLandscapeId == -1) return false;
		const auto engine = External_Landscape_GetEngineIdOfMapLandscape(point);
		if (!GameMap_Misc_IsLandscapeAllowedOnPattern(point, static_cast<std::uint32_t>(data.landscapeType))) return false;
		if (engine == data.engineLandscapeId) return (kind == SwitchIds::MapMicroObjectType::Landscape ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0u) != External_Landscape_GetMaximumValency(engine);
		return kind != SwitchIds::MapMicroObjectType::Landscape || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 1;
	}

	// Original function: FUN_0045d76e; even a single candidate consumes a random number.
	bool Human_FindPilePointNearby(const void* human, void* realPosition, void* workPosition)
	{
		SPilePointSearch data{ Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), 0,{ } };
		auto source = Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapMigPointOffset::Coordinates);
		AdjustPileSearchPosition(&source);
		CMapPathfinderElement::SearchOverMap(&source, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchPilePointFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (data.count == 0) return false;
		const auto point = data.points[Game_Random_GetNumber() % data.count];
		Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, point);
		Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, point);
		AdjustPickupWorkPosition(workPosition);
		return true;
	}

	// Original function: FUN_0045d9f0; equipment searches use the requested good, not current cargo.
	bool Human_FindPilePointNearby(const void* human, std::uint32_t good, void* realPosition, void* workPosition)
	{
		SPilePointSearch data{ good, 0,{ } };
		auto source = Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapMigPointOffset::Coordinates);
		AdjustPileSearchPosition(&source);
		CMapPathfinderElement::SearchOverMap(&source, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchPilePointFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (data.count == 0) return false;
		const auto point = data.points[Game_Random_GetNumber() % data.count];
		Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, point);
		Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, point);
		AdjustPickupWorkPosition(workPosition);
		return true;
	}

	// Original function: FUN_0045cc21; a house drop reserves future output stock.
	bool Human_l_StartAtomicWalkAndDrop(void* human, const void* realPosition, const void* workPosition, std::uint32_t good)
	{
		if (!Human_l_StartAtomicWalk(human, realPosition, workPosition)) return false;
		auto* house = Human_GetTargetHousePtr(human);
		if (house != nullptr && !House_Stock_ModifyFutureAmount(house, good, 1, false))
		{
			Human_Target_Reset(human); return false;
		}
		return true;
	}

	// Original function: FUN_0045d8a5; explicit goods and source use radius 40.
	bool Human_FindPilePointNearPosition(const void* human, std::uint32_t good, const void* source, void* realPosition, void* workPosition)
	{
		SPilePointSearch data{ good, 0,{ } };
		auto point = Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates);
		AdjustPileSearchPosition(&point);
		CMapPathfinderElement::SearchOverMap(&point, NC2Runtime::GoodSearchLimits::SearchRadius, true, slHumanSearchPilePointFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (data.count == 0) return false;
		const auto selected = data.points[Game_Random_GetNumber() % data.count];
		Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, selected);
		Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, selected);
		AdjustPickupWorkPosition(workPosition);
		return true;
	}

	// Original function: FUN_0045d7fd; quarry jobs use the smaller search radius.
	bool Human_FindPilePointNearWorkCenter(const void* human, void* realPosition, void* workPosition)
	{
		SPilePointSearch data{ Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), 0,{ } };
		auto source = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter);
		AdjustPileSearchPosition(&source);
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		CMapPathfinderElement::SearchOverMap(&source, job == 18 || job == 29 ? NC2Runtime::GoodSearchLimits::QuarryPileRadius : NC2Runtime::GoodSearchLimits::WorkCenterPileRadius, true, slHumanSearchPilePointFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (data.count == 0) return false;
		const auto point = data.points[Game_Random_GetNumber() % data.count];
		Write(realPosition, NC2Layout::MapMigPointOffset::Coordinates, point);
		Write(workPosition, NC2Layout::MapMigPointOffset::Coordinates, point);
		AdjustPickupWorkPosition(workPosition);
		return true;
	}

	// Original function: FUN_00444ee8.
	void Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(void* human)
	{
		Human_Target_ResetSecondaryTarget(human);
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != SwitchIds::GoodCirculationMode::None)
		{
			Human_StopCurrentGoodCirculationMode(human); Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		}
		auto startPile = [human] (std::int32_t direction)
		{
			Human_l_StartAtomicAnimation(human, 23, direction); Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		};
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);
		if (Human_IsAtTargetPoint(human) && Map_GoodTool_CanBePutOnMap(good, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition))
		{
			startPile(6); return;
		}
		auto* vehicle = Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 ? CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize : nullptr;
		auto reserveVehicle = [human, good, vehicle] ()
		{
			if (!Vehicle_Stock_ModifyFutureAmount(vehicle, good, 1, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId))) return false;
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 6);
			Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId));
			Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision));
			return true;
		};
		if (Human_Misc_IsHidden(human))
		{
			if (vehicle != nullptr)
			{
				if (Vehicle_Stock_CanStoreGoodType(vehicle, good) && reserveVehicle())
				{
					startPile(6); return;
				}
			}
			else
			{
				auto* house = Human_GetHousePtrAtMapPosition(human, Human_GetMapPositionRef(human));
				if (house != nullptr)
				{
					if ((House_Stock_CanStoreGoodType(house, good, true) || House_Stock_CanStoreGoodType(house, good, false)) && House_Stock_HasDemandForGood(house, good))
					{
						Human_Target_SetTargetPositions(human, static_cast<std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, Human_GetMapPositionRef(human));
						startPile(6);
						return;
					}
					if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DefenceModeFlag) != 0 && Human_IsDefendingHouse(human, Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId)))
					{
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood, 0);
						CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 0);
						return;
					}
				}
			}
		}
		std::uint32_t real = 0, work = 0;
		if (Human_FindPilePointNearby(human, &real, &work))
		{
			Human_Target_SetTargetPositions(human, &real, &work);
			if (Human_IsAtTargetPoint(human))
			{
				startPile(6); return;
			}
			if (Human_l_StartAtomicWalk(human, &real, &work))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 23); return;
			}
		}
		if (vehicle != nullptr && Vehicle_Stock_CanStoreGoodType(vehicle, good) && !Vehicle_Stock_IsFullSoon(vehicle) && reserveVehicle())
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != SwitchIds::GoodCirculationMode::FlushStock) Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, SwitchIds::GoodCirculationMode::FillStockBringAway);
			startPile(-1);
			return;
		}
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 7);
	}

	// Original function: FUN_00444e9d; the copy deliberately follows drop startup.
	void Human_l_StartTask_Tool_DropCarriedGoodNearby_Remember(void* human)
	{
		Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
		std::memcpy(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::SecondaryInteractionTarget, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::InteractionTarget, NC2Layout::RecordSize::InteractionTargetData);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget, 2);
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);
		Write(human, NC2Layout::HumanOffset::SecondaryTargetLandscapeType, gStaticGoodTypeStorage.records[good].landscapeType);
		Write(human, NC2Layout::HumanOffset::SecondaryTargetGoodType, good);
	}

	// Original function: FUN_00468b91.
	void Human_l_PerformTaskEvent_Pickup(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == SwitchIds::GoodCirculationMode::None) Human_Target_Reset(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 3);
		if (!Human_Target_IsValid(human))
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) == 1) Human_DoAddNewUserCommand(human, 7, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
			return;
		}
		auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood);
		bool taken = false;
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
			if (Vehicle_Stock_CanStoreGoodType(vehicle, good) && Vehicle_Stock_GetAmountOfGoods(vehicle, good) > 0)
			{
				Vehicle_Stock_ModifyAmount(vehicle, good, -1, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
				taken = true;
			}
		}
		else
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == SwitchIds::GoodCirculationMode::FlushStock && !Human_FindConsumerNearWorkCenter(human, good))
			{
				Human_Target_Reset(human); return;
			}
			auto* house = Human_GetHousePtrAtTargetPosition(human);
			if (house != nullptr)
			{
				bool available = House_Stock_RequestAmountOfGoods(house, good, true) >= 1u;
				if (!available)
				{
					const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
					available = (job == 9 || job == 10) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId) != NXBasics::IntegerRange::UInt32Maximum && House_Stock_RequestAmountOfGoods(house, good, false) >= 1u;
				}
				if (available)
				{
					House_Stock_ModifyAmount(house, good, -1, 0, false); taken = true;
				}
			}
			else
			{
				const auto result = GameMap_MapMicroPoint_Landscape_ApplyTransition(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition, 3);
				if (result != -1)
				{
					good = static_cast<std::uint32_t>(result);
					// Windows changes the wanted good after invoking the carried-good callback.
					Write(human, NC2Layout::HumanOffset::CarriedGood, good);
					CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, good);
					if ((Human_IsJobHunter(human) && (good == 21 || good == 9)) || (Human_IsJobFisher(human) && (good == 22 || good == 16))) Write(human, NC2Layout::HumanOffset::RequestedGood, good);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
					taken = true;
				}
			}
		}
		if (taken)
		{
			// Original outlined operation: FUN_0042db6c.
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) != 2)
			{
				Write(human, NC2Layout::HumanOffset::CarriedGood, good);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, good);
			}
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == SwitchIds::GoodCirculationMode::FetchForSelf)
			{
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 7) Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, SwitchIds::GoodCirculationMode::None);
			}
			else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != SwitchIds::GoodCirculationMode::FlushStock) Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, SwitchIds::GoodCirculationMode::FillStockBringAway);
		}
		else Human_StopCurrentGoodCirculationMode(human);
		Human_Target_Reset(human);
	}

	// Original function: FUN_00468d91; stock changes precede experience and carried-good callbacks.
	void Human_l_PerformTaskEvent_Pileup(void* human)
	{
		if (!Human_Target_IsValid(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 0); Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3); return;
		}
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);
		std::uint32_t failure = 11;
		bool stored = false;
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
			if (Vehicle_Stock_CanStoreGoodType(vehicle, good) && !Vehicle_Stock_IsFull(vehicle)) stored = Vehicle_Stock_ModifyAmount(vehicle, good, 1, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
		}
		else if (auto* house = Human_GetHousePtrAtTargetPosition(human))
		{
			if (!House_Stock_IsFull(house, good))
			{
				const auto id = Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId);
				const auto construction = (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 5 && Human_IsAttachedToHomeHouse(human) && id == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)) || (Human_IsAttachedToWorkHouse(human) && id == Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)) || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId) != NXBasics::IntegerRange::UInt32Maximum && id == Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId)) || Human_IsDefendingHouse(human, static_cast<std::int32_t>(id));
				stored = House_Stock_ModifyAmount(house, good, 1, 0, construction);
			}
		}
		else
		{
			failure = 7; stored = Map_GoodTool_PutOnMap(good, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition, false);
		}
		if (stored)
		{
			if ((Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 || Human_IsAtTargetPoint(human)) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood))
			{
				if (Human_IsJobCarrier(human)) Human_l_JobExperience_Gain(human);
				else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)) Human_Production_DidProduceGoods(human, 1);
			}
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood, 0);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, SwitchIds::GoodCirculationMode::None);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write(human, NC2Layout::HumanOffset::CurrentTaskStep, failure);
			Human_StopCurrentGoodCirculationMode(human);
		}
	}

	// Original function: FUN_0042d019; queued orders use the secondary target before clearing it.
	void Human_Target_RestoreSecondaryTarget(void* human)
	{
		const auto type = Read<std::int32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget);
		if (type == 7 || type == 8) Human_DoAddNewUserCommand(human, 4, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), MapMigPoint_GetLong(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::SecondaryTargetPosition), 0);
		else if (type == 6)
		{
			if (Human_IsAttachedToVehicle(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryTargetObjectRevision)) Human_DoAddNewUserCommand(human, 40, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryTargetObjectId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryTargetObjectRevision));
		}
		else if (type >= 1 && type <= 5)
		{
			const auto mode = Read<std::uint32_t>(human, NC2Layout::HumanOffset::SuspendedGoodCirculationMode);
			if (mode != SwitchIds::GoodCirculationMode::None && !(TGoodTypes_IsFood(Read<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryTargetGoodType)) && mode == SwitchIds::GoodCirculationMode::FetchForSelf))
			{
				Write(human, NC2Layout::HumanOffset::RequestedGood, Read<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryTargetGoodType));
				std::memcpy(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::InteractionTarget, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::SecondaryInteractionTarget, NC2Layout::RecordSize::InteractionTargetData);
				auto* real = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition;
				auto* work = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition;
				if (mode >= SwitchIds::GoodCirculationMode::FetchForSelf && mode <= SwitchIds::GoodCirculationMode::FillStockBringAway) Human_l_StartAtomicWalkAndFetch(human, real, work, mode == SwitchIds::GoodCirculationMode::FetchForSelf);
				else if (mode == SwitchIds::GoodCirculationMode::FlushStock)
				{
					Human_l_StartAtomicWalk(human, real, work);
					Write(human, NC2Layout::HumanOffset::InteractionTarget, Read<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget));
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, SwitchIds::GoodCirculationMode::FlushStock);
				}
			}
		}
		Human_Target_ResetSecondaryTarget(human);
	}

	// Original function: FUN_0042cec4.
	std::uint32_t Human_Target_GetType(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget);
	}
	// Original function: FUN_0042cf47.
	const void* Human_Target_GetWorkPositionRef(const void* human)
	{
		return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition;
	}
	// Original function: FUN_0042cf4e.
	const void* Human_Target_GetRealPositionRef(const void* human)
	{
		return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition;
	}
	// Original function: FUN_0042d709.
	void* Human_GetHousePtrAtMapPosition(const void* human, const void* point)
	{
		const auto id = Logic_EntryPoint_House_GetOnPosition(point);
		if (id == -1) return nullptr;
		auto* house = House(static_cast<std::uint32_t>(id));
		return Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) ? house : nullptr;
	}

}

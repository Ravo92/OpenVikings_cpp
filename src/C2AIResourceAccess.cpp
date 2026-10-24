#include "RemasterAILog.h"
#include "C2AIBuildingAccess.h"
#include "C2AIResourceAccess.h"
#include "C2GameMapAccess.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticHouseTypeData.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		std::int32_t ResourceIndex(const void* manager, std::uint32_t good)
		{
			for (std::int32_t i = 0; i < Read<std::int32_t>(manager, NC2Layout::AIResourceManagerOffset::ResourceCount); ++i) if (Read<std::uint32_t>(manager, NC2Layout::AIResourceManagerOffset::FirstResourceGood + static_cast<std::uint32_t>(i) * NC2Layout::RecordSize::AIResourcePoint) == good) return i;
			return -1;
		}
		const std::byte* Resource(const void* manager, std::int32_t index)
		{
			return static_cast<const std::byte*>(manager) + index * NC2Layout::RecordSize::AIResourcePoint;
		}
		std::byte* Resource(void* manager, std::int32_t index)
		{
			return static_cast<std::byte*>(manager) + index * NC2Layout::RecordSize::AIResourcePoint;
		}
		std::uint32_t Distance(const void* first, const void* second)
		{
			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::uint16_t>(first, NC2Layout::MapCoordinatePairOffset::X), Read<std::uint16_t>(first, NC2Layout::MapCoordinatePairOffset::Y), Read<std::uint16_t>(second, NC2Layout::MapCoordinatePairOffset::X), Read<std::uint16_t>(second, NC2Layout::MapCoordinatePairOffset::Y)));
		}
	}

	// Original function: FUN_004537c9.
	std::int32_t AIVillagesData_GetVillageId(const void* villages, const void* point)
	{
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			const auto* slot = static_cast<const std::byte*>(villages) + i * NC2Layout::RecordSize::AIVillageSlot;
			if (Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::Active) != 0 && AIVillage_IsPointInside(slot + NC2Layout::AIVillageSlotViewOffset::Village, point)) return Read<std::int32_t>(slot, NC2Layout::AIVillageSlotViewOffset::VillageId);
		}
		return -1;
	}

	// Original function: FUN_00453c07; Windows returns the first slot when no id matches.
	void* AIVillagesData_GetVillageDataById(void* villages, std::int32_t id)
	{
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			auto* slot = static_cast<std::byte*>(villages) + i * NC2Layout::RecordSize::AIVillageSlot;
			if (Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::Active) != 0 && Read<std::int32_t>(slot, NC2Layout::AIVillageSlotViewOffset::VillageId) == id) return slot + NC2Layout::AIVillageSlotViewOffset::Village;
		}
		return static_cast<std::byte*>(villages) + 12;
	}

	const void* AIVillagesData_GetConstVillageDataById(const void* villages, std::int32_t id)
	{
		return AIVillagesData_GetVillageDataById(const_cast<void*>(villages), id);
	}

	// Original function: FUN_00456449; the constructor owns the initial clear, not this initializer.
	void AIResourceManager_Init(void* manager, std::uint32_t player, const void* villages)
	{
		Write(manager, NC2Layout::AIResourceManagerOffset::PlayerId, player); Write<std::int32_t>(manager, NC2Layout::AIResourceManagerOffset::ScanIndex, -1);
		for (std::uint32_t i = 0; i < 88; ++i) Write<std::int32_t>(manager, NC2Layout::AIResourceManagerOffset::LandscapeResourceIndices + i * NC2Layout::RecordSize::AIResourceIndexValue, -1);
		const auto tribe = NXBasics::Runtime_TimeSigned32(player) < 20 ? Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::TribeType) : 1;
		const auto* control = static_cast<const std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayerSigned;
		for (std::uint32_t job = 0; job < 48; ++job)
		{
			if (Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::AllowedJobs + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipment + job) == 0) continue;
			const auto* jobData = static_cast<const std::byte*>(gStaticJobTypeData) + job * NC2Layout::RecordSize::JobTypeSigned;
			if (Read<std::uint8_t>(jobData, NC2Layout::StaticJobTypeOffset::UserCanChangeProduction) == 0) continue;
			for (std::uint32_t good = 0; good < 66; ++good)
			{
				if (Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::AllowedGoods + tribe * NC2Runtime::PlayerAllowanceStride::Goods + good) == 0) continue;
				const auto* definition = StaticGoodTypeData_GetPtr(good);
				if (Read<std::uint8_t>(jobData, NC2Layout::StaticJobTypeOffset::AllowedAtomics + static_cast<std::uint32_t>(definition->atomicForHarvesting)) == 0 && Read<std::uint8_t>(jobData, NC2Layout::StaticJobTypeOffset::AllowedAtomics + static_cast<std::uint32_t>(definition->atomicForProduction)) == 0) continue;
				auto* goodData = static_cast<std::byte*>(manager) + NC2Layout::AIResourceManagerOffset::GoodRequirements + good * NC2Layout::RecordSize::AIResourceGood;
				if (definition->isProducedOnMap != 0 && Read<std::uint8_t>(jobData, NC2Layout::StaticJobTypeOffset::MustHaveWorkHouse) == 0)
				{
					Write(goodData, NC2Layout::AIResourceGoodOffset::WorkerJob, job); Write<std::uint32_t>(goodData, NC2Layout::AIResourceGoodOffset::Type, 1);
					const auto index = Read<std::uint32_t>(manager, NC2Layout::AIResourceManagerOffset::ResourceCount);
					if (index == 20) continue;
					auto* entry = Resource(manager, static_cast<std::int32_t>(index));
					Write(entry, NC2Layout::AIResourceSlotViewOffset::GoodType, good);
					Write(manager, NC2Layout::AIResourceManagerOffset::LandscapeResourceIndices + static_cast<std::uint32_t>(definition->landscapeToHarvest) * NC2Layout::RecordSize::AIResourceIndexValue, index);
					Write(manager, NC2Layout::AIResourceManagerOffset::LandscapeResourceIndices + static_cast<std::uint32_t>(definition->landscapeToPickup) * NC2Layout::RecordSize::AIResourceIndexValue, index);
					Write(manager, NC2Layout::AIResourceManagerOffset::LandscapeResourceIndices + static_cast<std::uint32_t>(definition->landscapeToStore) * NC2Layout::RecordSize::AIResourceIndexValue, index);
					Write(entry, NC2Layout::AIResourceSlotViewOffset::StoredLandscapeType, definition->landscapeToStore);
					if (definition->isBioLandscape != 0) Write<std::uint32_t>(entry, NC2Layout::AIResourceSlotViewOffset::MinimumValency, definition->landscapeToHarvest == 4 ? 3 : 5);
					Write(manager, NC2Layout::AIResourceManagerOffset::ResourceCount, index + 1);
				}
				else if (Read<std::uint8_t>(jobData, NC2Layout::StaticJobTypeOffset::MustHaveWorkHouse) != 0)
				{
					Write(goodData, NC2Layout::AIResourceGoodOffset::WorkerJob, job); Write<std::uint32_t>(goodData, NC2Layout::AIResourceGoodOffset::Type, 4);
					const auto house = CStaticHouseTypeDataManager::GetHouseType(static_cast<std::int32_t>(job), static_cast<std::int32_t>(good));
					if (house == 0)
					{
						Write<std::uint32_t>(goodData, NC2Layout::AIResourceGoodOffset::Type, 0); continue;
					}
					Write(goodData, NC2Layout::AIResourceGoodOffset::HouseType, house); Write(manager, NC2Layout::AIResourceManagerOffset::HouseGoods + static_cast<std::uint32_t>(house) * NC2Layout::RecordSize::AIResourceIndexValue, good);
					if (definition->atomicForCultivating != 0) Write<std::uint8_t>(goodData, NC2Layout::AIResourceGoodOffset::CheckTerrainFlag, 1);
				}
			}
		}
		for (std::uint32_t good = 0; good < 66; ++good)
		{
			if (Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::AllowedGoods + tribe * NC2Runtime::PlayerAllowanceStride::Goods + good) == 0) continue;
			auto* entry = static_cast<std::byte*>(manager) + NC2Layout::AIResourceManagerOffset::GoodRequirements + good * NC2Layout::RecordSize::AIResourceGood;
			if (Read<std::uint32_t>(entry, NC2Layout::AIResourceGoodOffset::Type) != 0) continue;
			const auto house = CStaticHouseTypeDataManager::GetHouseType_NoStock(static_cast<std::int32_t>(good));
			if (house == 0) continue;
			Write<std::uint32_t>(entry, NC2Layout::AIResourceGoodOffset::Type, 4); Write(entry, NC2Layout::AIResourceGoodOffset::HouseType, house); Write<std::uint32_t>(entry, NC2Layout::AIResourceGoodOffset::WorkerJob, 0); Write(manager, NC2Layout::AIResourceManagerOffset::HouseGoods + static_cast<std::uint32_t>(house) * NC2Layout::RecordSize::AIResourceIndexValue, good);
		}
		for (std::uint32_t house = 0; house < 55; ++house)
		{
			auto* exports = static_cast<std::byte*>(manager) + NC2Layout::AIResourceManagerOffset::HouseExports + house * NC2Layout::RecordSize::AIHouseExport;
			Write<std::uint32_t>(exports, NC2Layout::AIHouseExportOffset::Count, 0);
			if (house == 7 || house == 8 || house == 9 || house == 1 || Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::AllowedHouses + tribe * NC2Runtime::PlayerAllowanceStride::Houses + house) == 0) continue;
			const auto& definition = gStaticHouseTypeStorage.records[house];
			for (std::uint32_t stock = 0; stock < definition.stockCount; ++stock)
			{
				const auto& output = definition.stock[stock];
				if (output.isOutputStock == 0 || Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::AllowedGoods + tribe * NC2Runtime::PlayerAllowanceStride::Goods + static_cast<std::uint32_t>(output.goodType)) == 0) continue;
				for (std::uint32_t other = 0; other < 55; ++other)
				{
					if (other == 7 || other == 8 || other == 9 || other == 1 || Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::AllowedHouses + tribe * NC2Runtime::PlayerAllowanceStride::Houses + other) == 0) continue;
					const auto& consumer = gStaticHouseTypeStorage.records[other];
					for (std::uint32_t input = 0; input < consumer.stockCount; ++input)
					{
						if (consumer.stock[input].isOutputStock != 0 || consumer.stock[input].goodType != output.goodType || Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::AllowedGoods + tribe * NC2Runtime::PlayerAllowanceStride::Goods + static_cast<std::uint32_t>(output.goodType)) == 0) continue;
						const auto count = Read<std::uint32_t>(exports, NC2Layout::AIHouseExportOffset::Count);
						Write(exports, NC2Layout::AIHouseExportOffset::HouseTypes + count * NC2Layout::RecordSize::AIHouseTypeId, other); Write(exports, NC2Layout::AIHouseExportOffset::Count, count + 1); break;
					}
					if (Read<std::uint32_t>(exports, NC2Layout::AIHouseExportOffset::Count) == 5) break;
				}
			}
		}
		for (std::int32_t i = 0; i < Read<std::int32_t>(manager, NC2Layout::AIResourceManagerOffset::ResourceCount); ++i)
		{
			Write<std::uint8_t>(Resource(manager, i), NC2Layout::AIResourceSlotViewOffset::OnMapFlag, 1);
			AIResourceManager_l_UpdateResourcePoint(manager, player, i, villages);
		}
	}

	// Original function: FUN_00456861.
	void AIResourceManager_UpdateResourcePointArray(void* manager, std::uint32_t player, const void* villages)
	{
		auto index = Read<std::int32_t>(manager, NC2Layout::AIResourceManagerOffset::ScanIndex) + 1;
		Write(manager, NC2Layout::AIResourceManagerOffset::ScanIndex, index);
		if (index >= Read<std::int32_t>(manager, NC2Layout::AIResourceManagerOffset::ResourceCount))
		{
			index = 0; Write(manager, NC2Layout::AIResourceManagerOffset::ScanIndex, index);
		}
		if (Read<std::uint8_t>(Resource(manager, index), NC2Layout::AIResourceSlotViewOffset::OnMapFlag) != 0 && !AIResourceManager_l_CheckOldResourceArea(manager, index)) AIResourceManager_l_UpdateResourcePoint(manager, player, Read<std::int32_t>(manager, NC2Layout::AIResourceManagerOffset::ScanIndex), villages);
	}

	// Original function: FUN_004568b4; the upper edges are exclusive even after clamping to size minus one.
	bool AIResourceManager_l_CheckOldResourceArea(void* manager, std::int32_t index)
	{
		auto* entry = Resource(manager, index);
		auto* center = entry + NC2Layout::AIResourceSlotViewOffset::Point;
		Tool_Map_GetNotBlockedPosition(center, false);
		const auto x = Read<std::uint16_t>(center, NC2Layout::MapCoordinatePairOffset::X), y = Read<std::uint16_t>(center, NC2Layout::MapCoordinatePairOffset::Y);
		const auto lowX = static_cast<std::int16_t>(x - 8) < 0 ? 0 : x - 8;
		const auto lowY = static_cast<std::int16_t>(y - 8) < 0 ? 0 : y - 8;
		const auto highX = static_cast<std::int16_t>(x + 8) > GameMap_GetWidth() - 1 ? GameMap_GetWidth() - 1 : static_cast<std::int16_t>(x + 8);
		const auto highY = static_cast<std::int16_t>(y + 8) > GameMap_GetHeight() - 1 ? GameMap_GetHeight() - 1 : static_cast<std::int16_t>(y + 8);
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(center), NC2Layout::MapMicroPointOffset::ContinentId);
		for (std::int16_t py = static_cast<std::int16_t>(lowY); py < highY; ++py)
			for (std::int16_t px = static_cast<std::int16_t>(lowX); px < highX; ++px)
			{
				std::uint32_t point; MapMigPoint_SetPosition(&point, px, py, false);
				const auto* micro = GameMap_GetConstMapMicroPointPtr(&point);
				const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);
				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && type != 0 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) == continent && Read<std::int32_t>(manager, NC2Layout::AIResourceManagerOffset::LandscapeResourceIndices + type * NC2Layout::RecordSize::AIResourceIndexValue) == index && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) >= Read<std::uint32_t>(entry, NC2Layout::AIResourceSlotViewOffset::MinimumValency)) return true;
			}
		return false;
	}

	// Original function: FUN_004569b4.
	void AIResourceManager_l_UpdateResourcePoint(void* manager, std::uint32_t player, std::int32_t index, const void* villages)
	{
		if (index == -1) return;
		auto* entry = Resource(manager, index);
		Write<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::UpdatedFlag, 1);
		if (Read<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag) == 0) return;
		alignas(4) std::byte search[32];
		AIResourceSearchTool_Construct(search, villages, static_cast<const std::byte*>(manager) + NC2Layout::AIResourceManagerOffset::ResourcePoints, index);
		Write<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag, 0);
		for (std::int16_t y = 0; y < GameMap_GetHeight(); ++y)
			for (std::int16_t x = 0; x < GameMap_GetWidth(); ++x)
			{
				std::uint32_t point; MapMigPoint_SetPosition(&point, x, y, false);
				if (!AIResourceSearchTool_IsCandidate(search, &point)) continue;
				AIResourceSearchTool_CheckAndSetData(search, &point); Write<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag, 1);
			}
		if (Read<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag) != 0)
		{
			Write(entry, NC2Layout::AIResourceSlotViewOffset::Point, Read<std::uint32_t>(search, NC2Layout::AIResourceSearchOffset::BestPoint)); Tool_Map_GetNotBlockedPosition(entry + NC2Layout::AIResourceSlotViewOffset::Point, false);
		}
		else Write<std::int16_t>(entry, NC2Layout::AIResourceSlotViewOffset::Point, -100);
	}

	// Original function: FUN_00456a74.
	void* AIResourceSearchTool_Construct(void* search, const void* villages, const void* entries, std::int32_t index)
	{
		Write(search, NC2Layout::AIResourceSearchOffset::ResourcePoints, entries); Write<std::uint32_t>(search, NC2Layout::AIResourceSearchOffset::BestPoint, 0); Write(search, NC2Layout::AIResourceSearchOffset::Villages, villages); Write(search, NC2Layout::AIResourceSearchOffset::ResourceIndex, index);
		Write<std::uint32_t>(search, NC2Layout::AIResourceSearchOffset::ContinentId, Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(villages) + NC2Layout::AIVillagesOffset::FirstVillageCenter), NC2Layout::MapMicroPointOffset::ContinentId));
		Write<std::int32_t>(search, NC2Layout::AIResourceSearchOffset::BestVillageId, -1); Write<std::int32_t>(search, NC2Layout::AIResourceSearchOffset::BestScore, 10000);
		Write(search, NC2Layout::AIResourceSearchOffset::PlayerId, Read<std::uint32_t>(AIVillagesData_GetConstVillageDataById(villages, 0), NC2Layout::AIVillageOffset::PlayerId));
		return search;
	}

	// Original function: FUN_00456ac8.
	bool AIResourceSearchTool_IsCandidate(const void* search, const void* point)
	{
		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint32_t>(search, NC2Layout::AIResourceSearchOffset::ContinentId)) return false;
		const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);
		const auto* entries = Read<const void*>(search, NC2Layout::AIResourceSearchOffset::ResourcePoints);
		const auto index = Read<std::uint32_t>(search, NC2Layout::AIResourceSearchOffset::ResourceIndex);
		if (Read<std::uint32_t>(entries, NC2Layout::AIResourcePointArrayOffset::LandscapeResourceIndices + type * NC2Layout::RecordSize::AIResourceIndexValue) != index) return false;
		const auto* entry = static_cast<const std::byte*>(entries) + index * 20;
		return type == Read<std::uint32_t>(entry, NC2Layout::AIResourcePointOffset::StoredLandscapeType) || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) >= Read<std::uint32_t>(entry, NC2Layout::AIResourcePointOffset::MinimumValency);
	}

	// Original function: FUN_00456b23.
	void AIResourceSearchTool_CheckAndSetData(void* search, const void* point)
	{
		const auto* villages = Read<const void*>(search, NC2Layout::AIResourceSearchOffset::Villages);
		const auto village = AIVillagesData_GetVillageId(villages, point);
		const void* center = static_cast<const std::byte*>(villages) + NC2Layout::AIVillagesOffset::FirstVillageCenter;
		if (village == -1)
		{
			if (Read<std::int32_t>(search, NC2Layout::AIResourceSearchOffset::BestVillageId) != -1) return;
		}
		else center = static_cast<const std::byte*>(AIVillagesData_GetConstVillageDataById(villages, village)) + 12;
		const auto distance = NXBasics::Runtime_TimeSigned32(Distance(point, center));
		const auto danger = AIMilitaryManager_Military_GetCivilianDangerValueForPosition(point, Read<std::int32_t>(search, NC2Layout::AIResourceSearchOffset::PlayerId));
		const auto score = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(danger) + static_cast<std::uint32_t>(distance / 5));
		if (score > Read<std::int32_t>(search, NC2Layout::AIResourceSearchOffset::BestScore)) return;
		Write(search, NC2Layout::AIResourceSearchOffset::BestPoint, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates)); Write(search, NC2Layout::AIResourceSearchOffset::BestScore, score); Write(search, NC2Layout::AIResourceSearchOffset::BestVillageId, village);
	}

	// Original function: FUN_00456ba3; callers must request an initialized good, as in Windows.
	bool AIResourceManager_GetResourcePosition(const void* manager, std::uint32_t good, void* point)
	{
		const auto* entry = Resource(manager, ResourceIndex(manager, good));
		if (Read<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag) == 0) { RemasterAILog::Event(Read<std::uint32_t>(manager, NC2Layout::AIResourceManagerOffset::PlayerId), "resource_search", static_cast<std::int32_t>(good), "good=%u result=no_resource_point", good); return false; }
		RemasterAILog::Event(Read<std::uint32_t>(manager, NC2Layout::AIResourceManagerOffset::PlayerId), "resource_search", static_cast<std::int32_t>(good), "good=%u position=(%d,%d)", good, Read<std::int16_t>(entry, NC2Layout::AIResourceSlotViewOffset::Point), Read<std::int16_t>(entry, NC2Layout::AIResourceSlotViewOffset::Point + 2));
		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(entry, NC2Layout::AIResourceSlotViewOffset::Point)); return true;
	}

	// Original function: FUN_00456be8.
	void AIResourceManager_UpdateGoodResource(void* manager, std::uint32_t good, const void* invalidAreas, const void* villages)
	{
		const auto index = ResourceIndex(manager, good);
		auto* entry = Resource(manager, index);
		Write<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::UpdatedFlag, 1);
		if (Read<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag) == 0) return;
		alignas(4) std::byte search[32];
		AIResourceSearchTool_Construct(search, villages, static_cast<const std::byte*>(manager) + NC2Layout::AIResourceManagerOffset::ResourcePoints, index);
		Write<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag, 0);
		for (std::int16_t y = 0; y < GameMap_GetHeight(); ++y)
			for (std::int16_t x = 0; x < GameMap_GetWidth(); ++x)
			{
				std::uint32_t point; MapMigPoint_SetPosition(&point, x, y, false);
				if (AIInvalidAreaManager_IsInsideAnyArea(invalidAreas, &point) || !AIResourceSearchTool_IsCandidate(search, &point)) continue;
				AIResourceSearchTool_CheckAndSetData(search, &point); Write<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag, 1);
			}
		if (Read<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag) != 0)
		{
			Write(entry, NC2Layout::AIResourceSlotViewOffset::Point, Read<std::uint32_t>(search, NC2Layout::AIResourceSearchOffset::BestPoint)); Tool_Map_GetNotBlockedPosition(entry + NC2Layout::AIResourceSlotViewOffset::Point, false);
		}
		else Write<std::int16_t>(entry, NC2Layout::AIResourceSlotViewOffset::Point, -100);
	}

	// Original function: FUN_00456cdc.
	std::uint8_t AIResourceManager_WasResourceDataUpdated(const void* manager, std::uint32_t good)
	{
		const auto* entry = Resource(manager, ResourceIndex(manager, good));
		return Read<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::OnMapFlag) != 0 ? Read<std::uint8_t>(entry, NC2Layout::AIResourceSlotViewOffset::UpdatedFlag) : 0;
	}

	// Original function: FUN_00456d19.
	std::uint8_t AIResourceManager_IsResourceOnMap(const void* manager, std::uint32_t good)
	{
		return Read<std::uint8_t>(Resource(manager, ResourceIndex(manager, good)), NC2Layout::AIResourceSlotViewOffset::OnMapFlag);
	}

	// Original function: FUN_00456d4d.
	void AIResourceManager_ClearResourceUpdateFlags(void* manager)
	{
		for (std::int32_t i = 0; i < Read<std::int32_t>(manager, NC2Layout::AIResourceManagerOffset::ResourceCount); ++i) Write<std::uint8_t>(Resource(manager, i), NC2Layout::AIResourceSlotViewOffset::UpdatedFlag, 0);
	}

	// Original function: FUN_00456d6c.
	bool AIResourceManager_IsExportingHouseType(const void* manager, std::uint32_t house, std::uint32_t exportedHouse)
	{
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(manager, NC2Layout::AIResourceManagerOffset::HouseExports + house * NC2Layout::RecordSize::AIHouseExport); ++i) if (Read<std::uint32_t>(manager, NC2Layout::AIResourceManagerOffset::FirstHouseExportTypes + house * NC2Layout::RecordSize::AIHouseExport + i * NC2Layout::RecordSize::AIResourceIndexValue) == exportedHouse) return true;
		return false;
	}

	// Original function: FUN_0045715a.
	bool AIInvalidAreaManager_AddArea(void* manager, const void* area)
	{
		const auto count = Read<std::uint32_t>(manager, NC2Layout::AIInvalidAreaManagerOffset::Count);
		if (count >= 50) return false;
		Write(manager, NC2Layout::AIInvalidAreaManagerOffset::Areas + count * NC2Layout::RecordSize::AIInvalidArea, Read<std::uint32_t>(area, NC2Layout::AIInvalidAreaOffset::Center)); Write(manager, NC2Layout::AIInvalidAreaManagerOffset::FirstAreaRadius + count * NC2Layout::RecordSize::AIInvalidArea, Read<std::uint32_t>(area, NC2Layout::AIInvalidAreaOffset::Radius)); Write(manager, NC2Layout::AIInvalidAreaManagerOffset::Count, count + 1);
		return true;
	}

	// Original function: FUN_0045717f.
	bool AIInvalidAreaManager_IsInsideAnyArea(const void* manager, const void* point)
	{
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(manager, NC2Layout::AIInvalidAreaManagerOffset::Count); ++i)
		{
			const auto* area = static_cast<const std::byte*>(manager) + NC2Layout::AIInvalidAreaManagerOffset::Areas + i * NC2Layout::RecordSize::AIInvalidArea;
			if (Distance(area, point) <= Read<std::uint32_t>(area, NC2Layout::AIInvalidAreaOffset::Radius)) return true;
		}
		return false;
	}
}

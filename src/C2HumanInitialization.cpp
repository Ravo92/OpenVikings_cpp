#include "GameSetupStartup.h"
#include "C2CallbackManager.h"
#include "C2DefaultInteraction.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameEngineStartup.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanTaskAccess.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2StaticAtomicAnimationData.h"
#include "C2StaticTribeTypeData.h"
#include "C2VehicleAccess.h"
#include "CivilizationBuildings.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_00438df2.
	std::int32_t Marker_PlayerId_Get(const void* marker)
	{
		const auto humanId = NC2Lifetime::Read<std::int32_t>(marker, NC2Layout::MarkerOffset::HumanId);
		return humanId == -1 ? -1 : NC2Lifetime::Read<std::int32_t>(GetHumanPtr(humanId), NC2Layout::HumanOffset::PlayerId);
	}

	using namespace NC2Lifetime;

	// Original function: FUN_00438c79; one mode byte and 66 per-good enable flags.
	static void Human_ProductionGoodEnableFlags_Construct(void* data)
	{
		std::memset(static_cast<std::byte*>(data) + NC2Layout::ByteEnableSelectionOffset::Entries, 0, 66);
		Write<std::uint8_t>(data, NC2Layout::ByteEnableSelectionOffset::FirstEnabled, 1);
		Write<std::uint8_t>(data, NC2Layout::ByteEnableSelectionOffset::Mode, 0);
	}

	// Original function: FUN_0044b5e1; one mode byte and seven enable flags; exact domain unresolved.
	static void SevenEntryEnableFlags_Construct(void* data)
	{
		std::memset(static_cast<std::byte*>(data) + NC2Layout::ByteEnableSelectionOffset::Entries, 0, 7);
		Write<std::uint8_t>(data, NC2Layout::ByteEnableSelectionOffset::FirstEnabled, 1);
		Write<std::uint8_t>(data, NC2Layout::ByteEnableSelectionOffset::Mode, 0);
	}

	// Original function: FUN_0044b590; reset job, production and equipment capabilities for the new player.
	static void Human_SetPlayerIdAndResetCapabilities(void* human, std::int32_t player)
	{
		Write(human, NC2Layout::HumanOffset::PlayerId, player);
		std::memset(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::AllowedJobFlagsSigned, 0, 56);
		Human_ProductionGoodEnableFlags_Construct(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::ProductionSelectionMode);
		SevenEntryEnableFlags_Construct(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::UnresolvedSevenEntrySelection);
		std::memset(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::AllowedEquipmentFlagsSigned, 0, 56);
		Human_SetupPlayerSpecificStuff(human);
	}

	// Original function: FUN_0044b45d; the native player upper bound is signed without a lower guard.
	void Human_ChangePlayerId(void* human, std::int32_t player, bool detach)
	{
		if (player >= 20 || Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer) == 0) return;
		if (Human_IsJobSoldier(human) || Human_IsJobHero(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::MilitaryWorldRefreshmentFlag, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode, 2);
		}
		std::uint32_t point = 0;
		bool wasAttached = false;
		void* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
		{
			point = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
			MapMoveable_Attachment_DetachFromMap(moveable);
			wasAttached = true;
		}
		Human_SetPlayerIdAndResetCapabilities(human, player);
		void* pathfinder = GetUnsafePathfinderPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId));
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId, -1);
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideId, -1);
		Write(pathfinder, NC2Layout::MapPathfinderOffset::PlayerId, player);
		if (wasAttached)
		{
			MapMoveable_Attachment_AttachToMap(moveable, &point);
			const auto radius = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ExplorationRadius);
			const auto owner = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
			GameMap_Exploration_DoExplore(owner, Human_GetMapPositionRef(human), radius);
		}
		if (detach) Human_Detach(human, false);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 2, static_cast<std::uint32_t>(player));
		Human_SetupCurrentJobType(human);
		Human_UpdateCurrentJobType(human);
		if (gGameStaticVars.networkGame == 0 && NRemaster::CivilizationBuildings::IsActive())
		{
			const bool benefit = Read<std::uint32_t>(gGameControl, NC2Layout::GameControlOffset::DifficultyLevel) == 2 && Remaster_GameControl_Player_HasDifficultyBenefits(gGameControl, static_cast<std::uint32_t>(player));
			Write(human, NC2Layout::HumanOffset::MiscFlags, (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & ~1u) | (benefit ? 1u : 0u));
		}
		else if (player == 0 && Read<std::uint32_t>(gGameControl, NC2Layout::GameControlOffset::DifficultyLevel) == 2) Write(human, NC2Layout::HumanOffset::MiscFlags, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) | 1u);
	}

	// Original function: FUN_00436aad; Windows outlined work-marker eligibility predicate.
	bool Human_WorkCenterMarker_IsAllowed(const void* human)
	{
		if (!Human_IsSexMale(human) || !Human_IsAgeAdult(human)) return false;
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		return (Human_IsJobCarrier(human) && Human_IsAttachedToWorkHouse(human)) || job == 8 || Human_IsJobHunter(human) || job == 18 || Human_IsJobFisher(human) || job == 28 || Human_IsJobBuilder(human);
	}

	namespace
	{
		std::byte* Human(std::uint32_t id)
		{
			return CHumanArray::mStaticVars + id * CHumanArray::ElementSize;
		}
		std::byte* House(std::uint32_t id)
		{
			return CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
		}
		std::byte* Moveable(std::uint32_t id)
		{
			return gMapMoveableArrayStorage.entries + id * NC2Layout::RecordSize::MapMoveable;
		}

		// Original function: FUN_00438d39.
		bool Marker_Init(void* marker, std::int32_t id, std::uint32_t type, const void* point, std::uint32_t human)
		{
			if (Read<std::uint8_t>(marker, NC2Layout::MarkerOffset::Allocated) != 0) return false;
			std::memset(marker, 0, NC2Layout::RecordSize::Marker);
			Write<std::uint8_t>(marker, NC2Layout::MarkerOffset::Allocated, 1);
			Write(marker, NC2Layout::MarkerOffset::MarkerId, id);
			Write(marker, NC2Layout::MarkerOffset::UniqueId, Game_UniqueId_Generate());
			Write(marker, NC2Layout::MarkerOffset::Type, type);
			Write(marker, NC2Layout::MarkerOffset::HumanId, human);
			const auto moveable = MapMoveableArray_AllocateMapMoveable(gMapMoveableArrayStorage.manager, 4, id);
			Write(marker, NC2Layout::MarkerOffset::MapMoveableId, moveable);
			if (moveable == -1) return false;
			MapMoveable_Attachment_AttachToMap(Moveable(moveable), point);
			return true;
		}
		// Original function: FUN_00438da4.
		void Marker_Exit(void* marker)
		{
			if (Read<std::uint8_t>(marker, NC2Layout::MarkerOffset::Allocated) == 0) return;
			const auto id = Read<std::int32_t>(marker, NC2Layout::MarkerOffset::MapMoveableId);
			if (id != -1) MapMoveableArray_FreeMapMoveable(gMapMoveableArrayStorage.manager, id);
			Write<std::uint8_t>(marker, NC2Layout::MarkerOffset::Allocated, 0);
		}
		// Original function: FUN_00438dc5.
		void Marker_MapPosition_Set(void* marker, const void* point)
		{
			void* moveable = Moveable(Read<std::uint32_t>(marker, NC2Layout::MarkerOffset::MapMoveableId));
			MapMoveable_Attachment_DetachFromMap(moveable);
			MapMoveable_Attachment_AttachToMap(moveable, point);
		}
	}

	// Original function: FUN_0040badc.
	void __fastcall MarkerArray_FreeMarker(void* array, void*, std::int32_t id)
	{
		void* marker = static_cast<std::byte*>(gMarkerEntries) + id * NC2Layout::RecordSize::Marker;
		if (Read<std::uint8_t>(marker, NC2Layout::MarkerOffset::Allocated) != 0 && static_cast<CArrayHandler*>(array)->Element_Free(id, true)) Marker_Exit(marker);
	}
	// Original function: FUN_0040ba7d; remaster initializes in-map border markers and rejects invalid points before reserving a slot.
	std::int32_t __fastcall MarkerArray_AllocateMarker(void* array, void*, std::uint32_t type, const void* point, std::uint32_t human)
	{
		if (!MapMigPoint_IsInsideMap(point)) return -1;
		const auto id = static_cast<CArrayHandler*>(array)->Element_Allocate();
		if (id == -1) return -1;
		if (!Marker_Init(static_cast<std::byte*>(gMarkerEntries) + id * NC2Layout::RecordSize::Marker, id, type, point, human))
		{
			MarkerArray_FreeMarker(array, nullptr, id); return -1;
		}
		return id;
	}

	// Original function: FUN_0040b09b.
	void Logic_EntryPoint_Edit_Start()
	{
		GameMapModifyManager_StartGameMapModify();
	}
	// Original function: FUN_0040b0a0.
	void Logic_EntryPoint_Edit_Stop()
	{
		GameMapModifyManager_StopGameMapModify();
	}

	// Original function: FUN_0040add6; release the owning object, not just its moveable.
	void Logic_EntryPoint_FreeMapMoveable(std::uint32_t id)
	{
		if (id == NXBasics::IntegerRange::UInt32Maximum) return;
		const void* moveable = Moveable(id);
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) == 0) return;
		const auto objectId = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
		switch (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType))
		{
			case SwitchIds::MapMoveableType::Human: CHumanArray::sTheObjectPtr->FreeHuman(objectId); break;
			case SwitchIds::MapMoveableType::Animal: CAnimalArray::sTheObjectPtr->FreeAnimal(objectId); break;
			case SwitchIds::MapMoveableType::Vehicle: CVehicleArray::sTheObjectPtr->FreeVehicle(objectId); break;
			case SwitchIds::MapMoveableType::Marker: MarkerArray_FreeMarker(gMarkerArray, nullptr, objectId); break;
		}
	}

	// Original function: FUN_0040ad73.
	std::int32_t Logic_EntryPoint_Add_Human(std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags)
	{
		return CHumanArray::sTheObjectPtr->AllocateHuman(player, tribe, job, point, missionCheck, flags);
	}
	// Original function: FUN_0040b502.
	std::int32_t CHumanArray::AllocateHuman(std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags)
	{
		// Remaster: reject invalid positions before consuming a pool reservation.
		if (point == nullptr || !MapMigPoint_IsInsideMap(point) || MapMigPoint_IsBorderPoint(point)) return -1;
		if (GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId)) == 0) return -1;
		const auto id = Element_Allocate();
		if (id == -1) return -1;
		if (!Human_Init(Human(id), id, player, tribe, job, point, missionCheck, flags))
		{
			FreeHuman(id); return -1;
		}
		CCallbackManager::sTheObjectPtr->ExecuteCallback(4, true, id, 1, 0);
		return id;
	}
	// Original function: FUN_0040b5aa.
	void CHumanArray::FreeHuman(std::int32_t id)
	{
		void* human = Human(id);
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || !Element_Free(id, true)) return;
		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId) != -1) CCallbackManager::sTheObjectPtr->ExecuteCallback(4, false, id, 0, 0);
		Human_Exit(human);
	}
	// Original function: FUN_0041c5f1.
	static void MerchantData_Init(void* merchant)
	{
		Write<std::uint8_t>(merchant, NC2Layout::MerchantOffset::Allocated, 1);
		Write<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount, 0);
		Write<std::int32_t>(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex, -1);
		Write<std::int32_t>(merchant, NC2Layout::MerchantOffset::Houses, -1);
		Write<std::int32_t>(merchant, NC2Layout::MerchantOffset::SecondHouseId, -1);
		Write<std::int32_t>(merchant, NC2Layout::MerchantOffset::TradeAgreementId, -1);
		Write<std::uint32_t>(merchant, NC2Layout::MerchantOffset::ForeignHouseCount, 0);
		Write<std::uint32_t>(merchant, NC2Layout::MerchantOffset::GoodsDelivered, 0);
		Write<std::uint32_t>(merchant, NC2Layout::MerchantOffset::GoodsReceived, 0);
	}
	// Original function: FUN_0041c586; the human argument is unused by Windows.
	std::int32_t HumanMerchantArray_AllocateMerchant(void* manager, std::uint32_t /*human*/)
	{
		const auto id = static_cast<CArrayHandler*>(manager)->Element_Allocate();
		if (id != -1) MerchantData_Init(Read<std::byte*>(manager, NC2Layout::MerchantManagerOffset::Entries) + id * NC2Layout::RecordSize::Merchant);
		return id;
	}
	// Original function: FUN_0041c5af.
	void HumanMerchantArray_FreeMerchant(void* manager, std::int32_t id)
	{
		void* merchant = Read<std::byte*>(manager, NC2Layout::MerchantManagerOffset::Entries) + id * NC2Layout::RecordSize::Merchant;
		if (Read<std::uint8_t>(merchant, NC2Layout::MerchantOffset::Allocated) != 0 && static_cast<CArrayHandler*>(manager)->Element_Free(id, true)) Write<std::uint8_t>(merchant, NC2Layout::MerchantOffset::Allocated, 0);
	}
	// Original function: FUN_0042c817.
	bool Human_IsAttachedToLearnHouse(const void* human)
	{
		const auto id = Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);
		if (id == -1) return false;
		const void* house = House(id);
		if (Read<std::uint32_t>(gStaticHouseTypeData, Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) * NC2Layout::RecordSize::HouseTypeUnsigned) != 4) return false;
		return House_Inhabitants_IsHumanAttachedAsStudent(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
	}
	// Original function: FUN_0042c6aa.
	bool Human_IsAttachedToRoadPoint(const void* human)
	{
		if (!Human_IsJobBuilder(human)) return false;
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 41) return true;
		const void* point = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
		return !Human_IsAttachedToWorkHouse(human) && MapMigPoint_IsInsideMap(point) && RoadBuildManager_IsRoadBuildFlagSetForPlayer(gRoadBuildManager, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), point);
	}
	// Original function: FUN_0042c6f2.
	bool Human_IsAttachedToWallPoint(const void* human)
	{
		if (!Human_IsJobBuilder(human)) return false;
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 42) return true;
		const void* point = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
		if (Human_IsAttachedToWorkHouse(human) || !MapMigPoint_IsInsideMap(point)) return false;
		if (RoadBuildManager_IsRoadBuildFlagSetForPlayer(gWallBuildManager, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), point)) return true;
		const void* micro = GameMap_GetConstMapMicroPointPtr(point);
		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0 || Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) != Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId)) return false;
		std::int32_t engineId;
		switch (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData))
		{
			case SwitchIds::WallLandscapeType::Wall: engineId = NC2LogicExternals::External_Misc_GetEngineWallElementLandscapeId(); break;
			case SwitchIds::WallLandscapeType::GateOpen: engineId = NC2LogicExternals::External_Misc_GetEngineWallGateLandscapeId(0, true); break;
			case SwitchIds::WallLandscapeType::GateClosed: engineId = NC2LogicExternals::External_Misc_GetEngineWallGateLandscapeId(0, false); break;
			default: return false;
		}
		return Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) < Read<std::uint32_t>(NC2LogicExternals::External_Landscape_GetLogicDataPtr(engineId), NC2Layout::LandscapeLogicDataOffset::MaximumValency);
	}
	// Original function: FUN_0042c7b3; active build animations keep their attachment.
	bool Human_IsAttachedToBuildSite(const void* human)
	{
		const auto animation = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction);
		if (animation == 41 || animation == 42 || Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) != -1) return true;
		const void* point = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
		if (!MapMigPoint_IsInsideMap(point)) return false;
		const auto player = Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId);
		return RoadBuildManager_IsRoadBuildFlagSetForPlayer(gRoadBuildManager, player, point) || RoadBuildManager_IsRoadBuildFlagSetForPlayer(gWallBuildManager, player, point);
	}
	// Original function: FUN_00427a6d; shared CRoad/CWallBuildManager::DetachBuilder body.
	void BuildManager_DetachBuilder(void* manager, std::uint32_t player, const void* point)
	{
		void* data = Read<void**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount); ++i)
		{
			if (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointsUnsigned + i * NC2Layout::RecordSize::BuildPointUnsigned) != Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates)) continue;
			if (Read<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssignedUnsigned + i * NC2Layout::RecordSize::BuildPointUnsigned) != 0)
			{
				Write<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssignedUnsigned + i * NC2Layout::RecordSize::BuildPointUnsigned, 0);
				Write<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) - 1u);
			}
			break;
		}
	}

	// Original function: FUN_00453493.
	bool Human_WorkCenterMarker_IsVisible(const void* human)
	{
		if (!Human_WorkCenterMarker_IsAllowed(human)) return false;
		if (Human_IsAttachedToWorkHouse(human))
		{
			if (Read<std::uint32_t>(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), NC2Layout::HouseOffset::DoorEntryPoint) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter)) return false;
		}
		else
		{
			if (Human_IsForcedToHaveWorkHouse(human)) return false;
			if (Human_IsJobBuilder(human) && !Human_IsAttachedToRoadPoint(human) && !Human_IsAttachedToWallPoint(human)) return false;
		}
		return Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) == 0;
	}
	// Original function: FUN_00453518.
	std::int32_t Human_WorkCenterMarker_GetId(const void* human)
	{
		const auto* array = static_cast<const CArrayHandler*>(gMarkerArray);
		for (auto id = array->mFirstScanIndex; id <= array->mLastScanIndex; ++id)
		{
			const void* marker = static_cast<const std::byte*>(gMarkerEntries) + id * NC2Layout::RecordSize::Marker;
			if (Read<std::uint8_t>(marker, NC2Layout::MarkerOffset::Allocated) != 0 && Read<std::uint32_t>(marker, NC2Layout::MarkerOffset::Type) == 1 && Read<std::uint32_t>(marker, NC2Layout::MarkerOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)) return id;
		}
		return -1;
	}
	// Original function: FUN_00453558.
	void Human_WorkCenterMarker_Create(void* human)
	{
		MarkerArray_AllocateMarker(gMarkerArray, nullptr, 1, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
	}
	// Original function: FUN_00453570.
	void Human_WorkCenterMarker_Remove(void* human)
	{
		const auto id = Human_WorkCenterMarker_GetId(human); if (id != -1) MarkerArray_FreeMarker(gMarkerArray, nullptr, id);
	}
	// Original function: FUN_00453587.
	void Human_WorkCenterMarker_Update(void* human)
	{
		const auto id = Human_WorkCenterMarker_GetId(human);
		if (id != -1)
		{
			void* marker = static_cast<std::byte*>(gMarkerEntries) + id * NC2Layout::RecordSize::Marker;
			if (Human_WorkCenterMarker_IsVisible(human)) Marker_MapPosition_Set(marker, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);
			else MarkerArray_FreeMarker(gMarkerArray, nullptr, Read<std::int32_t>(marker, NC2Layout::MarkerOffset::MarkerId));
		}
		else if (Human_WorkCenterMarker_IsVisible(human)) Human_WorkCenterMarker_Create(human);
	}
	// Original function: FUN_0043472e; Windows outlined work-house assignment.
	void Human_l_AssignWorkHouse(void* human, std::uint32_t house)
	{
		Write(human, NC2Layout::HumanOffset::WorkAttachmentHouseId, house);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseRevision, Read<std::uint32_t>(House(house), NC2Layout::HouseOffset::UniqueId));
		Human_SetupCurrentJobType(human);
		Human_Production_Update(human);
	}
	// Original function: FUN_004419c8.
	bool Human_HouseMisc_Enter(void* human)
	{
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideHouseFlag) != 0) return false;
		std::uint32_t point = 0;
		if (!Human_GetMapPosition(human, &point)) return false;
		const void* micro = GameMap_GetConstMapMicroPointPtr(&point);
		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) == 0) return false;
		const auto type = Read<std::int32_t>(House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId)), NC2Layout::HouseOffset::LogicType);
		if (type >= 10 && type <= 11) return false;
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::InsideHouseFlag, 1);
		return true;
	}
	// Original function: FUN_0045c762.
	bool Human_l_StartAtomicAnimation(void* human, std::uint32_t action, std::int32_t direction)
	{
		if (action == 1 || !Human_l_IsAtomicInterruptable(human)) return false;
		auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		std::uint32_t animation;
		for (;;)
		{
			const auto* descriptor = static_cast<const std::byte*>(gStaticJobTypeData) + job * NC2Layout::RecordSize::JobType;
			if (Read<std::uint8_t>(descriptor, NC2Layout::StaticJobTypeOffset::AllowedAtomicsUnsigned + action) == 0) return false;
			animation = gStaticTribeTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)].atomicIds[job][action];
			if (animation != NXBasics::IntegerRange::UInt32Maximum) break;
			job = Read<std::uint32_t>(descriptor, NC2Layout::StaticJobTypeOffset::BaseJob);
			if (job == 0) return false;
		}
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationFrame, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationStartCount, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationStartCount) + 1u);
		Write(human, NC2Layout::HumanOffset::AtomicAnimationId, animation);
		Write(human, NC2Layout::HumanOffset::AtomicAction, action);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag, 1);
		void* moveable = Moveable(Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId));
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
		{
			const auto preferred = gStaticAtomicAnimationData[animation].startDirection;
			if (preferred != -1) MapMoveable_Attachment_StartTurn(moveable, preferred);
			else if (direction != -1) MapMoveable_Attachment_StartTurn(moveable, direction);
		}
		return true;
	}
	// Original function: FUN_0043819b.
	bool Human_Init(void* human, std::int32_t id, std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags)
	{
		const auto selectedTribe = NRemaster::GameSetup::Startup::ResolveStartupTribe(player, tribe);
		// Keep special map units whose profession has no graphics in the selected civilization.
		if (selectedTribe != tribe && job < 56 && gStaticTribeTypeData != nullptr && gStaticTribeTypeData[selectedTribe].allowedJobs[job] != 0) tribe = selectedTribe;
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 || GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId)) == 0) return false;
		std::memset(human, 0, NC2Layout::RecordSize::Human);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag, 1);
		Write(human, NC2Layout::HumanOffset::HumanId, id);
		Write(human, NC2Layout::HumanOffset::UniqueId, Game_UniqueId_Generate());
		Write(human, NC2Layout::HumanOffset::PlayerId, player);
		for (const auto offset : { NC2Layout::HumanOffset::HomeHouseIdUnsigned, NC2Layout::HumanOffset::WorkAttachmentHouseIdUnsigned, NC2Layout::HumanOffset::VehicleConstructionHouseIdUnsigned, NC2Layout::HumanOffset::AttachedVehicleIdUnsigned, NC2Layout::HumanOffset::MerchantIdUnsigned, NC2Layout::HumanOffset::FatherIdUnsigned, NC2Layout::HumanOffset::MotherIdUnsigned, NC2Layout::HumanOffset::SpouseIdUnsigned, NC2Layout::HumanOffset::WaitingHumanIdUnsigned, NC2Layout::HumanOffset::ChildIdUnsigned }) Write<std::int32_t>(human, offset, -1);
		const auto moveableId = MapMoveableArray_AllocateMapMoveable(gMapMoveableArrayStorage.manager, 1, id);
		Write(human, NC2Layout::HumanOffset::MapMoveableId, moveableId);
		if (moveableId == -1) return false;
		void* moveable = Moveable(moveableId);
		MapMoveable_Attachment_AttachToMap(moveable, point);
		const void* micro = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));
		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0 && Read<std::uint32_t>(House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId)), NC2Layout::HouseOffset::ConstructionState) == 3) Human_HouseMisc_Enter(human);
		Write(human, NC2Layout::HumanOffset::TribeType, tribe);
		// Original outlined operation 00438CEC: clear cargo before notifying its change.
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood, 0);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapMigPointOffset::Coordinates));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		Human_WorkCenterMarker_Update(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::ShoeCondition, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition, 0);
		Human_ProductionGoodEnableFlags_Construct(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::ProductionSelectionMode);
		std::memset(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::JobExperienceSigned, 0, 312);
		std::memset(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::AllowedJobFlagsSigned, 0, 56);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 1);
		std::memset(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::AllowedEquipmentFlagsSigned, 0, 56);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::JobSelectionEnabledFlag, 0);
		Write(human, NC2Layout::HumanOffset::MissionCheckId, missionCheck);
		Write(human, NC2Layout::HumanOffset::MiscFlags, flags);
		if (tribe == 5 || tribe == 6)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode, (flags & NXBasics::BitMask::Bit16) != 0 ? 2u : 1u);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::MilitaryWorldRefreshmentFlag, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor, 2);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 1);
		}
		else if (static_cast<std::int32_t>(job) >= 31 && static_cast<std::int32_t>(job) <= 41)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode, 2);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::MilitaryWorldRefreshmentFlag, 1);
			switch (job)
			{
				case SwitchIds::HumanJob::SoldierUnarmed: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 1); break;
				case SwitchIds::HumanJob::SoldierSpearWooden: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 4); break;
				case SwitchIds::HumanJob::SoldierSpearIron: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 5); Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedIronSpearSoldierFlag, 1); break;
				case SwitchIds::HumanJob::SoldierSwordShort: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 7); break;
				case SwitchIds::HumanJob::SoldierSwordLong: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 8); Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedLongSwordSoldierFlag, 1); break;
				case SwitchIds::HumanJob::SoldierBowShort: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 16); break;
				case SwitchIds::HumanJob::SoldierBowLong: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 17); Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedLongBowSoldierFlag, 1); break;
				default: job = 31; Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 1); break;
			}
		}
		else if (static_cast<std::int32_t>(job) >= 42 && static_cast<std::int32_t>(job) <= 47)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode, 2);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::MilitaryWorldRefreshmentFlag, 1);
			switch (job)
			{
				case SwitchIds::HumanJob::HeroSpearSIEGFRIED: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 6); Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor, 4); break;
				case SwitchIds::HumanJob::HeroSwordBJARNI: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 9); Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor, 3); break;
				case SwitchIds::HumanJob::HeroSaberHATSCHI: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 12); Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor, 3); break;
				case SwitchIds::HumanJob::HeroAxe: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 15); Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor, 3); break;
				case SwitchIds::HumanJob::HeroineBowXENA: Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 18); Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor, 2); break;
				default: job = 42; Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 3); Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor, 1); break;
			}
			const auto experience = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperienceSlot70);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperienceSlot70, experience > 9995u ? 10000u : experience + 5u);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) | NXBasics::BitMask::Bit0AndBit6);
			if (tribe != 2 || job != 47) Write<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) | NXBasics::BitMask::Bit7);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::MiscFlagsByte1, Read<std::uint8_t>(human, NC2Layout::HumanOffset::MiscFlagsByte1) | NXBasics::BitMask::Bits3Through4);
			Human_Equipment_SetShoeType(human, 30);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::ShoeCondition, 10000);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode, 2);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::MilitaryWorldRefreshmentFlag, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, job == 5 ? 2u : 1u);
		}
		Human_SetJobType(human, job);
		Human_SetupPlayerSpecificStuff(human);
		Human_Production_Setup(human);
		Write(human, NC2Layout::HumanOffset::Hitpoints, Human_GetHumanAttributeMaximum_Hitpoints(human));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::Food, Game_Random_GetNumber() % 5000u + 5000u);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::Religion, Game_Random_GetNumber() % 5000u + 5000u);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::Social, Game_Random_GetNumber() % 5000u + 5000u);
		const auto stamina = Game_Random_GetNumber() % 5000u + 5000u;
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		Write(human, NC2Layout::HumanOffset::Stamina, stamina);
		Human_ResetUserCommandArray(human);
		moveable = Moveable(Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId));
		void* pathfinder = GetUnsafePathfinderPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId));
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingTicksPerStep, 8);
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick, 1250);
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningTicksPerDirection, 1);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::AllowVerticalDirectionsFlag, 1);
		Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::IncludeVerticalDirectionsFlag, 1);
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId, -1);
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideId, -1);
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		if (Human_WorkCenterMarker_IsVisible(human)) Human_WorkCenterMarker_Create(human);
		GameMap_Exploration_DoExplore(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), point, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ExplorationRadius));
		Human_l_StartAtomicAnimation(human, 2, -1);
		if (Human_IsAgeBaby(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount, 10); // Original constant 004F3C34.
			if (gGameTime.tick > 1u)
			{
				const auto offset = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::BabiesBornUnsigned;
				Write<std::uint32_t>(gGameControl, offset, Read<std::uint32_t>(gGameControl, offset) + 1u);
			}
		}
		if (Read<std::uint32_t>(gGameControl, NC2Layout::GameControlOffset::DifficultyLevel) == 2)
		{
			if (gGameStaticVars.networkGame != 0 || NRemaster::CivilizationBuildings::IsActive())
			{
				if (static_cast<std::int32_t>(player) < 20 && Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TypeUnsigned) == 1) Write<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) | 1u);
			}
			else if (player == 0) Write<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) | 1u);
		}
		GameAssistantSingleHumanData_System_Init(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::AssistantData);
		return true;
	}

	// Original function: FUN_004418ec.
	void Human_DetachFromHomeHouse(void* human)
	{
		const auto circulation = Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode);
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 5 && circulation != 0 && circulation != 1)
		{
			Human_StopCurrentGoodCirculationMode(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		}
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId, NXBasics::IntegerRange::UInt32Maximum);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseRevision, 0);
	}

	// Original function: FUN_00441925.
	void Human_DetachFromWorkHouse(void* human)
	{
		const auto produced = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0 && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood) == produced || produced == 65 || produced == 58 || produced == 57))
		{
			Human_StopCurrentGoodCirculationMode(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		}
		Human_DetachFromWorkHouse2(human);
	}

	// Original function: FUN_00442028; setup precedes production update.
	void Human_DetachFromWorkHouse2(void* human)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId, NXBasics::IntegerRange::UInt32Maximum);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseRevision, 0);
		Human_SetupCurrentJobType(human);
		Human_Production_Update(human);
	}

	// Original function: FUN_00441962.
	void Human_DetachFromVehicle(void* human)
	{
		const auto circulation = Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode);
		if (circulation != 0 && circulation != 1)
		{
			Human_StopCurrentGoodCirculationMode(human); Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		}
		void* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
		if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision) && Vehicle_Passengers_GetAttachedHumanSlot(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)) != -1) Vehicle_Passengers_DetachHuman(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write<std::int32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId, -1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision, 0);
	}
	// Original function: FUN_00441f8c; passenger state changes before detaching from the map.
	std::uint32_t Human_VehicleMisc_Enter(void* human)
	{
		auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
		if (!Vehicle_Passengers_MoveIn_h(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId))) return 0;
		MapMoveable_Attachment_DetachFromMap(Moveable(Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId)));
		Human_Target_Reset(human);
		Human_Target_ResetSecondaryTarget(human);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag, 1);
		Human_WorkCenterMarker_Update(human);
		return 1;
	}

	// Original function: FUN_00441fe2.
	void Human_VehicleMisc_Leave(void* human, const void* point)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
		MapMoveable_Attachment_AttachToMap(Moveable(Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId)), point);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates));
		Human_WorkCenterMarker_Update(human);
	}
	// Original function: FUN_00438a26.
	void Human_Detach(void* human, bool forceVehicleDetach)
	{
		if (Human_IsAttachedToHomeHouse(human)) House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1);
		if (Human_IsAttachedToLearnHouse(human)) House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 8);
		if (Human_IsAttachedToWorkHouse(human)) House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Human_IsJobBuilder(human) ? 4u : 2u);
		if (Human_IsAttachedToRoadPoint(human)) BuildManager_DetachBuilder(gRoadBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);
		if (Human_IsAttachedToWallPoint(human)) BuildManager_DetachBuilder(gWallBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);
		const auto merchant = Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId);
		if (merchant != -1) HumanMerchantArray_FreeMerchant(gHumanMerchantArray, merchant);
		const auto vehicleId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId);
		if (vehicleId != NXBasics::IntegerRange::UInt32Maximum)
		{
			void* vehicle = CVehicleArray::mStaticVars + vehicleId * CVehicleArray::ElementSize;
			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision))
			{
				if (forceVehicleDetach)
				{
					if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 && Vehicle_Door_IsEntryPointOnLand(vehicle, 0)) Human_VehicleMisc_Leave(human, Human_GetMapPositionRef(human));
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag, 0);
					Human_DetachFromVehicle(human);
				}
				else if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)) Human_DoAddNewUserCommand(human, 39, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
			}
		}
		for (const auto parentOffset : { 0xB18u, 0xB14u })
		{
			const auto parent = Read<std::int32_t>(human, parentOffset);
			if (parent != -1)
			{
				Write<std::int32_t>(Human(parent), NC2Layout::HumanOffset::ChildId, -1); Write<std::int32_t>(human, parentOffset, -1);
			}
		}
		const auto child = Read<std::int32_t>(human, NC2Layout::HumanOffset::ChildId);
		if (child != -1)
		{
			Write<std::int32_t>(Human(child), Human_IsSexFemale(human) ? NC2Layout::HumanOffset::MotherIdUnsigned : NC2Layout::HumanOffset::FatherIdUnsigned, -1); Write<std::int32_t>(human, NC2Layout::HumanOffset::ChildId, -1);
		}
		const auto spouse = Read<std::int32_t>(human, NC2Layout::HumanOffset::SpouseId);
		if (spouse != -1)
		{
			Write<std::int32_t>(Human(spouse), NC2Layout::HumanOffset::SpouseId, -1);
			Write<std::int32_t>(Human(spouse), NC2Layout::HumanOffset::WaitingHumanId, -1);
			Write<std::int32_t>(human, NC2Layout::HumanOffset::SpouseId, -1);
			Write<std::int32_t>(human, NC2Layout::HumanOffset::WaitingHumanId, -1);
		}
		const auto fiance = Read<std::int32_t>(human, NC2Layout::HumanOffset::WaitingHumanId);
		if (fiance != -1)
		{
			if (Read<std::uint8_t>(Human(fiance), NC2Layout::HumanOffset::AllocatedFlag) != 0) Human_StopWaitingForOtherHuman(Human(fiance));
			Human_StopWaitingForOtherHuman(human);
		}
	}
	// Original function: FUN_0045d0a8.
	static bool Human_l_Item_IsItemUnused(const void* human, std::int32_t slot)
	{
		return slot != -1 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InventorySlots + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HumanInventorySlot) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InventoryConditions + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HumanInventorySlot) == 100;
	}
	// Original function: FUN_004386d9.
	void Human_Exit(void* human)
	{
		// Shared System_Exit body at 00430CA4 clears the assistant allocation flag.
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::AssistantData, 0);
		if (Read<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag) == 0)
		{
			const auto playerOffset = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;
			Write<std::uint32_t>(gGameControl, playerOffset + NC2Layout::GameControlPlayerOffset::HumansDiedUnsigned, Read<std::uint32_t>(gGameControl, playerOffset + NC2Layout::GameControlPlayerOffset::HumansDiedUnsigned) + 1u);
			if (Human_IsJobSoldier(human)) Write<std::uint32_t>(gGameControl, playerOffset + NC2Layout::GameControlPlayerOffset::SoldiersDiedUnsigned, Read<std::uint32_t>(gGameControl, playerOffset + NC2Layout::GameControlPlayerOffset::SoldiersDiedUnsigned) + 1u);
			else HappinessManager_Messages_CivilistDied(gHappinessManager, human);
		}
		Human_StopCurrentGoodCirculationMode(human);
		Human_Detach(human, true);
		const auto moveableId = Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId);
		if (Read<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag) == 0 && moveableId != -1 && Read<std::uint8_t>(Moveable(moveableId), NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
		{
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x25, true, 4, MapMigPoint_GetLong(Human_GetMapPositionRef(human)), 0);
			if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit14) == 0 && !(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 3 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 32))
			{
				const auto cadaver = NC2LogicExternals::External_Misc_GetEngineCadaverLandscapeId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), 87);
				if (cadaver != -1)
				{
					const void* micro = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));
					bool blocked = false;
					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0)
					{
						const auto engineId = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(Human_GetMapPositionRef(human));
						const auto* area0 = NC2LogicExternals::External_Landscape_GetAreaPtr(engineId, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0, 0);
						const auto* area1 = NC2LogicExternals::External_Landscape_GetAreaPtr(engineId, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0, 1);
						const auto* area3 = NC2LogicExternals::External_Landscape_GetAreaPtr(engineId, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0, 3);
						blocked = area0 != nullptr || area1 != nullptr || area3 != nullptr;
					}
					if (!blocked)
					{
						Logic_EntryPoint_Edit_Start();
						Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(Human_GetMapPositionRef(human), 87, 1, cadaver, 0, -1);
						Logic_EntryPoint_Edit_Stop();
					}
				}
				const auto drop = [&] (std::uint32_t good)
				{
					Map_GoodTool_PutOnMap(good, 1, Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapMigPointOffset::Coordinates), 10, true);
				};
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0) drop(Human_GetCarriedGood(human));
				// Windows multiplies as a DWORD before signed division; preserve overflow bits.
				const auto percentage = [&] (std::uint32_t offset)
				{
					return NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(human, offset) * 100u) / 10000;
				};
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedShoes) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::ShoeCondition) != 0 && percentage(NC2Layout::HumanOffset::ShoeCondition) >= 90) drop(Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedShoes));
				const auto tool = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools);
				if (tool != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition) != 0 && (tool == 31 || tool == 32) && Human_GetPercentageConditionOfTool(human) >= 90) drop(tool);
				const auto armor = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor);
				if (armor != 0) drop(Read<std::uint32_t>(gStaticArmorTypeData, armor * NC2Layout::RecordSize::ArmorTypeUnsigned + NC2Layout::StaticArmorTypeOffset::GoodTypeUnsigned));
				const auto weaponGood = Read<std::uint32_t>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)], Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon) * NC2Layout::RecordSize::WeaponTypeUnsigned + NC2Layout::StaticWeaponTypeOffset::GoodTypeUnsigned);
				if (weaponGood != 0 && Human_IsJobSoldier(human)) drop(weaponGood);
				for (std::uint32_t slot = 0; slot < 4; ++slot) if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InventorySlots + slot * NC2Layout::RecordSize::HumanInventorySlot) != 0 && Read<std::int32_t>(human, NC2Layout::HumanOffset::InventoryConditions + slot * NC2Layout::RecordSize::HumanInventorySlot) > 0 && Human_l_Item_IsItemUnused(human, slot)) drop(Human_Inventory_GetGoodInSlot(human, slot));
			}
		}
		Human_WorkCenterMarker_Remove(human);
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0)
		{
			const auto id = Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId);
			if (id != -1) MapMoveableArray_FreeMapMoveable(gMapMoveableArrayStorage.manager, id);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag, 0);
		}
	}
}

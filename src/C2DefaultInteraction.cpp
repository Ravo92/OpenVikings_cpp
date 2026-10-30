#include "C2AnimalAccess.h"
#include "C2DefaultInteraction.h"
#include "GameProfile.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMerchantAccess.h"
#include "C2HumanUserControl.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticHouseTypeData.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		const std::byte* Human(std::uint32_t id)
		{

			return CHumanArray::mStaticVars + id * CHumanArray::RuntimeElementSize;
		}

		std::byte* House(std::uint32_t id)
		{

			return CHouseArray::mStaticVars + id * CHouseArray::RuntimeElementSize;
		}

		const std::byte* Vehicle(std::uint32_t id)
		{

			return CVehicleArray::mStaticVars + id * CVehicleArray::RuntimeElementSize;
		}

		const std::byte* Moveable(const void* human)
		{

			return gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		}

		std::uint8_t Continent(const void* point)
		{

			return Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId);
		}

		void Prepare(void* command, std::uint8_t opcode, const void* actor, std::uint32_t first, std::uint32_t second = 0)
		{
			std::memset(command, 0, 20);
			Write(command, NC2Layout::UserNetworkCommandOffset::Opcode, opcode);
			Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(actor, NC2Layout::ActorIdentityOffset::Id));
			Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(actor, NC2Layout::ActorIdentityOffset::UniqueId));
			Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, first);
			Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, second);
		}

		// Windows outlined helper 00436B16; the separate original semantic name remains unresolved.
		const void* Merchant(const void* human)
		{

			return Human_IsJobTrader(human) ? HumanMerchantArray_GetMerchantDataPtr(gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId)) : nullptr;
		}
	}

	// Original function: FUN_00412248.
	bool StaticDependencies_Item_IsChest(std::int32_t landscape)
	{

		return landscape == 85 || landscape == 86;
	}

	// Original function: FUN_0041225d.
	bool StaticDependencies_Item_IsAbleToOpenChest(std::int32_t job, std::int32_t landscape)
	{
		if (landscape == 85)
			return job >= 5 && job < 48;

		if (landscape == 86)
			return job == 30 || (job >= 42 && job <= 47);

		return false;
	}

	// Original function: FUN_0042789b; Windows folds the matching wall-manager method into this entry.
	bool RoadBuildManager_IsRoadBuildFlagSetForPlayer(const void* manager, std::int32_t player, const void* point)
	{
		if (player >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount))
			return false;

		const auto* data = Read<std::byte* const*>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

		if (data == nullptr)
			return false;

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount); ++i)
		{
			if (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointsUnsigned + i * NC2Layout::RecordSize::BuildPointUnsigned) == Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates))
				return true;
		}

		return false;
	}

	// Original function: FUN_0042853b; Windows folds GetWallBuildFlagPlayerId into this entry.
	std::int32_t RoadBuildManager_GetRoadBuildFlagPlayerId(const void* manager, const void* point)
	{
		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			if (Read<std::byte* const*>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] != nullptr && RoadBuildManager_IsRoadBuildFlagSetForPlayer(manager, player, point))
				return player;
		}

		return -1;
	}

	// Original function: FUN_0042f68a.
	const void* Animal_GetConstMapMicroPointPtr(const void* animal)
	{

		return GameMap_GetConstMapMicroPointPtr(gMapMoveableArrayStorage.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable + 0x12u);
	}

	// Original function: FUN_00434c41.
	std::uint32_t House_JobMisc_GetPrimaryWorkerJobType(const void* house)
	{
		const auto* data = House_GetHouseTypeDataPtr(house);

		return data->mainType != 2 && data->workerCount > 0 ? static_cast<std::uint32_t>(data->workers[0].jobType) : 0;
	}

	// Original function: FUN_00434c62.
	bool House_JobMisc_CanJobBeAttachedAsWorker(const void* house, std::uint32_t job)
	{
		const auto* data = House_GetHouseTypeDataPtr(house);

		if (data->mainType == 2)
			return false;

		std::int32_t index = 0;

		while (index < data->workerCount && static_cast<std::uint32_t>(data->workers[index].jobType) != job)
			++index;

		if (index == data->workerCount)
			return false;

		return House_Inhabitants_GetNumberOfAttachedWorkersOfPassedJob(house, job) < data->workers[index].maximumWorkers;
	}

	// Original function: FUN_00435605; the complete nine-entry Windows selector is 00436216..00436239.
	std::uint32_t Human_UserControl_GetDefaultInteractionCommand_SingleHuman(const void* human, const void* target, void* command, void* secondary, bool modifier)
	{
		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit5) != 0 || Read<std::uint8_t>(Moveable(human), NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0)
			return 0;

		const auto continent = Continent(Human_GetMapPositionRef(human));
		SUserCommandsForHumanHandler available;
		Human_UserControl_GetCommands(human, &available, false, false);
		const auto* point = static_cast<const std::byte*>(target) + NC2Layout::DisplayObjectReferenceOffset::MapPosition;
		const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		const auto sendTo = [&]()
		{

			return Human_UserControl_GetDefaultInteractionCommand_TrySendTo(human, command, point, false, modifier);
		};

		switch (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type))
		{
			case SwitchIds::MapObjectUnderCursor::Human:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId);
				const auto* other = GetHumanPtr(targetId);

				if (Human_IsAgeAdult(other) && available.IsCommandAllowed(41) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(other, NC2Layout::HumanOffset::PlayerId)) == 3)
				{
					Prepare(command, 43, human, targetId, Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId));
					return 41;
				}

				if (Read<std::uint8_t>(Moveable(other), NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0)
					return 0;
				// Windows queries the active target's continent and still returns no command.
				Continent(Human_GetMapPositionRef(other));
				return 0;
			}
			case SwitchIds::MapObjectUnderCursor::Animal:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::AnimalId);
				const auto* animal = GetAnimalPtr(targetId);
				const auto* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;

				if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0 || Read<std::uint8_t>(Animal_GetConstMapMicroPointPtr(animal), NC2Layout::MapMicroPointOffset::ContinentId) != continent)
					return 0;

				if (Human_IsJobScout(human) && static_cast<const SStaticAnimalTypeData*>(gStaticAnimalTypeData)[Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) - NRemaster::GetFirstAnimalTribe()].warrantable != 0)
					return sendTo();

				if (available.IsCommandAllowed(43) && Animal_Misc_CanBeAttackedByHuman(animal, id, false))
				{
					Prepare(command, 45, human, targetId, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::UniqueId));
					return 43;
				}

				return 0;
			}
			case SwitchIds::MapObjectUnderCursor::Vehicle:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::VehicleId);
				const auto* vehicle = Vehicle(targetId);

				if (available.IsCommandAllowed(44) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId)) == 3)
				{
					Prepare(command, 46, human, targetId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
					return 44;
				}

				if (available.IsCommandAllowed(12) && Vehicle_Passengers_CanHumanBeAttached(vehicle, id))
				{
					Prepare(command, 38, human, targetId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
					return 12;
				}

				return 0;
			}
			case SwitchIds::MapObjectUnderCursor::House:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HouseId);
				auto* house = House(targetId);
				const auto houseCommand = [&](std::uint8_t opcode, std::uint32_t action)
				{
					Prepare(command, opcode, human, targetId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));

					return action;
				};

				if (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId) != player)
				{
					if (available.IsCommandAllowed(42) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId)) == 3)
						return houseCommand(44, 42);

					if (available.IsCommandAllowed(22) && Human_IsJobTrader(human) && MerchantData_Tool_CanHouseBeAdded(Merchant(human), id, targetId))
						return houseCommand(36, 22);

					if (available.IsCommandAllowed(23) && Human_IsJobTrader(human) && MerchantData_IsHouseInArray(Merchant(human), targetId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId)))
						return houseCommand(37, 23);

					return 0;
				}

				if (Continent(house + NC2Layout::HouseOffset::MapPosition) != continent)
					return 0;

				if (available.IsCommandAllowed(20) && House_Inhabitants_CanHumanBeAttachedAsBuilder(house, id) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId))
					return houseCommand(30, 20);

				if (available.IsCommandAllowed(10) && House_Inhabitants_CanHumanBeAttachedAsHomie(house, id) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId))
					return houseCommand(26, 10);

				if (available.IsCommandAllowed(22) && Human_IsJobTrader(human) && !MerchantData_IsHouseInArray(Merchant(human), targetId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId)))
					return houseCommand(36, 22);

				if (available.IsCommandAllowed(23) && Human_IsJobTrader(human) && MerchantData_IsHouseInArray(Merchant(human), targetId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId)))
					return houseCommand(37, 23);

				if (available.IsCommandAllowed(16) && House_Inhabitants_CanHumanBeAttachedAsWorker(house, id) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) && Human_UserControl_GetNextJobType(human) == 24 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 39)
					return houseCommand(28, 16);

				if (available.IsCommandAllowed(18) && House_Inhabitants_CanHumanBeAttachedAsStudent(house, id) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId))
					return houseCommand(32, 18);

				if (available.IsCommandAllowed(24) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId))
				{
					const auto job = House_JobMisc_GetPrimaryWorkerJobType(house);

					if (job != 0 && job != Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) && Human_IsAbleToDoJob(human, job) && House_JobMisc_CanJobBeAttachedAsWorker(house, job) && Human_UserControl_GetNextJobType(human) != 25)
					{
						Prepare(command, 21, human, job);
						Prepare(secondary, 28, human, targetId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
						return 24;
					}
				}

				if (available.IsCommandAllowed(16) && House_Inhabitants_CanHumanBeAttachedAsWorker(house, id) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId))
					return houseCommand(28, 16);

				return 0;
			}
			case SwitchIds::MapObjectUnderCursor::Guide:
				return sendTo();
			case SwitchIds::MapObjectUnderCursor::Landscape:
			{
				if (available.IsCommandAllowed(45) && Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId) != -1 && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId)) == 3)
				{
					Prepare(command, 47, human, MapMigPoint_GetLong(point));
					return 45;
				}

				if (Continent(point) != continent)
					return 0;

				const auto* landscape = NC2LogicExternals::External_Landscape_GetLogicDataPtr(Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::LandscapeId));
				const auto type = Read<std::int32_t>(landscape, NC2Layout::EngineLandscapeLogicViewOffset::LogicType);

				if (Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) == -1 && type == gStaticGoodTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)].landscapeToHarvest)
				{
					Prepare(command, 17, human, MapMigPoint_GetLong(point));
					return 28;
				}

				for (std::uint32_t good = 0; good < 66; ++good)
				{
					if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlagsSigned + good) != 0 && good != Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) && Human_IsAbleToProduceGoodNow(human, good) && type == gStaticGoodTypeData[good].landscapeToHarvest)
					{
						Prepare(command, 22, human, good);

						if (Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) == -1)
							Prepare(secondary, 17, human, MapMigPoint_GetLong(point));

						return 25;
					}
				}

				if (available.IsCommandAllowed(26))
				{
					const auto good = gStaticDependencyGoodByLandscape[type];

					if (good != 0 && Human_IsAbleToEquipGoodNow(human, good, true))
					{
						const auto kind = Tool_GoodEquipmentType_Convert(good);

						if (kind < 1 || kind > 5)
							return 0;

						if (kind != 5 && Human_HasEquippedItemType(human, good, -1))
							return 0;

						constexpr std::uint8_t opcodes[5]{ 81, 82, 80, 79, 83 };
						Prepare(command, opcodes[kind - 1], human, good, NXBasics::IntegerRange::UInt32Maximum);
						return 26;
					}
				}

				if (StaticDependencies_Item_IsChest(type) && StaticDependencies_Item_IsAbleToOpenChest(Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType), type))
				{
					Prepare(command, 18, human, MapMigPoint_GetLong(point));
					auto destination = MapMigPoint_GetLong(point);
					MapMigPoint_MoveCursorUnwrapped(destination, 2);

					if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&destination), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
					{
						MapMigPoint_MoveCursorUnwrapped(destination, 0);

						if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&destination), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
							destination = MapMigPoint_GetLong(point);
					}

					Prepare(secondary, 4, human, destination);
					return 30;
				}

				if (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId) == player && type >= 82 && type <= 83 && Human_IsJobBuilder(human) && Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::LandscapeVariant) < Read<std::uint32_t>(NC2LogicExternals::External_Landscape_GetLogicDataPtr(Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::LandscapeId)), NC2Layout::LandscapeLogicViewOffset::MaximumValency))
				{
					Prepare(command, 17, human, MapMigPoint_GetLong(point));
					return 28;
				}

				return sendTo();
			}
			case SwitchIds::MapObjectUnderCursor::RoadBuildPlace:
			case SwitchIds::MapObjectUnderCursor::WallBuildPlace:
			{
				if (Continent(point) != continent)
					return 0;

				if (available.IsCommandAllowed(20))
				{
					const auto* manager = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::RoadBuildPlace ? gRoadBuildManager : gWallBuildManager;

					if (RoadBuildManager_GetRoadBuildFlagPlayerId(manager, point) == static_cast<std::int32_t>(player) && Continent(point) == Continent(Moveable(human) + 0x12))
					{
						Prepare(command, 17, human, MapMigPoint_GetLong(point));
						return 28;
					}
				}

				return sendTo();
			}
			case SwitchIds::MapObjectUnderCursor::Position:
			{
				if (Continent(point) != continent)
					return 0;

				if (modifier)
				{
					if (available.IsCommandAllowed(28) && Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) == -1 && Continent(point) == Continent(Moveable(human) + 0x12) && (player >= 16 || (Read<std::uint16_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) != 0))
					{
						Prepare(command, 17, human, MapMigPoint_GetLong(point));
						return 28;
					}
				}
				else if (available.IsCommandAllowed(32) && player < 16 && (Read<std::uint16_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) == 0)
				{
					Prepare(command, 20, human, MapMigPoint_GetLong(point));
					return 32;
				}

				return sendTo();
			}
			default:

				return 0;
		}
	}

	// Original function: FUN_004362e9; count and modifier are unused in this Windows body.
	bool Human_UserControl_GetDefaultInteractionCommand_MultipleHuman(const void* human, std::uint32_t* action, const void* target, void* command, std::uint32_t index, std::uint32_t, bool)
	{
		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit5) != 0)
			return false;

		SUserCommandsForHumanHandler available;
		Human_UserControl_GetCommands(human, &available, true, true);
		const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		const auto commitTarget = [&](std::uint32_t wantedAction, std::uint8_t opcode, std::uint32_t targetId, const void* other)
		{
			if (index != 0)
				return *action == wantedAction && Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode) == opcode && Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1) == targetId;

			Prepare(command, opcode, human, targetId, Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId));
			*action = wantedAction;

			return true;
		};

		switch (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type))
		{
			case SwitchIds::MapObjectUnderCursor::Human:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId);
				const auto* other = GetHumanPtr(targetId);

				if (Human_IsAgeAdult(other) && available.IsCommandAllowed(41) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(other, NC2Layout::HumanOffset::PlayerId)) == 3)
					return commitTarget(41, 43, targetId, other);

				return false;
			}
			case SwitchIds::MapObjectUnderCursor::Animal:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::AnimalId);
				const auto* animal = GetAnimalPtr(targetId);
				return available.IsCommandAllowed(43) && Animal_Misc_CanBeAttackedByHuman(animal, id, false) && commitTarget(43, 45, targetId, animal);
			}
			case SwitchIds::MapObjectUnderCursor::Vehicle:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::VehicleId);
				const auto* vehicle = Vehicle(targetId);

				if (available.IsCommandAllowed(44) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId)) == 3)
					return commitTarget(44, 46, targetId, vehicle);

				if (available.IsCommandAllowed(12) && Vehicle_Passengers_CanHumanBeAttached(vehicle, id))
				{
					Prepare(command, 38, human, targetId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
					*action = 12;
					return true;
				}

				return false;
			}
			case SwitchIds::MapObjectUnderCursor::House:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HouseId);
				auto* house = House(targetId);

				if (available.IsCommandAllowed(42) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId)) == 3)
					return commitTarget(42, 44, targetId, house);

				if (available.IsCommandAllowed(16) && House_Inhabitants_CanHumanBeAttachedAsWorker(house, id) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId))
					return commitTarget(16, 28, targetId, house);

				if (available.IsCommandAllowed(20) && House_Inhabitants_CanHumanBeAttachedAsBuilder(house, id) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId))
					return commitTarget(20, 30, targetId, house);

				if (available.IsCommandAllowed(18) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 39 && House_Inhabitants_CanHumanBeAttachedAsStudent(house, id) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId))
					return commitTarget(18, 32, targetId, house);

				if (available.IsCommandAllowed(10) && House_Inhabitants_CanHumanBeAttachedAsHomie(house, id) && targetId != Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId))
					return commitTarget(10, 26, targetId, house);

				return false;
			}
			case SwitchIds::MapObjectUnderCursor::Landscape:
			{
				const auto* landscape = NC2LogicExternals::External_Landscape_GetLogicDataPtr(Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::LandscapeId));

				if (available.IsCommandAllowed(26))
				{
					const auto good = gStaticDependencyGoodByLandscape[Read<std::uint32_t>(landscape, NC2Layout::EngineLandscapeLogicViewOffset::LogicType)];

					if (good != 0 && Human_IsAbleToEquipGoodNow(human, good, true))
					{
						const auto kind = Tool_GoodEquipmentType_Convert(good);

						if (kind >= 1 && kind <= 5 && (kind == 5 || !Human_HasEquippedItemType(human, good, -1)))
						{
							constexpr std::uint8_t opcodes[5]{ 81, 82, 80, 79, 83 };
							Prepare(command, opcodes[kind - 1], human, good, NXBasics::IntegerRange::UInt32Maximum);
							*action = 26;
							return true;
						}
					}
				}

				const auto type = Read<std::int32_t>(landscape, NC2Layout::EngineLandscapeLogicViewOffset::LogicType);

				if (StaticDependencies_Item_IsChest(type) && StaticDependencies_Item_IsAbleToOpenChest(Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType), type))
				{
					Prepare(command, 18, human, Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::MapPosition));
					*action = 30;
					return true;
				}

				if (available.IsCommandAllowed(45) && Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId) != -1 && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId)) == 3)
				{
					Prepare(command, 47, human, Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::MapPosition));
					*action = 45;
					return true;
				}

				return false;
			}
			default:

				return false;
		}
	}

	// Original function: FUN_0040b073; unallocated vehicles leave the command list untouched.
	bool Logic_UserControl_GetVehicleCommands(std::int32_t id, SUserCommandsForVehicleHandler* commands, bool multiple)
	{
		const auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::intptr_t>(id) * CVehicleArray::RuntimeElementSize;

		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) == 0)
			return false;

		Vehicle_UserControl_GetCommands(vehicle, commands, multiple);

		return true;
	}

	// Original function: FUN_00437919.
	void Vehicle_UserControl_GetCommands(const void* vehicle, SUserCommandsForVehicleHandler* commands, bool multiple)
	{
		commands->Init();

		if (multiple)
		{
			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) == 0 || Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType) != 54)
				return;

			if (!Vehicle_Attachment_IsInsideVehicle(vehicle))
			{
				commands->AddCommand(5);
				commands->AddCommand(6);
				commands->AddCommand(7);
				commands->AddCommand(8);
			}

			commands->AddCommand(9);
			commands->AddCommand(10);
			commands->AddCommand(11);
			return;
		}

		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0)
		{
			if (Vehicle_Attachment_IsValid(vehicle))
				commands->AddCommand(13);

			if (!Vehicle_Attachment_IsInsideVehicle(vehicle))
			{
				commands->AddCommand(2);
				const auto type = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);
				commands->AddCommand(type == 3 || type == 4 ? 4 : 12);

				if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType) == 54)
				{
					commands->AddCommand(5);
					commands->AddCommand(6);
					commands->AddCommand(7);
					commands->AddCommand(8);
				}
				else
				{
					goto passengerCommands;
				}
			}

			if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType) == 54)
			{
				if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MilitaryMode) != 1)
					commands->AddCommand(9);

				if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MilitaryMode) != 2)
					commands->AddCommand(10);

				if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MilitaryMode) != 3)
					commands->AddCommand(11);
			}
		}

	passengerCommands:
		if (Vehicle_Passengers_GetNumberOfAttachedHumans(vehicle, true) == 0 || !Vehicle_Door_IsEntryPointOnLand(vehicle, 0))
			return;

		if (Vehicle_Passengers_IsCarrierAttached(vehicle) && Vehicle_Stock_GetTotalWantedAmount(vehicle) != 0)
			commands->AddCommand(14);

		const auto type = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

		if ((type == 3 || type == 4) && Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) == 0)
		{

			return;
		}

		commands->AddCommand(3);
	}

	// Original function: FUN_00437aaf; secondary command is unused in this Windows body.
	std::uint32_t Vehicle_UserControl_GetDefaultInteractionCommand_SingleVehicle(const void* vehicle, const void* target, void* command, void*, bool modifier)
	{
		SUserCommandsForVehicleHandler available;
		Vehicle_UserControl_GetCommands(vehicle, &available, false);
		const auto player = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId);
		const auto* point = static_cast<const std::byte*>(target) + NC2Layout::DisplayObjectReferenceOffset::MapPosition;

		switch (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type))
		{
			case SwitchIds::MapObjectUnderCursor::Human:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId);
				const auto* human = Human(targetId);

				if (Human_IsAgeAdult(human) && available.IsCommandAllowed(5) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)) == 3)
				{
					Prepare(command, 104, vehicle, targetId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
					return 5;
				}

				break;
			}
			case SwitchIds::MapObjectUnderCursor::Animal:
			case SwitchIds::MapObjectUnderCursor::Guide:
			case SwitchIds::MapObjectUnderCursor::Landscape:
			case SwitchIds::MapObjectUnderCursor::Position:
				break;
			case SwitchIds::MapObjectUnderCursor::Vehicle:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::VehicleId);
				const auto* other = Vehicle(targetId);

				if (available.IsCommandAllowed(7) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(other, NC2Layout::VehicleOffset::PlayerId)) == 3)
				{
					Prepare(command, 107, vehicle, targetId, Read<std::uint32_t>(other, NC2Layout::VehicleOffset::UniqueId));
					return 7;
				}

				if (available.IsCommandAllowed(12) && Vehicle_Passengers_CanVehicleBeAttached(other, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId)))
				{
					Prepare(command, 113, vehicle, targetId, Read<std::uint32_t>(other, NC2Layout::VehicleOffset::UniqueId));
					return 12;
				}

				break;
			}
			case SwitchIds::MapObjectUnderCursor::House:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HouseId);
				const auto* house = House(targetId);

				if (available.IsCommandAllowed(6) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId)) == 3)
				{
					Prepare(command, 105, vehicle, targetId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
					return 6;
				}

				return 0;
			}
			default:
				return 0;
		}

		return Vehicle_UserControl_GetDefaultInteractionCommand_TrySendTo(vehicle, command, point, &available, modifier);
	}

	// Original function: FUN_00437cb1; modified point attack is evaluated before the continent comparison.
	std::uint32_t Vehicle_UserControl_GetDefaultInteractionCommand_TrySendTo(const void* vehicle, void* command, const void* point, const SUserCommandsForVehicleHandler* commands, bool modifier)
	{
		if (!MapMigPoint_IsInsideMap(point))
			return 0;

		const auto* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;

		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0)
			return 0;

		const auto player = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId);

		if (player < 16 && (Read<std::uint16_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) == 0)
			return 0;

		if (modifier && commands->IsCommandAllowed(8))
		{
			Prepare(command, 108, vehicle, MapMigPoint_GetLong(point));
			return 8;
		}

		if (Continent(point) != Continent(moveable + NC2Layout::MapMoveableOffset::MapPosition))
			return 0;

		Prepare(command, 101, vehicle, MapMigPoint_GetLong(point));

		return 2;
	}

	// Original function: FUN_00437d9f; count is unused and group point attacks do not run single-vehicle movement checks.
	bool Vehicle_UserControl_GetDefaultInteractionCommand_MultipleVehicles(const void* vehicle, std::uint32_t* action, const void* target, void* command, std::uint32_t index, std::uint32_t, bool modifier)
	{
		SUserCommandsForVehicleHandler available;
		Vehicle_UserControl_GetCommands(vehicle, &available, true);
		const auto player = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId);
		const auto commitTarget = [&](std::uint32_t wantedAction, std::uint8_t opcode, std::uint32_t targetId, const void* other)
		{
			if (index != 0)
				return *action == wantedAction && Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode) == opcode && Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1) == targetId;

			Prepare(command, opcode, vehicle, targetId, Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId));
			*action = wantedAction;

			return true;
		};

		switch (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type))
		{
			case SwitchIds::MapObjectUnderCursor::Human:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId);
				const auto* human = Human(targetId);
				return Human_IsAgeAdult(human) && available.IsCommandAllowed(5) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)) == 3 && commitTarget(5, 104, targetId, human);
			}
			case SwitchIds::MapObjectUnderCursor::Vehicle:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::VehicleId);
				const auto* other = Vehicle(targetId);
				return available.IsCommandAllowed(7) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(other, NC2Layout::VehicleOffset::PlayerId)) == 3 && commitTarget(7, 107, targetId, other);
			}
			case SwitchIds::MapObjectUnderCursor::House:
			{
				const auto targetId = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HouseId);
				const auto* house = House(targetId);
				return available.IsCommandAllowed(6) && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId)) == 3 && commitTarget(6, 105, targetId, house);
			}
			case SwitchIds::MapObjectUnderCursor::Landscape:
				if (available.IsCommandAllowed(8) && Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId) != -1 && Logic_EntryPoint_GetDiplomacyState(player, Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId)) == 3)
				{
					Prepare(command, 108, vehicle, Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::MapPosition));
					*action = 8;
					return true;
				}

				[[fallthrough]];
			case SwitchIds::MapObjectUnderCursor::Position:
				if (modifier && available.IsCommandAllowed(8))
				{
					Prepare(command, 108, vehicle, Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::MapPosition));
					*action = 8;
					return true;
				}

				return false;
			default:

				return false;
		}
	}

	// Original function: FUN_004408ca.
	void SUserCommandsForVehicleHandler::Init()
	{
		count = 0;
		std::memset(commands, 0, sizeof(commands));
	}

	// Original function: FUN_004408da; only equality with the native ten-entry capacity is checked.
	void SUserCommandsForVehicleHandler::AddCommand(std::uint32_t command)
	{
		if (count != 10)
			commands[count++] = command;
	}

	// Windows shares 0044073F with the human handler; the count controls the same linear search.
	bool SUserCommandsForVehicleHandler::IsCommandAllowed(std::int32_t command) const
	{
		for (std::int32_t i = 0; i < count; ++i)
		{
			if (commands[i] == static_cast<std::uint32_t>(command))
				return true;
		}

		return false;
	}

	// Original function: FUN_0044da64; aligned bytes were read directly from Game.exe.
	bool Vehicle_Attachment_IsValid(const void* vehicle)
	{
		const auto id = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CarrierVehicleId);

		if (id == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const auto* other = Vehicle(id);

		return Read<std::uint8_t>(other, NC2Layout::VehicleOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(other, NC2Layout::VehicleOffset::UniqueId) == Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CarrierVehicleRevision);
	}
}
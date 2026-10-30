#include "RemasterAILog.h"
#include "C2AnimalAccess.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanChest.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanFamily.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMerchantAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanScout.h"
#include "C2HumanTaskAccess.h"
#include "C2HumanWalking.h"
#include "C2HumanWorkCommands.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MilitaryTargetSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2StaticAtomicAnimationData.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_0043623a; both trailing parameters are unused.
	std::uint32_t Human_UserControl_GetDefaultInteractionCommand_TrySendTo(const void* human, void* command, const void* point, bool, bool)
	{
		using namespace NC2Lifetime;

		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit5) != 0)
			return 0;

		const auto* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;

		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0)
			return 0;

		const auto* target = GameMap_GetConstMapMicroPointPtr(point);
		const auto* source = GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition);

		if (Read<std::uint8_t>(target, NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint8_t>(source, NC2Layout::MapMicroPointOffset::ContinentId))
			return 0;

		const auto player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);

		if (player < 16 && (Read<std::uint16_t>(target, NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) == 0)
			return 0;

		std::memset(command, 0, 20);
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 4);
		Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, MapMigPoint_GetLong(point));

		return 2;
	}

	// Original function: FUN_0042cc19.
	bool Human_IsAtWorkCenter(const void* human)
	{

		return NC2Lifetime::Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter);
	}

	// Original function: FUN_0042cecb.
	bool Human_Target_HasTarget(const void* human)
	{

		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) != 0;
	}

	// Original function: FUN_0042cc40; complete PE selector 0042CD3D..0042CD5C.
	bool Human_IsAtTargetPoint(const void* human)
	{
		using namespace NC2Lifetime;

		if (!Human_Target_IsValid(human))
			return false;

		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget))
		{
			case SwitchIds::InteractionTargetType::Position:
			case SwitchIds::InteractionTargetType::RoadBuildPlace:
			case SwitchIds::InteractionTargetType::WallBuildPlace:
			case SwitchIds::InteractionTargetType::House:
				return Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetWorkPosition);
			case SwitchIds::InteractionTargetType::Landscape:
				return Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetWorkPosition) && Human_GetLandscapeTypeAtTargetPosition(human) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetLandscapeType);
			case SwitchIds::InteractionTargetType::Vehicle:
			{
				const void* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::RuntimeElementSize;
				std::uint32_t entry = 0;

				if (!Vehicle_Door_GetEntryPoint(vehicle, &entry, 0))
					return false;

				const auto kind = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

				if ((kind == 3 || kind == 4) && Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) == 0)
					return false;

				return Vehicle_Door_IsEntryPointOnLand(vehicle, 0) && Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == entry;
			}
			case SwitchIds::InteractionTargetType::Human:
			case SwitchIds::InteractionTargetType::Animal:
				return true;
			default:

				return false;
		}
	}

	// Original function: FUN_0042ced7; a passenger may target its own vehicle
	// even when the general interaction-target validation fails.
	bool Human_Target_IsValid(const void* human)
	{
		using namespace NC2Lifetime;

		if (InteractionTargetData_IsValid(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::InteractionTarget))
			return true;

		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 6 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) && Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0;
	}

	// Original function: FUN_0044314a; events precede time advancement and atomic completion.
	void Human_CurrentTask_DoPerform(void* human)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			return;
		}

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag) == 0)
			return;

		const auto& animation = gStaticAtomicAnimationData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationId)];
		// Dispatch every event due at this animation time in stored order.
		// Distinct events at the same time each get a separate dispatcher call.

		for (std::uint32_t index = 0; index < animation.eventCount; ++index)
		{
			if (static_cast<std::uint32_t>(animation.events[index].time) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationFrame))
				Human_l_PerformTaskEvent(human, animation.events[index]);
		}

		const auto done = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationFrame) + 1u;
		Write(human, NC2Layout::HumanOffset::AtomicAnimationFrame, done);

		if (done != static_cast<std::uint32_t>(animation.length))
		{

			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 1u)
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);

		if (animation.endDirection != -1)
		{
			void* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
			Write(moveable, NC2Layout::MapMoveableOffset::Direction, animation.endDirection);
		}

		Human_l_ResetAtomic(human);
	}

	// Original function: FUN_0042c5aa.
	bool Human_IsJobWoman(const void* human)
	{

		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 5;
	}

	// Original function: FUN_00441cb4; all five Windows circulation modes release their reservations.
	void Human_StopCurrentGoodCirculationMode(void* human)
	{
		using namespace NC2Lifetime;
		const auto houseById = [](std::uint32_t id)
		{

			return CHouseArray::mStaticVars + id * CHouseArray::RuntimeElementSize;
		};

		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode))
		{
			case SwitchIds::GoodCirculationMode::FetchForSelf:
				if (auto* house = Human_GetTargetHousePtr(human))
					House_Stock_ModifyFutureAmount(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, false);

				break;
			case SwitchIds::GoodCirculationMode::FillStockFetch:
			{
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 8)
					break;

				auto* house = Human_GetTargetHousePtr(human);

				if (house != nullptr && (House_Stock_CanStoreGoodType(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), true) || House_Stock_CanStoreGoodType(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), false)))
					House_Stock_ModifyFutureAmount(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, false);

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 6 && InteractionTargetData_IsValid(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::InteractionTarget))
				{
					auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::RuntimeElementSize;

					if (Vehicle_Stock_CanStoreGoodType(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)))
						Vehicle_Stock_ModifyFutureAmount(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
				}

				if (Human_IsAttachedToWorkHouse(human))
					House_Stock_ModifyFutureAmount(houseById(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), -1, true);

				break;
			}
			case SwitchIds::GoodCirculationMode::FetchForHome:
				if (auto* house = Human_GetTargetHousePtr(human))
					House_Stock_ModifyFutureAmount(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, false);

				if (Human_IsAttachedToHomeHouse(human))
					House_Stock_ModifyFutureAmount(houseById(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), -1, true);

				break;
			case SwitchIds::GoodCirculationMode::FillStockBringAway:
			{
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 8)
					break;

				if (auto* house = Human_GetTargetHousePtr(human))
				{
					House_Stock_ModifyFutureAmount(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), -1, true);
				}
				else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 6)
				{
					if (!InteractionTargetData_IsValid(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::InteractionTarget))
						break;

					auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::RuntimeElementSize;

					if (Vehicle_Stock_CanStoreGoodType(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)))
						Vehicle_Stock_ModifyFutureAmount(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), -1, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
				}
				else if (Human_IsAttachedToWorkHouse(human))
				{
					House_Stock_ModifyFutureAmount(houseById(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), -1, true);
				}
				else if (Human_IsJobWoman(human) && Human_IsAttachedToHomeHouse(human))
				{
					House_Stock_ModifyFutureAmount(houseById(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), -1, true);
				}

				break;
			}
			case SwitchIds::GoodCirculationMode::FlushStock:
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
				{
					if (auto* house = Human_GetTargetHousePtr(human))
						House_Stock_ModifyFutureAmount(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), -1, false);
				}
				else if (Human_IsJobWoman(human))
				{
					if (Human_IsAttachedToHomeHouse(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 5)
						House_Stock_ModifyFutureAmount(houseById(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, false);
				}
				else
				{
					if (Human_IsAttachedToWorkHouse(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 5)
						House_Stock_ModifyFutureAmount(houseById(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, false);

					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) != 6)
						break;

					auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectId) * CVehicleArray::RuntimeElementSize;

					if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectRevision) != Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId))
						break;

					if (Vehicle_Stock_CanStoreGoodType(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)))
						Vehicle_Stock_ModifyFutureAmount(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
				}

				break;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);
	}

	// Original function: FUN_00443dab.
	bool Human_StopWaitingForOtherHuman(void* human)
	{
		using namespace NC2Lifetime;
		const auto waiting = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount);

		if (waiting == 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId, NXBasics::IntegerRange::UInt32Maximum);
			return false;
		}

		Write(human, NC2Layout::HumanOffset::WaitingHumanCount, waiting - 1u);

		if (waiting == 1)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId, NXBasics::IntegerRange::UInt32Maximum);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 9)
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
		}

		return true;
	}

	// Original function: FUN_00438d0a; the other human's occupied flag is checked.
	void Human_StopWaitingForOtherHuman_Detach(void* human)
	{
		using namespace NC2Lifetime;
		const auto otherId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId);

		if (otherId == NXBasics::IntegerRange::UInt32Maximum)
		{

			return;
		}

		auto* other = CHumanArray::mStaticVars + otherId * CHumanArray::RuntimeElementSize;

		if (Read<std::uint8_t>(other, NC2Layout::HumanOffset::AllocatedFlag) != 0)
			Human_StopWaitingForOtherHuman(other);

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId, NXBasics::IntegerRange::UInt32Maximum);
	}

	// Original function: FUN_0042cea7.
	void Human_Target_Reset(void* human)
	{
		NC2Lifetime::Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 0);
	}

	// Original function: FUN_0042ceb7.
	void Human_Target_SetType(void* human, std::uint32_t type)
	{
		NC2Lifetime::Write(human, NC2Layout::HumanOffset::InteractionTarget, type);
	}

	// Original function: FUN_0042d18b; a non-landscape target has type zero.
	std::uint32_t Human_GetLandscapeTypeAtTargetPosition(const void* human)
	{
		using namespace NC2Lifetime;
		const void* point = GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition);

		return Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ObjectData) : 0;
	}

	// Original function: FUN_0042d45a.
	std::uint32_t Human_Equipment_GetWeaponType(const void* human)
	{

		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon);
	}

	// Original function: FUN_00462ec6; arriving updates the current task before AI dispatch.
	void Human_AI_DoNewMapPositionReached_DestinationReached(void* human)
	{
		using namespace NC2Lifetime;
		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);

		switch (task)
		{
			case SwitchIds::HumanTask::Goto:
			case SwitchIds::HumanTask::GotoAndWait:
			case SwitchIds::HumanTask::GotoAndResetTarget:
			case SwitchIds::HumanTask::ChooseGoodie:
			case SwitchIds::HumanTask::GoodieIdle:
			case SwitchIds::HumanTask::GoodieWalkAround:
			case SwitchIds::HumanTask::GoodieSleep:
			case SwitchIds::HumanTask::GoodieTalkToAny:
			case SwitchIds::HumanTask::GoodiePlayWithAnimal:
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				return;
			case SwitchIds::HumanTask::Marry:
			case SwitchIds::HumanTask::ReceiveLoveOrEnterHome:
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 1)
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);

				return;
			case SwitchIds::HumanTask::FulfillNeeds:
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) != 1)
					return;

				switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter))
				{
					case SwitchIds::HumanNeed::Food:
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
						return;
					case SwitchIds::HumanNeed::Stamina:
						if (Human_GetLandscapeTypeAtTargetPosition(human) == 4)
						{
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
						}
						else
						{
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 3);
						}

						return;
					case SwitchIds::HumanNeed::Social:
					{
						CMapMoveableIterator iterator;

						for (auto id = iterator.GetFirstMapMoveableId(Human_GetMapPositionRef(human)); id != -1; id = iterator.GetNextMapMoveableId())
						{
							const void* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

							if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 1 && Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId))
							{
								Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
								return;
							}
						}

						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 6);
						return;
					}
					case SwitchIds::HumanNeed::Religion:
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
						return;
					default:
						return;
				}

			case SwitchIds::HumanTask::Attack:
			case SwitchIds::HumanTask::ApproachAttackTarget:
			{
				Human_Target_Reset(human);
				const auto* weapons = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)]);

				if (static_cast<std::uint32_t>(weapons[Human_Equipment_GetWeaponType(human)].minimumRange) > 1u)
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount, 0);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
				}
				else
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				}

				return;
			}
			default:

				return;
		}
	}

	// Original function: FUN_004632c5; Windows clears carrier circulation state.
	void Human_AI_DoPathfinderFailed(void* human)
	{
		using namespace NC2Lifetime;
		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 1);

		if (task == 5 || task == 13)
			Human_StopWaitingForOtherHuman_Detach(human);

		if (Human_IsJobCarrier(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionRotationIndex, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		}
	}

	// Original function: FUN_00463311; interrupt only at an interruptable atomic.
	void Human_AI_DoPathfinderStopped(void* human)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 2)
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);

		if (Human_l_IsAtomicInterruptable(human) && Human_UserControl_IsPriorityCommandInQueue(human))
			Human_CurrentTask_DoInterrupt(human);
	}

	// Original function: FUN_00434deb; complete PE table 00434E0E..00434E6A.
	bool Human_UserControl_IsAPriorityCommand(std::uint32_t opcode)
	{
		switch (opcode)
		{
			case SwitchIds::UserCommand::Stop:
			case SwitchIds::UserCommand::GotoAndWait:
			case SwitchIds::UserCommand::Marry:
			case SwitchIds::UserCommand::SetWorkCenter:
			case SwitchIds::UserCommand::ChangeJob:
			case SwitchIds::UserCommand::ChangeProducedGood:
			case SwitchIds::UserCommand::ChangeProduction:
			case SwitchIds::UserCommand::AttachHomeHouse:
			case SwitchIds::UserCommand::DetachHomeHouse:
			case SwitchIds::UserCommand::AttachWorkHouse:
			case SwitchIds::UserCommand::DetachWorkHouse:
			case SwitchIds::UserCommand::AttachBuildHouse:
			case SwitchIds::UserCommand::DetachBuildHouse:
			case SwitchIds::UserCommand::AttachLearnHouse:
			case SwitchIds::UserCommand::DetachLearnHouse:
			case SwitchIds::UserCommand::LearnJob:
			case SwitchIds::UserCommand::LearnGood:
			case SwitchIds::UserCommand::AttachTradeHouse:
			case SwitchIds::UserCommand::DetachTradeHouse:
			case SwitchIds::UserCommand::AttachVehicle:
			case SwitchIds::UserCommand::DetachVehicle:
			case SwitchIds::UserCommand::SetMilitaryMode:
			case SwitchIds::UserCommand::SetMilitaryWorldRefreshmentFlag:
			case SwitchIds::UserCommand::AttackHuman:
			case SwitchIds::UserCommand::AttackHouse:
			case SwitchIds::UserCommand::AttackAnimal:
			case SwitchIds::UserCommand::AttackVehicle:
			case SwitchIds::UserCommand::AttackMapPosition:
			case SwitchIds::UserCommand::StartAttack:
			case SwitchIds::UserCommand::ResetTraderImportFlags:
			case SwitchIds::UserCommand::ConfigureTradeGood:
			case SwitchIds::UserCommand::SelectTradeAgreement:
				return true;
			default:

				return false;
		}
	}

	// Original function: FUN_00434e6b; separate predicate from priority commands.
	bool Human_UserControl_IsAInterruptingCommand(std::uint32_t opcode)
	{
		switch (opcode)
		{
			case SwitchIds::UserCommand::Stop:
			case SwitchIds::UserCommand::GotoAndWait:
			case SwitchIds::UserCommand::Marry:
			case SwitchIds::UserCommand::ChangeJob:
			case SwitchIds::UserCommand::ChangeProducedGood:
			case SwitchIds::UserCommand::RestartJobExecution:
			case SwitchIds::UserCommand::AttachWorkHouse:
			case SwitchIds::UserCommand::DetachWorkHouse:
			case SwitchIds::UserCommand::AttachBuildHouse:
			case SwitchIds::UserCommand::DetachBuildHouse:
			case SwitchIds::UserCommand::AttachLearnHouse:
			case SwitchIds::UserCommand::DetachLearnHouse:
			case SwitchIds::UserCommand::LearnJob:
			case SwitchIds::UserCommand::LearnGood:
			case SwitchIds::UserCommand::AttachTradeHouse:
			case SwitchIds::UserCommand::DetachTradeHouse:
			case SwitchIds::UserCommand::AttachVehicle:
			case SwitchIds::UserCommand::DetachVehicle:
			case SwitchIds::UserCommand::AttackHuman:
			case SwitchIds::UserCommand::AttackHouse:
			case SwitchIds::UserCommand::AttackAnimal:
			case SwitchIds::UserCommand::AttackVehicle:
			case SwitchIds::UserCommand::AttackMapPosition:
			case SwitchIds::UserCommand::StartAttack:
				return true;
			default:

				return false;
		}
	}

	// Original function: FUN_00434ed0; all 87 command slots are checked in order.
	bool Human_UserControl_IsPriorityCommandInQueue(const void* human)
	{
		for (std::uint32_t index = 0; index < 87; ++index)
		{
			const auto opcode = NC2Lifetime::Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandQueue + index * NC2Layout::RecordSize::UserNetworkCommand);

			if (opcode != 0 && Human_UserControl_IsAPriorityCommand(opcode))
				return true;
		}

		return false;
	}

	// Original function: FUN_00434f03; return the first matching slot or -1.
	std::int32_t Human_UserControl_GetPriorityCommandIndexInQueue(const void* human)
	{
		for (std::int32_t index = 0; index < 87; ++index)
		{
			const auto opcode = NC2Lifetime::Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandQueue + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::UserNetworkCommand);

			if (opcode != 0 && Human_UserControl_IsAPriorityCommand(opcode))
				return index;
		}

		return -1;
	}

	// Original function: FUN_004355fc.
	std::uint32_t Human_UserControl_GetLastCommandType(const void* human)
	{

		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::LastUserCommandIndex) + 2u;
	}

	// Original function: FUN_004355da; zero is also a searchable command type.
	bool Human_UserControl_IsCommandTypeInQueue(const void* human, std::uint8_t opcode)
	{
		for (std::uint32_t slot = 0; slot < 87; ++slot)
		{
			if (NC2Lifetime::Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandQueue + slot * NC2Layout::RecordSize::UserNetworkCommand) == opcode)
				return true;
		}

		return false;
	}

	// Original function: FUN_0044320e; Windows tests atomic interruptibility once.
	void Human_CurrentTask_DoInterrupt(void* human)
	{
		using namespace NC2Lifetime;

		if (!Human_l_IsAtomicInterruptable(human))
		{

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount, 0);

		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask))
		{
			case SwitchIds::HumanTask::Goto:
			case SwitchIds::HumanTask::ExecuteJob:
				std::memcpy(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::SecondaryInteractionTarget, static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::InteractionTarget, NC2Layout::RecordSize::InteractionTargetData);
				Write(human, NC2Layout::HumanOffset::SecondaryTargetGoodType, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
				Write(human, NC2Layout::HumanOffset::SuspendedGoodCirculationMode, Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode));
				break;
			case SwitchIds::HumanTask::GoHome:
				Human_DoAddNewUserCommand(human, 6, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
				break;
			case SwitchIds::HumanTask::GotoAndWait:
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget, 1);
				Write(human, NC2Layout::HumanOffset::SecondaryTargetPosition, Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition));
				break;
			case SwitchIds::HumanTask::Marry:
			{
				const auto otherId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId);

				if (otherId != NXBasics::IntegerRange::UInt32Maximum)
				{
					auto* other = CHumanArray::mStaticVars + otherId * CHumanArray::RuntimeElementSize;
					Human_StopWaitingForOtherHuman(other);
					Human_StopWaitingForOtherHuman_Detach(other);
				}

				Human_DoAddNewUserCommand(human, 14, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
				break;
			}
			case SwitchIds::HumanTask::GetBoy:
				Human_DoAddNewUserCommand(human, 15, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
				break;
			case SwitchIds::HumanTask::GetGirl:
				Human_DoAddNewUserCommand(human, 16, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
				break;
			case SwitchIds::HumanTask::Equip:
			case SwitchIds::HumanTask::DropItem:
				Write(human, NC2Layout::HumanOffset::UserCommandQueue + Read<std::uint32_t>(human, NC2Layout::HumanOffset::LastUserCommandIndex) * NC2Layout::RecordSize::UserNetworkCommand, static_cast<std::uint8_t>(Human_UserControl_GetLastCommandType(human)));
				break;
			case SwitchIds::HumanTask::OpenChest:
				Human_DoAddNewUserCommand(human, 18, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), MapMigPoint_GetLong(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition), 0);
				break;
			case SwitchIds::HumanTask::ExploreArea:
				Human_DoAddNewUserCommand(human, 20, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), MapMigPoint_GetLong(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter), 0);
				break;
			default:
				break;
		}

		Human_StopCurrentGoodCirculationMode(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);
		Human_StopWaitingForOtherHuman_Detach(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
	}

	// Original function: FUN_0045cc64; an inactive non-moveable atomic is interruptable.
	bool Human_l_IsAtomicInterruptable(const void* human)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 1)
		{
			const auto* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
			return Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) == 0;
		}

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag) == 0)
			return true;

		return Read<std::uint8_t>(gStaticAtomicAnimationData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationId) * NC2Layout::RecordSize::AtomicAnimation + NC2Layout::AtomicAnimationOffset::InterruptableUnsigned) != 0;
	}

	// Original function: FUN_0043bb2d; the structured-command overload delegates.
	bool Human_DoAddNewUserCommand(void* human, const void* command)
	{
		using namespace NC2Lifetime;

		return Human_DoAddNewUserCommand(human, Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode), Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
	}

	// Original function: FUN_0043bb4e; the opcode selects an unchecked 20-byte slot.
	bool Human_DoAddNewUserCommand(void* human, std::uint8_t opcode, std::uint8_t player, std::uint32_t id, std::uint32_t revision, std::uint32_t parameter1, std::uint32_t parameter2)
	{
		using namespace NC2Lifetime;

		if (opcode == 0)
			return true;

		if (opcode == 4)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 1 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 15)
			{
				std::uint32_t target;
				MapMigPoint_AssignLong(&target, parameter1);

				if (target == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition))
					return true;
			}

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount, 0);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::QueuedExploreAreaAndUpdateCenterCommand, 0);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::QueuedBuildGuideCommand, 0);
		}

		if (opcode >= 0x2B && opcode <= 0x2F)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount, 0);

			for (const auto offset : { 0x9D0u, 0x9E4u, 0x9F8u, 0xA0Cu, 0xA20u, 0xA34u, 0x994u, 0x9A8u, 0x9BCu })
			{
				Write<std::uint8_t>(human, offset, 0);
			}
		}

		const auto offset = NC2Layout::HumanOffset::UserCommandQueue + (static_cast<std::uint32_t>(opcode) - 2u) * NC2Layout::RecordSize::UserNetworkCommand;
		Write(human, offset, opcode);
		Write(human, offset + NC2Layout::UserNetworkCommandOffset::PlayerId, player);
		Write(human, offset + NC2Layout::UserNetworkCommandOffset::ObjectId, id);
		Write(human, offset + NC2Layout::UserNetworkCommandOffset::ObjectRevision, revision);
		Write(human, offset + NC2Layout::UserNetworkCommandOffset::Parameter1, parameter1);
		Write(human, offset + NC2Layout::UserNetworkCommandOffset::Parameter2, parameter2);
		RemasterAILog::Event(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), "command_queued", Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId), "opcode=%u sender=%u object=%u revision=%u parameter1=%u parameter2=%u", opcode, player, id, revision, parameter1, parameter2);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 1 && Human_l_IsAtomicInterruptable(human))
			Human_CurrentTask_DoInterrupt(human);

		return true;
	}

	// Original function: FUN_0043bc7d.
	void Human_DoCheckAllUserCommands(void* human)
	{
		using namespace NC2Lifetime;

		for (std::uint32_t slot = 0; slot < 87; ++slot)
		{
			auto* command = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::UserCommandQueue + slot * NC2Layout::RecordSize::UserNetworkCommand;

			if (Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode) != 0 && Human_DoCheckUserCommand(human, command))
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 0);
		}
	}

	// Original function: FUN_0043bca9.
	// Complete Windows selector table 0043BD42..0043BD96.
	bool Human_DoCheckUserCommand(const void* human, const void* command)
	{
		switch (NC2Lifetime::Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode))
		{
			case SwitchIds::UserCommand::GoHome:
			case SwitchIds::UserCommand::Marry:
			case SwitchIds::UserCommand::MarryAlternate:
				return Human_IsJobSoldier(human) || Human_IsJobScout(human) || Human_IsJobTrader(human);
			case SwitchIds::UserCommand::AttachWorkHouse:
			case SwitchIds::UserCommand::DetachWorkHouse:
				return Human_IsJobBuilder(human);
			case SwitchIds::UserCommand::AttachBuildHouse:
			case SwitchIds::UserCommand::DetachBuildHouse:
				return !Human_IsJobBuilder(human);
			case SwitchIds::UserCommand::AttachTradeHouse:
			case SwitchIds::UserCommand::DetachTradeHouse:
				return !Human_IsJobTrader(human);
			case SwitchIds::UserCommand::ExploreArea:
			case SwitchIds::UserCommand::ExploreAreaAndUpdateCenter:
				return !Human_IsJobScout(human);
			default:
			{
				const auto opcode = NC2Lifetime::Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode);

				return opcode < 4 || opcode > 87;
			}
		}
	}

	// Original function: FUN_0043e49d.
	void Human_ResetUserCommandArray(void* human)
	{
		for (std::uint32_t slot = 0; slot < 87; ++slot)
		{
			NC2Lifetime::Write<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandQueue + slot * NC2Layout::RecordSize::UserNetworkCommand, 0);
		}
	}

	// Original function: FUN_0042d348.
	bool Human_HasEquippedItemType(const void* human, std::uint32_t good, std::int32_t slot)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedShoes) == good || Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools) == good)
			return true;

		const auto* weapons = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)]);
		const auto* armor = static_cast<const SStaticArmorTypeData*>(gStaticArmorTypeData);

		if (static_cast<std::uint32_t>(weapons[Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon)].goodType) == good || static_cast<std::uint32_t>(armor[Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor)].goodType) == good)
			return true;

		if (slot == -1)
			return Human_l_Item_IsGoodTypeEquipped(human, good);

		const auto offset = NC2Layout::HumanOffset::InventorySlots + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HumanInventorySlot;

		return Read<std::uint32_t>(human, offset) == good && Read<std::int32_t>(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition) > 0;
	}

	// Original function: FUN_0045c293; the attachment-mask argument is unused in Windows.
	void Human_UserMessage_SendToUserMessage_CannotDetachHouse(void* human, std::uint32_t attachmentMask)
	{
		using namespace NC2Lifetime;
		(void)attachmentMask;
		std::byte message[772];
		ToUserMessage_Construct(message);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 42);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0043cc88.
	std::uint32_t Human_DoExecuteUserCommand_DetachWorkHouse(void* human)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 7)
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (!Human_IsAttachedToWorkHouse(human) || Human_IsAttachedToLearnHouse(human))
			return 1;

		auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;

		if (House_Inhabitants_DetachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 2))
			return 2;

		Human_UserMessage_SendToUserMessage_CannotDetachHouse(human, 2);

		return 1;
	}

	// Original function: FUN_0043ce31.
	std::uint32_t Human_DoExecuteUserCommand_DetachLearnHouse(void* human)
	{
		using namespace NC2Lifetime;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (!Human_IsAttachedToWorkHouse(human))
			return 1;

		auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;

		if (House_Inhabitants_DetachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 8))
			return 2;

		Human_UserMessage_SendToUserMessage_CannotDetachHouse(human, 8);

		return 1;
	}

	// Original function: FUN_0043c486; fallback walking retains the original work-center point.
	std::uint32_t Human_DoExecuteUserCommand_GotoAndWait(void* human, const void* point)
	{
		using namespace NC2Lifetime;
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);
		Human_Target_Reset(human);
		Human_Target_ResetSecondaryTarget(human);

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		const bool direct = Human_l_StartAtomicWalkAndWait(human, point);

		if (!direct)
		{
			auto fallback = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human)), NC2Layout::MapMicroPointOffset::ContinentId))
				return 1;

			if (!Tool_Map_GetNotBlockedPosition(&fallback, false) || !Human_l_StartAtomicWalkAndWait(human, &fallback))
				return 1;
		}

		if (Human_IsAttachedToLearnHouse(human))
			Human_DoExecuteUserCommand_DetachLearnHouse(human);

		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		bool recenter = Human_IsJobSoldier(human) || Human_IsJobHero(human) || Human_IsJobScout(human);

		if (direct)
		{
			recenter = recenter || job == 6 || (job == 5 && !Human_IsAttachedToHomeHouse(human)) || (!Human_IsAgeAdult(human) && !Human_IsAttachedToHomeHouse(human));

			if (Human_IsJobCarrier(human) && !Human_IsAttachedToWorkHouse(human) && !Human_IsAttachedToVehicle(human))
				recenter = true;

			if (Human_IsForcedToHaveWorkHouse(human) && !Human_IsAttachedToWorkHouse(human))
				recenter = true;
		}

		if (recenter)
		{
			Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);

			if (Human_IsAttachedToWorkHouse(human))
				Human_DoExecuteUserCommand_DetachWorkHouse(human);
		}

		return 2;
	}

	// Original function: FUN_0043c7cf; speaker identity comes from command id/revision.
	std::uint32_t Human_DoExecuteUserCommand_Cheer(void* human, std::uint32_t speakerId, std::uint32_t revision)
	{
		using namespace NC2Lifetime;

		if (!Human_IsAbleToListen(human))
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		const auto* speaker = CHumanArray::mStaticVars + speakerId * CHumanArray::RuntimeElementSize;

		if (Read<std::uint8_t>(speaker, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(speaker, NC2Layout::HumanOffset::UniqueId) != revision)
			return 1;

		Human_Target_SetTargetPositions(human, Human_GetMapPositionRef(speaker), Human_GetMapPositionRef(human));
		Human_l_StartTask_Listen(human, Human_Target_GetDirectionTo(human));

		return 2;
	}

	// Original function: FUN_0043be95; the human argument is unused.
	bool Human_l_UserCommand_DetachFromVehicle(const void* human, const void* command)
	{
		using namespace NC2Lifetime;
		(void)human;
		const auto opcode = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode);

		return (opcode == 4 && Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2) == 0) || opcode == 30 || opcode == 32 || opcode == 28 || opcode == 45 || opcode == 44 || opcode == 43 || opcode == 47 || opcode == 46 || opcode == 3;
	}

	// Original function: FUN_0043c646.
	std::uint32_t Human_DoExecuteUserCommand_GoAway(void* human, const void* point, std::uint32_t radius)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		Human_l_StartTask_GoAway(human, NC2Lifetime::Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates), radius);

		return 2;
	}

	// Original function: FUN_0043c674.
	std::uint32_t Human_DoExecuteUserCommand_Eat(void* human)
	{
		using namespace NC2Lifetime;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (Human_HasEquippedItemType(human, 50, -1))
			return 1;

		Human_CurrentTask_DoInterrupt(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 7);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 1);

		return 2;
	}

	// Original function: FUN_0043c6be.
	std::uint32_t Human_DoExecuteUserCommand_Sleep(void* human)
	{
		using namespace NC2Lifetime;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (Human_HasEquippedItemType(human, 51, -1))
			return 1;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 2);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 7);

		return 2;
	}

	// Original function: FUN_0043c6fe.
	std::uint32_t Human_DoExecuteUserCommand_Talk(void* human)
	{
		using namespace NC2Lifetime;

		if (!Human_IsAbleToTalk(human))
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 7);

		return 2;
	}

	// Original function: FUN_0043c73d.
	std::uint32_t Human_DoExecuteUserCommand_Pray(void* human)
	{
		using namespace NC2Lifetime;

		if (!Human_IsAbleToPray(human))
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 4);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 7);

		return 2;
	}

	// Original function: FUN_0043c77c.
	std::uint32_t Human_DoExecuteUserCommand_Goodie(void* human, std::int32_t task)
	{
		using namespace NC2Lifetime;

		if (task < 24 || task > 29)
			task = 24;

		if (!Human_IsAbleToDoGoodie(human))
			return 1;

		if (!Human_IsAbleToDoGoodieNow(human) || !Human_l_IsAtomicInterruptable(human))
			return 0;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write(human, NC2Layout::HumanOffset::CurrentTask, task);

		return 2;
	}

	// Original function: FUN_0043e051.
	std::uint32_t Human_DoExecuteUserCommand_AttackHuman(void* human, std::int32_t id, std::uint32_t revision)
	{
		using namespace NC2Lifetime;
		void* target = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

		if (Read<std::uint8_t>(target, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(target, NC2Layout::HumanOffset::UniqueId) != revision)
			return 1;

		Human_IsAgeAdult(target);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 1);
		Write(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetValue, id);
		Write(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetRevision, revision);

		if ((Human_IsJobSoldier(human) || Human_IsJobHero(human)) && !Human_IsAttachedToWorkHouse(human))
		{
			Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(Human_GetMapPositionRef(target), NC2Layout::MapCoordinatePairOffset::Coordinates));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);

		return 2;
	}

	// Original function: FUN_0043e12b.
	std::uint32_t Human_DoExecuteUserCommand_AttackAnimal(void* human, std::int32_t id, std::uint32_t revision)
	{
		using namespace NC2Lifetime;
		void* target = CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::RuntimeElementSize;

		if (Read<std::uint8_t>(target, NC2Layout::AnimalOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(target, NC2Layout::AnimalOffset::UniqueId) != revision)
			return 1;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 3);
		Write(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetValue, id);
		Write(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetRevision, revision);

		if ((Human_IsJobSoldier(human) || Human_IsJobHero(human)) && !Human_IsAttachedToWorkHouse(human))
		{
			Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(Animal_GetMapPositionRef(target), NC2Layout::MapCoordinatePairOffset::Coordinates));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);

		return 2;
	}

	// Original function: FUN_0043e1fc.
	std::uint32_t Human_DoExecuteUserCommand_AttackHouse(void* human, std::int32_t id, std::uint32_t revision)
	{
		using namespace NC2Lifetime;
		void* target = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

		if (Read<std::uint8_t>(target, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(target, NC2Layout::HouseOffset::UniqueId) != revision)
			return 1;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 4);
		Write(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetValue, id);
		Write(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetRevision, revision);

		if ((Human_IsJobSoldier(human) || Human_IsJobHero(human)) && !Human_IsAttachedToWorkHouse(human))
		{
			Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(target, NC2Layout::HouseOffset::MapPosition));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);

		return 2;
	}

	// Original function: FUN_0043e2c9.
	std::uint32_t Human_DoExecuteUserCommand_AttackVehicle(void* human, std::int32_t id, std::uint32_t revision)
	{
		using namespace NC2Lifetime;
		void* target = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;

		if (Read<std::uint8_t>(target, NC2Layout::VehicleOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(target, NC2Layout::VehicleOffset::UniqueId) != revision)
			return 1;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 2);
		Write(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetValue, id);
		Write(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetRevision, revision);

		if ((Human_IsJobSoldier(human) || Human_IsJobHero(human)) && !Human_IsAttachedToWorkHouse(human))
		{
			Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(Vehicle_GetMapPositionRef(target), NC2Layout::MapCoordinatePairOffset::Coordinates));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);

		return 2;
	}

	// Original function: FUN_0043e39a.
	std::uint32_t Human_DoExecuteUserCommand_AttackMapPosition(void* human, std::uint32_t packedPoint)
	{
		using namespace NC2Lifetime;
		std::uint32_t point;
		MapMigPoint_AssignLong(&point, packedPoint);

		if (!Tool_IsEnemyLandscapeOnPoint(Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), &point))
			return 1;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 6);
		Write(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetValue, point);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetRevision, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);

		return 2;
	}

	// Original function: FUN_0043e436.
	std::uint32_t Human_DoExecuteUserCommand_MoveInsideVehicle(void* human, std::int32_t id, std::uint32_t revision)
	{
		using namespace NC2Lifetime;

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) != id || Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision) != revision || Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
			return 1;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 21 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) <= 1)
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 21);

		return 2;
	}

	// Original function: FUN_0043bed3; the complete Windows command selector.
	std::uint32_t Human_DoExecuteUserCommand(void* human, const void* command)
	{
		using namespace NC2Lifetime;
		const auto opcode = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode);

		if (Human_IsDefendingHouse(human, -1) && opcode != 4 && opcode != 17 && (opcode < 21 || opcode > 24))
			return 1;

		if (Human_IsAttachedToVehicle(human) && Human_l_UserCommand_DetachFromVehicle(human, command))
			Human_DoExecuteUserCommand_DetachVehicle(human);

		if ((opcode >= 52 && opcode <= 60) || (opcode >= 70 && opcode <= 78))
			return Human_DoExecuteUserCommand_Equipment_Drop(human, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));

		if ((opcode >= 61 && opcode <= 69) || (opcode >= 79 && opcode <= 87))
			return Human_DoExecuteUserCommand_Equipment_Change(human, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));

		switch (opcode)
		{
			case SwitchIds::UserCommand::GotoAndWait:
			{
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::QueuedAssignJobCommand, 0);
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
				return Human_DoExecuteUserCommand_GotoAndWait(human, &point);
			}
			case SwitchIds::UserCommand::GoAway:
			{
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
				return Human_DoExecuteUserCommand_GoAway(human, &point, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			}
			case SwitchIds::UserCommand::GoHome: // DoExecuteUserCommand_GoHome, inlined at 0043BF80.

				if (!Human_l_IsAtomicInterruptable(human))
					return 0;

				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 3);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				return 2;
			case SwitchIds::UserCommand::Eat:
				return Human_DoExecuteUserCommand_Eat(human);
			case SwitchIds::UserCommand::Sleep:
				return Human_DoExecuteUserCommand_Sleep(human);
			case SwitchIds::UserCommand::Talk:
				return Human_DoExecuteUserCommand_Talk(human);
			case SwitchIds::UserCommand::Pray:
				return Human_DoExecuteUserCommand_Pray(human);
			case SwitchIds::UserCommand::Goodie:
				return Human_DoExecuteUserCommand_Goodie(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
			case SwitchIds::UserCommand::Cheer:
				return Human_DoExecuteUserCommand_Cheer(human, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision));
			case SwitchIds::UserCommand::Marry:
			case SwitchIds::UserCommand::MarryAlternate:
				return Human_DoExecuteUserCommand_Marry(human);
			case SwitchIds::UserCommand::GetBoy:
				return Human_DoExecuteUserCommand_GetBoy(human);
			case SwitchIds::UserCommand::GetGirl:
				return Human_DoExecuteUserCommand_GetGirl(human);
			case SwitchIds::UserCommand::SetWorkCenter:
			{
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
				return Human_DoExecuteUserCommand_SetWorkCenter(human, &point);
			}
			case SwitchIds::UserCommand::OpenChest:
			{
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
				return Human_DoExecuteUserCommand_OpenChest(human, &point);
			}
			case SwitchIds::UserCommand::ExploreArea:
			case SwitchIds::UserCommand::ExploreAreaAndUpdateCenter:
			{
				if (opcode == 20 && Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2) != 0)
				{
					Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates));
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
					Human_WorkCenterMarker_Update(human);
				}

				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
				return opcode == 19 ? Human_DoExecuteUserCommand_BuildGuide(human, &point) : Human_DoExecuteUserCommand_ExploreArea(human, &point);
			}
			case SwitchIds::UserCommand::AttachHomeHouse:
				return Human_DoExecuteUserCommand_AttachHomeHouse(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::ChangeJob:
				return Human_DoExecuteUserCommand_ChangeJob(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
			case SwitchIds::UserCommand::ChangeProducedGood:
				return Human_DoExecuteUserCommand_ChangeProducedGood(human, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
			case SwitchIds::UserCommand::ChangeProduction:
				return Human_DoExecuteUserCommand_ChangeProduction(human, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::AssignJob:
				return Human_DoExecuteUserCommand_SetJob(human, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
			case SwitchIds::UserCommand::RestartJobExecution:
				return Human_DoExecuteUserCommand_RestartJobExecution(human);
			case SwitchIds::UserCommand::DetachHomeHouse:
				return Human_DoExecuteUserCommand_DetachHomeHouse(human);
			case SwitchIds::UserCommand::AttachWorkHouse:
				return Human_DoExecuteUserCommand_AttachWorkHouse(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::DetachWorkHouse:
				return Human_DoExecuteUserCommand_DetachWorkHouse(human);
			case SwitchIds::UserCommand::AttachBuildHouse:
				return Human_DoExecuteUserCommand_AttachBuildHouse(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::DetachBuildHouse:
				return Human_DoExecuteUserCommand_DetachBuildHouse(human);
			case SwitchIds::UserCommand::AttachLearnHouse:
				return Human_DoExecuteUserCommand_AttachLearnHouse(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::DetachLearnHouse:
				return Human_DoExecuteUserCommand_DetachLearnHouse(human);
			case SwitchIds::UserCommand::LearnJob:
				return Human_DoExecuteUserCommand_LearnJob(human, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
			case SwitchIds::UserCommand::LearnGood:
				return Human_DoExecuteUserCommand_LearnGood(human, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
			case SwitchIds::UserCommand::AttachTradeHouse:
				return Human_DoExecuteUserCommand_AttachTradeHouse(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::DetachTradeHouse:
				return Human_DoExecuteUserCommand_DetachTradeHouse(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::AttachVehicle:
				return Human_DoExecuteUserCommand_AttachVehicle(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::DetachVehicle:
				return Human_DoExecuteUserCommand_DetachVehicle(human);
			case SwitchIds::UserCommand::MoveInsideVehicle:
				return Human_DoExecuteUserCommand_MoveInsideVehicle(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::SetMilitaryMode:
				if (!Human_l_IsAtomicInterruptable(human))
					return 0;

				Write(human, NC2Layout::HumanOffset::MilitaryMode, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));

				if (Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1) == 3)
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);

				return 2;
			case SwitchIds::UserCommand::SetMilitaryWorldRefreshmentFlag:
				if (!Human_l_IsAtomicInterruptable(human))
					return 0;

				Write<std::uint8_t>(human, NC2Layout::HumanOffset::MilitaryWorldRefreshmentFlag, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1) != 0);
				// The subsequent native call 0045BFEC is a folded RET 4 no-op.
				return 2;
			case SwitchIds::UserCommand::AttackHuman:
				return Human_DoExecuteUserCommand_AttackHuman(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::AttackHouse:
				return Human_DoExecuteUserCommand_AttackHouse(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::AttackAnimal:
				return Human_DoExecuteUserCommand_AttackAnimal(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::AttackVehicle:
				return Human_DoExecuteUserCommand_AttackVehicle(human, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
			case SwitchIds::UserCommand::AttackMapPosition:
				return Human_DoExecuteUserCommand_AttackMapPosition(human, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
			case SwitchIds::UserCommand::StartAttack:
				if (!Human_l_IsAtomicInterruptable(human))
					return 0;

				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				return 2;
			case SwitchIds::UserCommand::ResetTraderImportFlags: // Inlined at 0043C2B2: enqueue restart before resetting flags.
				Human_DoAddNewUserCommand(human, 25, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
				Human_l_Trader_ResetImportFlags(human);
				return 1;
			case SwitchIds::UserCommand::ConfigureTradeGood:   // Inlined at 0043C2D9: house id, good and flag share parameter 1.
			case SwitchIds::UserCommand::SelectTradeAgreement: // Inlined at 0043C340: select a trade agreement without validation.
			{
				if (!Human_IsJobTrader(human))
					return 1;

				void* merchant = Human_l_GetMerchantDataPtr(human);
				const auto packed = Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1);
				void* data = nullptr;

				if (opcode == 50)
				{
					data = MerchantData_GetHouseDataPtr(merchant, static_cast<std::int32_t>(packed & NXBasics::IntegerRange::UInt16Maximum), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));

					if (data == nullptr)
						return 1;
				}

				Human_DoAddNewUserCommand(human, 25, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);

				if (opcode == 50)
				{
					MerchantHouseData_SetGoodTypeImportFlag(data, (packed >> 16u) & NXBasics::IntegerRange::UInt8Maximum, (packed & NXBasics::BitMask::DwordExceptBits0Through23) != 0);
				}
				else
				{
					Write(merchant, NC2Layout::MerchantOffset::TradeAgreementId, packed);
				}

				return 2;
			}
			default:

				return 1;
		}
	}

	// Original function: FUN_0043bd97.
	bool Human_DoExecuteNextUserCommand(void* human)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag) != 0 || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 6) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount) != 0)
			return false;

		const auto priority = Human_UserControl_GetPriorityCommandIndexInQueue(human);

		if (priority != -1)
		{
			void* command = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::UserCommandQueue + static_cast<std::uint32_t>(priority) * NC2Layout::RecordSize::UserNetworkCommand;
			const auto loggedOpcode = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode);
			const auto loggedParameter1 = Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1);
			const auto loggedParameter2 = Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2);
			const auto result = Human_DoExecuteUserCommand(human, command);

			if (result != 0 || RemasterAILog::Enabled(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 2))
				RemasterAILog::Event(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), "command_result", Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId), "opcode=%u result=%u (0=deferred,1=consumed,2=started) parameter1=%u parameter2=%u", loggedOpcode, result, loggedParameter1, loggedParameter2);

			if (result == 1)
			{
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 0);
			}
			else if (result != 0)
			{
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 0);

				if (result == 2)
					Write(human, NC2Layout::HumanOffset::LastUserCommandIndex, priority);

				Write<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag, 1);
				return true;
			}
		}

		for (std::uint32_t slot = 0; slot < 87; ++slot)
		{
			void* command = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::UserCommandQueue + slot * NC2Layout::RecordSize::UserNetworkCommand;

			if (Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode) == 0)
				continue;

			const auto loggedOpcode = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode);
			const auto loggedParameter1 = Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1);
			const auto loggedParameter2 = Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2);
			const auto result = Human_DoExecuteUserCommand(human, command);

			if (result != 0 || RemasterAILog::Enabled(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 2))
				RemasterAILog::Event(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), "command_result", Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId), "opcode=%u result=%u (0=deferred,1=consumed,2=started) parameter1=%u parameter2=%u", loggedOpcode, result, loggedParameter1, loggedParameter2);

			if (result == 1)
			{
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 0);
			}
			else if (result == 2)
			{
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 0);
				Write(human, NC2Layout::HumanOffset::LastUserCommandIndex, slot);
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag, 1);
				return true;
			}
		}

		return false;
	}

	// Original function: FUN_0043be40; native priority execution repeats until an interrupting command.
	void Human_DoExecuteAllPriorityUserCommandsWhichDoNotInterrupt(void* human)
	{
		using namespace NC2Lifetime;

		for (;;)
		{
			const auto index = Human_UserControl_GetPriorityCommandIndexInQueue(human);

			if (index == -1)
				return;

			void* command = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::UserCommandQueue + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::UserNetworkCommand;

			if (Human_UserControl_IsAInterruptingCommand(Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode)))
			{

				return;
			}

			const auto loggedOpcode = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode);
			const auto loggedParameter1 = Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1);
			const auto loggedParameter2 = Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2);
			const auto result = Human_DoExecuteUserCommand(human, command);

			if (result != 0 || RemasterAILog::Enabled(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 2))
				RemasterAILog::Event(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), "command_result", Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId), "opcode=%u result=%u (0=deferred,1=consumed,2=started) parameter1=%u parameter2=%u", loggedOpcode, result, loggedParameter1, loggedParameter2);

			if (result == 1 || result == 2)
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 0);

			if (result == 2)
			{
				Write(human, NC2Layout::HumanOffset::LastUserCommandIndex, index);
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag, 1);
			}
		}
	}

	// Original function: FUN_00445300; cargo mode selects pickup or equipment-drop startup.
	void Human_l_FinishTask_Goto(void* human)
	{
		using namespace NC2Lifetime;
		const void* micro = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0 && Read<std::uint32_t>(CHouseArray::mStaticVars + Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId) * CHouseArray::RuntimeElementSize, NC2Layout::HouseOffset::ConstructionState) == 3)
			Human_HouseMisc_Enter(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) != 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 7);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			return;
		}

		if ((!Human_IsAttachedToVehicle(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 6) || !Human_IsAtTargetPoint(human))
		{
			if (!Human_IsAttachedToVehicle(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 6)
				Human_Target_Reset(human);

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 8);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			return;
		}

		const auto target = Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget);
		// Native task 0x15 enters a vehicle; task 0x12 opens a chest.

		if (target == 6)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 21);
		}
		else if (target == 2 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood) == 64)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 18);
		}
		else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 1)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 16);

			if (!Human_l_StartTask_Tool_PickupWantedGood_FromTargetPosition(human))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 3);
			}

			return;
		}
		else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 6)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 17);
			Human_l_StartTask_Equip_DropItem(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));

			return;
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 8);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
	}

	// Original function: FUN_00445414.
	void Human_l_FinishTask_GotoAndWait(void* human)
	{
		using namespace NC2Lifetime;
		const auto* micro = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0 && Read<std::uint32_t>(CHouseArray::mStaticVars + Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId) * CHouseArray::RuntimeElementSize, NC2Layout::HouseOffset::ConstructionState) == 3)
			Human_HouseMisc_Enter(human);

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
	}

}
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanTaskAccess.h"
#include "C2HumanWorkCommands.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* House(std::int32_t id)
		{

			return CHouseArray::mStaticVars + static_cast<std::intptr_t>(id) * CHouseArray::RuntimeElementSize;
		}

		void* Vehicle(std::int32_t id)
		{

			return CVehicleArray::mStaticVars + static_cast<std::intptr_t>(id) * CVehicleArray::RuntimeElementSize;
		}

		void Queue(void* human, std::uint8_t opcode, std::uint32_t value, std::uint32_t other)
		{
			Human_DoAddNewUserCommand(human, opcode, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), value, other);
		}

		void SetCenter(void* human, std::uint32_t point)
		{
			Write(human, NC2Layout::HumanOffset::WorkCenter, point);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);
		}

		std::uint32_t WeaponGood(const void* human)
		{

			return static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)])[Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon)].goodType;
		}

		std::uint32_t ArmorGood(const void* human)
		{

			return static_cast<const SStaticArmorTypeData*>(gStaticArmorTypeData)[Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor)].goodType;
		}

		std::uint32_t DetachExistingWorkHouse(void* human)
		{
			auto* house = House(Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));

			if (House_GetHouseMainType(house) == 4 && House_Inhabitants_IsHumanAttachedAsStudent(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)))
				return Human_DoExecuteUserCommand_DetachLearnHouse(human);

			return Human_IsJobBuilder(human) ? Human_DoExecuteUserCommand_DetachBuildHouse(human) : Human_DoExecuteUserCommand_DetachWorkHouse(human);
		}
	}

	// Original function: FUN_0043cb68.
	std::uint32_t Human_DoExecuteUserCommand_AttachWorkHouse(void* human, std::int32_t id, std::uint32_t revision)
	{
		if (Human_IsJobBuilder(human))
			return 1;

		auto* house = House(id);

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) != revision)
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) == id)
			return 1;

		if (Human_IsAttachedToWorkHouse(human) && DetachExistingWorkHouse(human) == 0)
			return 1;

		if (Human_IsAttachedToVehicle(human) && Human_DoExecuteUserCommand_DetachVehicle(human) == 0)
		{
			Human_UserMessage_SendToUserMessage_CannotDetachVehicle(human);
			return 1;
		}

		if (!House_Inhabitants_AttachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 2))
		{
			Human_UserMessage_SendToUserMessage_CannotAttachHouse(human, 2);
			return 1;
		}

		Human_l_AssignWorkHouse(human, static_cast<std::uint32_t>(id));
		SetCenter(human, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));

		return 2;
	}

	// Original function: FUN_0043cce3; native prerequisites precede target validation.
	std::uint32_t Human_DoExecuteUserCommand_AttachLearnHouse(void* human, std::int32_t id, std::uint32_t revision)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) == id && Human_IsAttachedToLearnHouse(human))
			return 1;

		if (Human_IsAttachedToWorkHouse(human) && DetachExistingWorkHouse(human) == 0)
			return 1;

		if (Human_IsAttachedToVehicle(human) && Human_DoExecuteUserCommand_DetachVehicle(human) == 0)
		{
			Human_UserMessage_SendToUserMessage_CannotDetachVehicle(human);
			return 1;
		}

		auto* house = House(id);

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) != revision)
			return 1;

		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 39 && !Human_IsJobSoldier(human) && Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlagsSigned + SwitchIds::HumanJob::SoldierUnarmed) != 0)
		{
			Queue(human, 21, 31, 0);
			return 0;
		}

		if (!House_Inhabitants_AttachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 8))
		{
			Human_UserMessage_SendToUserMessage_CannotAttachHouse(human, 8);
			return 1;
		}

		Human_l_AssignWorkHouse(human, static_cast<std::uint32_t>(id));
		SetCenter(human, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));

		return 2;
	}

	// Original function: FUN_0043ce7f.
	std::uint32_t Human_DoExecuteUserCommand_AttachBuildHouse(void* human, std::int32_t id, std::uint32_t revision)
	{
		if (!Human_IsJobBuilder(human))
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (Human_IsAttachedToWorkHouse(human))
		{
			const auto result = Human_IsAttachedToLearnHouse(human) ? Human_DoExecuteUserCommand_DetachLearnHouse(human) : Human_DoExecuteUserCommand_DetachBuildHouse(human);

			if (result == 0)
				return 1;
		}

		if (Human_IsAttachedToWallPoint(human))
			BuildManager_DetachBuilder(gWallBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);

		if (Human_IsAttachedToRoadPoint(human))
			BuildManager_DetachBuilder(gRoadBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);

		auto* house = House(id);

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) != revision)
			return 1;

		if (!House_Inhabitants_AttachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 4))
		{
			Human_UserMessage_SendToUserMessage_CannotAttachHouse(human, 4);
			return 1;
		}

		Human_l_AssignWorkHouse(human, static_cast<std::uint32_t>(id));
		SetCenter(human, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));

		return 2;
	}

	// Original function: FUN_0043cf70.
	std::uint32_t Human_DoExecuteUserCommand_DetachBuildHouse(void* human)
	{
		if (!Human_IsJobBuilder(human))
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (!Human_IsAttachedToWorkHouse(human))
			return 1;

		if (House_Inhabitants_DetachHuman(House(Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 4))
			return 2;

		Human_UserMessage_SendToUserMessage_CannotDetachHouse(human, 4);

		return 1;
	}

	// Original function: FUN_0043d05f; existing connections are detached before target validation.
	std::uint32_t Human_DoExecuteUserCommand_AttachVehicle(void* human, std::int32_t id, std::uint32_t revision)
	{
		if (!Human_IsAbleToUseVehicle(human))
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (Human_IsAttachedToVehicle(human) && Human_DoExecuteUserCommand_DetachVehicle(human) == 0)
			return 1;

		if (Human_IsAttachedToWorkHouse(human) && DetachExistingWorkHouse(human) == 0)
			return 1;

		auto* vehicle = Vehicle(id);

		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId) != revision)
			return 1;

		if (!Vehicle_Passengers_AttachHuman(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)))
		{
			Human_UserMessage_SendToUserMessage_CannotAttachVehicle(human);
			return 1;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);
		Write(human, NC2Layout::HumanOffset::AttachedVehicleId, id);
		Write(human, NC2Layout::HumanOffset::AttachedVehicleRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
		Queue(human, 25, 0, 0);

		return 2;
	}

	// Original function: FUN_0043d181; passenger movement and detachment are distinct.
	std::uint32_t Human_DoExecuteUserCommand_DetachVehicle(void* human)
	{
		if (!Human_IsAbleToUseVehicle(human))
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (!Human_IsAttachedToVehicle(human))
			return 1;

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			auto* vehicle = Vehicle(Read<std::int32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId));
			const auto kind = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

			if ((kind == 3 || kind == 4) && Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) == 0)
				return 0;

			if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction) == 1)
			{
				std::byte command[20]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 101);
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
				Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
				Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
				std::uint32_t point = 0;
				MapMoveable_Attachment_GetNextMapPosition(gMapMoveableArrayStorage.entries + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable, &point);
				Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, MapMigPoint_GetLong(&point));
				Vehicle_DoAddNewUserCommand(vehicle, command);
				return 0;
			}

			if (!Vehicle_Passengers_MoveOut_h(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)))
				return 0;
		}

		if (Vehicle_Passengers_DetachHuman(Vehicle(Read<std::int32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)))
			return 2;

		Human_UserMessage_SendToUserMessage_CannotDetachVehicle(human);

		return 1;
	}

	// Original function: FUN_0043d297.
	std::uint32_t Human_DoExecuteUserCommand_SetWorkCenter(void* human, const void* point)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (!Human_WorkCenterMarker_IsAllowed(human))
			return 1;

		if (Human_IsJobBuilder(human))
		{
			const auto mode = Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode);

			if (mode != 0 && mode != 1)
				Human_StopCurrentGoodCirculationMode(human);

			if (Human_IsAttachedToWorkHouse(human))
			{
				if (Human_DoExecuteUserCommand_DetachBuildHouse(human) == 0)
					return 0;
			}
			else
			{
				if (Human_IsAttachedToWallPoint(human))
					BuildManager_DetachBuilder(gWallBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);

				if (Human_IsAttachedToRoadPoint(human))
					BuildManager_DetachBuilder(gRoadBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);
			}
		}

		SetCenter(human, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));

		return 2;
	}

	// Original function: FUN_0043d4ed; all 36 equipment command records are cleared.
	std::uint32_t Human_DoExecuteUserCommand_ChangeJob(void* human, std::int32_t job)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		std::memset(static_cast<std::byte*>(human) + 0x778, 0, 36u * 20u);

		if (static_cast<std::uint32_t>(job) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType))
			return 1;

		if (job >= 31 && job <= 41)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
				return 0;

			if (!Human_IsAbleToDoJob(human, static_cast<std::uint32_t>(job)))
			{
				switch (job)
				{
					case SwitchIds::HumanJob::SoldierUnarmed:
						Queue(human, 54, WeaponGood(human), NXBasics::IntegerRange::UInt32Maximum);
						break;
					case SwitchIds::HumanJob::SoldierSpearWooden:
						Queue(human, 63, 39, NXBasics::IntegerRange::UInt32Maximum);
						break;
					case SwitchIds::HumanJob::SoldierSpearIron:
						Queue(human, 63, 40, NXBasics::IntegerRange::UInt32Maximum);
						break;
					case SwitchIds::HumanJob::SoldierSwordShort:
						Queue(human, 63, 41, NXBasics::IntegerRange::UInt32Maximum);
						break;
					case SwitchIds::HumanJob::SoldierSwordLong:
						Queue(human, 63, 42, NXBasics::IntegerRange::UInt32Maximum);
						break;
					case SwitchIds::HumanJob::SoldierBowShort:
						Queue(human, 63, 37, NXBasics::IntegerRange::UInt32Maximum);
						break;
					case SwitchIds::HumanJob::SoldierBowLong:
						Queue(human, 63, 38, NXBasics::IntegerRange::UInt32Maximum);
						break;
					default:
						break;
				}

				switch (Human_Equipment_GetWeaponType(human))
				{
					case SwitchIds::WeaponType::Unarmed:
						job = 31;
						break;
					case SwitchIds::WeaponType::WoodenSpear:
						job = 32;
						break;
					case SwitchIds::WeaponType::IronSpear:
						job = 33;
						break;
					case SwitchIds::WeaponType::ShortSword:
						job = 34;
						break;
					case SwitchIds::WeaponType::LongSword:
						job = 35;
						break;
					case SwitchIds::WeaponType::ShortSaber:
						job = 36;
						break;
					case SwitchIds::WeaponType::LongSaber:
						job = 37;
						break;
					case SwitchIds::WeaponType::SmallAxe:
						job = 38;
						break;
					case SwitchIds::WeaponType::LargeAxe:
						job = 39;
						break;
					case SwitchIds::WeaponType::ShortBow:
						job = 40;
						break;
					case SwitchIds::WeaponType::LongBow:
						job = 41;
						break;
					default:
						break;
				}
			}
		}
		else
		{
			if (WeaponGood(human) != 0)
			{
				Human_Target_Reset(human);
				Human_CurrentTask_DoInterrupt(human);
				Queue(human, 72, WeaponGood(human), NXBasics::IntegerRange::UInt32Maximum);
			}
			else if (ArmorGood(human) != 0)
			{
				Human_Target_Reset(human);
				Human_CurrentTask_DoInterrupt(human);
				Queue(human, 73, ArmorGood(human), NXBasics::IntegerRange::UInt32Maximum);
			}

			Write<std::uint8_t>(human, NC2Layout::HumanOffset::MilitaryWorldRefreshmentFlag, 1);
		}

		return Human_DoExecuteUserCommand_SetJob(human, static_cast<std::uint32_t>(job));
	}

	// Original function: FUN_0043db6b; no additional unlock check occurs here.
	std::uint32_t Human_DoExecuteUserCommand_SetJob(void* human, std::uint32_t job)
	{
		if (job == Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType))
			return 1;

		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (Human_IsJobBuilder(human))
		{
			if (Human_IsAttachedToWorkHouse(human))
			{
				const auto result = Human_IsAttachedToLearnHouse(human) ? Human_DoExecuteUserCommand_DetachLearnHouse(human) : Human_DoExecuteUserCommand_DetachBuildHouse(human);

				if (result == 0)
					return 0;
			}
			else if (Human_IsAttachedToWallPoint(human))
			{
				BuildManager_DetachBuilder(gWallBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);
			}
			else if (Human_IsAttachedToRoadPoint(human))
			{
				BuildManager_DetachBuilder(gRoadBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);
			}
		}
		else if (Human_IsAttachedToWorkHouse(human))
		{
			const auto result = Human_IsAttachedToLearnHouse(human) ? Human_DoExecuteUserCommand_DetachLearnHouse(human) : Human_DoExecuteUserCommand_DetachWorkHouse(human);

			if (result == 0)
				return 0;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
		{
			Human_StopCurrentGoodCirculationMode(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		}

		Write<std::int32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId, -1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionWorkshopRevision, 0);
		Human_SetJobType(human, job);
		Human_Production_Setup(human);
		Human_Target_Reset(human);
		Human_Target_ResetSecondaryTarget(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);

		if (!Human_IsAttachedToWorkHouse(human))
			SetCenter(human, Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates));
		// The native notification call at 0045BFEC is a no-op (RET 4).

		return 2;
	}

	// Original function: FUN_0043ddc6; this command is not gated on interruptibility.
	std::uint32_t Human_DoExecuteUserCommand_ChangeProduction(void* human, std::uint32_t good, std::int32_t amount)
	{
		Human_Production_ChangeCounter(human, good, amount);

		if (amount > 0)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == 0)
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
				Write(human, NC2Layout::HumanOffset::ProductionRotationIndex, good);
				Write(human, NC2Layout::HumanOffset::ProducedGoodType, good);

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 1 && Human_l_IsAtomicInterruptable(human))
					Human_CurrentTask_DoInterrupt(human);
			}
		}
		else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == good && Human_Production_GetCounter(human, good) == 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 8 && Human_l_IsAtomicInterruptable(human))
		{
			Human_CurrentTask_DoInterrupt(human);
			Human_Production_Rotate(human);
		}

		Queue(human, 25, 0, 0);

		return 2;
	}

	// Original function: FUN_0043de68; unavailable schooling returns zero.
	std::uint32_t Human_DoExecuteUserCommand_LearnJob(void* human, std::uint32_t job)
	{
		if (!Human_l_IsAtomicInterruptable(human) || !Human_IsAttachedToLearnHouse(human))
			return 0;

		auto* house = House(Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));

		if (!House_Inhabitants_IsHumanAttachedAsStudent(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)))
			return 0;

		House_Inhabitants_SetStudentJob(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), job);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::EducationLevel, 0);
		// The native notification call at 0045BFEC is a no-op (RET 4).

		return 2;
	}

	// Original function: FUN_0043ded7; unavailable schooling returns one.
	std::uint32_t Human_DoExecuteUserCommand_LearnGood(void* human, std::uint32_t good)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (!Human_IsAttachedToLearnHouse(human))
			return 1;

		auto* house = House(Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));

		if (!House_Inhabitants_IsHumanAttachedAsStudent(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)))
			return 1;

		House_Inhabitants_SetStudentGood(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), good);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::EducationLevel, 0);
		// The native notification call at 0045BFEC is a no-op (RET 4).

		return 2;
	}

	// Original function: FUN_0043e023; restart shares the native 00445a6e leaf.
	std::uint32_t Human_DoExecuteUserCommand_RestartJobExecution(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 16 || !Human_l_IsAtomicInterruptable(human))
			return 0;

		Human_l_FinishTask_Idle(human);

		return 2;
	}

	// Original function: FUN_0045c331.
	void Human_UserMessage_SendToUserMessage_CannotDetachVehicle(void* human)
	{
		std::byte message[772];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 42);
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}
}
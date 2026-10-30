#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMilitaryJobs.h"
#include "C2HumanTaskAccess.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2MilitaryTargetSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		// Original function: FUN_0046ba2d; original semantic name remains unresolved.
		bool HasValidMilitaryTarget(const void* human)
		{

			return TargetData_IsValid(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget) || TargetData_IsValid(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::SecondaryMilitaryTarget);
		}

		std::uint32_t Distance(const void* source, const void* target)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(target, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(target, NC2Layout::MapCoordinatePairOffset::Y)));
		}
	}

	// Original function: FUN_0046b384.
	bool Human_l_CheckForEnemyTarget(void* human, const void* point, std::uint32_t minimumDistance, std::uint32_t maximumDistance, void* target)
	{
		const auto* weapon = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)]) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon);
		std::uint32_t candidate[3]{ };
		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);
		const bool allowLandscape = !HasValidMilitaryTarget(human) && task != 15 && task != 4;

		if (!CGameMapSectorAttachmentManager::sTheObjectPtr->CheckForEnemyTarget_Human(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), point, minimumDistance, maximumDistance, candidate, ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) >> 13) & 1u) != 0, allowLandscape))
			return false;

		if (TargetData_GetTargetType(candidate) == 6)
		{
			std::memcpy(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget, candidate, NC2Layout::RecordSize::TargetDataSigned);
			return false;
		}

		std::uint32_t candidatePosition = 0;
		TargetData_GetMapPosition(candidate, &candidatePosition);
		const auto* secondary = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::SecondaryMilitaryTarget;

		if (TargetData_IsValid(secondary))
		{
			if (Read<std::uint32_t>(secondary, NC2Layout::TargetDataOffset::Type) == candidate[0] && Read<std::uint32_t>(secondary, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition) == candidate[1] && Read<std::uint32_t>(secondary, NC2Layout::TargetDataOffset::ObjectRevision) == candidate[2])
				return false;

			std::uint32_t oldPosition = 0x27102710u;
			TargetData_GetMapPosition(secondary, &oldPosition);
			const auto oldDistance = Distance(&oldPosition, Human_GetMapPositionRef(human));

			if (oldDistance <= Distance(&candidatePosition, Human_GetMapPositionRef(human)))
				return false;
		}

		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&candidatePosition), NC2Layout::MapMicroPointOffset::ContinentId);

		if (continent != Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human)), NC2Layout::MapMicroPointOffset::ContinentId) && Distance(point, &candidatePosition) > static_cast<std::uint32_t>(weapon->maximumRange))
			return false;

		std::memcpy(target, candidate, NC2Layout::RecordSize::TargetDataSigned);

		return true;
	}

	// Original function: FUN_0046f41a; Windows dispatches jobs 31..47 to this same body.
	void Human_l_StartTask_ExecuteJob_Soldier(void* human)
	{
		const auto* weapon = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)]) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon);
		std::uint32_t target[3]{ };
		const auto* position = Human_GetMapPositionRef(human);
		auto* primary = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget;
		auto* secondary = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::SecondaryMilitaryTarget;
		auto* workCenter = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;

		if (Human_IsAttachedToWorkHouse(human))
		{
			auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId))
				House_Inhabitants_DetachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 2);

			const auto weaponType = NXBasics::Runtime_TimeSigned32(Human_Equipment_GetWeaponType(human));

			if (weaponType >= 16 && weaponType <= 17)
			{
				if (Human_l_IsAtomicInterruptable(human) && !TargetData_IsValid(primary) && Human_l_CheckForEnemyTarget(human, position, 0, static_cast<std::uint32_t>(weapon->maximumRange) + 5u, target))
				{
					std::memcpy(secondary, target, NC2Layout::RecordSize::TargetDataSigned);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);
					return;
				}
			}
			else if (!TargetData_IsValid(primary) && Human_l_CheckForEnemyTarget(human, workCenter, 0, 18, target))
			{
				std::memcpy(secondary, target, NC2Layout::RecordSize::TargetDataSigned);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);
				return;
			}
		}
		else
		{
			if (Human_IsAttachedToVehicle(human))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 ? 1u : 21u);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				return;
			}

			std::uint32_t searchPosition = 0, minimumDistance = 0, maximumDistance = 0, landscapeDistance = 5;

			switch (Human_GetMilitaryMode(human))
			{
				case SwitchIds::HumanMilitaryMode::SearchAroundPosition:
					searchPosition = Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates);
					maximumDistance = landscapeDistance = 18;
					break;
				case SwitchIds::HumanMilitaryMode::SearchAroundWorkCenter:
					searchPosition = Read<std::uint32_t>(workCenter, NC2Layout::MapCoordinatePairOffset::Coordinates);
					maximumDistance = 18;
					break;
				case SwitchIds::HumanMilitaryMode::SearchWithinWeaponRange:
					searchPosition = Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates);
					minimumDistance = static_cast<std::uint32_t>(weapon->minimumRange);
					maximumDistance = landscapeDistance = static_cast<std::uint32_t>(weapon->maximumRange);
					break;
				default:
					break;
			}

			if (!TargetData_IsValid(primary))
			{
				if (TargetData_GetTargetType(primary) == 6 && Human_GetPlayerType(human) != 2)
				{
					std::uint32_t landscape = 0;

					if (Tool_GetOneOfNearestEnemyLandscapes(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Human_GetMapPositionRef(human), static_cast<std::uint32_t>(weapon->minimumRange), landscapeDistance, true, 1, 0, &landscape))
					{
						Write(primary, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition, MapMigPoint_GetLong(&landscape));
						Write<std::uint32_t>(primary, NC2Layout::TargetDataOffset::Type, 6);
					}
					else
					{
						Write<std::uint32_t>(primary, NC2Layout::TargetDataOffset::Type, 0);
					}
				}
				else if (Human_l_CheckForEnemyTarget(human, &searchPosition, minimumDistance, maximumDistance, target))
				{
					std::memcpy(secondary, target, NC2Layout::RecordSize::TargetDataSigned);
				}
			}
		}

		if (TargetData_IsValid(primary) || TargetData_IsValid(secondary))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			return;
		}

		std::uint32_t returnDistance = 18;

		if (Human_IsAttachedToWorkHouse(human))
		{
			const auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;
			Write(workCenter, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);
			returnDistance = 0;
		}

		if (!Human_IsAtTargetPoint(human) && (Human_IsAttachedToWorkHouse(human) || Distance(position, workCenter) > returnDistance))
		{
			if (Human_l_StartAtomicWalk(human, workCenter, workCenter))
				return;

			auto replacement = Read<std::uint32_t>(workCenter, NC2Layout::MapCoordinatePairOffset::Coordinates);
			// Windows deliberately ignores the fallback lookup's return value.
			Tool_Map_GetNotBlockedPosition(&replacement, false);
			Write(workCenter, NC2Layout::MapCoordinatePairOffset::Coordinates, replacement);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);

			if (Human_l_StartAtomicWalk(human, workCenter, workCenter))
			{

				return;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
	}
}
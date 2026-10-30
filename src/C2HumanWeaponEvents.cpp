#include "C2AnimalAccess.h"
#include "C2DelayedActionManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanAttackTasks.h"
#include "C2HumanJobAccess.h"
#include "C2HumanWeaponEvents.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2MilitaryDamage.h"
#include "C2ObjectLayout.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		// Entire Windows static callback region 00554CF8..00554D1F; original type name unresolved.
		struct SArrowCallbackStorage
		{
			std::uint32_t source, target, sourceHeight, sourcePixelOffsetX, sourcePixelOffsetY, targetPixelOffsetX, targetPixelOffsetY, munition, flightTicks;
			std::uint8_t initialized;
			std::byte padding25[3];
		};

		static_assert(sizeof(SArrowCallbackStorage) == 40);
		static_assert(offsetof(SArrowCallbackStorage, flightTicks) == 0x20);
		static_assert(offsetof(SArrowCallbackStorage, initialized) == 0x24);

		// Original function: FUN_0046ba2c; registered Windows static exit callback has an empty body.
		int __cdecl HumanArrowCallback_OnExit()
		{

			return 0;
		}

		SArrowCallbackStorage gHumanArrowCallbackStorage{ };

		std::uint32_t Distance(const void* source, const void* target)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(target, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(target, NC2Layout::MapCoordinatePairOffset::Y)));
		}
	}

	// Original function: FUN_00469b78; primary target precedes secondary target, and invalid targets fail immediately.
	void Human_l_PerformTaskEvent_Attack(void* human)
	{
		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), weaponId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon);
		const auto& weapon = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[tribe])[weaponId];

		for (std::uint32_t index = 0; index < 2; ++index)
		{
			auto* target = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTargetUnsigned + index * NC2Layout::RecordSize::TargetData;
			const auto type = TargetData_GetTargetType(target);

			if (type == 0)
				continue;

			if (!TargetData_IsValid(target))
			{
				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
				return;
			}

			auto range = static_cast<std::uint32_t>(weapon.maximumRange);

			if (Human_IsAttachedToWorkHouse(human) && Human_IsJobSoldier(human) && weapon.hasMunition != 0)
				range += 5u;

			const auto id = TargetData_GetObjectId(target);
			std::uint32_t position = 0, moveableId = 0, distance = 0;
			bool hit = false;
			void* object = nullptr;

			switch (type)
			{
				case SwitchIds::TargetType::Landscape:
					TargetData_GetMapPosition(target, &position);
					distance = Distance(&position, Human_GetMapPositionRef(human));

					if (distance > range)
						continue;

					if (weapon.hasMunition == 0)
						hit = Tool_HitLandscapeOnPoint(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), tribe, weaponId, &position);

					break;
				case SwitchIds::TargetType::House:
					object = CHouseArray::mStaticVars + id * CHouseArray::RuntimeElementSize;

					if (!House_Military_GetNextFreeAttackPoint(object, Human_GetMapPositionRef(human), &position, human, false))
						continue;

					distance = Distance(&position, Human_GetMapPositionRef(human));

					if (distance > range)
						continue;

					if (weapon.hasMunition == 0)
					{
						hit = House_Military_StartGettingHitByWeapon(object, tribe, weaponId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 1, Human_GetExperienceForEquippedWeapon(human));
					}
					else
					{
						std::int16_t x = 0, y = 0;
						static_cast<const CMapArea*>(House_Misc_WalkBlockArea_GetPtr(object))->AreaTool_GetRandomPositionDelta(&x, &y);
						position = Read<std::uint32_t>(object, NC2Layout::HouseOffset::MapPosition);
						MapMigPoint_AddOffset(&position, x, y);
					}

					break;
				case SwitchIds::TargetType::Animal:
					object = CAnimalArray::mStaticVars + id * CAnimalArray::RuntimeElementSize;
					position = MapMigPoint_GetLong(Animal_GetMapPositionRef(object));
					distance = Distance(&position, Human_GetMapPositionRef(human));

					if (distance > range)
						continue;

					if (weapon.hasMunition == 0)
					{
						hit = Animal_Military_StartGettingHitByWeapon(object, tribe, weaponId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 1, Human_GetExperienceForEquippedWeapon(human), true);
					}
					else
					{
						moveableId = Read<std::uint32_t>(object, NC2Layout::AnimalOffset::MapMoveableId);
					}

					break;
				case SwitchIds::TargetType::Vehicle:
					object = CVehicleArray::mStaticVars + id * CVehicleArray::RuntimeElementSize;

					if (!Vehicle_Military_GetAttackPoint(object, Human_GetMapPositionRef(human), &position, nullptr, false))
					{
						// Native fallback calls direct damage even for a weapon with munition.
						hit = Vehicle_Military_StartGettingHitByWeapon(object, tribe, weaponId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 1, Human_GetExperienceForEquippedWeapon(human));

						if (hit)
						{
							Human_GainExperienceForEquippedWeapon(human, 1);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
							return;
						}

						continue;
					}

					distance = Distance(&position, Human_GetMapPositionRef(human));

					if (distance > range)
						continue;

					if (weapon.hasMunition == 0)
					{
						hit = Vehicle_Military_StartGettingHitByWeapon(object, tribe, weaponId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 1, 0);
					}
					else
					{
						moveableId = Read<std::uint32_t>(object, NC2Layout::VehicleOffset::MapMoveableId);
					}

					break;
				case SwitchIds::TargetType::Human:
					object = CHumanArray::mStaticVars + id * CHumanArray::RuntimeElementSize;
					position = MapMigPoint_GetLong(Human_GetMapPositionRef(object));
					distance = Distance(&position, Human_GetMapPositionRef(human));

					if (distance > range)
						continue;

					if (weapon.hasMunition == 0)
					{
						hit = Human_Military_StartGettingHitByWeapon(object, Human_GetMapPositionRef(human), tribe, weaponId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 1, Human_GetExperienceForEquippedWeapon(human), true);
					}
					else
					{
						moveableId = Read<std::uint32_t>(object, NC2Layout::HumanOffset::MapMoveableId);
					}

					break;
				default:
					continue;
			}

			if (weapon.hasMunition != 0)
			{
				if (type == 1 || type == 2 || type == 3)
				{
					const auto ticks = (distance << 3) / static_cast<std::uint32_t>(weapon.speed);
					const auto* moveable = gMapMoveableArrayStorage.entries + moveableId * NC2Layout::RecordSize::MapMoveable;

					if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0)
					{
						const auto direction = Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 ? Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::Direction) : 2u;
						const auto steps = (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick) * ticks + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress)) / 10000u;
						MapMigPoint_MoveCursorUnwrapped(&position, direction, steps);
					}
				}

				Human_Military_ShootArrowAtPosition(human, position);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				return;
			}

			if (hit)
			{
				Human_GainExperienceForEquippedWeapon(human, 1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);

				return;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_0046b7c7; signed low-word RNG remainders and callback-before-scheduling order.
	void Human_Military_ShootArrowAtPosition(void* human, std::uint32_t position)
	{
		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), weaponId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon);
		const auto& weapon = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[tribe])[weaponId];
		const auto chance = Human_IsJobHero(human) ? 0u : Game_Random_GetNumber() % 100u;
		auto source = MapMigPoint_GetLong(Human_GetMapPositionRef(human));
		std::uint32_t height = 0, offsetX = 20, offsetY = 20;

		if (Human_IsAttachedToWorkHouse(human) && Human_IsInWorkHouse(human))
		{
			const auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;
			const auto* data = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
			const auto count = Read<std::uint32_t>(data, NC2Layout::EngineHouseLogicViewOffset::ArrowSlotCount);

			if (count != 0)
			{
				const auto selected = Game_Random_GetNumber() % count;
				source = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);
				height = Read<std::uint32_t>(data, NC2Layout::EngineHouseLogicViewOffset::ArrowSlotParameterUnsigned + selected * NC2Layout::RecordSize::HouseArrowSlotUnsigned);
				offsetX = Read<std::uint32_t>(data, NC2Layout::EngineHouseLogicViewOffset::ArrowSlotXUnsigned + selected * NC2Layout::RecordSize::HouseArrowSlotUnsigned);
				offsetY = Read<std::uint32_t>(data, NC2Layout::EngineHouseLogicViewOffset::ArrowSlotYUnsigned + selected * NC2Layout::RecordSize::HouseArrowSlotUnsigned);
			}
		}

		const auto experience = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WeaponCategory6Experience);

		if (chance > experience + 10u)
		{
			const auto spread = (chance - experience - 10u) * (Distance(&position, &source) >> 2) / chance;
			const auto divider = NXBasics::Runtime_TimeSigned32(spread + 1u), half = NXBasics::Runtime_TimeSigned32(spread) / 2;
			const auto y = static_cast<std::int16_t>(Game_Random_GetNumber()) % divider - half;
			const auto x = static_cast<std::int16_t>(Game_Random_GetNumber()) % divider - half;
			MapMigPoint_AddOffset(&position, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
		}

		const auto ticks = (Distance(&position, &source) << 3) / static_cast<std::uint32_t>(weapon.speed);
		auto& callback = gHumanArrowCallbackStorage;

		if ((callback.initialized & 1u) == 0)
		{
			callback.initialized |= 1u;
			callback.source = 0;
			callback.target = 0;
			NXBasics::Runtime_RegisterOnExit(HumanArrowCallback_OnExit);
		}

		callback.source = source;
		callback.target = position;
		callback.sourceHeight = height;
		callback.sourcePixelOffsetX = offsetX;
		callback.sourcePixelOffsetY = offsetY;
		callback.targetPixelOffsetX = 20;
		callback.targetPixelOffsetY = 20;
		callback.munition = tribe == 2 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 47 ? 3u : static_cast<std::uint32_t>(weapon.munitionType);
		callback.flightTicks = ticks;
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x23, true, reinterpret_cast<std::uintptr_t>(&callback), 0, 0);
		DelayedActionManager_DelayedAction_AddWeaponHit(gDelayedActionManager, ticks, 1, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), &position, weaponId);
		GameMap_Exploration_DoExplore(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), &position, 1);
	}
}
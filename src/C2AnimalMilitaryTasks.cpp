#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "C2AnimalMilitaryTasks.h"
#include "C2AnimalTasks.h"
#include "C2GameControlNetwork.h"
#include "C2GameState.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2MilitaryAttackPointSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2VehicleAccess.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		const SStaticWeaponTypeData* Weapon(const void* animal)
		{

			return reinterpret_cast<const SStaticWeaponTypeData*>(reinterpret_cast<std::uintptr_t>(gStaticWeaponTypeArrays[Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType)]) + 0xC4u);
		}

		void* Animal(std::uint32_t id)
		{

			return CAnimalArray::mStaticVars + id * CAnimalArray::RuntimeElementSize;
		}

		std::uint32_t Distance(const void* first, const void* second)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(second, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(second, NC2Layout::MapCoordinatePairOffset::Y)));
		}

		// Original function: FUN_00446f48.
		bool __cdecl slAnimalSearchNextFreeMilitaryMapPointFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SAttackPointSearch*>(callback->userData);
			const auto sourceDistance = Distance(&data.source, point);

			if (sourceDistance > data.bestDistance)
				return true;

			const auto targetDistance = Distance(&data.target, point);

			if (targetDistance < data.minimumRange || targetDistance > data.maximumRange || Animal_l_Military_IsOtherAttackingAnimalOnPoint(data.owner, point))
				return false;

			data.bestDistance = sourceDistance;
			data.points[data.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return data.count == 6;
		}
	}

	// Original function: FUN_0044675c. Windows 004F432C is 20; search priority is soldiers, aggressive animals, adult civilians, other animals.
	bool Animal_l_Military_CheckForEnemy(void* animal)
	{
		const auto* center = static_cast<const std::byte*>(Animal(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId))) + NC2Layout::AnimalOffset::StayPoint;

		if (Distance(Animal_GetMapPositionRef(animal), center) > 40u || Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId) != Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId))
			return false;

		auto* target = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::MilitaryTarget;
		std::uint32_t targetDistance = 10000, bestDistance = 10000;

		if (TargetData_IsValid(target))
		{
			std::uint32_t point = 0;
			TargetData_GetMapPosition(target, &point);
			targetDistance = Distance(&point, center);
		}

		const auto owner = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId);
		auto consider = [&](const void* object, std::uint32_t kind, const void* position)
		{
			const auto id = Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::Id), revision = Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::UniqueId);

			if (Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type) == kind && Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint) == id && Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision) == revision)
				return;

			const auto distance = Distance(position, center);

			if (distance >= targetDistance || distance >= bestDistance)
			{

				return;
			}

			Write(target, NC2Layout::TargetDataOffset::Type, kind);
			Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, id);
			Write(target, NC2Layout::TargetDataOffset::ObjectRevision, revision);
			bestDistance = distance;
		};

		for (std::uint32_t pass = 0; pass < 4; ++pass)
		{
			const bool humans = (pass & 1u) == 0;

			for (std::uint32_t player = 0; player < (humans ? 20u : 21u); ++player)
			{
				if (player == owner || (player < 20 && Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer) == 0))
					continue;

				if (owner != 20 && player != 20 && GameControl_Diplomacy_GetState(gGameControl, owner, player) != 3)
					continue;

				if (humans)
				{
					CHumanIterator iterator;

					for (auto id = iterator.GetFirstSectorHumanId(player, Read<std::uint32_t>(center, NC2Layout::MapCoordinatePairOffset::Coordinates), 20, pass == 0); id != -1; id = iterator.GetNextSectorHumanId())
					{
						const auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

						if (Human_Misc_IsHidden(human) || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & ((1u << 3) | (1u << 13))) != 0 || (pass == 2 && !Human_IsAgeAdult(human)))
							continue;

						consider(human, 1, Human_GetMapPositionRef(human));
					}
				}
				else
				{
					CAnimalIterator iterator;

					for (auto id = iterator.GetFirstSectorAnimalId(player, Read<std::uint32_t>(center, NC2Layout::MapCoordinatePairOffset::Coordinates), 20, pass == 1, false); id != -1; id = iterator.GetNextSectorAnimalId())
					{
						const auto* other = Animal(static_cast<std::uint32_t>(id));
						consider(other, 3, Animal_GetMapPositionRef(other));
					}
				}
			}

			if (bestDistance != 10000)
				return true;
		}

		return false;
	}

	// Original function: FUN_00446d57.
	void Animal_l_StartTask_Attack(void* animal)
	{
		auto* target = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::MilitaryTarget;

		if (!TargetData_IsValid(target))
		{
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
			return;
		}

		Animal_l_Speed_Fast(animal);
		const auto* weapon = Weapon(animal);

		if (weapon == nullptr)
		{
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
			return;
		}

		const auto kind = TargetData_GetTargetType(target);

		if (kind != 1 && kind != 3)
		{
			Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 0);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 1);
			return;
		}

		const auto* object = kind == 1 ? CHumanArray::mStaticVars + TargetData_GetObjectId(target) * CHumanArray::RuntimeElementSize : static_cast<std::byte*>(Animal(TargetData_GetObjectId(target)));
		std::uint32_t point = 0;
		TargetData_GetMapPosition(target, &point);
		const auto* source = Animal_GetMapPositionRef(animal);
		const auto distance = Distance(&point, source);

		if (distance > static_cast<std::uint32_t>(weapon->maximumRange))
		{
			Animal_l_StartAtomicWalk(animal, Read<std::int32_t>(object, NC2Layout::HumanOffset::MapMoveableId), weapon->maximumRange);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 8);
			return;
		}

		if (distance < static_cast<std::uint32_t>(weapon->minimumRange))
		{
			if (!Animal_l_Military_GetNextPointToStartAttack(animal, &point))
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 1);
				return;
			}

			if (Animal_l_StartAtomicWalk(animal, &point))
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 8);
				return;
			}
		}

		if (Animal_l_Military_IsOtherAttackingAnimalOnPoint(animal, source))
		{
			auto next = point;

			if (Animal_l_Military_GetNextPointToStartAttack(animal, &next) && next != Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates) && Animal_l_StartAtomicWalk(animal, &next))
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 8);

				return;
			}
		}

		Animal_l_Military_AttackTarget(animal, &point);
	}

	// Original function: FUN_00446ec5.
	bool Animal_l_Military_GetNextPointToStartAttack(const void* animal, void* point)
	{
		SAttackPointSearch data;
		data.owner = animal;
		data.minimumRange = Weapon(animal)->minimumRange;
		data.maximumRange = Weapon(animal)->maximumRange;
		data.target = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		data.source = Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapCoordinatePairOffset::Coordinates);

		if (!CMapPathfinderElement::SearchOverMap(&data.source, 5, true, slAnimalSearchNextFreeMilitaryMapPointFunction, &data, false, Read<std::int32_t>(animal, NC2Layout::AnimalOffset::PlayerId)))
			return false;

		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, data.points[Game_Random_GetNumber() % data.count]);

		return true;
	}

	// Original function: FUN_0044703a.
	bool Animal_l_Military_IsOtherAttackingAnimalOnPoint(const void* animal, const void* point)
	{
		CMapMoveableIterator iterator;

		for (auto id = iterator.GetFirstMapMoveableId(point); id != -1; id = iterator.GetNextMapMoveableId())
		{
			if (id == Read<std::int32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId))
				continue;

			const auto* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

			if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) != 2)
				continue;

			const auto* other = Animal(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId));

			if (Read<std::uint32_t>(other, NC2Layout::AnimalOffset::PlayerId) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) && Read<std::uint32_t>(other, NC2Layout::AnimalOffset::AnimalType) == 49 && Read<std::uint8_t>(other, NC2Layout::AnimalOffset::AggressiveFlag) != 0 && Read<std::uint32_t>(other, NC2Layout::AnimalOffset::AtomicAction) == 81)
				return true;
		}

		return false;
	}

	// Original function: FUN_004470aa.
	void Animal_l_Military_AttackTarget(void* animal, const void* point)
	{
		const auto* source = Animal_GetMapPositionRef(animal);
		const auto direction = VE_HexagonDirection_GetDirectionFromPositions(Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), false);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, Animal_l_StartAtomicAnimation(animal, Weapon(animal)->atomicActionType, direction) ? 1u : 3u);
	}
}
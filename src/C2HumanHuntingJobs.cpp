#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "C2GameMapAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanHuntingJobs.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanLandscapeSearch.h"
#include "C2HumanProduction.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_0045e3c4; nearest prey is weighted by its hitpoints.
	void* Human_FindAnimalForHunting(const void* human)
	{
		const void* position = Human_GetMapPositionRef(human);
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId);
		CAnimalIterator iterator;
		void* best = nullptr;
		std::uint32_t bestDistance = 10000;
		// Remaster: double the original Windows live-prey search radius around the work flag.

		for (auto id = iterator.GetFirstSectorAnimalId(20, Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter), 100, false, true); id != -1; id = iterator.GetNextSectorAnimalId())
		{
			void* animal = CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::ElementSize;

			if ((Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MiscFlags) & 8u) != 0)
				continue;

			const void* point = Animal_GetMapPositionRef(animal);

			if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId) != continent)
				continue;

			const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y))) + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::Hitpoints);

			if (distance >= bestDistance)
				continue;

			best = animal;
			bestDistance = distance;
		}

		return best;
	}

	// Original function: FUN_00460a91; search the same cached landscape candidates twice, prioritizing type 80.
	bool Human_FindCadaverForHarvesting(const void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source)
	{
		SHumanSearchLandscapesData data;
		data.Add(79);
		data.Add(80);
		data.SetCurrent(80);

		if (Human_FindLandscapesNearPos_NoBlock(human, data, realPosition, workPosition, source))
		{
			*good = 21;
			return true;
		}

		data.SetCurrent(79);

		if (Human_FindLandscapesNearPos_NoBlock(human, data, realPosition, workPosition, source))
		{
			*good = 9;
			return true;
		}

		return false;
	}

	// Original function: FUN_0046c8de; only circulation states 0, 2, 4 and 5 have actions.
	void Human_l_StartTask_ExecuteJob_Hunter(void* human)
	{
		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode))
		{
			case SwitchIds::GoodCirculationMode::None:
				Human_l_StartTask_ExecuteJob_Hunter_TryToCollect(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockFetch:
				Human_l_StartTask_ExecuteJob_Hunter_Collect(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockBringAway:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
				return;
			case SwitchIds::GoodCirculationMode::FlushStock:
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
				return;
			default:

				return;
		}
	}

	// Original function: FUN_0046c914.
	void Human_l_StartTask_ExecuteJob_Hunter_Collect(void* human)
	{
		if (!Human_IsAtTargetPoint(human))
		{
			Human_StopCurrentGoodCirculationMode(human);
			Human_l_StartTask_ExecuteJob_Hunter_TryToCollect(human);

			return;
		}

		const auto landscape = Read<std::int32_t>(human, NC2Layout::HumanOffset::TargetLandscapeType);

		if (landscape == 25 || landscape == 44)
		{
			Human_l_StartAtomicAnimation(human, 22, static_cast<std::int32_t>(Human_Target_GetDirectionTo(human)));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		}
		else if (landscape > 78 && landscape <= 80)
		{
			Human_l_StartTask_ExecuteJob_Hunter_Harvest(human);
		}
		else
		{
			Human_Target_Reset(human);
		}
	}

	// Original function: FUN_0046c979; saved cadaver state is separate from the military target.
	void Human_l_StartTask_ExecuteJob_Hunter_TryToCollect(void* human)
	{
		Human_Target_Reset(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget) == 2 && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryTargetLandscapeType) == 79 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryTargetLandscapeType) == 80))
		{
			const void* real = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::SecondaryTargetPosition;
			const void* micro = GameMap_GetConstMapMicroPointPtr(real);
			const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);

			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && (landscape == 79 || landscape == 80))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, landscape == 79 ? 9 : 21);

				if (Human_l_StartAtomicWalkAndFetch(human, real, static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::SecondaryTargetWorkPosition, false))
					return;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget, 0);
		std::uint32_t real = 0, work = 0, good;

		if (Human_FindCadaverForHarvesting(human, &real, &work, &good, Human_GetMapPositionRef(human)) || Human_FindCadaverForHarvesting(human, &real, &work, &good, static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter))
		{
			Write(human, NC2Layout::HumanOffset::RequestedGood, good);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 0);
			Write(human, NC2Layout::HumanOffset::SecondaryTargetPosition, real);
			Write(human, NC2Layout::HumanOffset::SecondaryTargetWorkPosition, work);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget, 2);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryTargetLandscapeType, good == 9 ? 79 : 80);

			if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
				return;
		}

		void* target = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget;
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 56);

		if (TargetData_GetTargetType(target) != 3 || !TargetData_IsValid(target))
		{
			const void* animal = Human_FindAnimalForHunting(human);

			if (animal == nullptr)
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 3);

				return;
			}

			Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId));
			Write(target, NC2Layout::TargetDataOffset::ObjectRevision, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::UniqueId));
			Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 3);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);
	}

	// Original function: FUN_0046cb0e.
	void Human_l_StartTask_ExecuteJob_Hunter_Harvest(void* human)
	{
		Human_l_StartAtomicAnimation(human, 33, static_cast<std::int32_t>(Human_Target_GetDirectionTo(human)));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}
}
#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "C2GameControlStartup.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanAnimalJobs.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanProduction.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* House(std::uint32_t id)
		{

			return CHouseArray::mStaticVars + id * CHouseArray::RuntimeElementSize;
		}

		void* Animal(std::uint32_t id)
		{

			return CAnimalArray::mStaticVars + id * CAnimalArray::RuntimeElementSize;
		}

		std::uint32_t Distance(const void* first, const void* second)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(second, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(second, NC2Layout::MapCoordinatePairOffset::Y)));
		}

		void Fail(void* human, std::uint32_t reason)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write(human, NC2Layout::HumanOffset::CurrentTaskStep, reason);
		}

		struct SBreederAnimalCandidates
		{
			struct SEntry
			{
				const void* house;
				std::uint32_t count;
				void* animal;
				std::uint32_t distance;
			};

			std::uint32_t count = 0;
			SEntry entries[5];

			// Original function: FUN_0046d063; grouping is by house pointer.
			void Add(void* animal, const void* house, const void* point)
			{
				std::uint32_t index = 0;

				while (index < count && entries[index].house != house)
					++index;

				if (index == count)
				{
					if (index == 5)
					{

						return;
					}

					entries[index].house = house;
					entries[index].count = 0;
					entries[index].distance = 10000;
					++count;
				}

				auto& entry = entries[index];
				++entry.count;
				const auto distance = Distance(point, Animal_GetMapPositionRef(animal));

				if (distance < entry.distance)
				{
					entry.animal = animal;
					entry.distance = distance;
				}
			}

			// Original function: FUN_0046d0fb; only groups with more than two animals qualify.
			void* GetNearest() const
			{
				void* result = nullptr;
				std::uint32_t minimum = 10000;

				for (std::uint32_t index = 0; index < count; ++index)
				{
					if (entries[index].count > 2 && entries[index].distance < minimum)
					{
						result = entries[index].animal;
						minimum = entries[index].distance;
					}
				}

				return result;
			}
		};

		static_assert(sizeof(SBreederAnimalCandidates) == (sizeof(void*) == 8 ? 168 : 84));
		static_assert(sizeof(SBreederAnimalCandidates::SEntry) == (sizeof(void*) == 8 ? 32 : 16));
	}

	// Original function: FUN_0046cb2c.
	void Human_l_StartTask_ExecuteJob_Breeder(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (!Human_IsAttachedToWorkHouse(human) || good == 0)
		{
			Fail(human, 2);
			return;
		}

		if (!Human_IsAbleToProduceGoodNow(human, good))
		{
			Fail(human, 0);
			return;
		}

		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode))
		{
			case SwitchIds::GoodCirculationMode::FillStockFetch:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_Fetch(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockBringAway:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
				return;
			case SwitchIds::GoodCirculationMode::FlushStock:
				Human_l_StartTask_ExecuteJob_Tool_FlushStock_Fetch(human);
				return;
			case SwitchIds::GoodCirculationMode::None:
				break;
			default:
				return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		auto* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::ProductionStoppedFlag) != 0 || !House_Stock_CanStoreGoodType(house, good, true))
		{
			Fail(human, 2);
			return;
		}

		for (const auto animalGood : { 57u, 58u })
		{
			House_Stock_ModifyAmount(house, animalGood, -static_cast<std::int32_t>(House_Stock_GetAmountOfGoods(house, animalGood, false)), 0, false);
		}

		const auto animalType = good == 57 ? 19u : 10u;
		bool hasInteracting = false;
		std::uint32_t adults = 0, nearestDistance = 10000, slaughterDistance = 10000;
		void* nearest = nullptr;
		void* slaughter = nullptr;
		SBreederAnimalCandidates candidates;
		CAnimalIterator iterator;

		for (auto id = iterator.GetFirstAnimalId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)); id != -1; id = iterator.GetNextAnimalId())
		{
			auto* animal = Animal(static_cast<std::uint32_t>(id));
			const auto type = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType);

			if ((type != 19 && type != 10) || (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MiscFlags) & 8u) != 0 || Animal_IsAttachedToVehicle(animal))
				continue;

			if (Animal_IsAttachedToHouse(animal))
			{
				auto* currentHouse = House(TargetData_GetObjectId(static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget));

				if (Read<std::uint32_t>(currentHouse, NC2Layout::HouseOffset::HouseId) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId))
				{
					House_Stock_ModifyAmount(house, type == 19 ? 57u : 58u, 1, 0, false);

					if (type != animalType || Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) != 49)
						continue;

					++adults;

					if (hasInteracting)
						continue;

					const auto distance = Distance(Animal_GetMapPositionRef(animal), static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint);

					if (distance >= slaughterDistance)
						continue;

					if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask) == 10)
						hasInteracting = true;

					slaughter = animal;
					slaughterDistance = distance;
					continue;
				}

				if (type != animalType)
					continue;

				if (House_GetHouseMainType(currentHouse) != 1)
				{
					candidates.Add(animal, currentHouse, static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition);
					continue;
				}
			}

			const auto distance = Distance(static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, Animal_GetMapPositionRef(animal));

			if (distance < nearestDistance)
			{
				nearest = animal;
				nearestDistance = distance;
			}
		}

		if (!House_Stock_IsFullSoon(house, good))
		{
			if (nearest == nullptr && House_Stock_GetAmountOfGoods(house, good, false) < 2)
				nearest = candidates.GetNearest();

			if (nearest != nullptr)
			{
				Animal_AttachToHouse(nearest, Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId));
				House_Stock_ModifyAmount(house, good, 1, 0, false);
			}
		}

		const auto flush = [human](std::uint32_t output)
		{
			Write(human, NC2Layout::HumanOffset::RequestedGood, output);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 5);
			Human_l_StartTask_ExecuteJob_Tool_FlushStock_Fetch(human);
		};

		if (House_Stock_IsFullSoon(house, 21))
		{
			flush(21);
			return;
		}

		const auto secondaryGood = good == 57 ? 10u : 9u;

		if (House_Stock_IsFullSoon(house, secondaryGood))
		{
			flush(secondaryGood);
			return;
		}

		if (adults > 2)
		{
			auto slaughterPosition = Read<std::uint32_t>(Animal_GetMapPositionRef(slaughter), NC2Layout::MapCoordinatePairOffset::Coordinates);
			const auto workingPosition = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint), entryPosition = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);

			if (slaughterPosition == workingPosition)
			{
				if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == workingPosition)
				{
					CAnimalArray::sTheObjectPtr->FreeAnimal(Read<std::int32_t>(slaughter, NC2Layout::AnimalOffset::AnimalId));
					const auto* micro = GameMap_GetConstMapMicroPointPtr(&slaughterPosition);

					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 79 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 80))
					{
						Logic_EntryPoint_Edit_Start();
						Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(&slaughterPosition, 0, 0, -1, false, -1);
						Logic_EntryPoint_Edit_Stop();
					}

					Write(human, NC2Layout::HumanOffset::RequestedGood, good);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, Human_l_StartAtomicAnimation(human, animalType == 19 ? 87u : 88u, -1) ? 1u : 3u);
					return;
				}

				if (Human_l_StartAtomicWalk(human, &entryPosition, &workingPosition))
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
				}
				else
				{
					Fail(human, 1);
				}

				return;
			}

			const auto distance = Distance(Animal_GetMapPositionRef(slaughter), Human_GetMapPositionRef(human));

			if (distance <= 2)
				Animal_GoIntoHouseInteractionMode(slaughter);

			if (distance <= 1 || !Human_l_StartAtomicWalk(human, Read<std::int32_t>(slaughter, NC2Layout::AnimalOffset::MapMoveableId), 1))
				Human_l_StartAtomicAnimation(human, 2, -1);

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		if (House_Stock_IsFullSoon(house, good))
		{
			Write(human, NC2Layout::HumanOffset::RequestedGood, good);
			Fail(human, 10);
			return;
		}

		if (adults >= 2)
		{
			Human_l_StartTask_ExecuteJob_Tool_TryToProduce(human);

			return;
		}

		Write(human, NC2Layout::HumanOffset::RequestedGood, good);
		Fail(human, 3);
	}

	// Original function: FUN_004692e9; count two attached adult animals before allocation.
	void Human_l_PerformTaskEvent_Produce_Animal(void* human)
	{
		const auto* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));
		const auto type = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == 57 ? 19u : 10u;
		CAnimalIterator iterator;
		std::uint32_t adults = 0;

		for (auto id = iterator.GetFirstSectorAnimalId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition), 40, false, true); id != -1; id = iterator.GetNextSectorAnimalId())
		{
			auto* animal = Animal(static_cast<std::uint32_t>(id));

			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) != type || !Animal_IsAttachedToHouse(animal) || TargetData_GetUniqueId(static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) || Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) != 49)
				continue;

			if (++adults < 2)
				continue;

			const auto newborn = Logic_EntryPoint_Add_Animal(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), type, 48, Animal_GetMapPositionRef(animal), 0, 0);

			if (newborn == -1)
				return;

			Animal_AttachToHouse(Animal(static_cast<std::uint32_t>(newborn)), Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId));
			Human_l_JobExperience_Gain(human);
			Human_Production_DidProduceGoods(human, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);

			return;
		}
	}
}
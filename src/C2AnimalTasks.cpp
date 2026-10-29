#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "C2AnimalMilitaryTasks.h"
#include "C2AnimalTasks.h"
#include "GameProfile.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanInitialization.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2MilitaryDamage.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2ObjectMemberConstructors.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2StaticAtomicAnimationData.h"
#include "C2VehicleAccess.h"
#include "C2VehicleAnimalPassengers.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* Animal(std::uint32_t id)
		{

			return CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize;
		}

		void* Moveable(const void* animal)
		{

			return gMapMoveableArrayStorage.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		}

		const SStaticAnimalTypeData& Type(const void* animal)
		{

			return static_cast<const SStaticAnimalTypeData*>(gStaticAnimalTypeData)[Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) - NRemaster::GetFirstAnimalTribe()];
		}

		std::uint32_t Distance(const void* first, const void* second)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(second, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(second, NC2Layout::MapCoordinatePairOffset::Y)));
		}

		bool IsAggressive(const void* animal)
		{

			return Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 49 && Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AggressiveFlag) != 0;
		}

		const void* Macro(const void* point)
		{
			std::uint32_t macro = 0;
			MapMagPoint_ConstructFromMig(&macro, point, false);

			return GameMap_GetConstMapMacroPointPtr(&macro);
		}

		// Original functions: FUN_0042f231, FUN_0042f257, FUN_0042f27d.
		// Original helper names remain unresolved; Windows proves these limits.
		std::uint32_t LeaderDistance(const void* animal)
		{

			return Animal_IsAttachedToHouse(animal) ? 15u : static_cast<std::uint32_t>(Type(animal).maximumLeaderDistance);
		}

		std::uint32_t BirthDistance(const void* animal)
		{

			return Animal_IsAttachedToHouse(animal) ? 10u : static_cast<std::uint32_t>(Type(animal).maximumDistanceToBirthPoint);
		}

		std::uint32_t StayDistance(const void* animal)
		{

			return (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MiscFlags) & 4u) != 0 || Animal_IsAttachedToHouse(animal) ? 5u : static_cast<std::uint32_t>(Type(animal).maximumDistanceToStayPoint);
		}

		// Original function: FUN_0042ee89.
		bool Animal_l_AttachLeader(void* animal, void* leader)
		{
			if (animal == leader)
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::GroupSize, 0);

			if (Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::GroupSize) >= static_cast<std::uint32_t>(Type(animal).maximumGroupSize))
				return false;

			Write(animal, NC2Layout::AnimalOffset::LeaderId, Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::AnimalId));
			Write(animal, NC2Layout::AnimalOffset::LeaderRevision, Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::UniqueId));
			Write(leader, NC2Layout::AnimalOffset::GroupSize, Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::GroupSize) + 1u);

			return true;
		}

		// Original function: FUN_0042eb8b.
		bool Animal_SetLeaderId(void* animal, std::uint32_t id)
		{
			auto* leader = Animal(id);

			return Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::TribeType) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) && !Animal_l_IsLeaderValid(animal) && Animal_l_AttachLeader(animal, leader);
		}

		// Original function: FUN_0042ec3c.
		void Animal_l_CheckForValidLeader(void* animal)
		{
			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId) || Animal_l_IsLeaderValid(animal))
				return;

			Animal_l_DetachLeader(animal);

			if (Animal_IsAttachedToVehicle(animal))
			{
				Animal_SetLeaderId(animal, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId));
				return;
			}

			const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Animal_GetMapPositionRef(animal)), NC2Layout::MapMicroPointOffset::ContinentId);
			CAnimalIterator iterator;
			const bool attached = Animal_IsAttachedToHouse(animal);
			auto id = attached ? iterator.GetFirstAnimalId(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId)) : -1;

			if (!attached && (Type(animal).searchForLeader != 0 || Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 48))
				id = iterator.GetFirstAnimalId(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId), Animal_GetMapPositionRef(animal), 40);

			for (; id != -1; id = iterator.GetNextAnimalId())
			{
				auto* other = Animal(static_cast<std::uint32_t>(id));

				if (Read<std::uint32_t>(other, NC2Layout::AnimalOffset::TribeType) != Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) || Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Animal_GetMapPositionRef(other)), NC2Layout::MapMicroPointOffset::ContinentId) != continent || !Animal_l_IsLeaderValid(other))
					continue;

				auto* leader = Animal(Read<std::uint32_t>(other, NC2Layout::AnimalOffset::LeaderId));

				if (attached && (!Animal_IsAttachedToHouse(animal) || TargetData_GetUniqueId(static_cast<const std::byte*>(leader) + NC2Layout::AnimalOffset::AttachmentTarget) != TargetData_GetUniqueId(static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget)))
					continue;

				if (Animal_l_AttachLeader(animal, leader))
					return;
			}

			if (Read<std::int32_t>(animal, NC2Layout::AnimalOffset::LeaderId) != -1)
			{

				return;
			}

			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::EnvironmentalFoodValue, 0);
			Write(animal, NC2Layout::AnimalOffset::BirthPoint, Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapCoordinatePairOffset::Coordinates));
			Animal_l_AttachLeader(animal, animal);
			id = iterator.GetFirstAnimalId(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId), Animal_GetMapPositionRef(animal), 40);

			for (; id != -1; id = iterator.GetNextAnimalId())
			{
				Animal_SetLeaderId(Animal(static_cast<std::uint32_t>(id)), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId));
			}
		}

		struct SNearbyPoints
		{
			SNearbyPoints()
			{
				SearchNearbyMapPoints_ConstructMembers(this);
			}

			std::uint32_t count = 0;
			std::uint32_t points[8];
		};

		struct SLandPoints
		{
			// Original constructor: FUN_0046158c.
			SLandPoints()
			{
				for (auto& point : points)
				{
					point = 0;
				}
			}

			std::uint32_t minimumDistance;
			std::uint32_t count;
			std::uint32_t points[8];
		};

		static_assert(sizeof(SNearbyPoints) == 36);
		static_assert(sizeof(SLandPoints) == 40);

		// Windows uses the shared slHumanSearchMapPointNearbyFunction (0045D533).
		bool __cdecl SearchNearby(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& candidates = *static_cast<SNearbyPoints*>(callback->userData);
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || Read<std::int16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId) != -1)
				return false;

			candidates.points[candidates.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return candidates.count == 8;
		}

		// Original function: FUN_00461653.
		bool __cdecl slAnimalSearchLandMapPoint(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SLandPoints*>(callback->userData);

			if (callback->distance < data.minimumDistance)
				return false;

			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || Read<std::int16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId) != -1)
				return false;

			data.points[data.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return data.count == 1;
		}

		// Original function: FUN_0046128f.
		bool __cdecl slAnimalSearchFoodPatterns(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || Read<std::int16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId) != -1 || Read<std::uint8_t>(Macro(point), NC2Layout::MapMacroPointOffset::PatternA) != 2)
				return false;

			Write(callback->userData, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));

			return true;
		}

		// Original function: FUN_00461308.
		bool __cdecl CalculateFoodPattern(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 && Read<std::uint8_t>(Macro(point), NC2Layout::MapMacroPointOffset::PatternA) == 2)
				Write(callback->userData, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(callback->userData, NC2Layout::MapCoordinatePairOffset::Coordinates) + 100u);

			return false;
		}

		// Original function: FUN_004617e4.
		bool __cdecl slAnimalSearchMapPointNearbyFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SNearbyPoints*>(callback->userData);
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			const auto id = Read<std::int16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId);

			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || id == -1)
				return false;

			const auto* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

			if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) != 1 || Human_IsJobSoldier(CHumanArray::mStaticVars + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * CHumanArray::ElementSize))
				return false;

			data.points[data.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return data.count == 8;
		}

		// Original function: FUN_00461789.
		bool FindSheepPlayPoint(const void* animal, void* point)
		{
			SNearbyPoints data;
			CMapPathfinderElement::SearchOverMap(Animal_GetMapPositionRef(animal), 20, false, slAnimalSearchMapPointNearbyFunction, &data, false, Read<std::int32_t>(animal, NC2Layout::AnimalOffset::PlayerId));

			if (data.count == 0)
				return false;

			Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, data.points[Game_Random_GetNumber() % data.count]);

			return true;
		}

		// Original function: FUN_004612d9.
		void Animal_l_CalculateFoodValue(void* animal)
		{
			auto* value = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::EnvironmentalFoodValue;
			CMapPathfinderElement::SearchOverMap(Animal_GetMapPositionRef(animal), 10, true, CalculateFoodPattern, value, false, Read<std::int32_t>(animal, NC2Layout::AnimalOffset::PlayerId));

			if (Read<std::int32_t>(value, NC2Layout::MapCoordinatePairOffset::Coordinates) < 5000)
				Write<std::uint32_t>(value, NC2Layout::MapCoordinatePairOffset::Coordinates, 5000);
		}

		// Original function: FUN_00461531.
		bool Animal_FindMapPointNearby(const void* animal, void* point)
		{
			SNearbyPoints data;
			CMapPathfinderElement::SearchOverMap(Animal_GetMapPositionRef(animal), 40, false, SearchNearby, &data, false, Read<std::int32_t>(animal, NC2Layout::AnimalOffset::PlayerId));

			if (data.count == 0)
				return false;

			Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, data.points[Game_Random_GetNumber() % data.count]);

			return true;
		}

		// Original function: FUN_004615a4; the seed cursor advances cumulatively.
		bool FindNextLandPoint(const void* animal, void* result)
		{
			SLandPoints data;
			data.minimumDistance = 5;
			data.count = 0;
			auto point = MapMigPoint_GetLong(Animal_GetAnimalWorkCenterPointRef(animal));
			const auto direction = Game_Random_GetNumber() % 6u;

			for (std::uint32_t step = 0; step < 5; ++step)
			{
				if ((Read<std::uint8_t>(GameMap_GetMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
					break;

				MapMigPoint_MoveCursorUnwrapped(&point, direction, 1);
			}

			CMapPathfinderElement::SearchOverMap(&point, 40, true, slAnimalSearchLandMapPoint, &data, false, Read<std::int32_t>(animal, NC2Layout::AnimalOffset::PlayerId));

			if (data.count == 0)
				return false;

			Write(result, NC2Layout::MapCoordinatePairOffset::Coordinates, data.points[0]);

			return true;
		}

		// Original function: FUN_00461698; the ring cursor is deliberately not reset.
		bool FindBirthPoint(const void* current, const void* center, void* result, std::uint32_t radius)
		{
			const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(current), NC2Layout::MapMicroPointOffset::ContinentId);
			auto point = Read<std::uint32_t>(center, NC2Layout::MapCoordinatePairOffset::Coordinates);
			MapMigPoint_MoveCursorUnwrapped(&point, 4, radius);
			std::uint32_t minimum = 1000000;

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::uint32_t step = 0; step < radius; ++step)
				{
					MapMigPoint_MoveCursorUnwrapped(point, direction);

					if (MapMigPoint_IsBorderPoint(&point))
						continue;

					const auto* micro = GameMap_GetConstMapMicroPointPtr(&point);

					if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) != continent || (minimum != 1000000 && Read<std::uint8_t>(Macro(&point), NC2Layout::MapMacroPointOffset::PatternA) != 2))
						continue;

					const auto distance = Distance(&point, current);

					if (distance >= minimum)
						continue;

					minimum = distance;
					Write(result, NC2Layout::MapCoordinatePairOffset::Coordinates, point);
				}
			}

			return minimum != 1000000;
		}

		// Entire Windows static region 00512600..00512618: six candidate points and initialization flag.
		struct SBirthCandidates
		{
			std::uint32_t points[6]{ };
			std::uint8_t initialized = 0;
		};

		SBirthCandidates gAnimalBirthCandidates;
		static_assert(offsetof(SBirthCandidates, initialized) == 0x18);

		// Original function: FUN_0042f2ac; the six trivial point records need no cleanup.
		int __cdecl AnimalBirthCandidates_OnExit()
		{

			return 0;
		}

		// Original function: FUN_0042ef98.
		void Animal_l_MoveBirthPoint(void* animal)
		{
			if ((Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MiscFlags) & 1u) != 0)
				return;

			const auto* current = Animal_GetMapPositionRef(animal);
			auto* work = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::WorkCenter;
			auto* birth = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::BirthPoint;
			const auto birthRadius = BirthDistance(animal), stayRadius = StayDistance(animal);

			if (Distance(current, work) >= birthRadius + stayRadius)
			{
				FindBirthPoint(current, work, birth, birthRadius >> 1);
				return;
			}

			if ((gAnimalBirthCandidates.initialized & 1u) == 0)
			{
				gAnimalBirthCandidates.initialized |= 1u;

				for (auto& point : gAnimalBirthCandidates.points)
				{
					point = 0;
				}

				NXBasics::Runtime_AtExit(AnimalBirthCandidates_OnExit);
			}

			const auto leaderRadius = LeaderDistance(animal);

			if (Distance(current, birth) >= 30u)
			{
				FindBirthPoint(current, work, birth, 30);

				return;
			}

			const auto* micro = GameMap_GetConstMapMicroPointPtr(birth);
			std::uint32_t count = 0;

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
					continue;

				auto& candidate = gAnimalBirthCandidates.points[count];
				candidate = Read<std::uint32_t>(birth, NC2Layout::MapCoordinatePairOffset::Coordinates);
				MapMigPoint_MoveCursorUnwrapped(candidate, direction);
				std::uint32_t player = 0;

				for (; player < 20; ++player)
				{
					if (player == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) || Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer) == 0)
						continue;

					CHouseIterator iterator;

					if (iterator.GetFirstSectorHouseId(player, &candidate, stayRadius + leaderRadius, false) != -1 || iterator.GetFirstSectorHouseId(player, &candidate, stayRadius + leaderRadius, true) != -1)
						break;
				}

				if (player != 20 || Distance(&candidate, work) >= birthRadius || Distance(&candidate, current) >= 30 || Read<std::uint8_t>(Macro(&candidate), NC2Layout::MapMacroPointOffset::PatternA) != 2)
					continue;

				++count;
			}

			if (count != 0)
			{
				Write(birth, NC2Layout::MapCoordinatePairOffset::Coordinates, gAnimalBirthCandidates.points[Game_Random_GetNumber() % count]);
			}
			else
			{
				FindBirthPoint(current, work, birth, 30);
			}
		}

		// Original function: FUN_00461345.
		void Animal_l_Reproduction(void* animal)
		{
			const auto flags = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MiscFlags), count = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::GroupSize);

			if ((flags & 2u) != 0 || Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) != 20 || (gGameTime.tick / 12u) % 600u != 0 || count >= static_cast<std::uint32_t>(Type(animal).maximumGroupSize) || count <= 1)
			{

				return;
			}

			Logic_EntryPoint_Add_Animal(20, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType), 49, Animal_GetMapPositionRef(animal), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MissionCheckId), flags);
		}

		// Original function: FUN_00447199.
		void Animal_l_Speed_Normal(void* animal)
		{
			const auto speed = Type(animal).moveSpeed;
			Write(Moveable(animal), NC2Layout::MapMoveableOffset::WalkingTicksPerStep, speed);
			Write(Moveable(animal), NC2Layout::MapMoveableOffset::WalkingProgressPerTick, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(speed) + 9999u) / speed);
		}

		// Original function: FUN_0042e7eb.
		bool Animal_l_StartTask_GoAway(void* animal)
		{
			if (!Animal_l_IsAtomicInterruptable(animal))
				return false;

			std::uint32_t point = 0;

			return Animal_FindMapPointNearby(animal, &point) && Animal_l_StartAtomicWalk(animal, &point);
		}

		// Original function: FUN_0042e6aa; invalid dragging assignments are cleared before normal idle work.
		void Animal_l_StartTask_Idle(void* animal)
		{
			if (!Animal_l_IsAtomicInterruptable(animal))
				return;

			if (Animal_IsAttachedToVehicle(animal))
			{
				auto* target = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget;
				const auto* vehicle = CVehicleArray::mStaticVars + TargetData_GetObjectId(target) * CVehicleArray::ElementSize;

				if (Vehicle_Passengers_IsDraggingAnimal(vehicle, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::UniqueId)))
				{
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 9);
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
					return;
				}

				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 0);
			}

			if (Distance(Animal_GetMapPositionRef(Animal(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId))), Animal_GetMapPositionRef(animal)) > LeaderDistance(animal))
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 3);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
				return;
			}

			auto* moveable = Moveable(animal);
			const auto* micro = GameMap_GetConstMapMicroPointPtr(Animal_GetMapPositionRef(animal));

			if (Read<std::int16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId) != Read<std::int32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) || Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId) != -1)
			{
				if (Animal_l_StartTask_GoAway(animal))
				{

					return;
				}
			}
			else
			{
				CMapMoveableIterator iterator;
				iterator.GetFirstMapMoveableId(Animal_GetMapPositionRef(animal));
			}

			Animal_l_StartAtomicAnimation(animal, Game_Random_GetNumber() % 6u + 2u, -1);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 1);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 1);
		}

		// Original function: FUN_0042eef2.
		void Animal_l_StartTask_FollowLeader(void* animal)
		{
			auto* leader = Animal(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId));
			auto distance = Distance(Animal_GetMapPositionRef(leader), Animal_GetMapPositionRef(animal));

			if (distance != 0)
				--distance;

			if (distance > LeaderDistance(animal))
				distance = LeaderDistance(animal);

			if (!Animal_l_StartAtomicWalk(animal, Read<std::int32_t>(leader, NC2Layout::AnimalOffset::MapMoveableId), distance))
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);

			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 3);
		}

		// Original function: FUN_0042f61c.
		void Animal_l_StartTask_RunAway(void* animal)
		{
			Animal_l_Speed_Fast(animal);
			Animal_l_StartAtomicWalk(animal, static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::BirthPoint);
			Write(animal, NC2Layout::AnimalOffset::StayPoint, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::BirthPoint));
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 4);
		}

		// Original function: FUN_00460f3f.
		void Animal_l_StartTask_Stay(void* animal)
		{
			const bool aggressive = IsAggressive(animal);

			if (aggressive)
				Write(animal, NC2Layout::AnimalOffset::EnvironmentalFoodValue, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::EnvironmentalFoodValue) - 1u);

			if (!aggressive && Read<std::int32_t>(animal, NC2Layout::AnimalOffset::Food) <= 0)
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 7);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
				return;
			}

			const auto* current = Animal_GetMapPositionRef(animal);
			auto* leader = Animal(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId));

			if (leader == animal)
			{
				if (Distance(current, static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::WorkCenter) > StayDistance(animal) + BirthDistance(animal))
				{
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::EnvironmentalFoodValue, 0);
				}
				else if (aggressive && Animal_l_Military_CheckForEnemy(animal))
				{
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 8);
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
					return;
				}

				if (Read<std::int32_t>(animal, NC2Layout::AnimalOffset::EnvironmentalFoodValue) <= 0)
				{
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 6);
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
					return;
				}

				if (Distance(current, static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::StayPoint) > StayDistance(animal))
				{
					Animal_l_StartAtomicWalk(animal, static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::StayPoint);
					return;
				}
			}
			else
			{
				if (Distance(Animal_GetMapPositionRef(leader), current) > LeaderDistance(animal) || Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::CurrentTask) == 6)
				{
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 3);
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
					return;
				}

				if (Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::CurrentTask) == 4)
				{
					Animal_l_Speed_Fast(animal);
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 3);
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
					return;
				}

				if (Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::CurrentTask) == 8)
				{
					if (!TargetData_IsValid(static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::MilitaryTarget))
						for (std::uint32_t offset = 0; offset < 12; offset += 4)
						{
							Write(animal, NC2Layout::AnimalOffset::MilitaryTarget + offset, Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::MilitaryTarget + offset));
						}

					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 8);
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
					return;
				}
			}

			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) == 13 && Game_Random_GetNumber() % 3u != 0)
			{
				std::uint32_t point = 0;

				if (FindSheepPlayPoint(animal, &point) && Animal_l_StartAtomicWalk(animal, &point))
					return;
			}

			if (Game_Random_GetNumber() % 5u == 0)
			{
				Animal_l_StartTask_GoAway(animal);

				return;
			}

			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 1);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
		}

		// Original function: FUN_00461133.
		void Animal_l_StartTask_GotoNextPoint(void* animal)
		{
			std::uint32_t point = 0;

			if (FindNextLandPoint(animal, &point) && Animal_l_StartAtomicWalk(animal, &point))
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 6);
			}
			else
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 1);
			}
		}

		// Original function: FUN_00461177.
		void Animal_l_StartTask_Eat(void* animal)
		{
			const auto* position = Animal_GetMapPositionRef(animal);
			std::uint32_t macro = 0;
			MapMagPoint_ConstructFromMig(&macro, position, false);

			if (Read<std::uint8_t>(GameMap_GetConstMapMacroPointPtr(&macro), NC2Layout::MapMacroPointOffset::PatternA) == 2)
			{
				const auto food = Game_Random_GetNumber() % 400u + 100u;
				Write(animal, NC2Layout::AnimalOffset::Food, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::Food) + food);
				Write(animal, NC2Layout::AnimalOffset::Hitpoints, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::Hitpoints) + food / 10u);

				if (Read<std::int32_t>(animal, NC2Layout::AnimalOffset::Hitpoints) > static_cast<std::int32_t>(Animal_HitPoints_GetMaximum(animal)))
					Write(animal, NC2Layout::AnimalOffset::Hitpoints, Animal_HitPoints_GetMaximum(animal));

				auto* leader = Animal(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId));
				Write(leader, NC2Layout::AnimalOffset::EnvironmentalFoodValue, Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::EnvironmentalFoodValue) - food);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, Animal_l_StartAtomicAnimation(animal, 9, -1) ? 1u : 3u);
				return;
			}

			std::uint32_t point = 0;

			if (CMapPathfinderElement::SearchOverMap(position, LeaderDistance(animal), true, slAnimalSearchFoodPatterns, &point, true, Read<std::int32_t>(animal, NC2Layout::AnimalOffset::PlayerId)))
			{
				Animal_l_StartAtomicWalk(animal, &point);

				return;
			}

			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId))
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 6);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
			}
			else
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
			}
		}

		// Original function: FUN_0042e5f6; all ten entries of the selector are retained.
		void Animal_l_CurrentTask_DoSuccess(void* animal)
		{
			Animal_l_Speed_Normal(animal);

			switch (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask))
			{
				case SwitchIds::AnimalTask::Idle:
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, Animal_IsAttachedToVehicle(animal) ? 1u : 5u);
					break;
				case SwitchIds::AnimalTask::IdleAlternate:
				case SwitchIds::AnimalTask::RunAway:
				case SwitchIds::AnimalTask::Stay:
				case SwitchIds::AnimalTask::Eat:
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 5);
					break;
				case SwitchIds::AnimalTask::FollowLeader:
				case SwitchIds::AnimalTask::InteractWithVehicle:
				case SwitchIds::AnimalTask::InteractWithHouse:
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 1);
					break;
				case SwitchIds::AnimalTask::GotoNextPoint:
					Write(animal, NC2Layout::AnimalOffset::StayPoint, Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapCoordinatePairOffset::Coordinates));
					Animal_l_CalculateFoodValue(animal);
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 5);
					break;
				case SwitchIds::AnimalTask::Attack:
					Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 8);
					break;
				default:

					return;
			}

			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
		}

		// Original function: FUN_0042e67c.
		void Animal_l_CurrentTask_DoFailure(void* animal)
		{
			Animal_l_Speed_Normal(animal);

			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask) == 3)
			{
				Animal_l_DetachLeader(animal);
				Animal_l_AttachLeader(animal, animal);
			}

			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAction, 0);
			Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationRunningFlag, 0);
			Animal_l_StartTask_Idle(animal);
		}

		// Original function: FUN_0042e32e; melee hits use the native minimum-range check.
		bool Animal_l_CurrentTask_DoPerform(void* animal)
		{
			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAction) == 0)
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 1);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
				return true;
			}

			if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationRunningFlag) == 0)
				return true;

			const auto& animation = gStaticAtomicAnimationData[Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationId)];

			for (std::uint32_t index = 0; index < animation.eventCount; ++index)
			{
				const auto& event = animation.events[index];

				if (static_cast<std::uint32_t>(event.time) != Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationFrame))
					continue;

				if (event.type == SwitchIds::AtomicAnimationEvent::Attack)
				{
					auto* target = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::MilitaryTarget;

					if (!TargetData_IsValid(target))
					{
						Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
						continue;
					}

					const auto* weapon = reinterpret_cast<const SStaticWeaponTypeData*>(reinterpret_cast<std::uintptr_t>(gStaticWeaponTypeArrays[Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType)]) + 0xC4u);

					if (weapon == nullptr)
						continue;

					const auto kind = TargetData_GetTargetType(target);

					if (kind != 1 && kind != 3)
					{
						Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
						continue;
					}

					auto* object = kind == 1 ? CHumanArray::mStaticVars + TargetData_GetObjectId(target) * CHumanArray::ElementSize : static_cast<std::byte*>(Animal(TargetData_GetObjectId(target)));
					const auto* source = Animal_GetMapPositionRef(animal);
					const auto* destination = kind == 1 ? Human_GetMapPositionRef(object) : Animal_GetMapPositionRef(object);

					if (Distance(destination, source) > static_cast<std::uint32_t>(weapon->minimumRange))
						continue;

					const bool killed = kind == 1 ? Human_Military_StartGettingHitByWeapon(object, source, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType), 1, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::UniqueId), 3, 0, true) : Animal_Military_StartGettingHitByWeapon(object, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType), 1, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::UniqueId), 3, 0, true);

					if (killed)
						Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 2);
				}
				else if (event.type == 26)
				{
					auto* moveable = Moveable(animal);
					auto point = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
					MapMigPoint_MoveCursorUnwrapped(point, Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 ? Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::Direction) : 2u);

					if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0)
					{
						MapMoveable_Attachment_DetachFromMap(moveable);
						MapMoveable_Attachment_AttachToMap(moveable, &point);
					}
				}
				else if (event.type == SwitchIds::AtomicAnimationEvent::PlaySoundFX)
				{
					CCallbackManager::sTheObjectPtr->ExecuteCallback(0x17, true, event.parameter, MapMigPoint_GetLong(Animal_GetMapPositionRef(animal)), 0);
				}
			}

			const auto tick = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationFrame) + 1u;
			Write(animal, NC2Layout::AnimalOffset::AtomicAnimationFrame, tick);

			if (tick != static_cast<std::uint32_t>(animation.length))
				return true;

			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus) == 1)
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 2);

			if (animation.endDirection != -1)
				Write(Moveable(animal), NC2Layout::MapMoveableOffset::Direction, animation.endDirection);

			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAction, 0);
			Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationRunningFlag, 0);

			return true;
		}
	}

	// Original function: FUN_00447165.
	void Animal_l_Speed_Fast(void* animal)
	{
		const auto speed = Type(animal).runSpeed;

		if (speed <= 0)
		{

			return;
		}

		Write(Moveable(animal), NC2Layout::MapMoveableOffset::WalkingTicksPerStep, speed);
		Write(Moveable(animal), NC2Layout::MapMoveableOffset::WalkingProgressPerTick, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(speed) + 9999u) / speed);
	}

	// Original function: FUN_0042df37; growth notifies after detaching and reattaching.
	void Animal_DoUpdateConditions(void* animal)
	{
		Animal_l_Aggressive_Check(animal);
		Write(animal, NC2Layout::AnimalOffset::Food, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::Food) - 1u);
		const auto age = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AgeCounter) + 1u;
		Write(animal, NC2Layout::AnimalOffset::AgeCounter, age);

		if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 48 && NXBasics::Runtime_TimeSigned32(age) >= 3600)
		{
			const auto point = Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapCoordinatePairOffset::Coordinates);
			auto* moveable = Moveable(animal);
			MapMoveable_Attachment_DetachFromMap(moveable);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType, 49);
			MapMoveable_Attachment_AttachToMap(moveable, &point);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(6, true, Read<std::int32_t>(animal, NC2Layout::AnimalOffset::AnimalId), 1, 1);
		}

		if (Read<std::int32_t>(animal, NC2Layout::AnimalOffset::Hitpoints) <= 0)
			Write<std::uint8_t>(Moveable(animal), NC2Layout::MapMoveableOffset::RemovalRequestedFlag, 1);
	}

	// Original function: FUN_0042e8e7.
	bool Animal_l_StartAtomicWalk(void* animal, const void* point)
	{
		if (!Animal_l_IsAtomicInterruptable(animal) || !MapMoveable_Pathfinder_Start(Moveable(animal), point, 40, false, true, false))
			return false;

		Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::InsideHouseFlag, 0);
		Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationRunningFlag, 0);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAction, 1);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 2);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 1);

		return true;
	}

	// Original function: FUN_0042e949.
	bool Animal_l_StartAtomicWalk(void* animal, std::int32_t moveableId, std::uint32_t distance)
	{
		if (!Animal_l_IsAtomicInterruptable(animal) || !MapMoveable_Pathfinder_Start(Moveable(animal), moveableId, static_cast<std::int32_t>(distance), 40, true, true, false))
			return false;

		Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::InsideHouseFlag, 0);
		Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationRunningFlag, 0);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAction, 1);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 2);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 1);

		return true;
	}

	// Original function: FUN_004613b0; approach the vehicle before consuming the dragging animal.
	void Animal_l_StartTask_InteractWithVehicle(void* animal)
	{
		auto* target = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget;

		if (!Animal_IsAttachedToVehicle(animal))
		{
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
			return;
		}

		auto* vehicle = CVehicleArray::mStaticVars + TargetData_GetObjectId(target) * CVehicleArray::ElementSize;

		if (!Vehicle_Passengers_IsDraggingAnimal(vehicle, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::UniqueId)))
		{
			Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 0);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
			return;
		}

		const auto* destination = Vehicle_GetMapPositionRef(vehicle);

		if (Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates))
		{
			if (!Vehicle_Passengers_AnimalMoveIn(vehicle, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId)))
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
				return;
			}

			Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 0);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 2);

			return;
		}

		auto point = Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates);
		MapMigPoint_MoveCursorUnwrapped(point, VE_HexagonDirection_OppositeDirection(2));

		if (MapMigPoint_IsBorderPoint(&point) || (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
			MapMigPoint_MoveCursorUnwrapped(point, 2);

		Animal_l_StartAtomicWalk(animal, Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapCoordinatePairOffset::Coordinates) == point ? destination : &point);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 9);
	}

	// Original function: FUN_004614c0.
	void Animal_l_StartTask_InteractWithHouse(void* animal)
	{
		if (!Animal_IsAttachedToHouse(animal))
		{
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);

			return;
		}

		const auto* point = CHouseArray::mStaticVars + TargetData_GetObjectId(static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget) * CHouseArray::ElementSize + NC2Layout::HouseOffset::DoorEntryPoint;

		if (Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates))
		{
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 2);
		}
		else
		{
			Animal_l_StartAtomicWalk(animal, point);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 10);
		}
	}

	// Original function: FUN_0042e15c.
	bool Animal_DoNewMapPositionReached(void* animal, bool finalPosition)
	{
		if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus) == 0)
			return false;

		if (finalPosition)
		{
			if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::InsideHouseFlag) == 0)
			{
				const auto* micro = GameMap_GetConstMapMicroPointPtr(Animal_GetMapPositionRef(animal));

				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0 && Read<std::uint32_t>(CHouseArray::mStaticVars + Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId) * CHouseArray::ElementSize, NC2Layout::HouseOffset::PlayerId) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId))
					Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::InsideHouseFlag, 1);
			}

			const auto task = Read<std::int32_t>(animal, NC2Layout::AnimalOffset::CurrentTask);

			if (task > 0 && task <= 10)
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 2);
		}
		else if (IsAggressive(animal))
		{
			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId) && Animal_l_Military_CheckForEnemy(animal))
			{
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 8);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
				return false;
			}

			if (!Animal_l_IsLeaderValid(animal))
				return true;

			auto* target = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::MilitaryTarget;

			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId) && TargetData_IsValid(target))
			{
				if (Distance(Animal_GetMapPositionRef(animal), static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::StayPoint) <= 40u)
					return true;

				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 0);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 5);
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
				return false;
			}

			const auto* leaderTarget = static_cast<const std::byte*>(Animal(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId))) + NC2Layout::AnimalOffset::MilitaryTarget;
			// Windows compares target kind and revision here, without comparing the object id.

			if (Read<std::uint32_t>(leaderTarget, NC2Layout::TargetDataOffset::ObjectRevision) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision) && Read<std::uint32_t>(leaderTarget, NC2Layout::TargetDataOffset::Type) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type))
				return true;

			for (std::uint32_t offset = 0; offset < 12; offset += 4)
			{
				Write(target, offset, Read<std::uint32_t>(leaderTarget, offset));
			}

			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, TargetData_IsValid(target) ? 8u : 5u);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
			return false;
		}
		else if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId) != Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId) && Animal_l_IsLeaderValid(animal) && Read<std::uint32_t>(Animal(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId)), NC2Layout::AnimalOffset::CurrentTask) == 4)
		{
			Animal_l_Speed_Fast(animal);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 3);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
		}

		return true;
	}

	// Original function: FUN_0042e318.
	void Animal_DoPathfinderFailed(void* animal)
	{
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_0042e320.
	void Animal_DoPathfinderStopped(void* animal)
	{
		if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask) == 2)
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_0042dfc0.
	void Animal_DoUpdateAI(void* animal)
	{
		if (Read<std::uint8_t>(Moveable(animal), NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0)
			return;

		if (!Animal_IsAttachedToVehicle(animal) && !Animal_IsAttachedToHouse(animal) && Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) != 20)
			Animal_l_MoveWorkCenterPoint(animal);

		Animal_l_CheckForValidLeader(animal);

		if (!Animal_l_CurrentTask_DoPerform(animal) || !Animal_l_IsAtomicInterruptable(animal))
			return;

		if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Animal_GetMapPositionRef(animal)), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
			Animal_l_StartTask_GoAway(animal);

		if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId) != Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId) && Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask) != 3 && Read<std::uint32_t>(Animal(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId)), NC2Layout::AnimalOffset::CurrentTask) == 4)
		{
			Animal_l_Speed_Fast(animal);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 3);
		}

		for (;;)
		{
			const auto status = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus);

			if (status == 1)
				break;

			if (status == 2)
			{
				Animal_l_CurrentTask_DoSuccess(animal);
				continue;
			}

			if (status == 3)
			{
				Animal_l_CurrentTask_DoFailure(animal);
				continue;
			}

			if (status != 0)
				return;

			switch (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask))
			{
				case SwitchIds::AnimalTask::Idle:
				case SwitchIds::AnimalTask::IdleAlternate:
					Animal_l_StartTask_Idle(animal);
					break;
				case SwitchIds::AnimalTask::FollowLeader:
					Animal_l_StartTask_FollowLeader(animal);
					break;
				case SwitchIds::AnimalTask::RunAway:
					Animal_l_StartTask_RunAway(animal);
					break;
				case SwitchIds::AnimalTask::Stay:
					Animal_l_StartTask_Stay(animal);
					break;
				case SwitchIds::AnimalTask::GotoNextPoint:
					Animal_l_StartTask_GotoNextPoint(animal);
					break;
				case SwitchIds::AnimalTask::Eat:
					Animal_l_StartTask_Eat(animal);
					break;
				case SwitchIds::AnimalTask::Attack:
					Animal_l_StartTask_Attack(animal);
					break;
				case SwitchIds::AnimalTask::InteractWithVehicle:
					Animal_l_StartTask_InteractWithVehicle(animal);
					break;
				case SwitchIds::AnimalTask::InteractWithHouse:
					Animal_l_StartTask_InteractWithHouse(animal);
					break;
				default:

					return; // Explicit untranslated task boundary; never spin here.
			}
		}

		if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId) && (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId) + gGameTime.tick) % 60u == 0)
		{
			Animal_l_MoveWorkCenterPoint(animal);
			Animal_l_MoveBirthPoint(animal);
			Animal_l_Reproduction(animal);
		}
	}
}
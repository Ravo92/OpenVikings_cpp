#include "C2EngineDisplayAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanAttackTasks.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanTaskAccess.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2MilitaryAttackPointSearch.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2StaticVehicleTypeData.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		const void* Position(const void* human)
		{

			return Human_GetMapPositionRef(human);
		}

		const SStaticWeaponTypeData& Weapon(const void* human)
		{

			return static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)])[Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon)];
		}

		std::uint32_t Distance(const void* first, const void* second)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(second, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(second, NC2Layout::MapCoordinatePairOffset::Y)));
		}

		void* Human(std::uint32_t id)
		{

			return CHumanArray::mStaticVars + id * CHumanArray::RuntimeElementSize;
		}

		const void* Moveable(std::int32_t id)
		{

			return gMapMoveableArrayStorage.entries + static_cast<std::intptr_t>(id) * NC2Layout::RecordSize::MapMoveableSigned;
		}

		bool HasAttackingHuman(const void* human, const void* point, bool soldiersOnly, bool includeHeroes)
		{
			CMapMoveableIterator iterator;

			for (auto id = iterator.GetFirstMapMoveableId(point); id != -1; id = iterator.GetNextMapMoveableId())
			{
				if (id == Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId) || Read<std::uint32_t>(Moveable(id), NC2Layout::MapMoveableOffset::ObjectType) != SwitchIds::MapMoveableType::Human)
					continue;

				const auto* other = Human(Read<std::uint32_t>(Moveable(id), NC2Layout::MapMoveableOffset::ObjectId));

				if (soldiersOnly && !Human_IsJobSoldier(other) && !(includeHeroes && Human_IsJobHero(other)))
					continue;

				if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::PlayerId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) && Read<std::uint32_t>(other, NC2Layout::HumanOffset::AtomicAction) == NC2Runtime::HumanAtomicAction::Attack)
					return true;
			}

			return false;
		}

		// Original function: FUN_0046b61f. Equal source distances remain candidates.
		bool __cdecl slHumanSearchNextFreeMilitaryMapPointFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SAttackPointSearch*>(callback->userData);
			const auto sourceDistance = Distance(&data.source, point);

			if (sourceDistance > data.bestDistance)
				return true;

			const auto targetDistance = Distance(&data.target, point);

			if (targetDistance < data.minimumRange || targetDistance > data.maximumRange || HasAttackingHuman(data.owner, point, false, false))
				return false;

			data.bestDistance = sourceDistance;
			data.points[data.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return data.count == NC2Runtime::HumanAttackLimits::PositionCandidates;
		}
	}

	// Original function: FUN_0043499a; each engine footprint span advances only x.
	bool House_Military_GetNextFreeAttackPoint(const void* house, const void* source, void* point, const void* human, bool outside)
	{
		auto stage = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage);

		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 2)
			++stage;

		const auto* graphics = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		const auto* area = Read<const std::byte*>(graphics, NC2Layout::EngineHouseLogicViewOffset::WalkAreasUnsigned + stage * NC2Layout::RecordSize::EngineHouseAreaPointerUnsigned);

		if (area == nullptr)
			return false;

		std::uint32_t best = NC2Runtime::HumanAttackLimits::PointDistanceNotFound;
		const auto* spans = Read<const std::byte*>(area, NC2Layout::MapAreaOffset::Rows);

		for (std::uint32_t span = 0; span < Read<std::uint32_t>(area, NC2Layout::MapAreaOffset::RowCount); ++span)
		{
			std::uint32_t cursor = 0;
			MapMigPoint_ConstructOffset(&cursor, static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, Read<std::int16_t>(spans, static_cast<std::size_t>(span) * NC2Layout::RecordSize::MapAreaRowUnsigned), Read<std::int16_t>(spans, static_cast<std::size_t>(span) * NC2Layout::RecordSize::MapAreaRowUnsigned + NC2Layout::MapAreaRowOffset::Y), false);

			for (std::uint32_t step = 0; step < Read<std::uint32_t>(spans, static_cast<std::size_t>(span) * NC2Layout::RecordSize::MapAreaRowUnsigned + NC2Layout::MapAreaRowOffset::Width); ++step)
			{
				const auto distance = Distance(source, &cursor);

				if (distance < best)
				{
					auto edge = cursor;
					MapMigPoint_MoveCursorUnwrapped(edge, SwitchIds::HexagonDirection::NorthWest);

					for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
					{
						MapMigPoint_MoveCursorUnwrapped(edge, direction);

						if (!MapMigPoint_IsInsideMap(&edge) || (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&edge), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
							continue;

						if (outside && HasAttackingHuman(human, &edge, true, false))
							continue;

						Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, outside ? edge : cursor);
						best = distance;
						break;
					}
				}

				Write(&cursor, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&cursor, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}

		return best != NC2Runtime::HumanAttackLimits::PointDistanceNotFound;
	}

	// Original function: FUN_004424e1. Outside candidates retain the footprint distance,
	// including the native inside-map check of the inner cursor rather than the neighbor.
	bool Vehicle_Military_GetAttackPoint(const void* vehicle, const void* source, void* point, const void* human, bool outside)
	{
		const auto radius = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->logicSize);
		const auto* center = Vehicle_GetMapPositionRef(vehicle);

		if (radius == 0)
		{
			Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(center, NC2Layout::MapCoordinatePairOffset::Coordinates));
			return true;
		}

		if (!outside && Distance(center, source) <= radius)
			return false;

		auto cursor = Read<std::uint32_t>(center, NC2Layout::MapCoordinatePairOffset::Coordinates);
		MapMigPoint_MoveCursorUnwrapped(&cursor, SwitchIds::HexagonDirection::NorthWest, radius);
		std::uint32_t best = NC2Runtime::HumanAttackLimits::VehicleDistanceNotFound;

		for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
		{
			for (std::uint32_t step = 0; step < radius; ++step)
			{
				MapMigPoint_MoveCursorUnwrapped(cursor, direction);
				const auto distance = Distance(&cursor, source);

				if (distance >= best || !MapMigPoint_IsInsideMap(&cursor) || (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&cursor), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
					continue;

				if (!outside)
				{
					best = distance;
					Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, cursor);
					continue;
				}

				auto edge = cursor;
				MapMigPoint_MoveCursorUnwrapped(edge, SwitchIds::HexagonDirection::NorthWest);

				for (std::uint32_t neighbor = 0; neighbor < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++neighbor)
				{
					MapMigPoint_MoveCursorUnwrapped(edge, neighbor);

					if (Distance(&edge, center) <= radius || !MapMigPoint_IsInsideMap(&cursor) || (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&edge), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || HasAttackingHuman(human, &edge, true, true))
						continue;

					best = distance;
					Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, edge);
				}
			}
		}

		return best != NC2Runtime::HumanAttackLimits::VehicleDistanceNotFound;
	}

	// Original function: FUN_00461dda.
	bool TargetData_MapPos_StartCheckForAttackPoint(void* target, const void* point)
	{
		const auto direction = VE_HexagonDirection_GetDirectionFromPositions(Read<std::int16_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition), Read<std::int16_t>(target, NC2Layout::TargetDataOffset::MapPositionY), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), false);
		Write(target, NC2Layout::TargetDataOffset::AttackBaseDirection, static_cast<std::int16_t>((static_cast<std::int16_t>(direction) + NC2Runtime::HumanAttackLimits::PreviousDirectionOffset) % NC2Runtime::PathfinderGeometry::NeighbourDirections));
		Write<std::uint16_t>(target, NC2Layout::TargetDataOffset::AttackPointAttempts, 0);

		return TargetData_MapPos_NextAttackPoint(target);
	}

	// Original function: FUN_00461e24; return the first unblocked same-continent neighbor.
	bool TargetData_MapPos_NextAttackPoint(void* target)
	{
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(target) + NC2Layout::TargetDataOffset::ObjectIdOrMapPosition), NC2Layout::MapMicroPointOffset::ContinentId);
		auto attempts = static_cast<std::int16_t>(Read<std::uint16_t>(target, NC2Layout::TargetDataOffset::AttackPointAttempts) + 1u);
		Write(target, NC2Layout::TargetDataOffset::AttackPointAttempts, attempts);
		auto direction = (Read<std::int16_t>(target, NC2Layout::TargetDataOffset::AttackBaseDirection) + attempts) % NC2Runtime::PathfinderGeometry::NeighbourDirections;

		while (attempts <= NC2Runtime::PathfinderGeometry::NeighbourDirections)
		{
			auto point = Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition);
			MapMigPoint_MoveCursorUnwrapped(point, static_cast<std::uint32_t>(direction));

			if (MapMigPoint_IsInsideMap(&point))
			{
				const auto* micro = GameMap_GetConstMapMicroPointPtr(&point);

				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) == continent && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0)
					return true;
			}

			attempts = static_cast<std::int16_t>(static_cast<std::uint16_t>(attempts) + 1u);
			Write(target, NC2Layout::TargetDataOffset::AttackPointAttempts, attempts);
			direction = (direction + 1) % NC2Runtime::PathfinderGeometry::NeighbourDirections;
		}

		return false;
	}

	// Original function: FUN_00461ec5.
	void TargetData_MapPos_GetAttackPosition(const void* target, void* point)
	{
		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition));
		MapMigPoint_MoveCursorUnwrapped(*static_cast<std::uint32_t*>(point), static_cast<std::uint32_t>((Read<std::int16_t>(target, NC2Layout::TargetDataOffset::AttackBaseDirection) + Read<std::int16_t>(target, NC2Layout::TargetDataOffset::AttackPointAttempts)) % NC2Runtime::PathfinderGeometry::NeighbourDirections));
	}

	// Original function: FUN_0046b527.
	bool Human_l_IsOtherAttackingHumanOnPoint(const void* human, const void* point)
	{

		return HasAttackingHuman(human, point, false, false);
	}

	// Original function: FUN_0046b592; search at most six candidates, then sample once.
	bool Human_l_GetNextPointToStartAttack(void* human, void* point)
	{
		const auto& weapon = Weapon(human);
		SAttackPointSearch data;
		data.target = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		data.source = Read<std::uint32_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates);
		data.minimumRange = static_cast<std::uint32_t>(weapon.minimumRange);
		data.maximumRange = static_cast<std::uint32_t>(weapon.maximumRange);
		data.owner = human;
		CMapPathfinderElement::SearchOverMap(&data.source, NC2Runtime::HumanAttackLimits::PositionSearchRadius, true, slHumanSearchNextFreeMilitaryMapPointFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));

		if (data.count == 0)
			return false;

		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, data.points[Game_Random_GetNumber() % data.count]);

		return true;
	}

	// Original function: FUN_0046b703; retain strict best-distance ties and the NC2Runtime::HumanAttackLimits::PointDistanceNotFound sentinel.
	bool Human_l_GetDistancePointFromTarget(const void*, const void* source, const void* target, void* point, std::uint32_t radius)
	{
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::ContinentId);
		auto cursor = Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type);
		MapMigPoint_MoveCursorUnwrapped(&cursor, SwitchIds::HexagonDirection::NorthWest, radius);
		std::uint32_t best = NC2Runtime::HumanAttackLimits::PointDistanceNotFound;

		for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
		{
			for (std::uint32_t step = 0; step < radius; ++step)
			{
				MapMigPoint_MoveCursorUnwrapped(cursor, direction);

				if (MapMigPoint_IsBorderPoint(&cursor))
					continue;

				const auto* micro = GameMap_GetConstMapMicroPointPtr(&cursor);

				if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) != continent)
					continue;

				const auto distance = Distance(source, &cursor);

				if (distance >= best)
					continue;

				best = distance;
				Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, cursor);
			}
		}

		return best != NC2Runtime::HumanAttackLimits::PointDistanceNotFound;
	}

	// Original function: FUN_0046b323; status is set to running even when animation startup fails.
	void Human_l_AttackTarget(void* human, const void* point)
	{
		const auto& weapon = Weapon(human);
		const auto direction = VE_HexagonDirection_GetDirectionFromPositions(Read<std::int16_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates), Read<std::int16_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), false);
		Human_l_StartAtomicAnimation(human, static_cast<std::uint32_t>(weapon.atomicActionType), static_cast<std::int32_t>(direction));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::Running);
	}

	// Original function: FUN_0046a806; both target records are visited in native order.
	void Human_l_StartTask_Attack(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		const auto& weapon = Weapon(human);
		std::uint32_t point = 0;

		for (std::uint32_t index = 0; index < NC2Runtime::HumanAttackLimits::TargetRecords; ++index)
		{
			auto* target = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTargetUnsigned + index * NC2Layout::RecordSize::TargetData;

			if (TargetData_GetTargetType(target) == SwitchIds::TargetType::None)
				continue;

			if (!TargetData_IsValid(target))
			{
				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);
				continue;
			}

			const auto player = TargetData_GetPlayerId(target);
			const auto relation = player == NC2Runtime::GameControlLimits::Players || player == -1 ? SwitchIds::DiplomacyState::Enemy : GameControl_Diplomacy_GetState(gGameControl, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), player);

			if (relation == 1 || relation == 2)
			{
				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);
				continue;
			}

			if (Human_IsAttachedToWorkHouse(human) && Human_IsJobSoldier(human))
			{
				std::uint32_t enemy = 0;
				TargetData_GetMapPosition(target, &enemy);
				const auto distance = Distance(&enemy, Position(human));
				const auto limit = weapon.hasMunition != 0 ? static_cast<std::uint32_t>(weapon.maximumRange) + NC2Runtime::HumanAttackLimits::RangedWorkHouseMarginUnsigned : Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == SwitchIds::HumanJob::Farmer || Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == SwitchIds::HumanJob::HerbMushGuy ? NC2Runtime::HumanAttackLimits::ShortWorkHouseDistanceUnsigned
				                                                                                                                                                                                                                                                                                                                                                            : NC2Runtime::HumanAttackLimits::WorkHouseDistanceUnsigned;

				if (NXBasics::Runtime_TimeSigned32(distance) > NXBasics::Runtime_TimeSigned32(limit))
				{
					Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);
					continue;
				}

				if (weapon.hasMunition != 0)
				{
					if (Human_IsInWorkHouse(human))
					{
						Human_l_AttackTarget(human, &enemy);
					}
					else
					{
						const auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;

						if (!Human_l_StartAtomicWalk(human, static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint))
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::Failed);
					}

					return;
				}
			}

			if (index != 0)
			{
				TargetData_GetMapPosition(target, &point);
				const auto mode = Read<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode);
				const auto distance = Distance(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter, &point);

				if ((mode == SwitchIds::HumanMilitaryMode::SearchWithinWeaponRange && (distance > NC2Runtime::HumanAttackLimits::WeaponModeWorkCenterDistanceUnsigned || distance < static_cast<std::uint32_t>(weapon.minimumRange))) || (mode == SwitchIds::HumanMilitaryMode::SearchAroundWorkCenter && distance > NC2Runtime::HumanAttackLimits::WorkCenterDistanceUnsigned))
				{
					auto* center = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
					Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);

					if (Human_l_StartAtomicWalk(human, center, center))
						return;

					auto replacement = Read<std::uint32_t>(center, NC2Layout::MapCoordinatePairOffset::Coordinates);
					Tool_Map_GetNotBlockedPosition(&replacement, false);
					Write(center, NC2Layout::MapCoordinatePairOffset::Coordinates, replacement);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
					Human_WorkCenterMarker_Update(human);
					return;
				}
			}

			auto range = static_cast<std::uint32_t>(weapon.minimumRange);

			if (static_cast<std::uint32_t>(weapon.maximumRange) > range)
				range = weapon.hasMunition != 0 ? (static_cast<std::uint32_t>(weapon.maximumRange) * 2u - range) >> 1 : static_cast<std::uint32_t>(weapon.maximumRange);

			const auto type = TargetData_GetTargetType(target);
			const void* object = nullptr;
			const auto* source = Position(human);
			bool walk = false;

			if (type == SwitchIds::TargetType::Landscape)
			{
				if (static_cast<std::uint32_t>(weapon.damageValues[NC2Runtime::WeaponDamageSlot::Landscape]) / 100u == 0 && weapon.hasMunition != 0)
				{
					Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);
					Human_UserMessage_SendToUserMessage(human, 17);
					continue;
				}

				TargetData_GetMapPosition(target, &point);
			}
			else if (type == SwitchIds::TargetType::House)
			{
				object = CHouseArray::mStaticVars + TargetData_GetObjectId(target) * CHouseArray::RuntimeElementSize;
				House_Military_GetNextFreeAttackPoint(object, source, &point, human, false);
			}
			else if (type == SwitchIds::TargetType::Vehicle)
			{
				object = CVehicleArray::mStaticVars + TargetData_GetObjectId(target) * CVehicleArray::RuntimeElementSize;

				if (!Vehicle_Military_GetAttackPoint(object, source, &point, nullptr, false))
				{
					Vehicle_Military_GetAttackPoint(object, source, &point, human, true);

					if (Human_l_StartAtomicWalk(human, &point, &point))
					{
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, SwitchIds::HumanTask::ApproachAttackTarget);
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::Running);
						return;
					}
				}
			}
			else if (type == SwitchIds::TargetType::Animal || type == SwitchIds::TargetType::Human)
			{
				object = type == SwitchIds::TargetType::Animal ? static_cast<const void*>(CAnimalArray::mStaticVars + TargetData_GetObjectId(target) * CAnimalArray::RuntimeElementSize) : Human(TargetData_GetObjectId(target));
				MapMoveable_Attachment_GetNextMapPosition(Moveable(Read<std::int32_t>(object, NC2Layout::AnimalOffset::MapMoveableId)), &point);
			}
			else
			{
				continue;
			}

			const auto distance = Distance(&point, source);

			if (distance < static_cast<std::uint32_t>(weapon.minimumRange))
			{
				if (!Human_l_GetNextPointToStartAttack(human, &point))
				{
					Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);
					continue;
				}

				if (Human_l_StartAtomicWalk(human, &point, &point))
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, SwitchIds::HumanTask::ApproachAttackTarget);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::Running);
					return;
				}
			}

			if (distance > static_cast<std::uint32_t>(weapon.maximumRange))
			{
				const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::ContinentId);

				if (continent == Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId))
				{
					if (type == SwitchIds::TargetType::Landscape || type == SwitchIds::TargetType::House)
					{
						if (type == SwitchIds::TargetType::House || static_cast<std::uint32_t>(weapon.maximumRange) > 1u)
						{
							auto candidate = point;

							if (!Human_l_GetDistancePointFromTarget(human, source, &point, &candidate, range))
							{
								Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::PendingStartup);
								Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, SwitchIds::HumanTask::Idle);
								return;
							}

							walk = Human_l_StartAtomicWalk(human, &candidate, &candidate);
						}
						else
						{
							if (!TargetData_MapPos_StartCheckForAttackPoint(target, source))
							{
								Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);
								continue;
							}

							TargetData_MapPos_GetAttackPosition(target, &point);
							walk = Human_l_StartAtomicWalk(human, &point, &point);
						}
					}
					else
					{
						auto pursuitRange = range;

						if (type == SwitchIds::TargetType::Vehicle && static_cast<std::uint32_t>(weapon.maximumRange) <= static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(object)->logicSize))
							pursuitRange += static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(object)->logicSize);

						walk = Human_l_StartAtomicWalk(human, Read<std::int32_t>(object, type == SwitchIds::TargetType::Vehicle ? NC2Layout::VehicleOffset::MapMoveableId : NC2Layout::HumanOffset::MapMoveableId), pursuitRange);
					}
				}
				else
				{
					if (!Human_l_GetNextPointToStartAttack(human, &point))
					{
						Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);
						continue;
					}

					walk = Human_l_StartAtomicWalk(human, &point, &point);
				}

				if (walk)
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, SwitchIds::HumanTask::ApproachAttackTarget);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::Running);
					return;
				}
			}

			if (type == SwitchIds::TargetType::Landscape && distance > static_cast<std::uint32_t>(weapon.maximumRange))
			{
				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);
				continue;
			}

			if (Human_l_IsOtherAttackingHumanOnPoint(human, source))
			{
				if ((type == SwitchIds::TargetType::House || type == SwitchIds::TargetType::Vehicle) && static_cast<std::uint32_t>(weapon.maximumRange) > 1u)
				{
					if (Human_l_GetNextPointToStartAttack(human, &point) && Human_l_StartAtomicWalk(human, &point, &point))
					{
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, SwitchIds::HumanTask::ApproachAttackTarget);
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::Running);
						return;
					}

					Human_l_AttackTarget(human, &point);
					return;
				}

				auto candidate = point;
				bool found;

				if (type == SwitchIds::TargetType::House && static_cast<std::uint32_t>(weapon.maximumRange) <= 1u)
				{
					found = House_Military_GetNextFreeAttackPoint(object, source, &candidate, human, true);
				}
				else if (type == SwitchIds::TargetType::Vehicle && static_cast<std::uint32_t>(weapon.maximumRange) <= 1u)
				{
					found = Vehicle_Military_GetAttackPoint(object, source, &candidate, human, true);
				}
				else
				{
					found = Human_l_GetNextPointToStartAttack(human, &candidate);
				}

				if (found && candidate != Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates) && Human_l_StartAtomicWalk(human, &candidate, &candidate))
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, SwitchIds::HumanTask::ApproachAttackTarget);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::Running);
					return;
				}
			}

			Human_l_AttackTarget(human, &point);

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::Failed);
	}

	// Original function: FUN_0046b29a; Windows thresholds at 004F3B48/4C are 8000.
	void Human_l_FinishTask_Attack(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode) == SwitchIds::HumanMilitaryMode::SearchWithinWeaponRange)
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, SwitchIds::TargetType::None);

		const auto next = Human_UserControl_IsPriorityCommandInQueue(human) ? SwitchIds::HumanTask::IdleUnsigned : Human_IsJobSoldier(human) || Human_IsJobHero(human) || Human_IsJobHunter(human)                                                                                                                                                                 ? SwitchIds::HumanTask::ExecuteJobUnsigned
		                                                                                                       : Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) > NC2Runtime::HumanAttackLimits::NeedsRequiredForRepeatAttack && Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) > NC2Runtime::HumanAttackLimits::NeedsRequiredForRepeatAttack ? SwitchIds::HumanTask::AttackUnsigned
		                                                                                                                                                                                                                                                                                                                                                           : SwitchIds::HumanTask::IdleUnsigned;
		Write(human, NC2Layout::HumanOffset::CurrentTask, next);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, SwitchIds::HumanTaskStatus::PendingStartup);
	}

	// Original function: FUN_00445e3a; discard the secondary target first in military mode 3.
	void Human_l_FailTask_Attack(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode) == SwitchIds::HumanMilitaryMode::SearchWithinWeaponRange)
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, SwitchIds::TargetType::None);

		auto* target = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::SecondaryMilitaryTarget;

		if (!TargetData_IsValid(target))
		{
			target = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget;

			if (!TargetData_IsValid(target))
				return;
		}

		const auto type = TargetData_GetTargetType(target);

		if (type != SwitchIds::TargetType::House && type != SwitchIds::TargetType::Landscape)
		{

			return;
		}

		Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, SwitchIds::TargetType::None);
		Human_UserMessage_SendToUserMessage(human, 17);
	}
}
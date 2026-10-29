#include "C2CallbackManager.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanTaskAccess.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2ObjectMemberConstructors.h"
#include "C2OverviewObjectIterators.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_0042cb60; the listening permission differs from ListenNow.
	bool Human_IsAbleToListen(const void* human)
	{

		return Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::ListenAllowedUnsigned) != 0;
	}

	// Original function: FUN_00444b79; carrying a good leaves task state unchanged.
	void Human_l_StartTask_Listen(void* human, std::uint32_t direction)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{

			return;
		}

		Human_l_StartAtomicAnimation(human, 17, static_cast<std::int32_t>(direction));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 6);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_00444aaf; the approach animation uses task 13 at the home entrance.
	void Human_l_StartTask_GoHome(void* human)
	{
		if (!Human_IsAttachedToHomeHouse(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			return;
		}

		auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId) * CHouseArray::ElementSize;
		const auto work = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint);

		if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == work)
		{
			Human_l_StartAtomicAnimation(human, 78, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 13);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		if (!Human_l_StartAtomicWalk(human, static_cast<std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, &work))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);

			return;
		}
		// Windows adds a zero-filled opcode-6 command, without human id/revision.
		Human_DoAddNewUserCommand(human, 6, 0, 0, 0, 0, 0);
	}

	// Original function: FUN_00445d5b; resume the go-home command after detaching paired waits.
	void Human_l_FailTask_GoHome(void* human)
	{
		Human_StopWaitingForOtherHuman_Detach(human);
		Human_DoAddNewUserCommand(human, 6, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
	}

	// Original function: FUN_00445ce7; callback is retained even for empty cargo.
	void Human_l_FailTask_Misc(void* human)
	{
		const auto failure = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep);

		if (failure != 7 && failure != 11)
		{

			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
		{
			Human_StopCurrentGoodCirculationMode(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood, 0);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 0);
	}

	namespace
	{
		struct SDamagedWallSearch
		{
			std::uint32_t player;
			std::uint32_t count;
			std::uint32_t points[10];
		};

		// Original function: FUN_0045eb2a.
		bool __cdecl slHumanSearchDamagedWallFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SDamagedWallSearch*>(callback->userData);
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0 || Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) != data.player)
				return false;

			const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);

			if (landscape < 82 || landscape > 84 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) >= 100)
				return false;

			data.points[data.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return data.count == 10;
		}
	}

	// Original function: FUN_0045eaa9; native attempts sample with replacement.
	bool Human_FindDamagedWallNearWorkCenter(const void* human, void* realPosition, void* workPosition)
	{
		SDamagedWallSearch data;
		SearchDamagedWall_ConstructMembers(&data);
		data.player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		data.count = 0;
		CMapPathfinderElement::SearchOverMap(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter, 5, false, slHumanSearchDamagedWallFunction, &data, true, static_cast<std::int32_t>(data.player));

		for (std::uint32_t attempt = 0; attempt < data.count * 2u; ++attempt)
		{
			Write(realPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, data.points[Game_Random_GetNumber() % data.count]);

			if (GameMap_Misc_GetLandscapeWorkPosition(realPosition, workPosition))
				return true;
		}

		return false;
	}

	// Original function: FUN_0045ccb6; the Mac predicate is inlined and has no recovered standalone name.
	bool Human_UsesExtendedPathfinder(const void* human)
	{
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		return Human_IsJobSoldier(human) || Human_IsJobHero(human) || Human_IsJobScout(human) || Human_IsJobHunter(human) || job == 30 || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit8) != 0;
	}

	namespace
	{
		void* Moveable(const void* human)
		{

			return gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		}

		const void* Position(const void* human)
		{

			return Human_GetMapPositionRef(human);
		}

		struct NearbyPoints
		{
			NearbyPoints()
			{
				SearchNearbyMapPoints_ConstructMembers(this);
			}

			std::uint32_t count = 0;
			std::uint32_t points[8];
		};

		static_assert(sizeof(NearbyPoints) == 36);

		struct SleepLandscapes
		{
			std::uint32_t landscapeType = 4;
			std::uint32_t unresolved04 = 0;
			std::uint32_t count = 0;
			std::uint32_t points[10]{ };
		};

		static_assert(sizeof(SleepLandscapes) == 52);
		static_assert(offsetof(SleepLandscapes, count) == 8);

		// Original function: FUN_0045e586; the callback stops at exactly ten points.
		bool __cdecl slHumanSearchLandscapeFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data)
		{
			auto* candidates = static_cast<SleepLandscapes*>(data->userData);
			const void* micro = GameMap_GetConstMapMicroPointPtr(point);
			const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) : 0u;

			if (type != candidates->landscapeType)
				return false;

			candidates->points[candidates->count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return candidates->count == 10;
		}

		// Original function: FUN_0045d533.
		bool __cdecl slHumanSearchMapPointNearbyFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data)
		{
			auto* candidates = static_cast<NearbyPoints*>(data->userData);
			const void* micro = GameMap_GetConstMapMicroPointPtr(point);

			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || Read<std::int16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId) != -1)
				return false;

			candidates->points[candidates->count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return candidates->count == 8;
		}

		void SetHumanTarget(void* human, const void* other)
		{
			Human_Target_SetTargetPositions(human, Position(other), Position(human));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 7);
			Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId));
			Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId));
		}
	}

	// Original function: FUN_0042cb1c; listen eligibility is not the goodie predicate.
	bool Human_IsAbleToListenNow(const void* human)
	{
		const auto task = Read<std::int32_t>(human, NC2Layout::HumanOffset::CurrentTask);

		return Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::ListenNowAllowedUnsigned) != 0 && (task == 1 || (task >= 24 && task <= 29));
	}

	// Original function: FUN_0042c98b.
	bool Human_IsDoingGoodie(const void* human)
	{
		const auto task = Read<std::int32_t>(human, NC2Layout::HumanOffset::CurrentTask);

		return task >= 24 && task <= 29;
	}

	// Original function: FUN_0042d00a.
	void Human_Target_SetWorkPosition(void* human, const void* position)
	{
		Write(human, NC2Layout::HumanOffset::TargetWorkPosition, Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates));
	}

	// Original function: FUN_0042cf55.
	std::uint32_t Human_Target_GetDirectionTo(const void* human)
	{

		return VE_HexagonDirection_GetDirectionFromPositions(Read<std::int16_t>(human, NC2Layout::HumanOffset::TargetWorkPositionX), Read<std::int16_t>(human, NC2Layout::HumanOffset::TargetWorkPositionY), Read<std::int16_t>(human, NC2Layout::HumanOffset::TargetPositionX), Read<std::int16_t>(human, NC2Layout::HumanOffset::TargetPositionY), false);
	}

	// Original function: FUN_0042cf88; both positions are stored before house lookup.
	void Human_Target_SetTargetPositions(void* human, const void* realPosition, const void* workPosition)
	{
		Write(human, NC2Layout::HumanOffset::TargetPosition, Read<std::uint32_t>(realPosition, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write(human, NC2Layout::HumanOffset::TargetWorkPosition, Read<std::uint32_t>(workPosition, NC2Layout::MapCoordinatePairOffset::Coordinates));
		const auto id = Logic_EntryPoint_House_GetOnPosition(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition);

		if (id != -1)
		{
			const void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 5);
				Write(human, NC2Layout::HumanOffset::TargetObjectId, id);
				Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));

				return;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 1);
	}

	// Original function: FUN_00443d74; verified from PE bytes after a misdecoded table.
	bool Human_StartWaitingForOtherHuman(void* human)
	{
		const auto count = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount) + 1u;
		Write(human, NC2Layout::HumanOffset::WaitingHumanCount, count);

		if (static_cast<std::int32_t>(count) >= 10)
			return false;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 1)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 9);
		}

		return true;
	}

	// Original function: FUN_00443e79; keep the return value independent of animation startup.
	bool Human_StartListening(void* human, void* speaker)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 9 || !Human_l_IsAtomicInterruptable(human))
			return false;

		SetHumanTarget(human, speaker);
		Human_l_StartAtomicAnimation(human, 15, static_cast<std::int32_t>(Human_Target_GetDirectionTo(human)));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);

		return true;
	}

	// Original function: FUN_00444e46.
	void Human_l_StartTask_WaitForOtherHuman(void* human)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
			Human_StopCurrentGoodCirculationMode(human);

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		Human_Target_Reset(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Remember(human);

			return;
		}

		Human_l_StartAtomicAnimation(human, 5, -1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_00441a39.
	bool Human_HouseMisc_Leave(void* human)
	{
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideHouseFlag) == 0)
			return false;

		Write<std::uint32_t>(Moveable(human), NC2Layout::MapMoveableOffset::Direction, 2);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::InsideHouseFlag, 0);

		return true;
	}

	// Original function: FUN_00441a63; forced house exit is distinct from animation-driven Leave.
	void Human_HouseMisc_Exit(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) != 1)
		{
			Human_l_ResetAtomic(human);
			Human_CurrentTask_DoInterrupt(human);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::UrgentNeedsCheckedFlag, 1);
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag, 1);
			Human_AI_DoUpdateTaskMachine(human);
			Write<std::uint32_t>(Moveable(human), NC2Layout::MapMoveableOffset::Direction, 2);
		}

		Write<std::uint8_t>(human, NC2Layout::HumanOffset::InsideHouseFlag, 0);
	}

	// Original function: FUN_0045c82f.
	bool Human_l_StartAtomicWalk(void* human, const void* realPosition, const void* workPosition)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return false;

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
			const auto kind = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

			if (((kind == 3 || kind == 4) && Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) == 0) || !Vehicle_Passengers_MoveOut_h(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)))
				return false;
		}

		if (Read<std::uint32_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(workPosition, NC2Layout::MapCoordinatePairOffset::Coordinates))
		{
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction, 1);
			Human_Target_SetTargetPositions(human, realPosition, workPosition);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 2);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
			return true;
		}

		if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(workPosition), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
			return false;

		const auto useSectors = Human_UsesExtendedPathfinder(human);
		// Windows 004F4300/004F4304: normal/carrier search radii 50/63.

		if (!MapMoveable_Pathfinder_Start(Moveable(human), workPosition, Human_IsJobCarrier(human) ? 63u : 50u, true, useSectors, true))
			return false;

		Write<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction, 1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 2);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		Human_Target_SetTargetPositions(human, realPosition, workPosition);
		const void* micro = GameMap_GetConstMapMicroPointPtr(Position(human));

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			Human_HouseMisc_Leave(human);

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::MapPositionReachedCount, 0);

		return true;
	}

	// Original function: FUN_0045c984.
	bool Human_l_StartAtomicWalk(void* human, std::int32_t moveableId, std::uint32_t distance)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return false;

		const auto useSectors = Human_UsesExtendedPathfinder(human);

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
			const auto kind = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

			if (((kind == 3 || kind == 4) && Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) == 0) || !Vehicle_Passengers_MoveOut_h(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)))
				return false;
		}

		if (!MapMoveable_Pathfinder_Start(Moveable(human), moveableId, static_cast<std::int32_t>(distance), Human_IsJobCarrier(human) ? 63u : 50u, true, useSectors, true))
			return false;

		Write<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction, 1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 2);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		const void* micro = GameMap_GetConstMapMicroPointPtr(Position(human));

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			Human_HouseMisc_Leave(human);

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::MapPositionReachedCount, 0);

		return true;
	}

	// Original function: FUN_0045ca72.
	bool Human_l_StartAtomicWalkAndWait(void* human, const void* point)
	{
		if (!Human_l_StartAtomicWalk(human, point, point))
			return false;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 4);

		return true;
	}

	// Original function: FUN_0045d4d4; eight candidates and Windows radius 40.
	bool Human_FindMapPointNearby(const void* human, void* point)
	{
		NearbyPoints candidates;
		CMapPathfinderElement::SearchOverMap(Position(human), 40, true, slHumanSearchMapPointNearbyFunction, &candidates, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));

		if (candidates.count == 0)
			return false;

		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, candidates.points[Game_Random_GetNumber() % candidates.count]);

		return true;
	}

	// Original function: FUN_0045de98.
	void* Human_FindHumanToTalkInTalkRange_Any(const void* human)
	{
		CHumanIterator iterator;

		for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Position(human), 1); id != -1; id = iterator.GetNextHumanId())
		{
			void* other = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;

			if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) || Human_Misc_IsHidden(other))
				continue;

			if (Read<std::uint32_t>(Position(other), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates) || !Human_IsAbleToListenNow(other))
				continue;
			// Windows retains one candidate and still consumes random % 1.
			Game_Random_GetNumber();
			return other;
		}

		return nullptr;
	}

	// Original function: FUN_0045df56; retain a separate original search entry.
	void* Human_FindHumanToTalkInSearchRange_Any(const void* human)
	{
		CHumanIterator iterator;

		for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Position(human), 10); id != -1; id = iterator.GetNextHumanId())
		{
			void* other = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;

			if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) || Human_Misc_IsHidden(other))
				continue;

			if (Read<std::uint32_t>(Position(other), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates) || !Human_IsAbleToListenNow(other))
				continue;

			Game_Random_GetNumber();
			return other;
		}

		return nullptr;
	}

	// Original function: FUN_0046a407.
	void Human_l_StartTask_Goodie(void* human)
	{
		if (Human_UserControl_IsCommandTypeInQueue(human, 11))
		{
			if (Human_UserControl_GetLastCommandType(human) == 11)
			{
				const auto task = Read<std::int32_t>(human, NC2Layout::HumanOffset::FirstUserCommandParameter1Unsigned + Read<std::uint32_t>(human, NC2Layout::HumanOffset::LastUserCommandIndex) * NC2Layout::RecordSize::UserNetworkCommand);
				Write<std::int32_t>(human, NC2Layout::HumanOffset::CurrentTask, task >= 24 && task <= 29 ? task : 24);
			}

			Write<std::uint8_t>(human, NC2Layout::HumanOffset::QueuedGoodieCommand, 0);
		}

		if (Human_IsAgeBaby(human))
		{
			Human_l_StartTask_Goodie_Helper_Baby(human);
		}
		else if (Human_IsAgeChild(human))
		{
			Human_l_StartTask_Goodie_Helper_Child(human);
		}
		else
		{
			Human_l_StartTask_Goodie_Helper_TalkToAny(human);
		}
	}

	// Original function: FUN_0046a481. Windows folds Helper_Idle into both age dispatchers.
	void Human_l_StartTask_Goodie_Helper_Baby(void* human)
	{
		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask))
		{
			case SwitchIds::HumanTask::ChooseGoodie:
				Write(human, NC2Layout::HumanOffset::CurrentTask, (Game_Random_GetNumber() & 1u) + 25u);
				return;
			case SwitchIds::HumanTask::GoodieIdle:
				Human_l_StartAtomicAnimation(human, 5, -1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 25);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
				return;
			case SwitchIds::HumanTask::GoodieWalkAround:
				Human_l_StartTask_Goodie_Helper_WalkAround(human);
				return;
			default:
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 24);

				return;
		}
	}

	// Original function: FUN_0046a4dc.
	void Human_l_StartTask_Goodie_Helper_Child(void* human)
	{
		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask))
		{
			case SwitchIds::HumanTask::ChooseGoodie:
				Write(human, NC2Layout::HumanOffset::CurrentTask, Game_Random_GetNumber() % 5u + 25u);
				return;
			case SwitchIds::HumanTask::GoodieIdle:
				Human_l_StartAtomicAnimation(human, 5, -1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 25);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
				return;
			case SwitchIds::HumanTask::GoodieWalkAround:
				Human_l_StartTask_Goodie_Helper_WalkAround(human);
				return;
			case SwitchIds::HumanTask::GoodieSleep:
				Human_l_StartTask_Goodie_Helper_Sleep(human);
				return;
			case SwitchIds::HumanTask::GoodieTalkToAny:
				Human_l_StartTask_Goodie_Helper_TalkToAny(human);
				return;
			case SwitchIds::HumanTask::GoodiePlayWithAnimal:
				Human_l_StartTask_Goodie_Helper_PlayWithAnimal(human);
				return;
			default:
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 24);

				return;
		}
	}

	// Original function: FUN_0046a55f.
	void Human_l_StartTask_Goodie_Helper_WalkAround(void* human)
	{
		std::uint32_t point = 0;

		if (Human_FindMapPointNearby(human, &point) && Human_l_StartAtomicWalk(human, &point, &point))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 26);

			return;
		}

		Human_l_StartAtomicAnimation(human, 5, -1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 25);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_00460b31; Windows searches radius 40 / 2 and retains
	// up to twice the candidate count in random work-position retries.
	bool Human_FindPlaceToSleep(const void* human, void* realPosition, void* workPosition)
	{
		if (!Human_IsJobSoldier(human))
		{
			SleepLandscapes candidates;
			CMapPathfinderElement::SearchOverMap(Position(human), 20, false, slHumanSearchLandscapeFunction, &candidates, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));

			if (candidates.count != 0)
			{
				Write(realPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, candidates.points[0]);

				if (GameMap_Misc_GetLandscapeWorkPosition(realPosition, workPosition))
					return true;

				for (auto attempts = candidates.count * 2u; attempts != 0; --attempts)
				{
					Write(realPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, candidates.points[Game_Random_GetNumber() % candidates.count]);

					if (GameMap_Misc_GetLandscapeWorkPosition(realPosition, workPosition))
						return true;
				}
			}
		}
		// Windows folds CanSleepAtMapWorkPosition and IsAbleToUseVehicle into the same unconditional predicate.
		Human_IsAbleToUseVehicle(human);
		Human_CanSleepAtMapRealPosition(Position(human));
		Write(realPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write(workPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates));

		return true;
	}

	// Original function: FUN_0046a5be.
	void Human_l_StartTask_Goodie_Helper_Sleep(void* human)
	{
		if (Human_IsAtTargetPoint(human) && Human_CanSleepAtTargetMapPosition(human))
		{
			Human_l_StartAtomicAnimation(human, 8, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		std::uint32_t realPosition = 0, workPosition = 0;

		if (Human_FindPlaceToSleep(human, &realPosition, &workPosition) && Human_l_StartAtomicWalk(human, &realPosition, &workPosition))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 27);
			Human_DoAddNewUserCommand(human, 11, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 27, 0);

			return;
		}

		Human_l_StartAtomicAnimation(human, 5, -1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 25);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_0045e498; strict weighted-distance comparison retains the first tie.
	void* Human_FindAnimalForPlaying(const void* human)
	{
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Position(human)), NC2Layout::MapMicroPointOffset::ContinentId);
		CAnimalIterator iterator;
		void* selected = nullptr;
		std::uint32_t nearest = 10000;

		for (auto id = iterator.GetFirstSectorAnimalId(20, Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter), 50, false, false); id != -1; id = iterator.GetNextSectorAnimalId())
		{
			void* animal = CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::ElementSize;

			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 49 && Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AggressiveFlag) != 0)
				continue;

			const void* position = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable + 0x12;

			if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId) != continent)
				continue;

			auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(Position(human), NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y)));
			const auto tribe = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType);

			if (tribe != 16 && tribe != 17)
				distance *= 2u;

			if (distance < nearest)
			{
				selected = animal;
				nearest = distance;
			}
		}

		return selected;
	}

	// Original function: FUN_0046a662.
	void Human_l_StartTask_Goodie_Helper_PlayWithAnimal(void* human)
	{
		if (const void* animal = Human_FindAnimalForPlaying(human))
		{
			const void* point = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable + 0x12;

			if (Human_l_StartAtomicWalk(human, point, point))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 29);

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) != 2)
					Human_DoAddNewUserCommand(human, 11, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 29, 0);

				return;
			}
		}

		Human_l_StartAtomicAnimation(human, 5, -1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 25);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_0046a6d7; Windows has one shared adult/any talk helper.
	void Human_l_StartTask_Goodie_Helper_TalkToAny(void* human)
	{
		void* other = Human_FindHumanToTalkInTalkRange_Any(human);

		if (other != nullptr && Human_StartWaitingForOtherHuman(other))
		{
			SetHumanTarget(human, other);
			Human_StartListening(other, human);
			Human_StopWaitingForOtherHuman(other);
			const auto action = Human_IsAgeAdult(human) ? 14u : 15u;
			Human_l_StartAtomicAnimation(human, action, static_cast<std::int32_t>(Human_Target_GetDirectionTo(human)));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 28);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		other = Human_FindHumanToTalkInSearchRange_Any(human);

		if (other != nullptr && Human_StartWaitingForOtherHuman(other))
		{
			Human_StopWaitingForOtherHuman(other);

			if (Human_l_StartAtomicWalk(human, Read<std::int32_t>(other, NC2Layout::HumanOffset::MapMoveableId), 1))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 28);
				Human_DoAddNewUserCommand(human, 11, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 28, 0);

				return;
			}
		}

		Human_l_StartAtomicAnimation(human, 5, -1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 25);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_0046a7d7; Windows clears AD0 before the idle finish.
	void Human_l_FinishTask_Goodie(void* human)
	{
		if (Human_UserControl_IsCommandTypeInQueue(human, 11))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
			Human_l_FinishTask_Idle(human);
		}
	}

	// Original function: FUN_0046a7fe.
	void Human_l_FailTask_Goodie(void* human)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
	}

	// Original function: FUN_0045e014.
	void* Human_FindHumanToTalkInTalkRange_Adult(const void* human)
	{
		CHumanIterator iterator;

		for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Position(human), 1); id != -1; id = iterator.GetNextHumanId())
		{
			void* other = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;

			if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) || Human_Misc_IsHidden(other) || !Human_IsAgeAdult(other))
				continue;

			if (Read<std::uint32_t>(Position(other), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates) || !Human_IsAbleToListenNow(other))
				continue;
			// Windows retains one candidate and still consumes random % 1.
			Game_Random_GetNumber();
			return other;
		}

		return nullptr;
	}

	// Original function: FUN_0045e0dd.
	void* Human_FindHumanToTalkInSearchRange_Adult(const void* human)
	{
		CHumanIterator iterator;

		for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Position(human), 10); id != -1; id = iterator.GetNextHumanId())
		{
			void* other = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;

			if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) || Human_Misc_IsHidden(other) || !Human_IsAgeAdult(other))
				continue;

			if (Read<std::uint32_t>(Position(other), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates) || !Human_IsAbleToListenNow(other))
				continue;

			Game_Random_GetNumber();
			return other;
		}

		return nullptr;
	}

}
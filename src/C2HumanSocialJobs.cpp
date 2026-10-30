#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanProduction.h"
#include "C2HumanSocialJobs.h"
#include "C2HumanTaskAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void Fail(void* human, std::uint32_t reason)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write(human, NC2Layout::HumanOffset::CurrentTaskStep, reason);
		}

		bool IsWorking(const void* human)
		{
			const auto status = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus);

			return status == 1 || status == 2;
		}

		bool HasUnsharedPosition(const void* human)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));
			const auto moveable = Read<std::int16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId);

			return moveable == Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId) && Read<std::int32_t>(gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(moveable) * NC2Layout::RecordSize::MapMoveable, NC2Layout::MapMoveableOffset::NextCellMoveableId) == -1;
		}

		void TargetHuman(void* human, const void* other)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 7);
			Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId));
			Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId));
			Write(human, NC2Layout::HumanOffset::TargetWorkPosition, Read<std::uint32_t>(Human_GetMapPositionRef(other), NC2Layout::MapCoordinatePairOffset::Coordinates));
			Write(human, NC2Layout::HumanOffset::TargetPosition, Read<std::uint32_t>(Human_GetMapPositionRef(other), NC2Layout::MapCoordinatePairOffset::Coordinates));
		}
	}

	// Original function: FUN_0045e261; the first eligible human wins, then RNG is consumed.
	void* Human_FindHumanToEnjoy(const void* human)
	{
		CHumanIterator iterator;
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter, job == 18 || job == 29 ? 10 : 20); id != -1; id = iterator.GetNextHumanId())
		{
			auto* other = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

			if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) || Read<std::uint8_t>(other, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 || Read<std::int32_t>(other, NC2Layout::HumanOffset::Social) > 8000 || !Human_IsJobNeedingSocial(other))
				continue;

			Game_Random_GetNumber();
			return other;
		}

		return nullptr;
	}

	// Original function: FUN_0045e31a; Windows checks hitpoints, not religion.
	void* Human_FindHumanToBless(const void* human)
	{
		CHumanIterator iterator;
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter, job == 18 || job == 29 ? 10 : 20); id != -1; id = iterator.GetNextHumanId())
		{
			auto* other = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

			if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) || Read<std::int32_t>(other, NC2Layout::HumanOffset::Hitpoints) > static_cast<std::int32_t>(Human_GetHumanAttributeMaximum_Hitpoints(other)))
				continue;

			Game_Random_GetNumber();
			return other;
		}

		return nullptr;
	}

	// Original function: FUN_0046f035.
	void Human_l_StartTask_ExecuteJob_Jester(void* human)
	{
		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode))
		{
			case SwitchIds::GoodCirculationMode::None:
				Human_l_StartTask_ExecuteJob_Jester_TryToJest(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockFetch:
				Human_l_StartTask_ExecuteJob_Jester_Collect(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockBringAway:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
				return;
			case SwitchIds::GoodCirculationMode::FlushStock:
				Fail(human, 0);

				return;
		}
	}

	// Original function: FUN_0046f06b.
	void Human_l_StartTask_ExecuteJob_Jester_Collect(void* human)
	{
		if (Human_IsAtTargetPoint(human))
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 7)
			{
				Human_l_StartTask_ExecuteJob_Jester_Jest(human);
			}
			else
			{
				Human_l_StartTask_ExecuteJob_Tool_CollectWantedGood(human);
			}

			return;
		}

		Human_StopCurrentGoodCirculationMode(human);
		Human_l_StartTask_ExecuteJob_Jester_TryToJest(human);
	}

	// Original function: FUN_0046f0a0.
	void Human_l_StartTask_ExecuteJob_Jester_TryToJest(void* human)
	{
		Human_Target_Reset(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		auto* other = Human_FindHumanToEnjoy(human);

		if (other != nullptr && Human_l_StartAtomicWalk(human, Read<std::int32_t>(other, NC2Layout::HumanOffset::MapMoveableId), 1))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 2);
			TargetHuman(human, other);

			return;
		}

		Fail(human, 6);
	}

	// Original function: FUN_0046f134.
	void Human_l_StartTask_ExecuteJob_Jester_Jest(void* human)
	{
		if (!HasUnsharedPosition(human))
		{
			Human_l_StartTask_GoAway(human, 0, 0);

			return;
		}

		Human_Target_SetWorkPosition(human, Human_GetMapPositionRef(human));
		Human_l_StartAtomicAnimation(human, 16, Human_Target_GetDirectionTo(human));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_0046f26d.
	void Human_l_StartTask_ExecuteJob_Druid(void* human)
	{
		if (Human_IsAttachedToWorkHouse(human))
		{
			const auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3)
			{
				if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 37)
				{
					Human_l_StartTask_ExecuteJob_Druid_Priest(human);
				}
				else
				{
					Human_l_StartTask_ExecuteJob_Tool_GoodProducer(human);
				}

				return;
			}
		}

		Human_l_StartTask_ExecuteJob_Druid_Healer(human);
	}

	// Original function: FUN_0046f2b2.
	void Human_l_StartTask_ExecuteJob_Druid_Priest(void* human)
	{
		if (!Human_Target_HasTarget(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) != 7)
			Human_l_StartTask_ExecuteJob_Tool_FillTemple(human);

		if (IsWorking(human))
		{

			return;
		}

		Human_l_StartTask_ExecuteJob_Druid_Healer(human);

		if (!IsWorking(human))
			Fail(human, 2);
	}

	// Original function: FUN_0046f30b.
	void Human_l_StartTask_ExecuteJob_Druid_Healer(void* human)
	{
		if (Human_Target_IsValid(human))
		{
			Human_l_StartTask_ExecuteJob_Druid_Heal(human);
		}
		else
		{
			Human_l_StartTask_ExecuteJob_Druid_TryToHeal(human);
		}
	}

	// Original function: FUN_0046f327.
	void Human_l_StartTask_ExecuteJob_Druid_TryToHeal(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		auto* other = Human_FindHumanToBless(human);

		if (other != nullptr && Human_l_StartAtomicWalk(human, Read<std::int32_t>(other, NC2Layout::HumanOffset::MapMoveableId), 1))
		{
			Human_Target_Reset(human);
			TargetHuman(human, other);

			return;
		}

		Fail(human, 6);
	}

	// Original function: FUN_0046f3b1.
	void Human_l_StartTask_ExecuteJob_Druid_Heal(void* human)
	{
		if (!HasUnsharedPosition(human))
		{
			Human_l_StartTask_GoAway(human, 0, 0);

			return;
		}

		Human_Target_SetWorkPosition(human, Human_GetMapPositionRef(human));
		Human_l_StartAtomicAnimation(human, 18, Human_Target_GetDirectionTo(human));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_004757a6; the Windows entry contains dispatch, delivery and collection.
	void Human_l_StartTask_ExecuteJob_Tool_FillTemple(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 0)
		{
			Human_l_StartTask_ExecuteJob_Tool_FillTemple_Fetch(human);
			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) == 0)
		{
			if (Human_IsAtTargetPoint(human))
			{
				if (Human_l_StartTask_Tool_PickupWantedGood_FromTargetPosition(human))
					return;

				Human_Target_Reset(human);
				Human_StopCurrentGoodCirculationMode(human);
			}
			else
			{
				Human_StopCurrentGoodCirculationMode(human);
				Human_l_StartTask_ExecuteJob_Tool_FillTemple_Fetch(human);
			}

			return;
		}

		std::uint32_t real = 0, work = 0;

		if (Human_IsAtTargetPoint(human))
		{
			auto* house = Human_GetHousePtrAtTargetPosition(human);

			if (house != nullptr)
			{
				if (!House_Stock_CanStoreGoodType(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), false) || House_Stock_IsFull(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood)))
				{
					Human_Target_Reset(human);
					return;
				}
			}
			else
			{
				if (Human_FindConsumerNearWorkCenter(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), &real, &work) && Human_l_StartAtomicWalkAndBring(human, &real, &work))
					return;

				if (!Map_GoodTool_CanBePutOnMap(Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), Human_Target_GetRealPositionRef(human)))
				{
					if (Human_FindPilePointNearby(human, &real, &work) && Human_l_StartAtomicWalk(human, &real, &work))
						return;

					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
					return;
				}
			}

			Human_l_StartAtomicAnimation(human, 23, 6);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;

		if (House_Stock_CanStoreGoodType(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), false) && House_Stock_HasDemandForGood(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood)) && Human_l_StartAtomicWalk(human, house + NC2Layout::HouseOffset::MapPosition, house + NC2Layout::HouseOffset::DoorEntryPoint))
			return;

		if (Human_FindConsumerNearWorkCenter(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), &real, &work) && Human_l_StartAtomicWalkAndBring(human, &real, &work))
			return;

		if (Human_FindPilePointNearby(human, &real, &work) && Human_l_StartAtomicWalk(human, &real, &work))
		{

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_004759ab.
	void Human_l_StartTask_ExecuteJob_Tool_FillTemple_Fetch(void* human)
	{
		Human_Target_Reset(human);
		auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;

		if (House_Stock_HasDemandForGood(house, 15))
		{
			std::uint32_t real = 0, work = 0;

			if (Human_FindGood_Complex(human, 15, &real, &work, Human_GetWorkCenterPointRef(human)))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 15);

				if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
				{

					return;
				}
			}
		}

		Fail(human, 2);
	}

}
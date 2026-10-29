#include "C2Debug.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMessageAccess.h"
#include "C2HumanProduction.h"
#include "C2HumanTransportJobs.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* Vehicle(const void* human)
		{

			return CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
		}

		void* WorkHouse(const void* human)
		{

			return CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::ElementSize;
		}

		bool IsUsable(const void* vehicle)
		{
			const auto type = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

			return (type != 3 && type != 4) || Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) != 0;
		}

		void TargetVehicle(void* human)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 6);
			Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId));
			Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision));
		}

		void TargetHouse(void* human, const void* house)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 5);
			Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
			Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
		}

		void Start(void* human, std::uint32_t animation)
		{
			Human_l_StartAtomicAnimation(human, animation, 6);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		}

		std::uint32_t PickupAnimation(const void* house)
		{
			const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

			return type == 10 ? 44u : type == 11 ? 45u
			                                     : 22u;
		}

		bool WalkToConsumer(void* human)
		{
			std::uint32_t real = 0, work = 0;

			return Human_FindConsumerNearWorkCenter(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), &real, &work) && Human_l_StartAtomicWalkAndBring(human, &real, &work);
		}

		bool WalkToPile(void* human, const void* source)
		{
			std::uint32_t real = 0, work = 0;

			return Human_FindPilePointNearPosition(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), source, &real, &work) && Human_l_StartAtomicWalk(human, &real, &work);
		}

		bool SameContinent(const void* human, const void* point)
		{

			return Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId) == Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human)), NC2Layout::MapMicroPointOffset::ContinentId);
		}
	}

	// Original function: FUN_00445e98.
	void Human_l_StartTask_MoveInsideVehicle(void* human)
	{
		auto* vehicle = Vehicle(human);

		if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision))
		{
			Human_DetachFromVehicle(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			return;
		}

		const auto cargo = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

		if (cargo != 0 && Vehicle_Stock_CanStoreGoodType(vehicle, cargo) && Vehicle_Stock_IsFull(vehicle))
		{
			Human_StopCurrentGoodCirculationMode(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) != 3)
				Human_DoAddNewUserCommand(human, 40, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision));

			return;
		}

		std::uint32_t door = 0;

		if (!IsUsable(vehicle))
		{
			std::byte message[772];
			ToUserMessage_Construct(message);
			Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
			Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
			Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 43);
			Write(message, NC2Layout::ToUserMessageOffset::EquipmentItems, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType));
			ToUserMessage_FinishMessage(message);
			NC2LogicExternals::External_Misc_SendToUserMessage(message);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 1);
			return;
		}

		if (!Vehicle_Door_GetEntryPoint(vehicle, &door, 0) || !SameContinent(human, &door))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 1);
			return;
		}

		if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == door)
		{
			if (!Human_VehicleMisc_Enter(human))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
				return;
			}

			const auto circulation = Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode);
			const bool bringing = Human_IsBringingCarriedGood(human);

			if (!bringing && circulation != 5 && circulation != 2)
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				return;
			}

			if (!Human_l_StartAtomicAnimation(human, bringing ? 23 : 22, -1))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);

				return;
			}

			TargetVehicle(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 8);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		}
		else if (Human_l_StartAtomicWalk(human, Vehicle_GetMapPositionRef(vehicle), &door))
		{
			TargetVehicle(human);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		}
	}

	// Original function: FUN_0044611b.
	void Human_l_FinishTask_MoveInsideVehicle(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 0)
			Human_Target_Reset(human);

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 8);
	}

	// Original function: FUN_0046d3bd.
	void Human_l_StartTask_ExecuteJob_Carrier(void* human)
	{
		if (Human_IsAttachedToVehicle(human))
		{
			auto* vehicle = Vehicle(human);

			if (IsUsable(vehicle))
			{
				if ((Vehicle_IsPriorityCommandInQueue(vehicle) || Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction) == 1) && Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
					return;
				}

				std::uint32_t door = 0;
				const bool hasDoor = Vehicle_Door_GetEntryPoint(vehicle, &door, 0);

				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 || (hasDoor && SameContinent(human, &door)))
				{
					const auto circulation = Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode);

					if (circulation == 2)
					{
						Human_l_StartTask_ExecuteJob_Carrier_Collect(human);
						return;
					}

					if (circulation == 5)
					{
						const bool started = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) == 0 ? Human_l_StartTask_ExecuteJob_Carrier_FlushVehicle(human) : Human_l_StartTask_ExecuteJob_Carrier_Flush_Bring(human);

						if (!started)
						{
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 2);
						}

						return;
					}

					if (circulation == 4)
					{
						const auto cargo = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

						if (Human_IsAtTargetPoint(human))
						{
							if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 6)
							{
								if (Vehicle_Stock_CanStoreGoodType(vehicle, cargo) && Vehicle_Stock_GetAmountOfWantedGoods(vehicle, cargo) >= Vehicle_Stock_GetFutureAmount(vehicle, cargo) && !Vehicle_Stock_IsFull(vehicle))
								{
									TargetVehicle(human);
									Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 21);
									Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
								}
								else
								{
									Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
								}

								return;
							}
						}
						else if (Vehicle_Stock_CanStoreGoodType(vehicle, cargo) && Vehicle_Stock_GetFutureAmount(vehicle, cargo) < Vehicle_Stock_GetAmountOfWantedGoods(vehicle, cargo) && !Vehicle_Stock_IsFullSoon(vehicle))
						{
							const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId);

							if (Vehicle_Stock_ModifyFutureAmount(vehicle, cargo, 1, revision))
							{
								if (Human_l_StartAtomicWalkAndBring(human, Vehicle_GetMapPositionRef(vehicle), &door))
								{
									TargetVehicle(human);
									return;
								}

								Vehicle_Stock_ModifyFutureAmount(vehicle, cargo, -1, revision);
							}
						}

						Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
						return;
					}

					if (Human_l_StartTask_ExecuteJob_Carrier_Vehicle(human))
					{

						return;
					}
				}
			}
		}

		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode))
		{
			case SwitchIds::GoodCirculationMode::FillStockFetch:
				Human_l_StartTask_ExecuteJob_Carrier_Collect(human);
				break;
			case SwitchIds::GoodCirculationMode::FillStockBringAway:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
				break;
			case SwitchIds::GoodCirculationMode::FlushStock:
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) == 0)
				{
					Human_l_StartTask_ExecuteJob_Carrier_FlushStock(human);
				}
				else
				{
					Human_l_StartTask_ExecuteJob_Tool_FlushStock_BringAway(human);
				}

				break;
			default:
				Human_l_StartTask_ExecuteJob_Carrier_TryToCollect(human);
				break;
		}
	}

	// Original function: FUN_0046d66f.
	bool Human_l_StartTask_ExecuteJob_Carrier_Flush_Bring(void* human)
	{
		auto* vehicle = Vehicle(human);

		if (IsUsable(vehicle))
		{
			std::uint32_t door = 0;

			if (!Vehicle_Door_GetEntryPoint(vehicle, &door, 0))
				return false;

			if (Human_IsAtTargetPoint(human))
			{
				const auto id = Logic_EntryPoint_House_GetOnPosition(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition);

				if (id != -1)
				{
					auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
					const auto cargo = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

					if (House_Stock_CanStoreGoodType(house, cargo, false) && House_Stock_HasDemandForGood(house, cargo))
					{
						Start(human, 23);
					}
					else
					{
						Human_Target_Reset(human);
					}

					return true;
				}

				if (WalkToConsumer(human))
					return true;

				if (Map_GoodTool_CanBePutOnMap(Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition))
				{
					Start(human, 23);
					return true;
				}

				if (WalkToPile(human, &door))
					return true;
			}
			else
			{
				if ((!Human_IsAttachedToWorkHouse(human) || House_GetHouseMainType(WorkHouse(human)) != 1) && WalkToConsumer(human))
					return true;

				if (WalkToPile(human, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter) || WalkToPile(human, &door))
					return true;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);

		return false;
	}

	// Original function: FUN_0046d8e0; remaster rolls back reservations when walking fails.
	bool Human_l_StartTask_ExecuteJob_Carrier_FlushVehicle(void* human)
	{
		auto* vehicle = Vehicle(human);

		if (!IsUsable(vehicle))
			return false;

		for (std::uint32_t good = 0; good < 66; ++good)
		{
			if (!Vehicle_Stock_CanStoreGoodType(vehicle, good) || Vehicle_Stock_GetAmountOfWantedGoods(vehicle, good) >= Vehicle_Stock_GetFutureAmount(vehicle, good))
				continue;

			const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId);

			if (!Vehicle_Stock_ModifyFutureAmount(vehicle, good, -1, revision))
				continue;

			Write(human, NC2Layout::HumanOffset::RequestedGood, good);
			std::uint32_t door = 0;

			if (Vehicle_Door_GetEntryPoint(vehicle, &door, 0) && Human_l_StartAtomicWalk(human, Vehicle_GetMapPositionRef(vehicle), &door))
			{
				TargetVehicle(human);
				return true;
			}

			Vehicle_Stock_ModifyFutureAmount(vehicle, good, 1, revision);
		}

		return false;
	}

	// Original function: FUN_0046d9ba.
	bool Human_l_StartTask_ExecuteJob_Carrier_Vehicle(void* human)
	{
		auto* vehicle = Vehicle(human);

		if (!IsUsable(vehicle))
			return false;

		std::uint32_t door = 0, real = 0, work = 0, good = 0;

		if (!Vehicle_Door_GetEntryPoint(vehicle, &door, 0))
			return false;

		if (Human_FindGoodsDemandedForVehicle(human, &real, &work, &good, &door))
		{
			Write(human, NC2Layout::HumanOffset::RequestedGood, good);

			if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
				return true;
		}

		for (good = 0; good < 66; ++good)
		{
			if (!Vehicle_Stock_CanStoreGoodType(vehicle, good) || Vehicle_Stock_GetAmountOfWantedGoods(vehicle, good) >= Vehicle_Stock_GetFutureAmount(vehicle, good))
				continue;

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 5);
			Human_Target_Reset(human);
			return true;
		}

		return false;
	}

	// Original function: FUN_0046daa8; no replacement-good search on failed pickup.
	void Human_l_StartTask_ExecuteJob_Carrier_Collect(void* human)
	{
		if (!Human_IsAtTargetPoint(human))
		{
			Human_StopCurrentGoodCirculationMode(human);
			Human_l_StartTask_ExecuteJob_Carrier_TryToCollect(human);
			return;
		}

		auto* house = Human_GetHousePtrAtTargetPosition(human);

		if (house != nullptr)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetWorkPosition) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint))
				C2Debug_WarningBox("standing at invalid position for interaction with house stock!");

			const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood);

			if (House_Stock_CanStoreGoodType(house, good, true) && House_Stock_RequestAmountOfGoods(house, good, true) > 0)
			{
				Start(human, PickupAnimation(house));
				return;
			}
		}
		else if (Human_GetLandscapeTypeAtTargetPosition(human) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetLandscapeType))
		{
			Start(human, 22);

			return;
		}

		Human_Target_Reset(human);
		Human_StopCurrentGoodCirculationMode(human);
	}

	// Original function: FUN_0046db75.
	void Human_l_StartTask_ExecuteJob_Carrier_TryToCollect(void* human)
	{
		Human_Target_Reset(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 2);

		if (Human_IsAttachedToWorkHouse(human))
		{
			std::uint32_t real = 0, work = 0, good = 0;

			if (Human_FindGoodsDemandedForStockHouse(human, &real, &work, &good, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter))
			{
				Write(human, NC2Layout::HumanOffset::RequestedGood, good);

				if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
					return;
			}
		}

		if (Human_IsAttachedToWorkHouse(human))
		{
			auto* house = WorkHouse(human);

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && House_GetHouseMainType(house) == 3)
			{
				std::uint32_t fullest = 0, largest = 0, amount = 0;

				for (std::uint32_t good = 1; good <= 55; ++good)
				{
					if (!House_Stock_CanStoreGoodType(house, good, true))
						continue;

					if (House_Stock_IsFullSoon(house, good))
					{
						fullest = good;
					}
					else
					{
						const auto current = House_Stock_GetAmountOfGoods(house, good, true);

						if (current > amount)
						{
							largest = good;
							amount = current;
						}
					}
				}

				if (fullest != 0)
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 5);

					if (Human_FindConsumerNearWorkCenter(human, fullest))
					{
						Write(human, NC2Layout::HumanOffset::RequestedGood, fullest);
						Human_l_StartTask_ExecuteJob_Carrier_FlushStock(human);
						return;
					}
				}

				if (largest != 0)
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 5);

					if (Human_FindConsumerNearWorkCenter(human, largest))
					{
						Write(human, NC2Layout::HumanOffset::RequestedGood, largest);
						Human_l_StartTask_ExecuteJob_Carrier_FlushStock(human);

						return;
					}
				}
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_0046dce0.
	void Human_l_StartTask_ExecuteJob_Carrier_FlushStock(void* human)
	{
		if (Human_IsAttachedToWorkHouse(human))
		{
			auto* house = WorkHouse(human);
			const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood);

			if (Human_Target_HasTarget(human) && Human_IsAtTargetPoint(human))
			{
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) != 5)
				{
					Start(human, 22);
				}
				else if (House_Stock_GetAmountOfGoods(house, good, true) > 0)
				{
					Start(human, PickupAnimation(house));
				}
				else
				{
					Human_Target_Reset(human);
				}

				return;
			}

			if (House_Stock_GetAmountOfGoods(house, good, true) > 0)
			{
				auto* work = static_cast<std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint;
				auto* real = static_cast<std::byte*>(house) + NC2Layout::HouseOffset::MapPosition;

				if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(work, NC2Layout::MapCoordinatePairOffset::Coordinates))
				{
					Human_Target_SetTargetPositions(human, real, work);
					TargetHouse(human, house);
					Start(human, PickupAnimation(house));
					return;
				}

				if (Human_l_StartAtomicWalk(human, real, work))
				{
					TargetHouse(human, house);

					return;
				}
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_004464a9; Windows outlined delivery predicate, name inferred from callers.
	bool Human_IsBringingCarriedGood(const void* human)
	{
		const auto mode = Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode);

		return mode == 4 || (mode == 5 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0);
	}

}
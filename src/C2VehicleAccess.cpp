#include "C2GameMapAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanInitialization.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2ObjectLayout.h"
#include "C2StaticVehicleTypeData.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "C2VehicleLifecycle.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_0044da5d; recovered from Game.exe's aligned instruction bytes.
	bool Vehicle_Attachment_IsInsideVehicle(const void* vehicle)
	{

		return NC2Lifetime::Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::InsideVehicleFlag) != 0;
	}

	// Original function: FUN_004413b3; remaster rejects a failed door search.
	bool Vehicle_Door_IsEntryPointOnLand(const void* vehicle, std::uint32_t requiredSpace)
	{
		std::uint32_t point = 0;

		if (!Vehicle_Door_GetEntryPoint(vehicle, &point, requiredSpace))
			return false;

		return MapMigPoint_IsInsideMap(&point) && GameMap_Continent_GetContinentTyp(NC2Lifetime::Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId)) == 1;
	}

	// Original function: FUN_00462ac1.
	std::uint32_t Vehicle_GetCurrentAtomicAnimationLengthInTicks(const void* vehicle)
	{

		return NC2Lifetime::Read<std::uint32_t>(gStaticAtomicAnimationData, NC2Lifetime::Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationId) * NC2Layout::RecordSize::AtomicAnimation + NC2Layout::AtomicAnimationOffset::LengthInTicks);
	}

	// Original function: FUN_0042ce12; copy the packed position returned by the shared accessor.
	bool Human_GetMapPosition(const void* human, void* point)
	{
		NC2Lifetime::Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, NC2Lifetime::Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates));

		return true;
	}

	using NC2Lifetime::Read;

	static const std::byte* AddressAt(const void* object, std::uint32_t offset)
	{

		return reinterpret_cast<const std::byte*>(reinterpret_cast<std::uintptr_t>(object) + offset);
	}

	// Original function: FUN_0044155a; the native path always reattaches the saved map position.
	void Vehicle_ChangePlayerId(void* vehicle, std::uint32_t player)
	{
		using NC2Lifetime::Write;
		void* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		const auto point = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
		MapMoveable_Attachment_DetachFromMap(moveable);
		Write(vehicle, NC2Layout::VehicleOffset::PlayerId, player);
		void* pathfinder = GetUnsafePathfinderPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId));
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId, -1);
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideId, -1);
		Write(pathfinder, NC2Layout::MapPathfinderOffset::PlayerId, player);
		MapMoveable_Attachment_AttachToMap(moveable, &point);
		const auto radius = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::ExplorationRadius);
		const auto owner = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId);
		GameMap_Exploration_DoExplore(owner, Vehicle_GetMapPositionRef(vehicle), radius);
	}

	// Original function: FUN_0044ba62.
	std::uint32_t Vehicle_Stock_GetTotalAmount(const void* vehicle)
	{
		const auto count = Vehicle_GetVehicleTypeDataPtr(vehicle)->goodTypeCount;
		std::uint32_t total = 0;

		for (std::uint32_t slot = 0; slot < count; ++slot)
		{
			total += Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::StockAmounts + slot);
		}

		return total;
	}

	// Original function: FUN_0044bb37; both stock-slot lookups are retained.
	std::uint32_t Vehicle_Stock_GetFutureAmount(const void* vehicle, std::uint32_t goodId)
	{
		if (Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, goodId) == -1)
			return 0;

		return Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::FutureStockAmounts + static_cast<std::uint32_t>(Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, goodId)));
	}

	// Original function: FUN_0044bb63.
	std::uint32_t Vehicle_Stock_GetTotalFutureAmount(const void* vehicle)
	{
		const auto count = Vehicle_GetVehicleTypeDataPtr(vehicle)->goodTypeCount;
		std::uint32_t total = 0;

		for (std::uint32_t slot = 0; slot < count; ++slot)
		{
			total += Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::FutureStockAmounts + slot);
		}

		return total;
	}

	// Original function: FUN_0044ba47.
	bool Vehicle_Stock_IsFull(const void* vehicle)
	{

		return Vehicle_Stock_GetTotalAmount(vehicle) >= static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots);
	}

	// Original function: FUN_0044bb8b.
	bool Vehicle_Stock_IsFullSoon(const void* vehicle)
	{

		return Vehicle_Stock_GetTotalFutureAmount(vehicle) >= static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots);
	}

	// Original function: FUN_0044bce5; complete 21-record stock reservation list.
	static bool VehicleStock_IsHumanInList(const void* stock, std::uint32_t revision)
	{
		for (std::uint32_t slot = 0; slot < 21; ++slot)
		{
			if (Read<std::uint8_t>(stock, NC2Layout::VehicleStockOffset::HumanReservations + slot * NC2Layout::RecordSize::VehicleStockReservation) != 0 && Read<std::uint32_t>(stock, NC2Layout::VehicleStockOffset::ReservationHumanRevisions + slot * NC2Layout::RecordSize::VehicleStockReservation) == revision)
				return true;
		}

		return false;
	}

	// Original function: FUN_0044bd6c; remaster retains partial reservations and clears exhausted records.
	static void VehicleStock_RemoveHumanFromList(void* stock, std::uint32_t revision, std::int32_t delta)
	{
		if (revision == NXBasics::IntegerRange::UInt32Maximum || delta == 0)
			return;

		for (std::uint32_t slot = 0; slot < 21; ++slot)
		{
			const auto offset = NC2Layout::VehicleStockOffset::HumanReservations + slot * NC2Layout::RecordSize::VehicleStockReservation;

			if (Read<std::uint8_t>(stock, offset + NC2Layout::VehicleStockReservationOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(stock, offset + NC2Layout::VehicleStockReservationOffset::HumanRevision) != revision)
				continue;

			const auto current = Read<std::int32_t>(stock, offset + NC2Layout::VehicleStockReservationOffset::RetainedAmount);

			if ((current > 0 && delta < 0) || (current < 0 && delta > 0))
				return;

			auto next = static_cast<std::int64_t>(current) - delta;

			if ((current >= 0 && next < 0) || (current <= 0 && next > 0))
				next = 0;

			NC2Lifetime::Write(stock, offset + NC2Layout::VehicleStockReservationOffset::RetainedAmount, static_cast<std::int32_t>(next));

			if (next == 0)
			{
				NC2Lifetime::Write<std::uint32_t>(stock, offset + NC2Layout::VehicleStockReservationOffset::HumanRevision, 0);
				NC2Lifetime::Write<std::uint8_t>(stock, offset + NC2Layout::VehicleStockReservationOffset::AllocatedFlag, 0);
			}

			return;
		}
	}

	// Original function: FUN_0044bd0d; remaster reports capacity failure before changing future stock.
	static bool VehicleStock_AddHumanToList(void* stock, std::uint32_t revision, std::int32_t delta)
	{
		if (revision == NXBasics::IntegerRange::UInt32Maximum || delta == 0)
			return true;

		const bool existing = VehicleStock_IsHumanInList(stock, revision);

		for (std::uint32_t slot = 0; slot < 21; ++slot)
		{
			const auto offset = NC2Layout::VehicleStockOffset::HumanReservations + slot * NC2Layout::RecordSize::VehicleStockReservation;
			const bool active = Read<std::uint8_t>(stock, offset + NC2Layout::VehicleStockReservationOffset::AllocatedFlag) != 0;

			if (existing ? (!active || Read<std::uint32_t>(stock, offset + NC2Layout::VehicleStockReservationOffset::HumanRevision) != revision) : active)
				continue;

			const auto current = existing ? Read<std::int32_t>(stock, offset + NC2Layout::VehicleStockReservationOffset::RetainedAmount) : 0;
			const auto next = static_cast<std::int64_t>(current) + delta;

			if (next < -2147483648LL || next > 2147483647LL)
				return false;

			NC2Lifetime::Write(stock, offset + NC2Layout::VehicleStockReservationOffset::RetainedAmount, static_cast<std::int32_t>(next));
			NC2Lifetime::Write(stock, offset + NC2Layout::VehicleStockReservationOffset::HumanRevision, next == 0 ? 0u : revision);
			NC2Lifetime::Write<std::uint8_t>(stock, offset + NC2Layout::VehicleStockReservationOffset::AllocatedFlag, next != 0 ? 1 : 0);
			return true;
		}

		return false;
	}

	// Original function: FUN_0044ba8a; remaster updates the reservation by the actual stock change.
	bool Vehicle_Stock_ModifyAmount(void* vehicle, std::uint32_t goodId, std::int32_t delta, std::uint32_t humanRevision)
	{
		const auto found = Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, goodId);

		if (found < 0)
			return false;

		const auto slot = static_cast<std::uint32_t>(found);
		const auto current = static_cast<std::int32_t>(Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::StockAmounts + slot));
		auto applied = delta;

		if (delta < 0)
		{
			if (static_cast<std::int64_t>(current) + delta < 0)
				return false;
		}
		else
		{
			const auto capacity = Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots;
			const auto available = static_cast<std::int64_t>(capacity) - Vehicle_Stock_GetTotalAmount(vehicle);

			if (available <= 0)
			{
				applied = 0;
			}
			else if (applied > available)
			{
				applied = static_cast<std::int32_t>(available);
			}

			if (applied > 255 - current)
				applied = 255 - current;
		}

		if (delta != 0 && applied == 0)
			return false;

		NC2Lifetime::Write(vehicle, NC2Layout::VehicleOffset::StockAmounts + slot, static_cast<std::uint8_t>(current + applied));

		if (!Vehicle_Passengers_IsCarrierAttached(vehicle))
			Vehicle_Stock_SetWantedAmount(vehicle, goodId, static_cast<std::uint32_t>(current + applied));

		VehicleStock_RemoveHumanFromList(static_cast<std::byte*>(vehicle) + NC2Layout::VehicleOffset::StockAmountsSigned, humanRevision, applied);

		return true;
	}

	// Original function: FUN_0044bba6; remaster keeps the ledger and byte-sized future stock consistent.
	bool Vehicle_Stock_ModifyFutureAmount(void* vehicle, std::uint32_t goodId, std::int32_t delta, std::uint32_t humanRevision)
	{
		const auto found = Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, goodId);

		if (found < 0)
			return false;

		const auto slot = static_cast<std::uint32_t>(found);
		const auto current = static_cast<std::int32_t>(Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::FutureStockAmounts + slot));
		auto applied = delta;

		if (delta < 0)
		{
			if (static_cast<std::int64_t>(current) + delta < 0)
				return false;
		}
		else
		{
			const auto capacity = Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots;
			const auto available = static_cast<std::int64_t>(capacity) - Vehicle_Stock_GetTotalFutureAmount(vehicle);

			if (available <= 0)
			{
				applied = 0;
			}
			else if (applied > available)
			{
				applied = static_cast<std::int32_t>(available);
			}

			if (applied > 255 - current)
				applied = 255 - current;
		}

		if (delta != 0 && applied == 0)
			return false;

		if (!VehicleStock_AddHumanToList(static_cast<std::byte*>(vehicle) + NC2Layout::VehicleOffset::StockAmountsSigned, humanRevision, applied))
			return false;

		NC2Lifetime::Write(vehicle, NC2Layout::VehicleOffset::FutureStockAmounts + slot, static_cast<std::uint8_t>(current + applied));

		return true;
	}

	// Original function: FUN_004412e3.
	void Vehicle_GetMapPosition(const void* vehicle, void* point)
	{
		NC2Lifetime::Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(Vehicle_GetMapPositionRef(vehicle), NC2Layout::MapCoordinatePairOffset::Coordinates));
	}

	// Original function: FUN_004378a1; each candidate advances the same packed cursor.
	static bool Vehicle_Door_ValidateEntryPoint(void* point, std::uint32_t requiredSpace)
	{
		if (!MapMigPoint_IsInsideMap(point))
			return false;

		const auto original = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		const auto* cell = GameMap_GetConstMapMicroPointPtr(point);
		const auto continent = Read<std::uint8_t>(cell, NC2Layout::MapMicroPointOffset::ContinentId);

		if ((Read<std::uint8_t>(cell, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 && requiredSpace <= (Read<std::uint8_t>(cell, NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u))
			return true;

		auto cursor = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		MapMigPoint_MoveCursorUnwrapped(cursor, 4);
		NC2Lifetime::Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, cursor);

		for (std::uint32_t candidate = 0; candidate < 6; ++candidate)
		{
			MapMigPoint_MoveCursorUnwrapped(cursor, 0);
			NC2Lifetime::Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, cursor);

			if (!MapMigPoint_IsInsideMap(point))
				continue;

			cell = GameMap_GetConstMapMicroPointPtr(point);

			if (Read<std::uint8_t>(cell, NC2Layout::MapMicroPointOffset::ContinentId) == continent && (Read<std::uint8_t>(cell, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 && requiredSpace <= (Read<std::uint8_t>(cell, NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u))
				return true;
		}

		NC2Lifetime::Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, original);

		return false;
	}

	// Original function: FUN_0043779d; retain the signed direction and distance calculations.
	bool Vehicle_Door_GetEntryPoint(const void* vehicle, void* point, std::uint32_t requiredSpace)
	{
		const auto kind = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

		if ((kind == 3 || kind == 4) && Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) != 0)
		{
			NC2Lifetime::Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DoorEntryPoint));
			return Vehicle_Door_ValidateEntryPoint(point, requiredSpace);
		}

		const auto* type = Vehicle_GetVehicleTypeDataPtr(vehicle);
		Vehicle_GetMapPosition(vehicle, point);
		const auto* moveable = AddressAt(gMapMoveableArrayStorage.entries, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable);
		auto direction = Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 ? Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::Direction) : 2;
		auto distance = Read<std::int32_t>(type, NC2Layout::StaticVehicleTypeOffset::PassengerVectorY);
		const auto offset = type->passengerVectorX;

		if (direction < 6)
		{
			direction = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(offset) + static_cast<std::uint32_t>(direction)) % 6;
		}
		else
		{
			bool halveDistance = false;

			if (direction == 6)
			{
				switch (offset)
				{
					case SwitchIds::VehicleDoorTurn::Straight:
						direction = 6;
						halveDistance = true;
						break;
					case SwitchIds::VehicleDoorTurn::ClockwiseStep:
						direction = 5;
						break;
					case SwitchIds::VehicleDoorTurn::ClockwiseTwoSteps:
						direction = 1;
						break;
					case SwitchIds::VehicleDoorTurn::Opposite:
						direction = 7;
						halveDistance = true;
						break;
					case SwitchIds::VehicleDoorTurn::CounterclockwiseTwoSteps:
						direction = 2;
						break;
					case SwitchIds::VehicleDoorTurn::CounterclockwiseStep:
						direction = 4;
						break;
				}
			}
			else
			{
				switch (offset)
				{
					case SwitchIds::VehicleDoorTurn::Straight:
						direction = 7;
						halveDistance = true;
						break;
					case SwitchIds::VehicleDoorTurn::ClockwiseStep:
						direction = 2;
						break;
					case SwitchIds::VehicleDoorTurn::ClockwiseTwoSteps:
						direction = 4;
						break;
					case SwitchIds::VehicleDoorTurn::Opposite:
						direction = 6;
						halveDistance = true;
						break;
					case SwitchIds::VehicleDoorTurn::CounterclockwiseTwoSteps:
						direction = 5;
						break;
					case SwitchIds::VehicleDoorTurn::CounterclockwiseStep:
						direction = 1;
						break;
				}
			}
			// Windows combines SAR by one with the signed IDIV remainder.

			if (halveDistance)
				distance = (distance >> 1) + distance % 2;
		}

		MapMigPoint_MoveCursorUnwrapped(point, static_cast<std::uint32_t>(direction), static_cast<std::uint32_t>(distance));

		return Vehicle_Door_ValidateEntryPoint(point, requiredSpace);
	}

	// Original function: FUN_0043f34a; twenty-two 20-byte command slots start at +10h.
	bool Vehicle_DoAddNewUserCommand(void* vehicle, const void* command)
	{
		using NC2Lifetime::Write;

		if (Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode) == 0 || Vehicle_l_UserCommands_TryToExecuteRightAway(vehicle, command))
			return true;

		if (Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode) == SwitchIds::UserCommand::VehicleUnload)
		{
			const auto type = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

			if ((type != 3 && type != 4) || Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) != 0)
			{
				for (std::uint32_t slot = 0; slot < 22; ++slot)
				{
					const auto offset = NC2Layout::VehicleOffset::UserCommandQueue + slot * NC2Layout::RecordSize::UserNetworkCommand;
					const auto opcode = Read<std::uint8_t>(vehicle, offset);

					if (opcode == SwitchIds::UserCommand::VehicleGotoAndWait || opcode == SwitchIds::UserCommand::VehicleDock)
						Write<std::uint8_t>(vehicle, offset, 0);
				}
			}
		}

		for (std::uint32_t slot = 0; slot < 22; ++slot)
		{
			const auto offset = NC2Layout::VehicleOffset::UserCommandQueue + slot * NC2Layout::RecordSize::UserNetworkCommand;

			if (Read<std::uint8_t>(vehicle, offset) != 0)
				continue;

			std::memcpy(static_cast<std::byte*>(vehicle) + offset, command, 20);
			return true;
		}

		return false;
	}

	// Original function: FUN_0043f3c4; commands handled here are never enqueued.
	bool Vehicle_l_UserCommands_TryToExecuteRightAway(void* vehicle, const void* command)
	{
		using NC2Lifetime::Write;

		switch (Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode))
		{
			case SwitchIds::UserCommand::VehicleGotoAndWait:
			{
				if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction) != 1)
					return false;

				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
				return point == Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::TargetWorkPosition);
			}
			case SwitchIds::UserCommand::SetVehicleWantedAmount:
			case SwitchIds::UserCommand::ResetVehicleWantedAmounts:
			{
				if (Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode) == SwitchIds::UserCommand::SetVehicleWantedAmount)
				{
					Vehicle_Stock_SetWantedAmount(vehicle, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
				}
				else
				{
					for (std::uint32_t good = 1; good < 66; ++good)
					{
						Vehicle_Stock_SetWantedAmount(vehicle, good, 0);
					}
				}

				Vehicle_l_Passengers_StopAllIdleTasks(vehicle);

				if (!Vehicle_Passengers_IsCarrierAttached(vehicle))
				{
					alignas(4) std::byte message[772];
					ToUserMessage_Construct(message);
					Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId));
					Write(message, NC2Layout::ToUserMessageOffset::VehicleId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
					Write(message, NC2Layout::ToUserMessageOffset::EquipmentItems, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType));
					Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 0x3A);
					ToUserMessage_FinishMessage(message);
					NC2LogicExternals::External_Misc_SendToUserMessage(message);
				}

				return true;
			}
			case SwitchIds::UserCommand::SetVehicleMilitaryMode:
				Write(vehicle, NC2Layout::VehicleOffset::MilitaryMode, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1));
				return true;
			case SwitchIds::UserCommand::VehicleMoveIntoCarrier:
				return Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CurrentTask) == 6;
			default:

				return false;
		}
	}

	// Original function: FUN_0044bc3a.
	std::uint32_t Vehicle_Stock_GetTotalWantedAmount(const void* vehicle)
	{
		const auto count = Vehicle_GetVehicleTypeDataPtr(vehicle)->goodTypeCount;
		std::uint32_t total = 0;

		for (std::uint32_t slot = 0; slot < count; ++slot)
		{
			total += Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::WantedStockAmounts + slot);
		}

		return total;
	}

	// Original function: FUN_0044bcb9; Windows performs the slot lookup twice on success.
	std::uint32_t Vehicle_Stock_GetAmountOfWantedGoods(const void* vehicle, std::uint32_t goodId)
	{
		if (Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, goodId) == -1)
			return 0;

		return Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::WantedStockAmounts + static_cast<std::uint32_t>(Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, goodId)));
	}

	// Original function: FUN_0044bc62; unsigned capacity arithmetic precedes byte storage.
	bool Vehicle_Stock_SetWantedAmount(void* vehicle, std::uint32_t good, std::uint32_t wanted)
	{
		if (Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, good) == -1)
			return false;

		const auto* type = Vehicle_GetVehicleTypeDataPtr(vehicle);
		const auto slot = static_cast<std::uint32_t>(Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, good));
		const auto total = Vehicle_Stock_GetTotalWantedAmount(vehicle);
		auto available = static_cast<std::uint32_t>(type->stockSlots) + Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::WantedStockAmounts + slot) - total;

		if (wanted <= available)
			available = wanted;

		NC2Lifetime::Write(vehicle, NC2Layout::VehicleOffset::WantedStockAmounts + slot, static_cast<std::uint8_t>(available));

		return true;
	}

	// Original function: FUN_0043f563; the three flag commands do not count as priorities.
	bool Vehicle_IsPriorityCommandInQueue(const void* vehicle)
	{
		for (std::uint32_t slot = 0; slot < 22; ++slot)
		{
			const auto type = Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::UserCommandQueue + slot * NC2Layout::RecordSize::UserNetworkCommand);

			if (type != 0 && type != 109 && type != 110 && type != 111)
				return true;
		}

		return false;
	}

	// Original function: FUN_0044c02c; entering retains the passenger reference.
	bool Vehicle_Passengers_MoveIn_h(void* vehicle, std::uint32_t humanId)
	{
		const auto slot = Vehicle_Passengers_GetAttachedHumanSlot(vehicle, humanId);

		if (slot == -1)
			return false;

		std::uint32_t entry = 0;

		if (!Vehicle_Door_GetEntryPoint(vehicle, &entry, 0))
			return false;

		if (Read<std::uint32_t>(Human_GetMapPositionRef(CHumanArray::mStaticVars + humanId * CHumanArray::RuntimeElementSize), NC2Layout::MapCoordinatePairOffset::Coordinates) != entry)
			return false;

		NC2Lifetime::Write<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::HumanPassengerInsideFlags + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::VehiclePassenger, 1);

		return true;
	}

	// Original function: FUN_0044c104.
	bool Vehicle_Passengers_IsCommander(const void* vehicle, std::uint32_t humanId)
	{

		return Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0 && static_cast<std::uint32_t>(Vehicle_Passengers_GetAttachedCommanderId(vehicle)) == humanId;
	}

	// Original function: FUN_00441377.
	void Vehicle_GetTargetWorkPosition(const void* vehicle, void* point)
	{
		NC2Lifetime::Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Vehicle_Passengers_GetAttachedCommanderPtr(vehicle) != nullptr ? Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::TargetWorkPosition) : NXBasics::IntegerRange::UInt32Maximum);
	}

	// Original function: FUN_0044c095; moving outside retains the passenger reference.
	bool Vehicle_Passengers_MoveOut_h(void* vehicle, std::uint32_t humanId)
	{
		if (Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::Hitpoints) > 0 && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction) == 1)
			return false;

		const auto slot = Vehicle_Passengers_GetAttachedHumanSlot(vehicle, humanId);

		if (slot == -1)
			return false;

		const auto offset = NC2Layout::VehicleOffset::HumanPassengerInsideFlags + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::VehiclePassenger;

		if (Read<std::uint8_t>(vehicle, offset) != 0)
		{
			std::uint32_t point = 0;

			if (!Vehicle_Door_GetEntryPoint(vehicle, &point, 0))
				return false;

			Human_VehicleMisc_Leave(CHumanArray::mStaticVars + humanId * CHumanArray::RuntimeElementSize, &point);
			NC2Lifetime::Write<std::uint8_t>(vehicle, offset, 0);
		}

		return true;
	}

	// Original function: FUN_0044c1d2; invalid references produce a null pointer.
	void* Vehicle_Passengers_GetAttachedHumanPtr(const void* vehicle, std::uint32_t slot)
	{
		const auto* reference = AddressAt(vehicle, NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger);

		if (!HumanReference_IsAllocated(reference))
			return nullptr;

		return CHumanArray::mStaticVars + Read<std::uint32_t>(reference, NC2Layout::VehiclePassengerOffset::ObjectId) * CHumanArray::RuntimeElementSize;
	}

	// Original function: FUN_0044c282; the commander reference is not validated.
	void* Vehicle_Passengers_GetAttachedCommanderPtr(const void* vehicle)
	{
		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) == 0)
			return nullptr;

		const auto slot = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots);

		return CHumanArray::mStaticVars + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger) * CHumanArray::RuntimeElementSize;
	}

	// Original function: FUN_0044c94e; snapshot passenger count, commander before passengers.
	void Vehicle_l_Passengers_StopAllIdleTasks(void* vehicle)
	{
		const auto count = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots);

		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0)
		{
			auto* human = Vehicle_Passengers_GetAttachedCommanderPtr(vehicle);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 1)
				Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::RestartJobExecution, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
		}

		for (std::uint32_t slot = 0; slot < count; ++slot)
		{
			if (!HumanReference_IsAllocated(AddressAt(vehicle, NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger)))
				continue;

			auto* human = Vehicle_Passengers_GetAttachedHumanPtr(vehicle, slot);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 1)
				Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::ChangeJob, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), 0);
		}
	}

	// Original function: FUN_0044ce38; the inclusive limit also examines the commander slot.
	bool Vehicle_Passengers_IsCarrierAttached(const void* vehicle)
	{
		Vehicle_GetVehicleTypeDataPtr(vehicle);
		std::uint32_t slot = 0;
		do
		{
			const auto* reference = AddressAt(vehicle, NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger);

			if (HumanReference_IsAllocated(reference))
			{
				const auto* human = CHumanArray::mStaticVars + Read<std::uint32_t>(reference, NC2Layout::VehiclePassengerOffset::ObjectId) * CHumanArray::RuntimeElementSize;

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 24)
					return true;
			}

			++slot;
		} while (slot <= static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots));

		return false;
	}

	// Original function: FUN_0042ce24; CHuman::GetMapPositionRef.
	const std::uint16_t* Human_GetMapPositionRef(const void* human)
	{
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			const void* carrier = AddressAt(CVehicleArray::mStaticVars, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::RuntimeElementSize);

			if (Read<std::uint8_t>(carrier, NC2Layout::VehicleOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(carrier, NC2Layout::VehicleOffset::UniqueId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision))
			{
				const auto slot = static_cast<std::uint32_t>(Vehicle_Passengers_GetAttachedHumanSlot(carrier, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)));
				// The original has no missing-slot guard here.

				if (Read<std::uint8_t>(carrier, NC2Layout::VehicleOffset::HumanPassengerInsideFlags + slot * NC2Layout::RecordSize::VehiclePassenger) != 0)
				{
					const auto state = Read<std::uint32_t>(carrier, NC2Layout::VehicleOffset::LogicType);

					if ((state == 3 || state == 4) && Read<std::uint8_t>(carrier, NC2Layout::VehicleOffset::DockedFlag) != 0)
						return static_cast<const std::uint16_t*>(Vehicle_GetConstMapMicroPointPtr(carrier));

					return static_cast<const std::uint16_t*>(Vehicle_GetMapPositionRef(carrier));
				}
			}
		}

		return reinterpret_cast<const std::uint16_t*>(AddressAt(gMapMoveableArrayStorage.entries, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable + NC2Layout::MapMoveableOffset::MapPositionUnsigned));
	}

	// Original function: FUN_004375a6.
	const SStaticVehicleTypeData* Vehicle_GetVehicleTypeDataPtr(const void* vehicle)
	{

		return reinterpret_cast<const SStaticVehicleTypeData*>(AddressAt(gStaticVehicleTypeData, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType) * NC2Layout::RecordSize::StaticVehicleType));
	}

	// Original function: FUN_0044ce87; include the commander slot, require job 25.
	std::int32_t Vehicle_Passengers_GetWorkTraderHumanId(const void* vehicle)
	{
		const auto type = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

		if (type == 3 || type == 4)
			return -1;

		Vehicle_GetVehicleTypeDataPtr(vehicle);

		for (std::uint32_t slot = 0;; ++slot)
		{
			const void* target = static_cast<const std::byte*>(vehicle) + NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger;

			if (InteractionTargetData_IsValid(target) && Read<std::uint32_t>(CHumanArray::mStaticVars + Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type) * CHumanArray::RuntimeElementSize, NC2Layout::HumanOffset::JobType) == 25)
				return Read<std::int32_t>(target, NC2Layout::TargetDataOffset::Type);

			if (slot + 1u > static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots))
			{

				return -1;
			}
		}
	}

	// Original function: FUN_00437912.
	const void* Vehicle_GetConstMapMicroPointPtr(const void* vehicle)
	{

		return AddressAt(vehicle, NC2Layout::VehicleOffset::DoorEntryPoint);
	}

	// Original function: FUN_004412f3.
	const void* Vehicle_GetMapPositionRef(const void* vehicle)
	{
		// 0044da5d: MOV AL,[ECX+548]; RET, confirmed from the executable.

		while (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::InsideVehicleFlag) != 0)
		{
			const void* carrier = AddressAt(CVehicleArray::mStaticVars, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CarrierVehicleId) * CVehicleArray::RuntimeElementSize);

			if (Read<std::uint8_t>(carrier, NC2Layout::VehicleOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(carrier, NC2Layout::VehicleOffset::UniqueId) != Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CarrierVehicleRevision))
				break;

			const auto slot = static_cast<std::uint32_t>(Vehicle_Passengers_GetAttachedVehicleSlot(carrier, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId)));

			if (Read<std::uint8_t>(carrier, NC2Layout::VehicleOffset::VehiclePassengerInsideFlags + slot * NC2Layout::RecordSize::VehiclePassenger) == 0)
				break;

			const auto state = Read<std::uint32_t>(carrier, NC2Layout::VehicleOffset::LogicType);

			if ((state == 3 || state == 4) && Read<std::uint8_t>(carrier, NC2Layout::VehicleOffset::DockedFlag) != 0)
				return Vehicle_GetConstMapMicroPointPtr(carrier);

			vehicle = carrier;
		}

		return AddressAt(gMapMoveableArrayStorage.entries, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable + NC2Layout::MapMoveableOffset::MapPositionUnsigned);
	}

	// Original function: FUN_0044ba1b.
	std::uint32_t Vehicle_Stock_GetAmountOfGoods(const void* vehicle, std::uint32_t goodId)
	{
		if (Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, goodId) == -1)
			return 0;

		return Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::StockAmounts + static_cast<std::uint32_t>(Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, goodId)));
	}

	// Original function: FUN_0044bda9.
	bool Vehicle_Stock_CanStoreGoodType(const void* vehicle, std::uint32_t goodId)
	{

		return Vehicle_l_Stock_GetStockSlotIdForGoodType(vehicle, goodId) != -1;
	}

	// Original function: FUN_0044bdbb.
	std::int32_t Vehicle_l_Stock_GetStockSlotIdForGoodType(const void* vehicle, std::uint32_t goodId)
	{
		const auto* type = Vehicle_GetVehicleTypeDataPtr(vehicle);
		const auto slot = type->goodTypeIndices[goodId];

		if (slot != -1)
			return slot;

		if (goodId == 18 || goodId == 19 || goodId == 21 || goodId == 22)
			return type->goodTypeIndices[16];

		if (goodId == 20)
			return type->goodTypeIndices[17];

		return -1;
	}

	// Original function: FUN_0044bfd2.
	std::int32_t Vehicle_Passengers_GetAttachedHumanSlot(const void* vehicle, std::uint32_t humanId)
	{
		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0 && static_cast<std::uint32_t>(Vehicle_Passengers_GetAttachedCommanderId(vehicle)) == humanId)
			return Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots;

		const auto count = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::HumanPassengers + index * NC2Layout::RecordSize::VehiclePassenger) == humanId)
				return NXBasics::Runtime_TimeSigned32(index);
		}

		return -1;
	}

	// Original function: FUN_0044c2b5.
	std::int32_t Vehicle_Passengers_GetAttachedCommanderId(const void* vehicle)
	{
		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) == 0)
			return -1;

		return Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::HumanPassengers + static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots) * NC2Layout::RecordSize::VehiclePassenger);
	}

	// Original function: FUN_0044cb4b.
	std::int32_t Vehicle_Passengers_GetAttachedVehicleSlot(const void* vehicle, std::uint32_t vehicleId)
	{
		const auto count = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->vehicleSlots);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehiclePassengers + index * NC2Layout::RecordSize::VehiclePassenger) == vehicleId)
				return NXBasics::Runtime_TimeSigned32(index);
		}

		return -1;
	}

	// Original function: FUN_0044bdfb.
	bool Vehicle_Passengers_AttachHuman(void* vehicle, std::uint32_t humanId)
	{
		using namespace NC2Lifetime;

		if (!Vehicle_Passengers_CanHumanBeAttached(vehicle, humanId))
			return false;

		const void* human = GetHumanPtr(static_cast<std::int32_t>(humanId));
		const auto capacity = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots);
		std::uint32_t slot;

		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) == 0 && Vehicle_Passengers_CanBeCommander(vehicle, human))
		{
			Write<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag, 1);
			slot = capacity;
		}
		else
		{
			for (slot = 0; slot < capacity; ++slot)
			{
				if (!HumanReference_IsAllocated(static_cast<const std::byte*>(vehicle) + NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger))
					break;
			}

			if (slot == capacity)
				return false;
		}

		const auto offset = NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger;
		Write<std::uint32_t>(vehicle, offset, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write<std::uint32_t>(vehicle, offset + NC2Layout::VehiclePassengerOffset::ObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
		Write<std::uint8_t>(vehicle, offset + NC2Layout::VehiclePassengerOffset::InsideFlag, 0);

		return true;
	}

	// Original function: FUN_0044bf65.
	bool Vehicle_Passengers_CanHumanBeAttached(const void* vehicle, std::uint32_t humanId)
	{
		using namespace NC2Lifetime;
		const void* human = GetHumanPtr(static_cast<std::int32_t>(humanId));

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) != Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId))
			return false;

		if (Vehicle_GetVehicleTypeDataPtr(vehicle)->allowedPassengerTypes[Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType)] == 0)
			return false;

		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) == 0 && Vehicle_Passengers_CanBeCommander(vehicle, human))
			return true;

		const auto capacity = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots);

		return capacity > Vehicle_Passengers_GetNumberOfAttachedHumans(vehicle, false);
	}

	// Original function: FUN_0044c122; Windows checks the passenger job mask here as well.
	bool Vehicle_Passengers_CanBeCommander(const void* vehicle, const void* human)
	{
		if (Vehicle_GetVehicleTypeDataPtr(vehicle)->allowedPassengerTypes[NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType)] == 0)
			return false;

		return NC2Lifetime::Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) == 0;
	}

	// Original function: FUN_0044c237; the commander flag is counted without a revision check.
	std::uint32_t Vehicle_Passengers_GetNumberOfAttachedHumans(const void* vehicle, bool includeCommander)
	{
		std::uint32_t count = includeCommander && NC2Lifetime::Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0 ? 1u : 0u;
		const auto capacity = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots);

		for (std::uint32_t slot = 0; slot < capacity; ++slot)
		{
			if (HumanReference_IsAllocated(static_cast<const std::byte*>(vehicle) + NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger))
				++count;
		}

		return count;
	}

	// Original function: FUN_0044bea0.
	bool Vehicle_Passengers_DetachHuman(void* vehicle, std::uint32_t humanId)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0 && static_cast<std::uint32_t>(Vehicle_Passengers_GetAttachedCommanderId(vehicle)) == humanId)
		{
			Write<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag, 0);
			const auto offset = NC2Layout::VehicleOffset::HumanPassengers + static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots) * NC2Layout::RecordSize::VehiclePassenger;
			// PE opcode 83 sign-extends its FF immediate to 0xFFFFFFFF.
			Write<std::int32_t>(vehicle, offset, -1);
			Write<std::uint8_t>(vehicle, offset + NC2Layout::VehiclePassengerOffset::InsideFlag, 0);
			Human_DetachFromVehicle(CHumanArray::mStaticVars + humanId * CHumanArray::RuntimeElementSize);
			Vehicle_Passengers_ScanForAttachedCommander(vehicle);
			return true;
		}

		const auto capacity = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots);

		for (std::uint32_t slot = 0; slot < capacity; ++slot)
		{
			const auto offset = NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger;

			if (Read<std::uint32_t>(vehicle, offset) != humanId)
				continue;

			Write<std::int32_t>(vehicle, offset, -1);
			Write<std::uint8_t>(vehicle, offset + NC2Layout::VehiclePassengerOffset::InsideFlag, 0);
			Human_DetachFromVehicle(CHumanArray::mStaticVars + humanId * CHumanArray::RuntimeElementSize);
			return true;
		}

		return false;
	}

	// Original function: FUN_0044c150.
	void Vehicle_Passengers_ScanForAttachedCommander(void* vehicle)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0)
			return;

		const auto capacity = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots);

		for (std::uint32_t slot = 0; slot < capacity; ++slot)
		{
			auto* entry = static_cast<std::byte*>(vehicle) + NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger;

			if (!HumanReference_IsAllocated(entry))
				continue;

			if (!Vehicle_Passengers_CanBeCommander(vehicle, CHumanArray::mStaticVars + Read<std::uint32_t>(entry, NC2Layout::VehiclePassengerOffset::ObjectId) * CHumanArray::RuntimeElementSize))
				continue;

			Write<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag, 1);
			std::memcpy(static_cast<std::byte*>(vehicle) + NC2Layout::VehicleOffset::HumanPassengers + capacity * NC2Layout::RecordSize::VehiclePassenger, entry, 12);
			Write<std::int32_t>(entry, NC2Layout::ActorIdentityOffset::Allocated, -1);
			Write<std::uint8_t>(entry, NC2Layout::ActorIdentityOffset::UniqueId, 0);

			return;
		}
	}

	// Original function: FUN_0044cab9.
	bool Vehicle_Passengers_CanVehicleBeAttached(const void* vehicle, std::uint32_t vehicleId)
	{
		const auto* passenger = CVehicleArray::mStaticVars + vehicleId * CVehicleArray::RuntimeElementSize;

		if (NC2Lifetime::Read<std::uint32_t>(passenger, NC2Layout::VehicleOffset::PlayerId) != NC2Lifetime::Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId))
			return false;

		const auto type = NC2Lifetime::Read<std::uint32_t>(passenger, NC2Layout::VehicleOffset::JobType);

		if (Vehicle_GetVehicleTypeDataPtr(vehicle)->allowedPassengerTypes[type] == 0)
			return false;

		const auto capacity = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->vehicleSlots);

		return capacity > Vehicle_Passengers_GetNumberOfAttachedVehicles(vehicle);
	}

	// Original function: FUN_0044cb15.
	std::uint32_t Vehicle_Passengers_GetNumberOfAttachedVehicles(const void* vehicle)
	{
		std::uint32_t count = 0;
		const auto capacity = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->vehicleSlots);

		for (std::uint32_t i = 0; i < capacity; ++i)
		{
			if (VehicleReference_IsAllocated(static_cast<const std::byte*>(vehicle) + NC2Layout::VehicleOffset::VehiclePassengers + i * NC2Layout::RecordSize::VehiclePassenger))
				++count;
		}

		return count;
	}

}
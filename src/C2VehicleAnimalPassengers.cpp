#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2ObjectLayout.h"
#include "C2StaticVehicleTypeData.h"
#include "C2VehicleAccess.h"
#include "C2VehicleAnimalPassengers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_0044c3dd.
	bool Vehicle_Passengers_IsDraggingAnimal(const void* vehicle, std::uint32_t animalId, std::uint32_t revision)
	{

		return Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalId) == animalId && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalRevision) == revision;
	}

	// Original function: FUN_0044c3ff; this tests the allocated animal waiting outside.
	bool Vehicle_Passengers_IsHavingDraggingAnimal(const void* vehicle)
	{
		if (Vehicle_GetVehicleTypeDataPtr(vehicle)->draggingAnimalTribe == 0 || Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalInsideFlag) != 0 || Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalId) == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const auto* animal = CAnimalArray::mStaticVars + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalId) * CAnimalArray::ElementSize;

		return Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::UniqueId) == Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalRevision) && Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) == Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId);
	}

	// Original function: FUN_0044c453; the first two eligible animals are deliberately reserved.
	// The native return value remains false even when an animal was attached successfully.
	bool Vehicle_Passengers_TryToGetDraggingAnimal(void* vehicle)
	{
		auto id = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalId);

		if (id != NXBasics::IntegerRange::UInt32Maximum)
		{
			const auto* animal = CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize;

			if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::UniqueId) != Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalRevision))
			{
				Write<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalId, NXBasics::IntegerRange::UInt32Maximum);
				id = NXBasics::IntegerRange::UInt32Maximum;
			}
		}

		if (id != NXBasics::IntegerRange::UInt32Maximum)
		{
			if (!Animal_AttachToVehicle(CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize, Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId)))
				Write<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalId, NXBasics::IntegerRange::UInt32Maximum);

			return false;
		}

		std::uint32_t point = 0;
		Vehicle_GetMapPosition(vehicle, &point);
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId);
		const auto* data = Vehicle_GetVehicleTypeDataPtr(vehicle);
		std::uint32_t best = 10000, count = 0;
		void* candidate = nullptr;
		CAnimalIterator iterator;

		for (auto animalId = iterator.GetFirstAnimalId(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId)); animalId != -1; animalId = iterator.GetNextAnimalId())
		{
			auto* animal = CAnimalArray::mStaticVars + static_cast<std::uint32_t>(animalId) * CAnimalArray::ElementSize;

			if ((Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MiscFlags) & 8u) != 0 || Read<std::int32_t>(animal, NC2Layout::AnimalOffset::TribeType) != data->draggingAnimalTribe || Animal_IsAttachedToVehicle(animal))
				continue;

			const auto* position = Animal_GetMapPositionRef(animal);

			if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId) != continent || ++count <= 2)
				continue;

			const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y)));

			if (distance >= best)
				continue;

			best = distance;
			candidate = animal;
		}

		if (candidate != nullptr && Animal_AttachToVehicle(candidate, Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId)))
		{
			Write(vehicle, NC2Layout::VehicleOffset::DraggingAnimalId, Read<std::uint32_t>(candidate, NC2Layout::ActorIdentityOffset::Id));
			Write(vehicle, NC2Layout::VehicleOffset::DraggingAnimalRevision, Read<std::uint32_t>(candidate, NC2Layout::ActorIdentityOffset::UniqueId));
		}

		return false;
	}

	// Original function: FUN_0044c5ce; the map object is marked for removal before vehicle transformation.
	bool Vehicle_Passengers_AnimalMoveIn(void* vehicle, std::uint32_t animalId)
	{
		if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalId) != animalId)
			return false;

		const auto* animal = CAnimalArray::mStaticVars + animalId * CAnimalArray::ElementSize;
		Write<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DraggingAnimalInsideFlag, 1);
		Write<std::uint8_t>(gMapMoveableArrayStorage.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable, NC2Layout::MapMoveableOffset::RemovalRequestedFlag, 1);
		const auto type = Vehicle_GetVehicleTypeDataPtr(vehicle)->transformVehicleType;
		Write(vehicle, NC2Layout::VehicleOffset::LogicType, type);
		Write<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType, type >= 1 && type <= 6 ? static_cast<std::uint32_t>(type) + 49u : 0u);
		const auto* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		auto* pathfinder = GetUnsafePathfinderPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId));
		Write(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance, Vehicle_GetVehicleTypeDataPtr(vehicle)->logicSize);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(8, true, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId), 1, 0);

		return true;
	}
}
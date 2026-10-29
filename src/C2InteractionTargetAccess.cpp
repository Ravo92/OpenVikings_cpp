#include "C2AnimalAccess.h"
#include "C2GameMapAccess.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		const void* TargetObject(std::uint32_t type, std::uint32_t id)
		{
			switch (type)
			{
				case SwitchIds::TargetType::Human:
					return CHumanArray::mStaticVars + id * CHumanArray::ElementSize;
				case SwitchIds::TargetType::Vehicle:
					return CVehicleArray::mStaticVars + id * CVehicleArray::ElementSize;
				case SwitchIds::TargetType::Animal:
					return CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize;
				case SwitchIds::TargetType::House:
					return CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
				default:

					return nullptr;
			}
		}

		bool HiddenHumanIsInAttachedHouse(const void* human)
		{

			return (Human_IsAttachedToWorkHouse(human) && Human_IsInWorkHouse(human)) || (Human_IsAttachedToHomeHouse(human) && Human_IsInHomeHouse(human));
		}
	}

	// Original function: FUN_0046184b; STargetData is distinct from SInteractionTargetData.
	bool TargetData_IsValid(const void* target)
	{
		const auto type = TargetData_GetTargetType(target);

		if (type == SwitchIds::TargetType::MapPosition)
			return !MapMigPoint_IsBorderPoint(static_cast<const std::byte*>(target) + NC2Layout::TargetDataOffset::ObjectIdOrMapPosition);

		if (type == SwitchIds::TargetType::Landscape)
		{
			const auto* point = GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(target) + NC2Layout::TargetDataOffset::ObjectIdOrMapPosition);
			return Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ObjectData) != 0;
		}

		if (type < SwitchIds::TargetType::Human || type > SwitchIds::TargetType::House)
			return false;

		const auto id = TargetData_GetObjectId(target);

		if (type == SwitchIds::TargetType::Human && id == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const auto* object = TargetObject(type, id);

		return Read<std::uint8_t>(object, NC2Layout::ActorIdentityOffset::Allocated) != 0 && Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::UniqueId) == TargetData_GetUniqueId(target) && (type != SwitchIds::TargetType::House || Read<std::uint8_t>(object, NC2Layout::HouseOffset::DemolitionFlag) == 0);
	}

	// Original function: FUN_00461970; raw-object validity precedes container redirection.
	bool TargetData_l_IsValid(const void* target)
	{
		const auto type = Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type);

		if (type == SwitchIds::TargetType::MapPosition)
			return !MapMigPoint_IsBorderPoint(static_cast<const std::byte*>(target) + NC2Layout::TargetDataOffset::ObjectIdOrMapPosition);

		if (type == SwitchIds::TargetType::Landscape)
		{
			const auto* point = GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(target) + NC2Layout::TargetDataOffset::ObjectIdOrMapPosition);
			return Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ObjectData) != 0;
		}

		if (type < SwitchIds::TargetType::Human || type > SwitchIds::TargetType::House)
			return false;

		const auto id = Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition);

		if (type == SwitchIds::TargetType::Human && id == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const auto* object = TargetObject(type, id);

		return Read<std::uint8_t>(object, NC2Layout::ActorIdentityOffset::Allocated) != 0 && Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::UniqueId) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision) && (type != SwitchIds::TargetType::House || Read<std::uint8_t>(object, NC2Layout::HouseOffset::DemolitionFlag) == 0);
	}

	// Original function: FUN_00461a5e; point targets copy microgrid coordinates unchanged.
	void TargetData_GetMapPosition(const void* target, void* point)
	{
		const auto type = TargetData_GetTargetType(target);

		if (type == SwitchIds::TargetType::MapPosition || type == SwitchIds::TargetType::Landscape)
		{
			Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition));
			return;
		}

		if (type < SwitchIds::TargetType::Human || type > SwitchIds::TargetType::House)
			return;

		const auto* object = TargetObject(type, TargetData_GetObjectId(target));

		if (type == SwitchIds::TargetType::Animal)
		{
			Animal_GetMapPosition(object, point);

			return;
		}

		const void* position = type == SwitchIds::TargetType::Human ? Human_GetMapPositionRef(object) : type == SwitchIds::TargetType::Vehicle ? Vehicle_GetMapPositionRef(object)
		                                                                                                                                       : static_cast<const std::byte*>(object) + NC2Layout::HouseOffset::MapPosition;
		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates));
	}

	// Original function: FUN_00461b27; landscape ownership is a signed byte.
	std::int32_t TargetData_GetPlayerId(const void* target)
	{
		const auto type = TargetData_GetTargetType(target);

		if (type == SwitchIds::TargetType::Landscape)
		{
			const auto* point = GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(target) + NC2Layout::TargetDataOffset::ObjectIdOrMapPosition);
			return Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::int8_t>(point, NC2Layout::MapMicroPointOffset::LandscapePlayerId) : -1;
		}

		if (type < SwitchIds::TargetType::Human || type > SwitchIds::TargetType::House)
			return -1;

		return Read<std::int32_t>(TargetObject(type, TargetData_GetObjectId(target)), type == SwitchIds::TargetType::House ? NC2Layout::HouseOffset::PlayerIdUnsigned : type == SwitchIds::TargetType::Vehicle ? NC2Layout::VehicleOffset::PlayerIdUnsigned : NC2Layout::ActorPlayerOffset::HumanOrAnimalPlayerIdUnsigned);
	}

	// Original function: FUN_00461bc6; hidden humans expose their vehicle or house as target.
	std::uint32_t TargetData_GetTargetType(const void* target)
	{
		if (Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type) == SwitchIds::TargetType::Human && TargetData_l_IsValid(target))
		{
			const auto* human = TargetObject(SwitchIds::TargetType::Human, Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition));

			if (Human_Misc_IsHidden(human))
			{
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
					return SwitchIds::TargetType::Vehicle;

				if (HiddenHumanIsInAttachedHouse(human))
					return SwitchIds::TargetType::House;
			}
		}

		return Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type);
	}

	// Original function: FUN_00461c35; transported vehicles redirect their object ID.
	std::uint32_t TargetData_GetObjectId(const void* target)
	{
		const auto type = Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type);

		if (type == SwitchIds::TargetType::Human && TargetData_l_IsValid(target))
		{
			const auto* human = TargetObject(SwitchIds::TargetType::Human, Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition));

			if (Human_Misc_IsHidden(human))
			{
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
					return Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId);

				if (HiddenHumanIsInAttachedHouse(human))
				{
					const auto* point = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));
					return Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ObjectType) == 2 ? Read<std::uint16_t>(point, NC2Layout::MapMicroPointOffset::HouseId) : NXBasics::IntegerRange::UInt32Maximum;
				}
			}
		}
		else if (type == SwitchIds::TargetType::Vehicle && TargetData_l_IsValid(target))
		{
			const auto* vehicle = TargetObject(SwitchIds::TargetType::Vehicle, Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition));

			if (Vehicle_Attachment_IsInsideVehicle(vehicle))
				return Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CarrierVehicleId);
		}

		return Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition);
	}

	// Original function: FUN_00461cfc; container revisions accompany the redirected ID.
	std::uint32_t TargetData_GetUniqueId(const void* target)
	{
		const auto type = Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type);

		if (type == SwitchIds::TargetType::Human && TargetData_l_IsValid(target))
		{
			const auto* human = TargetObject(SwitchIds::TargetType::Human, Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition));

			if (Human_Misc_IsHidden(human))
			{
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
					return Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision);

				if (HiddenHumanIsInAttachedHouse(human))
				{
					const auto* point = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));
					const auto id = Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ObjectType) == 2 ? Read<std::uint16_t>(point, NC2Layout::MapMicroPointOffset::HouseId) : NXBasics::IntegerRange::UInt32Maximum;
					return Read<std::uint32_t>(TargetObject(SwitchIds::TargetType::House, id), NC2Layout::HouseOffset::UniqueId);
				}
			}
		}
		else if (type == SwitchIds::TargetType::Vehicle && TargetData_l_IsValid(target))
		{
			const auto* vehicle = TargetObject(SwitchIds::TargetType::Vehicle, Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition));

			if (Vehicle_Attachment_IsInsideVehicle(vehicle))
				return Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CarrierVehicleRevision);
		}

		return Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision);
	}

	// Original function: FUN_0045ccff; complete nine-entry table at 0045ce70..0045ce93.
	bool InteractionTargetData_IsValid(const void* target)
	{
		using namespace NC2Lifetime;
		const auto* bytes = static_cast<const std::byte*>(target);

		switch (Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::Type))
		{
			case SwitchIds::InteractionTargetType::Position:
				return MapMigPoint_IsInsideMap(bytes + NC2Layout::InteractionTargetDataOffset::RealPosition) && MapMigPoint_IsInsideMap(bytes + NC2Layout::InteractionTargetDataOffset::WorkPosition);
			case SwitchIds::InteractionTargetType::Landscape:
			{
				if (!MapMigPoint_IsInsideMap(bytes + NC2Layout::InteractionTargetDataOffset::RealPosition) || !MapMigPoint_IsInsideMap(bytes + NC2Layout::InteractionTargetDataOffset::WorkPosition))
					return false;

				const auto* cell = GameMap_GetConstMapMicroPointPtr(bytes + NC2Layout::InteractionTargetDataOffset::RealPosition);
				const auto pattern = Read<std::uint8_t>(cell, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? static_cast<std::uint32_t>(Read<std::uint8_t>(cell, NC2Layout::MapMicroPointOffset::ObjectData)) : 0u;
				return pattern == Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::LandscapeType);
			}
			case SwitchIds::InteractionTargetType::RoadBuildPlace:
			case SwitchIds::InteractionTargetType::WallBuildPlace:
				return true;
			case SwitchIds::InteractionTargetType::House:
			{
				const auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::ObjectId) * CHouseArray::ElementSize;
				return Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::ObjectRevision) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) && Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0;
			}
			case SwitchIds::InteractionTargetType::Vehicle:
			{
				const auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::ObjectId) * CVehicleArray::ElementSize;

				if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::ObjectRevision) != Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId))
					return false;

				std::uint32_t entry = 0;

				if (!Vehicle_Door_GetEntryPoint(vehicle, &entry, 0))
					return false;

				if (Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::WorkPosition) != entry)
					return false;

				return Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::RealPosition) == Read<std::uint32_t>(Vehicle_GetMapPositionRef(vehicle), NC2Layout::MapCoordinatePairOffset::Coordinates);
			}
			case SwitchIds::InteractionTargetType::Human:
			{
				const auto* human = CHumanArray::mStaticVars + Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::ObjectId) * CHumanArray::ElementSize;

				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::ObjectRevision) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId))
					return false;

				return Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::RealPosition) == Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates);
			}
			case SwitchIds::InteractionTargetType::Animal:
			{
				const auto* animal = CAnimalArray::mStaticVars + Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::ObjectId) * CAnimalArray::ElementSize;
				return Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(target, NC2Layout::InteractionTargetDataOffset::ObjectRevision) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::UniqueId);
			}
			default:

				return false;
		}
	}

	// Original function: FUN_00443134; Windows outlined map target setter.
	void TargetData_SetMapPosition(void* target, const void* point)
	{
		NC2Lifetime::Write(target, NC2Layout::TargetDataOffset::MapPositionX, NC2Lifetime::Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X));
		NC2Lifetime::Write(target, NC2Layout::TargetDataOffset::MapPositionY, NC2Lifetime::Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y));
	}

}
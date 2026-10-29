#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2ScriptObjectAccess.h"
#include "C2StaticVehicleTypeData.h"
#include "C2VehicleAccess.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_00422fe6.
	CCallbackManager* CallbackManager_GetExistingObjectPtr()
	{

		return CCallbackManager::sTheObjectPtr;
	}

	// Original function: FUN_00422fec.
	std::uint32_t __fastcall MapMicroPoint_GetContinent(const void* micro, void*)
	{

		return Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId);
	}

	// Original function: FUN_00422ff1.
	void* __fastcall MapMoveable_GetMarkerPtr(const void* moveable, void*)
	{

		return static_cast<std::byte*>(gMarkerEntries) + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * 24u;
	}

	// Original function: FUN_00423001.
	std::uint32_t __fastcall HumanReference_GetHumanId(const void* reference, void*)
	{

		return Read<std::uint32_t>(reference, NC2Layout::HumanReferenceOffset::HumanId);
	}

	// Original function: FUN_00423004.
	std::uint32_t __fastcall Vehicle_GetMapMoveableId(const void* vehicle, void*)
	{

		return Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId);
	}

	// Original function: FUN_0042300b.
	std::uint32_t __fastcall Vehicle_GetMissionCheckId(const void* vehicle, void*)
	{

		return Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MissionCheckId);
	}

	// Original function: FUN_00423012.
	void __fastcall Vehicle_SetMissionCheckId(void* vehicle, void*, std::uint32_t id)
	{
		Write(vehicle, NC2Layout::VehicleOffset::MissionCheckId, id);
	}

	// Original function: FUN_0042301f.
	bool __fastcall Vehicle_IsDocked(const void* vehicle, void*)
	{

		return Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) != 0;
	}

	// Original function: FUN_00423026.
	const void* __fastcall Vehicle_Passengers_GetAttachedHumanSlotPtr(const void* vehicle, void*, std::uint32_t slot)
	{

		return static_cast<const std::byte*>(vehicle) + NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger;
	}

	// Original function: FUN_00423037.
	bool __fastcall Vehicle_Passengers_IsCommanderAttached(const void* vehicle, void*)
	{

		return Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0;
	}

	// Original function: FUN_0042303e.
	std::uint32_t __fastcall Vehicle_Passengers_GetNumberOfSlots(const void* vehicle, void*)
	{

		return static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots);
	}

	// Original function: FUN_0042304a; native flag 005109C8.
	bool Game_GetSuppressDeathStatisticsFlag()
	{

		return Read<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag) != 0;
	}

	// Original function: FUN_00423050.
	void Game_SetSuppressDeathStatisticsFlag(bool flag)
	{
		Write<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag, flag);
	}

	// Original function: FUN_0042305a; native flag 005109C9.
	bool Game_GetSuppressVehicleWreckFlag()
	{

		return Read<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDestructionLandscapeFlag) != 0;
	}

	// Original function: FUN_00423060.
	void Game_SetSuppressVehicleWreckFlag(bool flag)
	{
		Write<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDestructionLandscapeFlag, flag);
	}

	// Original function: FUN_0042306a.
	std::uint32_t __fastcall House_GetMissionCheckId(const void* house, void*)
	{

		return Read<std::uint32_t>(house, NC2Layout::HouseOffset::MissionCheckId);
	}

	// Original function: FUN_004230d9.
	void* MarkerArray_GetExistingObjectPtr()
	{

		return gMarkerArray;
	}

	// Original function: FUN_004230df.
	std::uint32_t __fastcall Human_GetExplorationRange(const void* human, void*)
	{

		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::ExplorationRadius);
	}

	// Original function: FUN_004230e6; folded across different array object types.
	std::uint32_t __fastcall ArrayObject_GetId(const void* object, void*)
	{

		return Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::Id);
	}
}
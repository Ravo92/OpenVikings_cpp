#include "C2E2HumanSoundAccess.h"
#include "C2GameMapAccess.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2ObjectLayout.h"
#include "C2StaticVehicleTypeData.h"
#include "C2UserNetworkCommandAccess.h"
#include "C2VehicleAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiInputAccess.h"
#include "XNumericConstants.h"

#include <cstring>

// Original command queue/execute routine; no replacement queue is fabricated.


namespace
{
	template<typename T> T Input_Read(const void* object, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
		return value;
	}
}

namespace NC2InGameGuiInputManager
{
	// Original function: FUN_004bab58; folded singleton getter, original method name unresolved.
	void* GetInputManagerPtr()
	{
		return gInputManager;
	}
	// Original function: FUN_004b74ba.
	std::int32_t Selection_GetSelectedSingleAnimalId(const void* manager)
	{
		return Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 3 ? Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleAnimalId) : -1;
	}
	// Original function: FUN_004b7585.
	std::int32_t Selection_GetSelectedEngineHouseTypeId(const void* manager)
	{
		return Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 7 ? Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::HouseType) : -1;
	}
	// Original function: FUN_004b7596.
	std::uint32_t Selection_GetSelectedEngineHouseExtensionLevel(const void* manager)
	{
		return Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 7 ? Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseDevelopmentStage) : 0;
	}
	// Original function: FUN_004b75a6.
	std::int32_t Selection_GetSelectedGuideId(const void* manager)
	{
		return Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 8 ? Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::GuideId) : -1;
	}
	// Original function: FUN_004b75b7.
	std::int32_t Selection_GetSelectedEngineLandscapeTypeId(const void* manager)
	{
		return Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 9 ? Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::LandscapeType) : -1;
	}
	// Original function: FUN_004b75c8.
	std::uint32_t Selection_GetSelectedEngineLandscapeValency(const void* manager)
	{
		return Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 9 ? Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::LandscapeVariant) : 0;
	}
	// Original function: FUN_004b75d8.
	const void* Selection_GetSelectedEngineLandscapeMapPointRef(void* manager)
	{
		if (Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) != 9) NC2Lifetime::Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::LandscapeMapPoint, NXBasics::IntegerRange::UInt32Maximum); return static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::LandscapeMapPoint;
	}
	// Original function: FUN_004b75fa.
	std::int32_t Selection_GetSelectedEngineLandscapePlayerId(const void* manager)
	{
		return Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 9 ? Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::LandscapePlayerId) : -1;
	}
	// Original function: FUN_004b760b; invalid selection writes the native (-1,-1) point.
	const void* Selection_GetSelectedRoadBuildPlacePositionRef(void* manager)
	{
		if (Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) != 10) NC2Lifetime::Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::RoadBuildPoint, NXBasics::IntegerRange::UInt32Maximum); return static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::RoadBuildPoint;
	}
	// Original function: FUN_004b762d.
	const void* Selection_GetSelectedWallBuildPlacePositionRef(void* manager)
	{
		if (Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) != 11) NC2Lifetime::Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::WallBuildPoint, NXBasics::IntegerRange::UInt32Maximum); return static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::WallBuildPoint;
	}
	// Original function: FUN_004b7e92; all 29 native cases use miscellaneous table 2 via 004E1E35.
	const char* StringTool_GetInputModeNamePtr(const void* manager)
	{
		const auto mode = Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode); return mode <= 28 ? NC2GuiToolsBase::StringTool_GetMiscMenuStringPtr(mode + 10) : "<UNKNOWN>";
	}

	struct SInputManagerTargetPointSearch
	{
		void* targetPoints;
		std::uint32_t spacing;
		std::uint32_t radius;
		std::uint32_t visited;
		std::uint32_t wanted;
	};
	static_assert(sizeof(SInputManagerTargetPointSearch) == (sizeof(void*) == 8 ? 24 : 20));
	static_assert(offsetof(SInputManagerTargetPointSearch, wanted) == sizeof(void*) + 12);

	static bool __cdecl slInputManagerSearchForTargetPoints(const void* point, const NC2Logic::SMapPathfinderManagerSearchOverMapCallbackData* data);

	// Original function: FUN_004b74cb.
	std::int32_t Selection_GetSelectedSingleVehicleId(const void* manager)
	{
		return Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 4 ? Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId) : -1;
	}

	// Original function: FUN_004b74dc.
	std::uint32_t Selection_GetSelectedMultipleVehiclesNumber(const void* manager)
	{
		return Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 5 ? Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount) : 0;
	}

	// Original function: FUN_004b74ec.
	std::int32_t Selection_GetSelectedMultipleVehicleId(const void* manager, std::uint32_t index)
	{
		if (Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) != 5 || index >= Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount)) return -1;
		return Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupUnsigned + index * 8u);
	}

	// Original function: FUN_004b7b62; CInputManager::Misc_Tool_SendSelectedHuman.
	void Misc_Tool_SendSelectedHuman(void* manager, const void* point)
	{
		const std::int32_t selection = Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
		if (selection == 2)
		{
			l_Selection_FillTargetPointArray(manager, point, 0, Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupCount));
			for (std::uint32_t index = 0; index < Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupCount); ++index)
			{
				const std::uint32_t offset = 0x14u + index * 8u;
				const std::uint32_t id = Input_Read<std::uint32_t>(manager, offset);
				const void* human = NC2Logic::CHumanArray::mStaticVars + id * NC2Logic::CHumanArray::ElementSize;
				if ((Input_Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit5) != 0) continue;
				std::byte command[20]{ };
				NC2Lifetime::Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
				const std::uint32_t targetIndex = index % Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::TargetPointCount);
				NC2Lifetime::Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 4);
				NC2Lifetime::Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1, NC2Logic::MapMigPoint_GetLong(static_cast<const std::byte*>(manager) + NC2Layout::InputManagerOffset::TargetPointsUnsigned + targetIndex * NC2Layout::RecordSize::PackedMapPointUnsigned));
				NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, command);
				NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Input_Read<std::int32_t>(manager, offset));
			}
		}
		else if (selection == 1)
		{
			std::byte command[20]{ };
			NC2Lifetime::Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 4);
			NC2Lifetime::Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId));
			NC2Lifetime::Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1, NC2Logic::MapMigPoint_GetLong(point));
			NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, command);
			NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Input_Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId));
		}
	}

	// Original function: FUN_004b7c5a; the adjusted point is a native by-value argument.
	void Misc_Tool_SendSelectedVehicle(void* manager, std::uint32_t point)
	{
		NC2Logic::Tool_Map_GetNotBlockedPosition(&point, true);
		const auto selection = Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
		if (selection == 5)
		{
			std::uint32_t radius = 0;
			for (std::uint32_t index = 0; index < Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount); ++index)
			{
				const void* vehicle = NC2Logic::CVehicleArray::mStaticVars + Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupUnsigned + index * 8u) * NC2Logic::CVehicleArray::ElementSize;
				const auto size = static_cast<std::uint32_t>(NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->logicSize);
				if (size > radius) radius = size;
			}
			l_Selection_FillTargetPointArray(manager, &point, radius, Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount));
			for (std::uint32_t index = 0; index < Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount); ++index)
			{
				std::byte command[20]{ };
				NC2Lifetime::Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 101);
				NC2Lifetime::Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupUnsigned + index * 8u));
				const auto count = Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::TargetPointCount);
				const void* target = count == 0 ? static_cast<const void*>(&point) : static_cast<const std::byte*>(manager) + NC2Layout::InputManagerOffset::TargetPointsUnsigned + (index % count) * NC2Layout::RecordSize::PackedMapPointUnsigned;
				NC2Lifetime::Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, NC2Logic::MapMigPoint_GetLong(target));
				NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, command);
				const void* vehicle = NC2Logic::CVehicleArray::mStaticVars + Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupUnsigned + index * 8u) * NC2Logic::CVehicleArray::ElementSize;
				if (Input_Read<std::uint8_t>(vehicle, 0x2EC) != 0) NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, NC2Logic::Vehicle_Passengers_GetAttachedCommanderId(vehicle));
			}
		}
		else if (selection == 4)
		{
			std::byte command[20]{ };
			NC2Lifetime::Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 101);
			NC2Lifetime::Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId));
			NC2Lifetime::Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, NC2Logic::MapMigPoint_GetLong(&point));
			NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, command);
			const void* vehicle = NC2Logic::CVehicleArray::mStaticVars + Input_Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId) * NC2Logic::CVehicleArray::ElementSize;
			if (Input_Read<std::uint8_t>(vehicle, 0x2EC) != 0) NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, NC2Logic::Vehicle_Passengers_GetAttachedCommanderId(vehicle));
		}
	}

	// Original function: FUN_004b7ded.
	void l_Selection_FillTargetPointArray(void* manager, const void* point, std::uint32_t radius, std::uint32_t wanted)
	{
		NC2Lifetime::Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::TargetPointCount, 0);
		SInputManagerTargetPointSearch search{ static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::TargetPointCount, NC2Logic::VE_MicroPoint_GetMicroPointCount(radius) * 2u, radius, 0, wanted };
		NC2Logic::CMapPathfinderElement::SearchOverMap(point, 20, true, slInputManagerSearchForTargetPoints, &search, true, -1);
	}

	// Original function: FUN_004b7e3e. Visits include rejected candidates and the initial point.
	static bool __cdecl slInputManagerSearchForTargetPoints(const void* point, const NC2Logic::SMapPathfinderManagerSearchOverMapCallbackData* data)
	{
		auto& search = *static_cast<SInputManagerTargetPointSearch*>(data->userData);
		++search.visited;
		if (search.spacing > 1u)
		{
			const void* microPoint = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
			if ((Input_Read<std::uint8_t>(microPoint, 0x0C) & 7u) < search.radius) return false;
			if (Input_Read<std::uint32_t>(search.targetPoints, NC2Layout::InputTargetPointListOffset::Count) * search.spacing > search.visited) return false;
		}
		const std::uint32_t count = Input_Read<std::uint32_t>(search.targetPoints, NC2Layout::InputTargetPointListOffset::Count);
		NC2Lifetime::Write<std::uint32_t>(search.targetPoints, NC2Layout::InputTargetPointListOffset::Points + count * NC2Layout::RecordSize::PackedMapPointUnsigned, Input_Read<std::uint32_t>(point, 0));
		NC2Lifetime::Write<std::uint32_t>(search.targetPoints, NC2Layout::InputTargetPointListOffset::Count, count + 1u);
		return Input_Read<std::uint32_t>(search.targetPoints, NC2Layout::InputTargetPointListOffset::Count) >= search.wanted;
	}
}

namespace NC2Logic
{
	// Original function: FUN_00410e2e; CUserNetworkCommandManager::AddHumanCommand.
	void UserNetworkCommand_AddHumanCommand(void* manager, void* command)
	{
		const std::uint8_t type = Input_Read<std::uint8_t>(command, 0);
		if (type <= 1u || type >= 0x58u) return;
		const std::uint32_t id = Input_Read<std::uint32_t>(command, 4);
		const void* human = CHumanArray::mStaticVars + id * CHumanArray::ElementSize;
		if (Input_Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0) return;
		NC2Lifetime::Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Input_Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
		NC2Lifetime::Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Input_Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
		UserNetworkCommandManager_AddCommand(manager, nullptr, command, false, false);
	}

	// Original function: FUN_00410e73; CUserNetworkCommandManager::AddHouseCommand.
	void UserNetworkCommand_AddHouseCommand(void* manager, void* command)
	{
		const std::uint8_t type = Input_Read<std::uint8_t>(command, 0);
		if (type <= 0x59 || type >= 0x62) return;
		const std::uint32_t id = Input_Read<std::uint32_t>(command, 4);
		const void* house = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(CHouseArray::mStaticVars) + id * CHouseArray::ElementSize);
		if (Input_Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0) return;
		NC2Lifetime::Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, Input_Read<std::uint8_t>(house, NC2Layout::HouseOffset::PlayerId));
		NC2Lifetime::Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Input_Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
		UserNetworkCommandManager_AddCommand(manager, nullptr, command, false, false);
	}

	// Original function: FUN_00410eb8; CUserNetworkCommandManager::AddVehicleCommand.
	void UserNetworkCommand_AddVehicleCommand(void* manager, void* command)
	{
		const std::uint8_t type = Input_Read<std::uint8_t>(command, 0);
		if (type <= 0x63 || type >= 0x74) return;
		const std::uint32_t id = Input_Read<std::uint32_t>(command, 4);
		const void* vehicle = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(CVehicleArray::mStaticVars) + id * CVehicleArray::ElementSize);
		if (Input_Read<std::uint8_t>(vehicle, 0) == 0) return;
		const std::uint8_t player = static_cast<std::uint8_t>(Input_Read<std::uint32_t>(vehicle, 0x1E0));
		std::memcpy(static_cast<std::byte*>(command) + 1, &player, 1);
		const std::uint32_t value = Input_Read<std::uint32_t>(vehicle, 8);
		std::memcpy(static_cast<std::byte*>(command) + 8, &value, sizeof(value));
		NC2Logic::UserNetworkCommandManager_AddCommand(manager, nullptr, command, false, false);
	}
}

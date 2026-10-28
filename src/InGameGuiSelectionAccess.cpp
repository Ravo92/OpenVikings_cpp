#include "C2AnimalAccess.h"
#include "C2CallbackManager.h"
#include "C2DefaultInteraction.h"
#include "C2E2HumanSoundAccess.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDisplayManager.h"
#include "C2EngineMoveableDrawing.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GuideAccess.h"
#include "C2HouseAccess.h"
#include "C2HouseBuildPosition.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMerchantAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticVehicleTypeData.h"
#include "C2UserNetworkCommandAccess.h"
#include "C2VehicleAccess.h"
#include "C2WallRoadConstruction.h"
#include "C2WallRoadPathfinder.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSupportAccess.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XNumericConstants.h"

#include <cstring>
#include <initializer_list>

extern void __fastcall InGameGuiManager_Selection_UpdateWindows(void* manager);
extern void __fastcall InGameGuiManager_InputMode_Changed(void* manager);
extern bool __fastcall InputManager_InputMode_Set(void* manager, void* unusedEdx, int mode);

namespace NC2InGameGuiInputManager
{
	// Original function: FUN_004b750d.
	bool Selection_IsVehicleSelected(const void* manager, std::int32_t id)
	{
		using namespace NC2Lifetime;
		const auto selection = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
		if (selection == 4) return Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId) == id;
		if (selection == 5) for (std::uint32_t i = 0; i < Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount); ++i) if (Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupUnsigned + i * NC2Layout::RecordSize::InputSelectedObjectUnsigned) == id) return true;
		return false;
	}

	// Original function: FUN_004b7558.
	std::int32_t Selection_GetSelectedHouseId(const void* manager)
	{
		return NC2Lifetime::Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 6 ? NC2Lifetime::Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::HouseId) : -1;
	}

	// Original function: FUN_004b7569.
	bool Selection_IsHouseSelected(const void* manager, std::int32_t id)
	{
		return NC2Lifetime::Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 6 && NC2Lifetime::Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::HouseId) == id;
	}

	// Original function: FUN_004b65e5; all four fields participate in selection identity.
	bool Selection_SelectEngineHouseType(void* manager, std::uint32_t type, std::uint32_t developmentStage, std::uint32_t specialItemType, std::uint32_t specialItem)
	{
		using namespace NC2Lifetime;
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 7 && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseType) == type && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseDevelopmentStage) == developmentStage && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseSpecialItemType) == specialItemType && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseSpecialItem) == specialItem) return false;
		Selection_UnselectAll(manager);
		InputMode_Set(manager, 0);
		Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionChangeCount, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionChangeCount) + 1);
		Write(manager, NC2Layout::InputManagerOffset::HouseType, type); Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType, 7);
		Write(manager, NC2Layout::InputManagerOffset::HouseDevelopmentStage, developmentStage); Write(manager, NC2Layout::InputManagerOffset::HouseSpecialItemType, specialItemType); Write(manager, NC2Layout::InputManagerOffset::HouseSpecialItem, specialItem);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Selection_UpdateWindows();
		return true;
	}
	using namespace NC2Lifetime;
	namespace
	{
		void* Human(std::uint32_t id)
		{
			return NC2Logic::CHumanArray::mStaticVars + id * NC2Logic::CHumanArray::ElementSize;
		}
		void* Animal(std::uint32_t id)
		{
			return NC2Logic::CAnimalArray::mStaticVars + id * NC2Logic::CAnimalArray::ElementSize;
		}
		void* Vehicle(std::uint32_t id)
		{
			return NC2Logic::CVehicleArray::mStaticVars + id * NC2Logic::CVehicleArray::ElementSize;
		}
		void* House(std::uint32_t id)
		{
			return NC2Logic::CHouseArray::mStaticVars + id * NC2Logic::CHouseArray::ElementSize;
		}
		void* Guide(std::uint32_t id)
		{
			return NC2Logic::CGuideArray::mStaticVars + id * NC2Logic::CGuideArray::ElementSize;
		}
		std::uint32_t Player()
		{
			return static_cast<std::uint32_t>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId());
		}

		// l_Selection_Changed is inlined in Windows callers before 004a0e9d.
		void Changed(void* manager)
		{
			Write(manager, NC2Layout::InputManagerOffset::SelectionChangeCount, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionChangeCount) + 1u);
			InGameGuiManager_Selection_UpdateWindows(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr);
		}

		// Original function: FUN_004b7303.
		void l_Selection_Inform_Human(std::uint32_t id, std::uint32_t revision, bool selected)
		{
			if (id == NXBasics::IntegerRange::UInt32Maximum) return;
			const auto* object = Human(id);
			if (Read<std::uint8_t>(object, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(object, NC2Layout::HumanOffset::UniqueId) != revision) return;
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0xE, true, id, selected ? 3u : 2u, 0);
			if (!selected) static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager)->Message_RemoveAllForHuman(id);
		}

		// Original function: FUN_004b7361.
		void l_Selection_Inform_Animal(std::uint32_t id, std::uint32_t revision, bool selected)
		{
			if (id == NXBasics::IntegerRange::UInt32Maximum) return;
			const auto* object = Animal(id);
			if (Read<std::uint8_t>(object, NC2Layout::AnimalOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(object, NC2Layout::AnimalOffset::UniqueId) != revision) return;
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0xF, true, id, selected ? 3u : 2u, 0);
		}

		// Original function: FUN_004b73ac.
		void l_Selection_Inform_Vehicle(std::uint32_t id, std::uint32_t revision, bool selected)
		{
			if (id == NXBasics::IntegerRange::UInt32Maximum) return;
			const auto* object = Vehicle(id);
			if (Read<std::uint8_t>(object, NC2Layout::VehicleOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(object, NC2Layout::VehicleOffset::UniqueId) != revision) return;
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0x10, true, id, selected ? 3u : 2u, 0);
		}

		// Original function: FUN_004b73f7.
		void l_Selection_Inform_House(std::uint32_t id, std::uint32_t revision, bool selected)
		{
			if (id == NXBasics::IntegerRange::UInt32Maximum) return;
			const auto* object = House(id);
			if (Read<std::uint8_t>(object, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(object, NC2Layout::HouseOffset::UniqueId) != revision) return;
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0x11, true, id, selected ? 3u : 2u, 0);
		}

		// Integration dispatch retains the four distinct original helpers.
		void Inform(std::uint32_t type, std::uint32_t id, std::uint32_t revision, bool selected)
		{
			switch (type)
			{
				case SwitchIds::CallbackType::HumanDisplayStateChanged: l_Selection_Inform_Human(id, revision, selected); break;
				case SwitchIds::CallbackType::AnimalDisplayStateChanged: l_Selection_Inform_Animal(id, revision, selected); break;
				case SwitchIds::CallbackType::VehicleDisplayStateChanged: l_Selection_Inform_Vehicle(id, revision, selected); break;
				case SwitchIds::CallbackType::HouseDisplayStateChanged: l_Selection_Inform_House(id, revision, selected); break;
				default: break;
			}
		}

		bool SelectSingle(void* manager, std::uint32_t mode, std::uint32_t offset, std::uint32_t id, const void* object, std::uint32_t revisionOffset, std::uint32_t callback)
		{
			auto revision = Read<std::uint32_t>(object, revisionOffset);
			if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == mode && Read<std::uint32_t>(manager, offset) == id && Read<std::uint32_t>(manager, offset + NC2Layout::InputSelectedObjectOffset::UniqueId) == revision) return false;
			Selection_UnselectAll(manager);
			InputMode_Set(manager, 0);
			revision = Read<std::uint32_t>(object, revisionOffset);
			Write(manager, NC2Layout::InputManagerOffset::SelectionType, mode); Write(manager, offset, id); Write(manager, offset + NC2Layout::InputSelectedObjectOffset::UniqueId, revision);
			Changed(manager);
			if (callback != 0) Inform(callback, id, revision, true);
			return true;
		}

		void ResetInvalid(void* manager)
		{
			Selection_UnselectAll(manager); InputMode_Set(manager, 0);
		}

		bool IsLive(const void* object, std::uint32_t revision, std::uint32_t offset = NC2Layout::ActorIdentityOffset::UniqueId)
		{
			return Read<std::uint8_t>(object, NC2Layout::ActorIdentityOffset::Allocated) != 0 && Read<std::uint32_t>(object, offset) == revision;
		}

		void ValidateGroup(void* manager, bool vehicles)
		{
			const auto countOffset = vehicles ? NC2Layout::InputManagerOffset::VehicleGroupCountUnsigned : NC2Layout::InputManagerOffset::HumanGroupCountUnsigned, entriesOffset = vehicles ? NC2Layout::InputManagerOffset::VehicleGroupUnsigned : NC2Layout::InputManagerOffset::HumanGroupUnsigned;
			bool changed = false;
			for (std::uint32_t index = 0; index < Read<std::uint32_t>(manager, countOffset);)
			{
				auto* record = static_cast<std::byte*>(manager) + entriesOffset + index * NC2Layout::RecordSize::InputSelectedObjectUnsigned;
				const auto id = Read<std::uint32_t>(record, NC2Layout::InputSelectedObjectOffset::ObjectId), revision = Read<std::uint32_t>(record, NC2Layout::InputSelectedObjectOffset::UniqueId);
				const auto* object = vehicles ? Vehicle(id) : Human(id);
				if (IsLive(object, revision) && Read<std::uint32_t>(object, vehicles ? NC2Layout::VehicleOffset::PlayerId : NC2Layout::HumanOffset::PlayerId) == Player())
				{
					++index; continue;
				}
				Inform(vehicles ? 0x10 : 0xE, id, revision, false);
				const auto count = Read<std::uint32_t>(manager, countOffset) - 1u;
				Write(manager, countOffset, count);
				if (index != count) std::memmove(record, record + NC2Layout::RecordSize::InputSelectedObject, (count - index) * NC2Layout::RecordSize::InputSelectedObjectUnsigned);
				changed = true;
			}
			const auto count = Read<std::uint32_t>(manager, countOffset);
			if (count == 0) ResetInvalid(manager);
			else if (count == 1)
			{
				const auto id = Read<std::int32_t>(manager, entriesOffset);
				if (vehicles) Selection_SelectSingleVehicle(manager, id); else Selection_SelectSingleHuman(manager, id);
			}
			else if (changed) Changed(manager);
		}

		void RemoveFromSelection(void* manager, std::uint32_t id, std::uint32_t singleMode, std::uint32_t singleOffset, std::uint32_t groupMode, std::uint32_t countOffset, std::uint32_t recordsOffset, std::uint32_t callback)
		{
			Selection_Validate(manager);
			const auto mode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
			if (mode == singleMode)
			{
				if (Read<std::uint32_t>(manager, singleOffset) == id) ResetInvalid(manager);
				return;
			}
			if (groupMode == 0 || mode != groupMode) return;
			bool changed = false;
			for (std::uint32_t index = 0; index < Read<std::uint32_t>(manager, countOffset);)
			{
				auto* record = static_cast<std::byte*>(manager) + recordsOffset + index * NC2Layout::RecordSize::InputSelectedObjectUnsigned;
				if (Read<std::uint32_t>(record, NC2Layout::InputSelectedObjectOffset::ObjectId) != id)
				{
					++index; continue;
				}
				Inform(callback, id, Read<std::uint32_t>(record, NC2Layout::InputSelectedObjectOffset::UniqueId), false);
				const auto count = Read<std::uint32_t>(manager, countOffset) - 1u;
				Write(manager, countOffset, count);
				if (index != count) std::memmove(record, record + NC2Layout::RecordSize::InputSelectedObject, (count - index) * NC2Layout::RecordSize::InputSelectedObjectUnsigned);
				changed = true;
			}
			const auto count = Read<std::uint32_t>(manager, countOffset);
			if (count == 0) ResetInvalid(manager);
			else if (count == 1)
			{
				const auto remaining = Read<std::int32_t>(manager, recordsOffset);
				if (groupMode == 5) Selection_SelectSingleVehicle(manager, remaining); else Selection_SelectSingleHuman(manager, remaining);
			}
			else if (changed) Changed(manager);
		}
	}

	// Original function: FUN_004b764f; changes to the enable flag clear the complete engine map.
	void Highlighting_Map_SetState(void* manager, bool enabled)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::InputManagerOffset::MapHighlightingEnabled) != static_cast<std::uint8_t>(enabled))
		{
			Write<std::uint8_t>(manager, NC2Layout::InputManagerOffset::MapHighlightingEnabled, enabled);
			// Original function: FUN_004832dc; clear only the two highlight bits.
			for (std::int32_t y = NC2Logic::GameMap_GetHeight() - 1; y >= 0; --y)
				for (std::int32_t x = NC2Logic::GameMap_GetWidth() - 1; x >= 0; --x)
				{
					const std::int16_t point[2]{ static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) };
					auto* micro = const_cast<void*>(NC2E2::EngineMap_GetConstMapMicroPointPtr(point));
					Write<std::uint8_t>(micro, NC2Layout::EngineMapMicroPointOffset::HighlightFlags, Read<std::uint8_t>(micro, NC2Layout::EngineMapMicroPointOffset::HighlightFlags) & NXBasics::BitMask::ByteExceptBits0Through1);
				}
			auto* world = Read<void*>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement);
			auto* display = Read<void*>(world, NC2Layout::WorldGuiElementOffset::Display);
			if (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::HighlightingEnabled) != static_cast<std::uint8_t>(enabled))
			{
				Write<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::HighlightingEnabled, enabled); Write<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag, 1);
			}
		}
	}

	// Original function: FUN_004b4854; Windows validates selection before changing highlights.
	bool InputMode_Set(void* manager, std::int32_t mode)
	{
		if (Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::InputMode) == mode) return true;
		const auto selection = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
		if (static_cast<std::uint32_t>(mode) <= 28)
		{
			bool allowed = true;
			if ((mode >= 1 && mode <= 9) || mode == 13) allowed = selection == 1;
			else if (mode == 10 || mode == 22 || mode == 23 || mode == 25) allowed = selection == 1 || selection == 2;
			else if (mode == 11 || mode == 20 || mode == 21 || mode == 24 || mode == 28) allowed = selection == 4 || selection == 5;
			else if (mode == 12) allowed = selection == 7;
			else if (mode == 19) allowed = selection == 4;
			else if (mode == 26 || mode == 27) allowed = selection == 1 || selection == 2 || selection == 4;
			if (!allowed) return false;
			Highlighting_Map_SetState(manager, mode != 0);
			if (mode == 0)
			{
				Highlighting_Humans_Update(manager, true, true);
				Highlighting_Animals_Update(manager, true, true);
				Highlighting_Vehicles_Update(manager, true, true);
			}
			else if (mode >= 23)
			{
				Write(manager, NC2Layout::InputManagerOffset::InputMode, mode);
				if (mode == 23 || mode == 24) Highlighting_Humans_Update(manager, true, false);
				else if (mode == 25) Highlighting_Animals_Update(manager, true, false);
				else Highlighting_Vehicles_Update(manager, true, false);
			}
			NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->ActivateDesktop();
			if (mode == 0) NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Exploration_Set(true);
		}
		Write<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent, 0); Write<std::int32_t>(manager, NC2Layout::InputManagerOffset::InputMode, mode);
		Cursor_SetPosition(manager, static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::CursorMapPoint);
		InGameGuiManager_InputMode_Changed(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr);
		NC2InGameGuiBase::BaseToolDesktop_SelectionActiveChanged();
		return true;
	}

	// Original function: FUN_004b4a5f.
	void InputMode_Cancel(void* manager)
	{
		const auto mode = Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::InputMode);
		if (mode == 12)
		{
			InputMode_Set(manager, 0); Selection_UnselectAll(manager);
		}
		else if (mode == 0) Selection_UnselectAll(manager);
		else if (mode > 0 && mode <= 28) InputMode_Set(manager, 0);
		NC2InGameGuiBase::BaseToolDesktop_SelectionActiveChanged();
	}

	// Original function: FUN_004b4a4c; reset selection before leaving the input mode.
	void UnselectAll(void* manager)
	{
		Selection_UnselectAll(manager); InputMode_Set(manager, 0);
	}

	// Original function: FUN_004b66e5; coordinates are two WORDs, not a pointer identity.
	bool Selection_SelectEngineLandscapeType(void* manager, std::uint32_t type, std::uint32_t valency, const void* point, std::int32_t player)
	{
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 9 && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::LandscapeType) == type && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::LandscapeVariant) == valency && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::LandscapeMapPoint) == Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates) && Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::LandscapePlayerId) == player) return false;
		UnselectAll(manager);
		Write(manager, NC2Layout::InputManagerOffset::LandscapeType, type); Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType, 9); Write(manager, NC2Layout::InputManagerOffset::LandscapeVariant, valency);
		Write(manager, NC2Layout::InputManagerOffset::SelectionChangeCount, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionChangeCount) + 1u);
		Write(manager, NC2Layout::InputManagerOffset::LandscapeMapPoint, Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates)); Write(manager, NC2Layout::InputManagerOffset::LandscapePlayerId, player);
		InGameGuiManager_Selection_UpdateWindows(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr);
		return true;
	}

	// Original function: FUN_004b5eb1.
	std::uint32_t Action_NormalSelectPossible(void* manager, const void* object, std::uint32_t* first, std::uint32_t* second, std::uint32_t* third, std::uint32_t* fourth)
	{
		*first = *second = *third = *fourth = NXBasics::IntegerRange::UInt32Maximum;
		std::int32_t action = 0, id = -1;
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode) == 0 && object != nullptr)
		{
			switch (Read<std::uint32_t>(object, NC2Layout::DisplayObjectReferenceOffset::Type))
			{
				case SwitchIds::MapObjectUnderCursor::Human: id = Read<std::int32_t>(object, NC2Layout::DisplayObjectReferenceOffset::HumanId); if (Read<std::uint32_t>(Human(id), NC2Layout::HumanOffset::PlayerId) == Player()) action = 1; break;
				case SwitchIds::MapObjectUnderCursor::Animal: id = Read<std::int32_t>(object, NC2Layout::DisplayObjectReferenceOffset::AnimalId); if (Read<std::uint32_t>(Animal(id), NC2Layout::AnimalOffset::PlayerId) == Player()) action = 3; break;
				case SwitchIds::MapObjectUnderCursor::Vehicle: id = Read<std::int32_t>(object, NC2Layout::DisplayObjectReferenceOffset::VehicleId); if (Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::PlayerId) == Player()) action = 4; break;
				case SwitchIds::MapObjectUnderCursor::House: id = Read<std::int32_t>(object, NC2Layout::DisplayObjectReferenceOffset::HouseId); action = 6; break;
				case SwitchIds::MapObjectUnderCursor::Guide: id = Read<std::int32_t>(object, NC2Layout::DisplayObjectReferenceOffset::GuideId); if (Read<std::uint32_t>(Guide(id), NC2Layout::GuideOffset::PlayerId) == Player()) action = 8; break;
				case SwitchIds::MapObjectUnderCursor::Landscape: id = Read<std::int32_t>(object, NC2Layout::DisplayObjectReferenceOffset::LandscapeId); *second = Read<std::uint32_t>(object, NC2Layout::DisplayObjectReferenceOffset::LandscapeVariant); *third = Read<std::uint32_t>(object, NC2Layout::DisplayObjectReferenceOffset::MapPosition); *fourth = Read<std::uint32_t>(object, NC2Layout::DisplayObjectReferenceOffset::PlayerId); action = 9; break;
				case SwitchIds::MapObjectUnderCursor::RoadBuildPlace: case SwitchIds::MapObjectUnderCursor::WallBuildPlace:
					{
						const bool road = Read<std::uint32_t>(object, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::RoadBuildPlace;
						const auto* point = static_cast<const std::byte*>(object) + NC2Layout::DisplayObjectReferenceOffset::MapPosition;
						if (NC2Logic::RoadBuildManager_GetRoadBuildDataPtr(road ? NC2Logic::gRoadBuildManager : NC2Logic::gWallBuildManager, Player(), point) != nullptr)
						{
							id = static_cast<std::int32_t>(NC2Logic::MapMigPoint_GetLong(point)); action = road ? 10 : 11;
						}
						break;
					}
				default: break;
			}
		}
		else if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode) == 0 && Read<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent) != 0)
		{
			const auto* point = static_cast<const std::byte*>(manager) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint;
			NC2Logic::CMapMoveableIterator iterator;
			const auto moveableId = iterator.GetFirstMapMoveableId(point);
			if (moveableId != -1)
			{
				const auto* moveable = NC2Logic::gMapMoveableArrayStorage.entries + moveableId * NC2Layout::RecordSize::MapMoveable;
				id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
				switch (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType))
				{
					case SwitchIds::MapMoveableType::Human: if (Read<std::uint32_t>(Human(id), NC2Layout::HumanOffset::PlayerId) == Player()) action = 1; break;
					case SwitchIds::MapMoveableType::Animal: if (Read<std::uint32_t>(Animal(id), NC2Layout::AnimalOffset::PlayerId) == Player() || Read<std::uint32_t>(Animal(id), NC2Layout::AnimalOffset::PlayerId) == 20) action = 3; break;
					case SwitchIds::MapMoveableType::Vehicle: if (Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::PlayerId) == Player() || Read<std::int32_t>(Vehicle(id), NC2Layout::VehicleOffset::PlayerId) == -1) action = 4; break;
					default: break;
				}
			}
			if (action == 0)
			{
				id = NC2Logic::Logic_EntryPoint_House_GetOnPosition(point); if (id != -1 && Read<std::uint32_t>(House(id), NC2Layout::HouseOffset::PlayerId) == Player()) action = 6;
			}
		}
		if (action != 0) *first = static_cast<std::uint32_t>(id);
		return static_cast<std::uint32_t>(action);
	}

	// Original function: FUN_004b4a9f.
	bool Action_DoNormalSelect(void* manager, const void* object, bool keepSelection, bool add, bool remove)
	{
		std::uint32_t first, second, third, fourth;
		const auto action = Action_NormalSelectPossible(manager, object, &first, &second, &third, &fourth);
		const auto id = static_cast<std::int32_t>(first);
		switch (action)
		{
			case SwitchIds::SelectionAction::Human: if (add) Selection_SelectAddSingleHuman(manager, id); else if (remove) UnSelect_Human(manager, id); else Selection_SelectSingleHuman(manager, id); break;
			case SwitchIds::SelectionAction::Animal: Selection_SelectSingleAnimal(manager, id); break;
			case SwitchIds::SelectionAction::Vehicle: if (add) Selection_SelectAddSingleVehicle(manager, id); else if (remove) UnSelect_Vehicle(manager, id); else Selection_SelectSingleVehicle(manager, id); break;
			case SwitchIds::SelectionAction::House: Selection_SelectHouse(manager, id); break;
			case SwitchIds::SelectionAction::Guide: Selection_SelectGuide(manager, id); break;
			case SwitchIds::SelectionAction::RoadBuildPlace: case SwitchIds::SelectionAction::WallBuildPlace:
				{
					std::uint32_t point; NC2Logic::MapMigPoint_AssignLong(&point, first);
					if (action == 10) Selection_SelectRoadBuildPlace(manager, &point);
					else Selection_SelectWallBuildPlace(manager, &point);
					break;
				}
			case SwitchIds::SelectionAction::Landscape:
				Selection_SelectEngineLandscapeType(manager, first, second, &third, NXBasics::Runtime_TimeSigned32(fourth));
				break;
			default: if (!keepSelection) Selection_UnselectAll(manager); return false;
		}
		return true;
	}

	// Original function: FUN_004b6bb0; record payloads remain untouched.
	void Selection_UnselectAll(void* manager)
	{
		switch (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType))
		{
			case SwitchIds::SelectionType::SingleHuman: Inform(0xE, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanUniqueId), false); break;
			case SwitchIds::SelectionType::MultipleHumans:
				for (std::uint32_t i = 0; i < Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupCount); ++i) Inform(0xE, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupUnsigned + i * NC2Layout::RecordSize::InputSelectedObjectUnsigned), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupUniqueIdsUnsigned + i * NC2Layout::RecordSize::InputSelectedObjectUnsigned), false);
				break;
			case SwitchIds::SelectionType::SingleAnimal: Inform(0xF, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleAnimalId), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleAnimalUniqueId), false); break;
			case SwitchIds::SelectionType::SingleVehicle: Inform(0x10, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleUniqueId), false); break;
			case SwitchIds::SelectionType::MultipleVehicles:
				for (std::uint32_t i = 0; i < Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount); ++i) Inform(0x10, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupUnsigned + i * NC2Layout::RecordSize::InputSelectedObjectUnsigned), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupUniqueIdsUnsigned + i * NC2Layout::RecordSize::InputSelectedObjectUnsigned), false);
				break;
			case SwitchIds::SelectionType::House: Inform(0x11, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseId), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseUniqueId), false); break;
			default: break;
		}
		Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType, 0);
		for (auto offset : { NC2Layout::InputManagerOffset::SingleHumanIdUnsigned, NC2Layout::InputManagerOffset::SingleAnimalIdUnsigned, NC2Layout::InputManagerOffset::SingleVehicleIdUnsigned, NC2Layout::InputManagerOffset::HouseIdUnsigned, NC2Layout::InputManagerOffset::HouseTypeUnsigned, NC2Layout::InputManagerOffset::HouseDevelopmentStageUnsigned, NC2Layout::InputManagerOffset::GuideIdUnsigned, NC2Layout::InputManagerOffset::LandscapeTypeUnsigned, NC2Layout::InputManagerOffset::LandscapeVariantUnsigned }) Write<std::uint32_t>(manager, offset, NXBasics::IntegerRange::UInt32Maximum);
		for (auto offset : { NC2Layout::InputManagerOffset::SingleHumanUniqueIdUnsigned, NC2Layout::InputManagerOffset::HumanGroupCountUnsigned, NC2Layout::InputManagerOffset::SingleAnimalUniqueIdUnsigned, NC2Layout::InputManagerOffset::SingleVehicleUniqueIdUnsigned, NC2Layout::InputManagerOffset::HouseUniqueIdUnsigned, NC2Layout::InputManagerOffset::GuideUniqueIdUnsigned }) Write<std::uint32_t>(manager, offset, 0);
		Changed(manager);
	}

	// Original functions: FUN_004b6115/004b62a9/004b632f/004b655f/004b6672.
	bool Selection_SelectSingleHuman(void* manager, std::int32_t id)
	{
		return SelectSingle(manager, SwitchIds::SelectionType::SingleHuman, NC2Layout::InputManagerOffset::SingleHumanId, id, Human(id), NC2Layout::HumanOffset::UniqueId, SwitchIds::CallbackType::HumanDisplayStateChanged);
	}
	bool Selection_SelectSingleAnimal(void* manager, std::int32_t id)
	{
		return SelectSingle(manager, SwitchIds::SelectionType::SingleAnimal, NC2Layout::InputManagerOffset::SingleAnimalId, id, Animal(id), NC2Layout::AnimalOffset::UniqueId, SwitchIds::CallbackType::AnimalDisplayStateChanged);
	}
	bool Selection_SelectSingleVehicle(void* manager, std::int32_t id)
	{
		return SelectSingle(manager, SwitchIds::SelectionType::SingleVehicle, NC2Layout::InputManagerOffset::SingleVehicleId, id, Vehicle(id), NC2Layout::VehicleOffset::UniqueId, SwitchIds::CallbackType::VehicleDisplayStateChanged);
	}
	bool Selection_SelectHouse(void* manager, std::int32_t id)
	{
		return SelectSingle(manager, SwitchIds::SelectionType::House, NC2Layout::InputManagerOffset::HouseId, id, House(id), NC2Layout::HouseOffset::UniqueId, SwitchIds::CallbackType::HouseDisplayStateChanged);
	}
	bool Selection_SelectGuide(void* manager, std::int32_t id)
	{
		return SelectSingle(manager, SwitchIds::SelectionType::Guide, NC2Layout::InputManagerOffset::GuideId, id, Guide(id), 0x10, 0);
	}

	// Original function: FUN_004b6788.
	bool Selection_SelectRoadBuildPlace(void* manager, const void* point)
	{
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 10 && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::RoadBuildPoint) == Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates)) return false;
		Selection_UnselectAll(manager); InputMode_Set(manager, 0);
		Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType, 10); Write(manager, NC2Layout::InputManagerOffset::RoadBuildPoint, Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates)); Changed(manager); return true;
	}
	// Original function: FUN_004b67ed.
	bool Selection_SelectWallBuildPlace(void* manager, const void* point)
	{
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 11 && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::WallBuildPoint) == Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates)) return false;
		Selection_UnselectAll(manager); InputMode_Set(manager, 0);
		Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType, 11); Write(manager, NC2Layout::InputManagerOffset::WallBuildPoint, Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates)); Changed(manager); return true;
	}

	// Original function: FUN_004b618b; at most 300 humans, preserving the old single first.
	bool Selection_SelectAddSingleHuman(void* manager, std::int32_t id)
	{
		const auto mode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
		if (mode != 1 && mode != 2) return Selection_SelectSingleHuman(manager, id);
		if (mode == 1)
		{
			const auto old = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId);
			if (old == static_cast<std::uint32_t>(id)) return false;
			Selection_UnselectAll(manager); InputMode_Set(manager, 0);
			Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType, 2); Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupCount, 2);
			Write(manager, NC2Layout::InputManagerOffset::HumanGroup, old); Write(manager, NC2Layout::InputManagerOffset::HumanGroupUniqueIds, Read<std::uint32_t>(Human(old), NC2Layout::HumanOffset::UniqueId));
			Write(manager, NC2Layout::InputManagerOffset::HumanGroupSecondId, id); Write(manager, NC2Layout::InputManagerOffset::HumanGroupSecondUniqueId, Read<std::uint32_t>(Human(id), NC2Layout::HumanOffset::UniqueId));
			Changed(manager);
			Inform(0xE, old, Read<std::uint32_t>(Human(old), NC2Layout::HumanOffset::UniqueId), true);
		}
		else
		{
			const auto count = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupCount);
			for (std::uint32_t i = 0; i < count; ++i) if (Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupUnsigned + i * NC2Layout::RecordSize::InputSelectedObjectUnsigned) == id) return false;
			if (count == NC2Runtime::InputSelectionLimits::Humans) return false;
			Write(manager, NC2Layout::InputManagerOffset::HumanGroupUnsigned + count * NC2Layout::RecordSize::InputSelectedObjectUnsigned, id); Write(manager, NC2Layout::InputManagerOffset::HumanGroupUniqueIdsUnsigned + count * NC2Layout::RecordSize::InputSelectedObjectUnsigned, Read<std::uint32_t>(Human(id), NC2Layout::HumanOffset::UniqueId)); Write(manager, NC2Layout::InputManagerOffset::HumanGroupCount, count + 1u);
			Changed(manager);
		}
		Inform(0xE, id, Read<std::uint32_t>(Human(id), NC2Layout::HumanOffset::UniqueId), true);
		return true;
	}

	// Original function: FUN_004b63b5; only type-five vehicles form a group, maximum 30.
	bool Selection_SelectAddSingleVehicle(void* manager, std::int32_t id)
	{
		const auto mode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
		if (mode != 4 && mode != 5) return Selection_SelectSingleVehicle(manager, id);
		if (mode == 5)
		{
			const auto count = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount);
			for (std::uint32_t i = 0; i < count; ++i) if (Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupUnsigned + i * NC2Layout::RecordSize::InputSelectedObjectUnsigned) == id) return false;
			if (count == NC2Runtime::InputSelectionLimits::Vehicles) return false;
			if (Read<std::uint32_t>(Vehicle(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroup)), NC2Layout::VehicleOffset::LogicType) != 5 || Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::LogicType) != 5) return Selection_SelectSingleVehicle(manager, id);
			Write(manager, NC2Layout::InputManagerOffset::VehicleGroupUnsigned + count * NC2Layout::RecordSize::InputSelectedObjectUnsigned, id); Write(manager, NC2Layout::InputManagerOffset::VehicleGroupUniqueIdsUnsigned + count * NC2Layout::RecordSize::InputSelectedObjectUnsigned, Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::UniqueId)); Write(manager, NC2Layout::InputManagerOffset::VehicleGroupCount, count + 1u);
			Changed(manager);
		}
		else
		{
			const auto old = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId);
			if (Read<std::uint32_t>(Vehicle(old), NC2Layout::VehicleOffset::LogicType) != 5 || Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::LogicType) != 5) return Selection_SelectSingleVehicle(manager, id);
			if (old == static_cast<std::uint32_t>(id)) return false;
			Selection_UnselectAll(manager); InputMode_Set(manager, 0);
			Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType, 5); Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount, 2);
			Write(manager, NC2Layout::InputManagerOffset::VehicleGroup, old); Write(manager, NC2Layout::InputManagerOffset::VehicleGroupUniqueIds, Read<std::uint32_t>(Vehicle(old), NC2Layout::VehicleOffset::UniqueId));
			Write(manager, NC2Layout::InputManagerOffset::VehicleGroupSecondId, id); Write(manager, NC2Layout::InputManagerOffset::VehicleGroupSecondUniqueId, Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::UniqueId));
			Changed(manager);
			Inform(0x10, old, Read<std::uint32_t>(Vehicle(old), NC2Layout::VehicleOffset::UniqueId), true);
		}
		Inform(0x10, id, Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::UniqueId), true);
		return true;
	}

	// Original function: FUN_004b6ce1; type-only selections 7 and 9 deliberately require no validation.
	void Selection_Validate(void* manager)
	{
		const auto mode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
		bool valid = true;
		switch (mode)
		{
			case SwitchIds::SelectionType::SingleHuman: valid = IsLive(Human(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId)), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanUniqueId)) && Read<std::uint32_t>(Human(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId)), NC2Layout::HumanOffset::PlayerId) == Player(); break;
			case SwitchIds::SelectionType::MultipleHumans: ValidateGroup(manager, false); return;
			case SwitchIds::SelectionType::SingleAnimal: valid = IsLive(Animal(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleAnimalId)), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleAnimalUniqueId)) && Read<std::uint32_t>(Animal(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleAnimalId)), NC2Layout::AnimalOffset::PlayerId) == Player(); break;
			case SwitchIds::SelectionType::SingleVehicle: valid = IsLive(Vehicle(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId)), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleUniqueId)) && Read<std::uint32_t>(Vehicle(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId)), NC2Layout::VehicleOffset::PlayerId) == Player(); break;
			case SwitchIds::SelectionType::MultipleVehicles: ValidateGroup(manager, true); return;
			case SwitchIds::SelectionType::House: valid = IsLive(House(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseId)), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseUniqueId)); break;
			case SwitchIds::SelectionType::Guide: valid = IsLive(Guide(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::GuideId)), Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::GuideUniqueId), NC2Layout::GuideOffset::UniqueId) && Read<std::uint32_t>(Guide(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::GuideId)), NC2Layout::GuideOffset::PlayerId) == Player(); break;
			case SwitchIds::SelectionType::RoadBuildPlace: valid = NC2Logic::RoadBuildManager_GetRoadBuildDataPtr(NC2Logic::gRoadBuildManager, Player(), static_cast<const std::byte*>(manager) + NC2Layout::InputManagerOffset::RoadBuildPoint) != nullptr; break;
			case SwitchIds::SelectionType::WallBuildPlace: valid = NC2Logic::RoadBuildManager_GetRoadBuildDataPtr(NC2Logic::gWallBuildManager, Player(), static_cast<const std::byte*>(manager) + NC2Layout::InputManagerOffset::WallBuildPoint) != nullptr; break;
			default: break;
		}
		if (!valid) ResetInvalid(manager);
	}

	// Original functions: FUN_004b7059/004b7159/004b7187/004b72a7/004b72d5.
	void UnSelect_Human(void* manager, std::int32_t id)
	{
		RemoveFromSelection(manager, id, SwitchIds::SelectionType::SingleHuman, NC2Layout::InputManagerOffset::SingleHumanId, SwitchIds::SelectionType::MultipleHumans, NC2Layout::InputManagerOffset::HumanGroupCount, NC2Layout::InputManagerOffset::HumanGroup, SwitchIds::CallbackType::HumanDisplayStateChanged);
	}
	void UnSelect_Animal(void* manager, std::int32_t id)
	{
		RemoveFromSelection(manager, id, SwitchIds::SelectionType::SingleAnimal, NC2Layout::InputManagerOffset::SingleAnimalId, 0, 0, 0, SwitchIds::CallbackType::AnimalDisplayStateChanged);
	}
	void UnSelect_Vehicle(void* manager, std::int32_t id)
	{
		RemoveFromSelection(manager, id, SwitchIds::SelectionType::SingleVehicle, NC2Layout::InputManagerOffset::SingleVehicleId, SwitchIds::SelectionType::MultipleVehicles, NC2Layout::InputManagerOffset::VehicleGroupCount, NC2Layout::InputManagerOffset::VehicleGroup, SwitchIds::CallbackType::VehicleDisplayStateChanged);
	}
	void UnSelect_House(void* manager, std::int32_t id)
	{
		RemoveFromSelection(manager, id, SwitchIds::SelectionType::House, NC2Layout::InputManagerOffset::HouseId, 0, 0, 0, SwitchIds::CallbackType::HouseDisplayStateChanged);
	}
	void UnSelect_Guide(void* manager, std::int32_t id)
	{
		RemoveFromSelection(manager, id, SwitchIds::SelectionType::Guide, NC2Layout::InputManagerOffset::GuideId, 0, 0, 0, 0);
	}

	// Original functions: FUN_004b7442/004b7450/004b745d/004b7478.
	std::int32_t Selection_GetSelectedSingleHumanId(const void* manager)
	{
		return Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 1 ? Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId) : -1;
	}
	std::uint32_t Selection_GetSelectedMultipleHumanNumber(const void* manager)
	{
		return Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 2 ? Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupCount) : 0;
	}
	std::int32_t Selection_GetSelectedMultipleHumanId(const void* manager, std::uint32_t index)
	{
		return Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 2 && index < Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupCount) ? Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::HumanGroupUnsigned + index * NC2Layout::RecordSize::InputSelectedObjectUnsigned) : -1;
	}
	bool Selection_IsHumanSelected(const void* manager, std::int32_t id)
	{
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 1) return Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId) == id;
		for (std::uint32_t i = 0; i < Selection_GetSelectedMultipleHumanNumber(manager); ++i) if (Selection_GetSelectedMultipleHumanId(manager, i) == id) return true;
		return false;
	}

	namespace
	{
		// Integration loop preserves the signed-word rectangle endpoints used by the native helpers.
		template<class Predicate> bool UpdateHighlightArea(const std::int16_t* rectangle, Predicate mark)
		{
			bool changed = false;
			const auto right = static_cast<std::int16_t>(rectangle[0] + rectangle[2] - 1), bottom = static_cast<std::int16_t>(rectangle[1] + rectangle[3] - 1);
			for (std::int32_t y = rectangle[1]; y <= bottom; ++y)
				for (std::int32_t x = rectangle[0]; x <= right; ++x)
				{
					const std::int16_t point[2]{ static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) };
					auto* micro = const_cast<void*>(NC2E2::EngineMap_GetConstMapMicroPointPtr(point));
					const auto wanted = mark(point), flags = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);
					if ((flags & 3u) == wanted) continue;
					Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData, (flags & NXBasics::BitMask::ByteExceptBits0Through1) | wanted);
					changed = true;
				}
			return changed;
		}

		// Original function: FUN_004c9f57; the folded helper has no recovered Mac name.
		bool HighlightInvalidArea(const std::int16_t* rectangle)
		{
			return UpdateHighlightArea(rectangle, [] (const void*) -> std::uint8_t
			{
				return 1;
			});
		}

		// Original function: FUN_004ca169.
		bool ls_Highlighting_Update_Tool_CalculationMapIntoHighlightFlags(const std::int16_t* rectangle)
		{
			return UpdateHighlightArea(rectangle, [] (const void* point) -> std::uint8_t
			{
				return NC2Logic::GameMap_CalculationMap_GetValue(point) != 0 ? 3 : 1;
			});
		}

		// Original function: FUN_004ca218.
		bool __cdecl ls_Highlighting_Update_Tool_PathfinderSearchOverMapFunction_AlwaysGoOn(const void*, const NC2Logic::SMapPathfinderManagerSearchOverMapCallbackData*)
		{
			return false;
		}

		bool HighlightHumanContinent(const std::int16_t* rectangle, std::uint32_t continent)
		{
			const auto mark = [continent] (const void* point) -> std::uint8_t
			{
				const auto* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
				return Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) == continent && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 ? 3 : 1;
			};
			return UpdateHighlightArea(rectangle, mark);
		}

		// Original function: FUN_004c9fdb; Windows outlined helper, original Mac name unavailable.
		bool HighlightHumanMovement(void* manager, const std::int16_t* rectangle)
		{
			const auto* human = Human(Selection_GetSelectedSingleHumanId(manager));
			std::uint32_t position = 0;
			if (!NC2Logic::Human_GetMapPosition(human, &position)) return HighlightInvalidArea(rectangle);
			if (NC2Logic::Human_UsesExtendedPathfinder(human))
			{
				const auto continent = Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(&position), NC2Layout::MapMicroPointOffset::ContinentId);
				if (continent == 0 || NC2Logic::GameMap_Continent_GetContinentTyp(continent) != 1) return HighlightInvalidArea(rectangle);
				return HighlightHumanContinent(rectangle, continent);
			}
			NC2Logic::GameMap_CalculationMap_Clear(rectangle);
			// Game.exe 004F4300/004F4304: ordinary/carrier point-search radii.
			NC2Logic::CMapPathfinderElement::SearchOverMap(NC2Logic::Human_GetMapPositionRef(human), NC2Logic::Human_IsJobCarrier(human) ? 63u : 50u, true, ls_Highlighting_Update_Tool_PathfinderSearchOverMapFunction_AlwaysGoOn, nullptr, true, -1);
			return ls_Highlighting_Update_Tool_CalculationMapIntoHighlightFlags(rectangle);
		}

		// Original function: FUN_004ca21b; the displayed work area has a different origin from marker placement.
		bool HighlightHumanWorkCenter(void* manager, const std::int16_t* rectangle, bool displayArea)
		{
			const auto* human = Human(Selection_GetSelectedSingleHumanId(manager));
			std::uint32_t position = 0;
			if (!NC2Logic::Human_GetMapPosition(human, &position)) return HighlightInvalidArea(rectangle);
			if (NC2Logic::Human_UsesExtendedPathfinder(human))
			{
				const auto continent = Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(&position), NC2Layout::MapMicroPointOffset::ContinentId);
				if (continent == 0 || NC2Logic::GameMap_Continent_GetContinentTyp(continent) != 1) return HighlightInvalidArea(rectangle);
				return HighlightHumanContinent(rectangle, continent);
			}
			NC2Logic::GameMap_CalculationMap_Clear(rectangle);
			const void* source;
			std::uint32_t radius;
			if (displayArea)
			{
				const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
				radius = job == 18 || job == 29 ? 10u : 20u; // Game.exe 004F430C/004F4310.
				source = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
			}
			else if (NC2Logic::Human_IsAttachedToWorkHouse(human))
			{
				radius = NC2Logic::Human_IsJobCarrier(human) ? 43u : 5u; // Game.exe 004F4320/004F4324.
				source = static_cast<const std::byte*>(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId))) + 0x24;
			}
			else
			{
				radius = NC2Logic::Human_IsJobCarrier(human) ? 63u : 50u;
				source = NC2Logic::Human_GetMapPositionRef(human);
			}
			NC2Logic::CMapPathfinderElement::SearchOverMap(source, radius, true, ls_Highlighting_Update_Tool_PathfinderSearchOverMapFunction_AlwaysGoOn, nullptr, true, -1);
			return ls_Highlighting_Update_Tool_CalculationMapIntoHighlightFlags(rectangle);
		}

		// Original function: FUN_004ca427; merchant queries precede the house-area scan.
		bool HighlightHumanAttachment(const std::int16_t* rectangle, std::uint32_t mode)
		{
			const auto id = Selection_GetSelectedSingleHumanId(gInputManager);
			const void* human = Human(id);
			std::uint32_t position = 0;
			if (!NC2Logic::Human_GetMapPosition(human, &position)) return HighlightInvalidArea(rectangle);
			void* merchant = NC2Logic::Human_GetMerchantDataPtr(human);
			return UpdateHighlightArea(rectangle, [id, merchant, mode] (const void* point) -> std::uint8_t
			{
				const void* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 1u) == 0) return 1;
				const auto houseId = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId);
				void* house = House(houseId);
				bool valid = false;
				switch (mode)
				{
					case SwitchIds::InputMode::AttachHomeHouse: valid = NC2Logic::House_Inhabitants_CanHumanBeAttachedAsHomie(house, id); break;
					case SwitchIds::InputMode::AttachWorkHouse: valid = NC2Logic::House_Inhabitants_CanHumanBeAttachedAsWorker(house, id); break;
					case SwitchIds::InputMode::AttachLearnHouse: valid = NC2Logic::House_Inhabitants_CanHumanBeAttachedAsStudent(house, id); break;
					case SwitchIds::InputMode::AttachBuildHouse: valid = NC2Logic::House_Inhabitants_CanHumanBeAttachedAsBuilder(house, id); break;
					case SwitchIds::InputMode::AttachTradeHouse: valid = merchant != nullptr && NC2Logic::MerchantData_Tool_CanHouseBeAdded(merchant, id, houseId); break;
					case SwitchIds::InputMode::DetachTradeHouse: valid = merchant != nullptr && NC2Logic::MerchantData_IsHouseInArray(merchant, houseId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId)); break;
				}
				return valid ? 3 : 1;
			});
		}

		// Original function: FUN_004cab5f; the six-step ring starts one step in direction four.
		bool HighlightVehicleCoast(const std::int16_t* rectangle)
		{
			const auto player = Player();
			const void* vehicle = Vehicle(Selection_GetSelectedSingleVehicleId(gInputManager));
			const auto continent = Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(NC2Logic::Vehicle_GetMapPositionRef(vehicle)), NC2Layout::MapMicroPointOffset::ContinentId);
			return UpdateHighlightArea(rectangle, [player, continent] (const void* point) -> std::uint8_t
			{
				if (!NC2Logic::GameMap_Exploration_HasBeenSeenOnce(player, point)) return 1;
				const void* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
				if (NC2Logic::GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId)) != 1 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0) return 1;
				auto cursor = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
				NC2Logic::MapMigPoint_MoveCursorUnwrapped(cursor, 4);
				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					NC2Logic::MapMigPoint_MoveCursorUnwrapped(cursor, direction);
					if (!NC2Logic::MapMigPoint_IsBorderPoint(&cursor) && Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(&cursor), NC2Layout::MapMicroPointOffset::ContinentId) == continent) return 3;
				}
				return 1;
			});
		}

		// Original function: FUN_004ca5c5; this outlined helper uses the GUI player, not the selection owner.
		bool HighlightEnemyHouses(const std::int16_t* rectangle)
		{
			const auto player = Player();
			const auto mark = [player] (const void* point) -> std::uint8_t
			{
				const auto* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 1u) == 0) return 1;
				return NC2Logic::Logic_EntryPoint_GetDiplomacyState(player, Read<std::int32_t>(House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId)), NC2Layout::HouseOffset::PlayerId)) == 3 ? 3 : 1;
			};
			return UpdateHighlightArea(rectangle, mark);
		}

		// Original function: FUN_004ca7e8.
		bool HighlightGuideConstruction(const std::int16_t* rectangle)
		{
			const auto player = Player();
			return UpdateHighlightArea(rectangle, [player] (const void* point) -> std::uint8_t
			{
				return NC2Logic::GameMap_Exploration_HasBeenSeenOnce(player, point) && NC2Logic::Tool_Guide_IsAllowedToBeBuild(player, point, true) ? 3 : 1;
			});
		}

		// Original function: FUN_004caf52.
		bool HighlightEnemyLandscapes(const std::int16_t* rectangle)
		{
			const auto player = Player();
			const auto mark = [player] (const void* point) -> std::uint8_t
			{
				const auto* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
				const auto owner = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0 ? Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::HouseId) : -1;
				return owner != -1 && NC2Logic::Logic_EntryPoint_GetDiplomacyState(player, owner) == 3 ? 3 : 1;
			};
			return UpdateHighlightArea(rectangle, mark);
		}

		// Original function: FUN_004ca6c4; seen terrain, placement and nearby human/commander checks remain ordered.
		bool HighlightHouseConstruction(const std::int16_t* rectangle)
		{
			const auto type = Selection_GetSelectedEngineHouseTypeId(gInputManager);
			const auto stage = Selection_GetSelectedEngineHouseExtensionLevel(gInputManager);
			const auto player = Player();
			return UpdateHighlightArea(rectangle, [type, stage, player] (const void* point) -> std::uint8_t
			{
				if (!NC2Logic::GameMap_Exploration_HasBeenSeenOnce(player, point)) return 1;
				if (!NC2Logic::Tool_House_IsAllowedToBeBuild(player, type, stage, Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates), false, true)) return 1;
				return NC2Logic::Tool_Human_IsAnyOfPlayerAround(player, point, 24, true) ? 3 : 1;
			});
		}

		// Original function: FUN_004cacd8; movement space uses the selected vehicle's continent and size.
		bool HighlightVehicleWorkCenter(const std::int16_t* rectangle)
		{
			const void* vehicle = Vehicle(Selection_GetSelectedSingleVehicleId(gInputManager));
			const auto continent = Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(NC2Logic::Vehicle_GetMapPositionRef(vehicle)), NC2Layout::MapMicroPointOffset::ContinentId);
			const auto size = static_cast<std::uint32_t>(NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->logicSize);
			return UpdateHighlightArea(rectangle, [continent, size] (const void* point) -> std::uint8_t
			{
				if (Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId) != continent) return 1;
				return (Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u) >= size ? 3 : 1;
			});
		}

		// Original function: FUN_004cade3; groups can target the complete rectangle.
		bool HighlightVehicleMovement(const std::int16_t* rectangle)
		{
			const auto selection = Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType);
			if (selection == 4)
			{
				const void* position = NC2Logic::Vehicle_GetMapPositionRef(Vehicle(Selection_GetSelectedSingleVehicleId(gInputManager)));
				NC2Logic::GameMap_GetConstMapMicroPointPtr(position);
				return UpdateHighlightArea(rectangle, [position] (const void* point) -> std::uint8_t
				{
					const auto distance = NC2Logic::VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapY), Read<std::int16_t>(position, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(position, NC2Layout::MapMigPointOffset::MapY));
					return static_cast<std::uint32_t>(distance) <= 30u ? 3 : 1;
				});
			}
			return selection == 5 ? UpdateHighlightArea(rectangle, [] (const void*) -> std::uint8_t
			{
				return 3;
			}) : false;
		}
	}

	namespace
	{
		// Original function: FUN_004ca8c1; Windows-only outlined road-start highlighting.
		bool HighlightRoadStart(const std::int16_t* rectangle)
		{
			const auto player = Player(); return UpdateHighlightArea(rectangle, [player] (const void* point) -> std::uint8_t
			{
				return NC2Logic::CMapPathfinderSpecialWallRoadElement::IsPointOkForBuilding(player, true, point) ? 3 : 1;
			});
		}
		// Original function: FUN_004ca97e; clear the visible rectangle before marking reachable endpoints.
		bool HighlightRoadEnd(const std::int16_t* rectangle)
		{
			NC2Logic::GameMap_CalculationMap_Clear(rectangle);
			NC2Logic::Tool_RoadBuildManager_MarkPossibleEndPoints(Player(), static_cast<const std::byte*>(gInputManager) + NC2Layout::InputManagerOffset::RoadStartPoint);
			return ls_Highlighting_Update_Tool_CalculationMapIntoHighlightFlags(rectangle);
		}
		// Original function: FUN_004ca9b1; Windows-only outlined wall-start highlighting.
		bool HighlightWallStart(const std::int16_t* rectangle)
		{
			const auto player = Player(); return UpdateHighlightArea(rectangle, [player] (const void* point) -> std::uint8_t
			{
				return NC2Logic::CMapPathfinderSpecialWallRoadElement::IsPointOkForBuilding(player, false, point) ? 3 : 1;
			});
		}
		// Original function: FUN_004caa70.
		bool HighlightWallEnd(const std::int16_t* rectangle)
		{
			NC2Logic::GameMap_CalculationMap_Clear(rectangle);
			NC2Logic::MapTool_Wall_Build_MarkPossibleEndPoints(Player(), static_cast<const std::byte*>(gInputManager) + NC2Layout::InputManagerOffset::WallStartPoint);
			return ls_Highlighting_Update_Tool_CalculationMapIntoHighlightFlags(rectangle);
		}
		// Original function: FUN_004caaa3; Windows-only outlined gate highlighting.
		bool HighlightWallGate(const std::int16_t* rectangle)
		{
			const auto player = Player(); return UpdateHighlightArea(rectangle, [player] (const void* point) -> std::uint8_t
			{
				return NC2Logic::MapTool_WallGate_IsPossibleMapPosition(player, point) ? 3 : 1;
			});
		}
	}
	// Original function: FUN_004c9e2a; complete native input-mode highlighting dispatcher.
	bool Highlighting_Map_Update(void* manager, const std::int16_t* rectangle)
	{
		// Windows reads its singleton in both the dispatcher and the outlined helpers.
		manager = gInputManager;
		const auto mode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode);
		if (mode == 1) return HighlightHumanMovement(gInputManager, rectangle);
		if (mode == 2 || mode == 3) return HighlightHumanWorkCenter(gInputManager, rectangle, mode == 3);
		if (mode == 10 || mode == 11) return HighlightEnemyHouses(rectangle);
		if (mode == 12) return HighlightHouseConstruction(rectangle);
		if (mode == 13) return HighlightGuideConstruction(rectangle);
		if (mode == 14) return HighlightRoadStart(rectangle);
		if (mode == 15) return HighlightRoadEnd(rectangle);
		if (mode == 16) return HighlightWallStart(rectangle);
		if (mode == 17) return HighlightWallEnd(rectangle);
		if (mode == 18) return HighlightWallGate(rectangle);
		if (mode == 19) return HighlightVehicleCoast(rectangle);
		if (mode == 20) return HighlightVehicleWorkCenter(rectangle);
		if (mode == 21) return HighlightVehicleMovement(rectangle);
		if (mode == 22) return HighlightEnemyLandscapes(rectangle);
		if (mode >= 23 && mode <= 28) return HighlightInvalidArea(rectangle);
		if (mode < 4 || mode > 9) return false;
		return HighlightHumanAttachment(rectangle, mode);
	}

	// Original function: FUN_004b767e; all point modes 1..22 use the native five target rings.
	void Cursor_SetPosition(void* manager, const void* point)
	{
		Write(manager, NC2Layout::InputManagerOffset::CursorMapPoint, Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates));
		auto* cursor = static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::CursorMapPoint;
		NC2Logic::MapMigPoint_ValidatePosition(cursor);
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode) >= 1 && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode) <= 22)
		{
			std::int16_t rectangle[4]{ static_cast<std::int16_t>(Read<std::int16_t>(cursor, NC2Layout::MapMigPointOffset::MapX) - 7), static_cast<std::int16_t>(Read<std::int16_t>(cursor, NC2Layout::MapMigPointOffset::MapY) - 7), 14, 14 };
			// Native SMapMigRectangle construction clips before highlighting.
			NC2Logic::MapMigRectangle_CutInsideMap(rectangle);
			Highlighting_Map_Update(manager, rectangle);
			const auto accept = [&] (const void* candidate)
			{
				if ((Read<std::uint8_t>(NC2E2::EngineMap_GetConstMapMicroPointPtr(candidate), NC2Layout::EngineMapMicroPointOffset::HighlightFlags) & 3u) != 3) return false;
				Write<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent, 1); Write(manager, NC2Layout::InputManagerOffset::AcceptedTargetMapPoint, Read<std::uint32_t>(candidate, NC2Layout::MapMigPointOffset::Coordinates)); return true;
			};
			if (accept(cursor)) return;
			Write<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent, 0);
			auto candidate = Read<std::uint32_t>(cursor, NC2Layout::MapMigPointOffset::Coordinates);
			for (std::uint32_t radius = 1; radius <= 5; ++radius)
			{
				NC2Logic::MapMigPoint_MoveCursorWrapped(&candidate, 4);
				for (std::uint32_t direction = 0; direction < 6; ++direction)
					for (std::uint32_t step = 0; step < radius; ++step)
					{
						if (NC2Logic::MapMigPoint_IsInsideMap(&candidate) && accept(&candidate)) return;
						NC2Logic::MapMigPoint_MoveCursorWrapped(&candidate, direction);
					}
			}
			return;
		}
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode) != 0) return;
		const bool valid = NC2Logic::MapMigPoint_IsInsideMap(cursor) && !NC2Logic::MapMigPoint_IsBorderPoint(cursor);
		Write<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent, valid);
		if (valid) Write(manager, NC2Layout::InputManagerOffset::AcceptedTargetMapPoint, Read<std::uint32_t>(cursor, NC2Layout::MapMigPointOffset::Coordinates));
	}

	namespace
	{
		// Windows 004B790D/004B799D/004B7A7A inline the same selection-owner lookup.
		std::int32_t SelectedAttackPlayer(const void* manager)
		{
			switch (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType))
			{
				case SwitchIds::SelectionType::SingleHuman: return Read<std::int32_t>(Human(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId)), NC2Layout::HumanOffset::PlayerId);
				case SwitchIds::SelectionType::MultipleHumans: return Read<std::int32_t>(Human(Selection_GetSelectedMultipleHumanId(manager, 0)), NC2Layout::HumanOffset::PlayerId);
				case SwitchIds::SelectionType::SingleVehicle: return Read<std::int32_t>(Vehicle(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId)), NC2Layout::VehicleOffset::PlayerId);
				case SwitchIds::SelectionType::MultipleVehicles: return Read<std::int32_t>(Vehicle(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroupCount) != 0 ? Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroup) : NXBasics::IntegerRange::UInt32Maximum), NC2Layout::VehicleOffset::PlayerId);
				default: return -1;
			}
		}

		void QueueHumanTarget(std::int32_t humanId, std::uint8_t opcode, std::uint32_t first, std::uint32_t second, bool respond)
		{
			std::byte command[20]{ };
			Write(command, NC2Layout::UserNetworkCommandOffset::Opcode, opcode); Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, humanId); Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, first); Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, second);
			NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, command);
			if (respond) NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, humanId);
		}

		void QueueSelectedHumanTargets(void* manager, std::uint8_t opcode, std::uint32_t targetId, const void* target)
		{
			const auto send = [&] (std::int32_t humanId)
			{
				const auto first = target != nullptr ? targetId : NC2Logic::MapMigPoint_GetLong(static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint);
				QueueHumanTarget(humanId, opcode, first, target != nullptr ? Read<std::uint32_t>(target, NC2Layout::ActorIdentityOffset::UniqueId) : 0u, false);
			};
			if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 1)
			{
				send(Selection_GetSelectedSingleHumanId(manager));
				NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Selection_GetSelectedSingleHumanId(manager));
			}
			for (std::uint32_t index = 0; Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 2 && index < Selection_GetSelectedMultipleHumanNumber(manager); ++index)
			{
				send(Selection_GetSelectedMultipleHumanId(manager, index));
				NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Selection_GetSelectedMultipleHumanId(manager, index));
			}
		}
	}

	// Original function: FUN_004b790d.
	bool Misc_House_IsEnemyToAttack(const void* manager, std::int32_t id)
	{
		const auto player = SelectedAttackPlayer(manager);
		return player != -1 && NC2Logic::Logic_EntryPoint_GetDiplomacyState(player, Read<std::int32_t>(House(id), NC2Layout::HouseOffset::PlayerId)) == 3;
	}

	// Original function: FUN_004b799d; children are not attack targets.
	bool Misc_Human_IsEnemyToAttack(const void* manager, std::int32_t id)
	{
		const auto player = SelectedAttackPlayer(manager);
		return player != -1 && NC2Logic::Human_IsAgeAdult(Human(id)) && NC2Logic::Logic_EntryPoint_GetDiplomacyState(player, Read<std::int32_t>(Human(id), NC2Layout::HumanOffset::PlayerId)) == 3;
	}

	// Original function: FUN_004b7a38; a group uses its first member for eligibility.
	bool Misc_Animal_CanBeAttacked(const void* manager, std::int32_t id, bool allowOwnAnimals)
	{
		auto human = Selection_GetSelectedSingleHumanId(manager);
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 2) human = Selection_GetSelectedMultipleHumanId(manager, 0);
		return human != -1 && NC2Logic::Animal_Misc_CanBeAttackedByHuman(Animal(id), human, allowOwnAnimals);
	}

	// Original function: FUN_004b7a7a.
	bool Misc_Vehicle_IsEnemyToAttack(const void* manager, std::int32_t id)
	{
		const auto player = SelectedAttackPlayer(manager);
		return player != -1 && NC2Logic::Logic_EntryPoint_GetDiplomacyState(player, Read<std::int32_t>(Vehicle(id), NC2Layout::VehicleOffset::PlayerId)) == 3;
	}

	// Original function: FUN_004b7b08.
	bool Misc_Vehicle_IsToEnter(const void* manager, std::int32_t id)
	{
		const auto selection = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
		if (selection == 1)
		{
			const auto human = Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId);
			return human != -1 && NC2Logic::Vehicle_Passengers_CanHumanBeAttached(Vehicle(id), human);
		}
		if (selection == 4)
		{
			const auto vehicle = Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId);
			return vehicle != -1 && NC2Logic::Vehicle_Passengers_CanVehicleBeAttached(Vehicle(id), vehicle);
		}
		return false;
	}
	// Original function: FUN_004cb04d.
	void Highlighting_Humans_Update(void* manager, bool force, bool clear)
	{
		const auto mode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode);
		if (!force && mode != 23 && mode != 24) return;
		NC2Logic::CHumanIterator iterator;
		for (auto id = iterator.GetFirstHumanId(); id != -1; id = iterator.GetNextHumanId())
		{
			const auto currentMode = Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::InputMode);
			const bool allowed = !clear && currentMode >= 23 && currentMode <= 24 && Misc_Human_IsEnemyToAttack(manager, id);
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0xE, true, id, allowed ? 12 : 8, 0);
		}
	}

	// Original function: FUN_004cb0c8.
	void Highlighting_Animals_Update(void* manager, bool force, bool clear)
	{
		if (!force && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode) != 25) return;
		const auto* array = NC2Logic::CAnimalArray::sTheObjectPtr;
		for (auto id = array->mFirstScanIndex; id <= array->mLastScanIndex; ++id)
		{
			if (Read<std::uint8_t>(Animal(id), NC2Layout::AnimalOffset::AllocatedFlag) == 0) continue;
			const bool allowed = !clear && Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode) == 25 && Misc_Animal_CanBeAttacked(manager, id, false);
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0xF, true, id, allowed ? 12 : 8, 0);
		}
	}

	// Original function: FUN_004cb137.
	void Highlighting_Vehicles_Update(void* manager, bool force, bool clear)
	{
		const auto mode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode);
		if (!force && mode != 26 && mode != 27 && mode != 28) return;
		// The unfiltered Windows iterator scans allocation bytes in ascending array order.
		const auto* array = NC2Logic::CVehicleArray::sTheObjectPtr;
		for (auto id = array->mFirstScanIndex; id <= array->mLastScanIndex; ++id)
		{
			if (Read<std::uint8_t>(Vehicle(id), NC2Layout::VehicleOffset::AllocatedFlag) == 0) continue;
			const auto currentMode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode);
			const bool allowed = !clear && (currentMode == 27 ? Misc_Vehicle_IsToEnter(manager, id) : (currentMode == 26 || currentMode == 28) && Misc_Vehicle_IsEnemyToAttack(manager, id));
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0x10, true, id, allowed ? 12 : 8, 0);
		}
	}
	// Original function: FUN_004b4bd6; complete native input-mode command dispatcher.
	bool Action_DoInputSelect(void* manager, const void* object)
	{
		const auto mode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode);
		if (mode == 12)
		{
			if (Read<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent) == 0) return false;
			std::byte command[20]{ };
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 117); Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Player()));
			Write(command, NC2Layout::HouseBuildCommandOffset::HouseType, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseType)); Write(command, NC2Layout::HouseBuildCommandOffset::DevelopmentStage, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseDevelopmentStage));
			Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, NC2Logic::MapMigPoint_GetLong(static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint));
			Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseSpecialItemType) << 16) | (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::HouseSpecialItem) & NXBasics::IntegerRange::UInt16Maximum));
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, command, false, false);
			InputMode_Cancel(manager); return true;
		}
		if (mode == 19 || mode == 20 || mode == 21)
		{
			if (Read<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent) == 0) return false;
			const auto* point = static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint;
			if (mode == 20) Misc_Tool_SendSelectedVehicle(manager, Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates));
			else
			{
				const auto send = [&] (std::int32_t id)
				{
					std::byte command[20]{ };
					Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, mode == 19 ? 103 : 108); Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
					Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, NC2Logic::MapMigPoint_GetLong(point));
					NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, command);
				};
				if (mode == 19) send(Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId));
				else
				{
					if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 4) send(Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleVehicleId));
					for (std::uint32_t index = 0; Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 5 && index < Selection_GetSelectedMultipleVehiclesNumber(manager); ++index) send(Selection_GetSelectedMultipleVehicleId(manager, index));
				}
			}
			InputMode_Cancel(manager); return true;
		}
		if (mode == 11 || mode == 24 || mode == 28)
		{
			std::int32_t targetId;
			const void* target;
			if (mode == 11)
			{
				if (Read<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent) == 0) return false;
				const auto* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint);
				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 1u) == 0) return false;
				targetId = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId);
				if (!Misc_House_IsEnemyToAttack(manager, targetId)) return false;
				target = House(targetId);
			}
			else
			{
				if (object == nullptr || Read<std::uint32_t>(object, NC2Layout::DisplayObjectReferenceOffset::Type) != (mode == 24 ? 1u : 3u)) return false;
				targetId = Read<std::int32_t>(object, mode == SwitchIds::InputMode::VehicleAttackHuman ? NC2Layout::DisplayObjectReferenceOffset::HumanId : NC2Layout::DisplayObjectReferenceOffset::VehicleId);
				if (!(mode == 24 ? Misc_Human_IsEnemyToAttack(manager, targetId) : Misc_Vehicle_IsEnemyToAttack(manager, targetId))) return false;
				target = mode == 24 ? Human(targetId) : Vehicle(targetId);
			}
			const auto send = [&] (std::int32_t id)
			{
				std::byte command[20]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, mode == 11 ? 105 : mode == 24 ? 104 : 107); Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
				Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, targetId); Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(target, NC2Layout::ActorIdentityOffset::UniqueId));
				NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, command);
			};
			if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 4) send(Selection_GetSelectedSingleVehicleId(manager));
			for (std::uint32_t index = 0; Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType) == 5 && index < Selection_GetSelectedMultipleVehiclesNumber(manager); ++index) send(Selection_GetSelectedMultipleVehicleId(manager, index));
			InputMode_Cancel(manager); return true;
		}
		if (mode >= 14 && mode <= 18)
		{
			if (Read<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent) == 0) return false;
			auto* point = static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint;
			if (mode == 14 || mode == 16)
			{
				Write(manager, mode == SwitchIds::InputMode::RoadStart ? NC2Layout::InputManagerOffset::RoadStartPoint : NC2Layout::InputManagerOffset::WallStartPoint, Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates));
				InputMode_Set(manager, mode == 14 ? 15 : 17); return true;
			}
			std::byte command[20]{ };
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, mode == 15 ? 0x76 : mode == 17 ? 0x78 : 0x7B); Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Player()));
			if (mode == 18) Write(command, NC2Layout::ConstructionLineCommandOffset::FirstPoint, NC2Logic::MapMigPoint_GetLong(point));
			else
			{
				Write(command, NC2Layout::ConstructionLineCommandOffset::FirstPoint, NC2Logic::MapMigPoint_GetLong(static_cast<std::byte*>(manager) + (mode == 15 ? NC2Layout::InputManagerOffset::RoadStartPoint : NC2Layout::InputManagerOffset::WallStartPoint))); Write(command, NC2Layout::ConstructionLineCommandOffset::SecondPoint, NC2Logic::MapMigPoint_GetLong(point));
			}
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, command, false, false);
			InputMode_Cancel(manager); return true;
		}
		if (mode == 3)
		{
			InputMode_Cancel(manager); return true;
		}
		if (mode == 1 || mode == 2 || mode == 13 || mode == 22)
		{
			if (Read<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent) == 0) return false;
			const auto* point = static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint;
			const auto packed = NC2Logic::MapMigPoint_GetLong(point);
			if (mode == 1) Misc_Tool_SendSelectedHuman(manager, point);
			else if (mode == 2)
			{
				QueueHumanTarget(Selection_GetSelectedSingleHumanId(manager), 17, packed, 0, false);
				NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Selection_GetSelectedSingleHumanId(manager));
			}
			else if (mode == 13)
			{
				QueueHumanTarget(Selection_GetSelectedSingleHumanId(manager), 4, packed, 0, false);
				QueueHumanTarget(Selection_GetSelectedSingleHumanId(manager), 19, NC2Logic::MapMigPoint_GetLong(point), 0, false);
				NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Selection_GetSelectedSingleHumanId(manager));
			}
			else QueueSelectedHumanTargets(manager, 47, 0, nullptr);
			InputMode_Cancel(manager);
			return true;
		}
		if (mode == 10)
		{
			if (Read<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent) == 0) return false;
			const auto* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 1u) == 0) return false;
			const auto id = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId);
			if (!Misc_House_IsEnemyToAttack(manager, id)) return false;
			QueueSelectedHumanTargets(manager, 44, id, House(id));
			InputMode_Cancel(manager);
			return true;
		}
		if (mode == 23 || mode == 25 || mode == 26)
		{
			const auto type = mode == 23 ? 1u : mode == 25 ? 2u : 3u;
			if (object == nullptr || Read<std::uint32_t>(object, NC2Layout::DisplayObjectReferenceOffset::Type) != type) return false;
			const auto id = Read<std::int32_t>(object, type == SwitchIds::MapMoveableType::Human ? NC2Layout::DisplayObjectReferenceOffset::HumanId : type == SwitchIds::MapMoveableType::Animal ? NC2Layout::DisplayObjectReferenceOffset::AnimalId : NC2Layout::DisplayObjectReferenceOffset::VehicleId);
			const bool allowed = mode == 23 ? Misc_Human_IsEnemyToAttack(manager, id) : mode == 25 ? Misc_Animal_CanBeAttacked(manager, id, true) : Misc_Vehicle_IsEnemyToAttack(manager, id);
			if (!allowed) return false;
			const auto* target = mode == 23 ? Human(id) : mode == 25 ? Animal(id) : Vehicle(id);
			QueueSelectedHumanTargets(manager, mode == 23 ? 43 : mode == 25 ? 45 : 46, id, target);
			InputMode_Cancel(manager);
			return true;
		}
		if (mode == 27)
		{
			if (object == nullptr || Read<std::uint32_t>(object, NC2Layout::DisplayObjectReferenceOffset::Type) != 3) return false;
			const auto id = Read<std::int32_t>(object, NC2Layout::DisplayObjectReferenceOffset::VehicleId);
			if (!Misc_Vehicle_IsToEnter(manager, id)) return false;
			const auto selection = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
			if (selection == 1 || selection == 4)
			{
				std::byte command[20]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, selection == 1 ? 38 : 113); Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(manager, selection == SwitchIds::SelectionType::SingleHuman ? NC2Layout::InputManagerOffset::SingleHumanId : NC2Layout::InputManagerOffset::SingleVehicleId));
				Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, id); Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::UniqueId));
				if (selection == 1)
				{
					NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, command);
					NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId));
				}
				else NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, command);
			}
			InputMode_Cancel(manager);
			return true;
		}
		if ((mode != 4 && mode != 5 && mode != 6 && mode != 7 && mode != 8 && mode != 9) || Read<std::uint8_t>(manager, NC2Layout::InputManagerOffset::AcceptedTargetPresent) == 0) return false;
		bool selectSchoolJob = false;
		const auto* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint);
		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 1u) != 0)
		{
			const auto id = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId);
			std::byte command[20]{ };
			const auto opcode = mode == 4 ? 26 : mode == 5 ? 28 : mode == 6 ? 32 : mode == 7 ? 30 : mode == 8 ? 36 : 37;
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, static_cast<std::uint8_t>(opcode)); Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId));
			Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1, id); Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(House(id), NC2Layout::HouseOffset::UniqueId));
			NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, command);
			selectSchoolJob = mode == 6 && Read<std::uint32_t>(House(id), NC2Layout::HouseOffset::LogicType) == 38;
			NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::SingleHumanId));
		}
		InputMode_Cancel(manager);
		if (selectSchoolJob) NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->MiscWindows_HumanLearnJobWindow_Open();
		return true;
	}

	// Original function: FUN_004b5cff; object is deliberately unused by Windows.
	bool Action_DoDefault(void* manager, const void*)
	{
		if (Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::InputMode) != 0) return false;
		const auto mode = Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType);
		if (mode != 1 && mode != 2 && mode != 4 && mode != 5) return false;
		auto* cursor = static_cast<std::byte*>(manager) + NC2Layout::InputManagerOffset::CursorMapPoint;
		std::uint32_t position = 0;
		if (mode == 1 || mode == 4)
		{
			const auto player = Player();
			if (player >= 16 || (Read<std::uint16_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(cursor), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) != 0) return false;
		}
		if (mode == 4)
		{
			Misc_Tool_SendSelectedVehicle(manager, Read<std::uint32_t>(cursor, NC2Layout::MapMigPointOffset::Coordinates));
			return true;
		}
		if (mode == 5)
		{
			NC2Logic::Vehicle_GetMapPosition(Vehicle(Read<std::uint32_t>(manager, NC2Layout::InputManagerOffset::VehicleGroup)), &position);
			if (Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(cursor), NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(&position), NC2Layout::MapMicroPointOffset::ContinentId)) return false;
			Misc_Tool_SendSelectedVehicle(manager, Read<std::uint32_t>(cursor, NC2Layout::MapMigPointOffset::Coordinates));
			return true;
		}
		const void* human = Human(Read<std::uint32_t>(manager, mode == SwitchIds::SelectionType::SingleHuman ? NC2Layout::InputManagerOffset::SingleHumanId : NC2Layout::InputManagerOffset::HumanGroup));
		if (!NC2Logic::Human_GetMapPosition(human, &position)) return false;
		if (Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(cursor), NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(&position), NC2Layout::MapMicroPointOffset::ContinentId)) return false;
		Misc_Tool_SendSelectedHuman(manager, cursor);
		return true;
	}

	// Original function: FUN_004b5e91.
	bool Action_DoCancel(void* manager, const void*)
	{
		const auto mode = Read<std::int32_t>(manager, NC2Layout::InputManagerOffset::InputMode);
		if (mode == 0)
		{
			Selection_UnselectAll(manager); return false;
		}
		if (mode > 0 && mode <= 28) InputMode_Cancel(manager);
		return true;
	}

	// Original function: FUN_004b6852; area hit testing uses every current animation bob.
	bool Selection_SelectHumansInArea(void* manager, const void* worldElement, NXBasics::SRectangle rectangle, std::uint32_t job, bool add, bool remove)
	{
		rectangle.Validate();
		const auto* display = Read<void*>(worldElement, NC2Layout::WorldGuiElementOffset::Display);
		std::int16_t area[4]{ };
		NC2E2::DisplayDescription_GetMapRectangle(display, rectangle.x, rectangle.y, rectangle.width, rectangle.height, area);
		area[0] = static_cast<std::int16_t>(area[0] - 5); area[2] = static_cast<std::int16_t>(area[2] + 10);
		area[1] = static_cast<std::int16_t>(area[1] - 4); area[3] = static_cast<std::int16_t>(area[3] + 24);
		NC2Logic::MapMigRectangle_CutInsideMap(area);
		const bool military = job >= 42 && job <= 47;
		if (military) job = 0;
		std::uint32_t ids[300], count = 0;
		NC2Logic::CMapMoveableIterator iterator;
		for (std::int32_t y = area[1]; y <= static_cast<std::int16_t>(area[1] + area[3] - 1) && count != 300; ++y)
		{
			for (std::int32_t x = area[0]; x <= static_cast<std::int16_t>(area[0] + area[2] - 1) && count != 300; ++x)
			{
				const std::int16_t point[2]{ static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) };
				std::int32_t pixel[2];
				NC2E2::DisplayDescription_MapMigToWorldPixelCoordinates(display, Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates), pixel, true);
				for (auto id = iterator.GetFirstMapMoveableId(point); id != -1 && count != 300; id = iterator.GetNextMapMoveableId())
				{
					const auto* moveable = NC2Logic::gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;
					if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) != 1) continue;
					const auto humanId = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
					const auto* human = Human(humanId);
					const auto humanJob = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) != Player() || NC2Logic::Human_Misc_IsHidden(human) || (job != 0 && humanJob != job) || (military && humanJob < 42) || humanJob > 47) continue;
					NC2E2::S2DEngineDisplaySearchObjectBobData bobs;
					NC2E2::DrawAtom_MapMoveable_GetBobData(static_cast<std::uint32_t>(id), display, &bobs, Player());
					std::uint32_t bob = 0;
					while (bob < bobs.count)
					{
						const auto& data = bobs.bobs[bob];
						const auto* bobRectangle = NXBasics::BobManager_GetBobAreaRectanglePtr(data.manager, data.id);
						if (bobRectangle != nullptr)
						{
							auto bobArea = *static_cast<const NXBasics::SRectangle*>(bobRectangle);
							bobArea.x += data.offsetX + pixel[0]; bobArea.y += data.offsetY + pixel[1];
							if (bobArea.IsTouching(rectangle)) break;
						}
						++bob;
					}
					if (bob != bobs.count) ids[count++] = humanId;
				}
			}
		}
		if (add) for (std::uint32_t i = 0; i < count; ++i) Selection_SelectAddSingleHuman(manager, ids[i]);
		else if (remove) for (std::uint32_t i = 0; i < count; ++i) UnSelect_Human(manager, ids[i]);
		else if (count == 1) return Selection_SelectSingleHuman(manager, ids[0]);
		else if (count > 1)
		{
			Selection_UnselectAll(manager); InputMode_Set(manager, 0);
			Write<std::uint32_t>(manager, NC2Layout::InputManagerOffset::SelectionType, 2); Write(manager, NC2Layout::InputManagerOffset::HumanGroupCount, count);
			for (std::uint32_t i = 0; i < count; ++i)
			{
				Write(manager, NC2Layout::InputManagerOffset::HumanGroupUnsigned + i * NC2Layout::RecordSize::InputSelectedObjectUnsigned, ids[i]); Write(manager, NC2Layout::InputManagerOffset::HumanGroupUniqueIdsUnsigned + i * NC2Layout::RecordSize::InputSelectedObjectUnsigned, Read<std::uint32_t>(Human(ids[i]), NC2Layout::HumanOffset::UniqueId));
			}
			Changed(manager);
			for (std::uint32_t i = 0; i < count; ++i) Inform(0xE, ids[i], Read<std::uint32_t>(Human(ids[i]), NC2Layout::HumanOffset::UniqueId), true);
		}
		return true;
	}
}

// ABI adapter used by the existing GUI constructors and command handlers.
// Original function: FUN_004a2f54; distinct native singleton-forwarding entry.
bool __cdecl C2IGG_EntryPoint_InputManager_Highlighting_Update(const std::int16_t* rectangle)
{
	return NC2InGameGuiInputManager::Highlighting_Map_Update(NC2InGameGuiInputManager::gInputManager, rectangle);
}
bool __fastcall InputManager_InputMode_Set(void* manager, void*, int mode)
{
	return NC2InGameGuiInputManager::InputMode_Set(manager, mode);
}

// Original function: FUN_004b77e8; tick validation also refreshes all attack target highlights.
bool __cdecl InputManager_ls_Callback_Logic(unsigned int type, std::uintptr_t, std::uintptr_t callbackValueId, std::uintptr_t callbackValueCreated, std::uintptr_t)
{
	std::uint32_t id = static_cast<std::uint32_t>(callbackValueId);
	std::uint32_t created = static_cast<std::uint32_t>(callbackValueCreated);
	using namespace NC2InGameGuiInputManager;
	auto* manager = gInputManager;
	if (type == 1)
	{
		Selection_Validate(manager); Highlighting_Humans_Update(manager, false, false); Highlighting_Animals_Update(manager, false, false); Highlighting_Vehicles_Update(manager, false, false);
	}
	else if (type == 22)
	{
		Selection_Validate(manager); Selection_UnselectAll(manager); const std::uint32_t point = 0; Cursor_SetPosition(manager, &point);
	}
	else if (created == 0)
	{
		switch (type)
		{
			case SwitchIds::CallbackType::HumanLifetime: UnSelect_Human(manager, id); break;
			case SwitchIds::CallbackType::AnimalLifetime: UnSelect_Animal(manager, id); break;
			case SwitchIds::CallbackType::VehicleLifetime: UnSelect_Vehicle(manager, id); break;
			case SwitchIds::CallbackType::HouseLifetime: UnSelect_House(manager, id); break;
			case SwitchIds::CallbackType::GuideLifetime: UnSelect_Guide(manager, id); break;
			default: break;
		}
	}
	return true;
}

// Original function: FUN_004a0d01; preserve the fourteen desktop element notifications.
void __fastcall InGameGuiManager_InputMode_Changed(void* manager)
{
	const bool normal = NC2Lifetime::Read<std::uint32_t>(NC2InGameGuiInputManager::gInputManager, NC2Layout::InputManagerOffset::InputMode) == 0;
	auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
	for (std::uint32_t offset = 0xC; offset <= 0x40; offset += 4)
	{
		auto* element = NC2Lifetime::Read<NXBaseGui::CBaseElement*>(manager, offset);
		if (normal) desktop->Element_UnHide(element); else desktop->Element_Hide(element);
	}
	NC2InGameGuiBase::BaseToolDesktop_SelectionActiveChanged();
}

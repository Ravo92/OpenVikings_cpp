#include "ApplicationGlobals.h"
#include "XTextEncoding.h"
#include "C2CallbackManager.h"
#include "C2EngineObjectData.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2PlayerCenterPosition.h"
#include "C2RuntimeConstants.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSupportAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"

#include <cstdint>
#include <cstring>
#include <initializer_list>

// Logic callbacks retain their original registration order.
extern bool __cdecl InputManager_ls_Callback_Logic(std::uint32_t, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);

namespace
{
	using namespace NC2Lifetime;
	// Complete one-byte InputManager::mStaticVars at 0056940C. Only its
	// constructor write is present in the Windows export; its field is unnamed.
	std::uint8_t gInputManagerStaticVars = 0;

	// Original function: FUN_004b476c; embedded input selection construction.
	void ConstructInputSelection(void* selection)
	{
		Write<std::uint16_t>(selection, NC2Layout::InputSelectionMemberOffset::LandscapeMapPointY, 0);
		Write<std::uint16_t>(selection, NC2Layout::InputSelectionMemberOffset::LandscapeMapPointX, 0);
		Write<std::uint16_t>(selection, NC2Layout::InputSelectionMemberOffset::RoadBuildPointY, 0);
		Write<std::uint16_t>(selection, NC2Layout::InputSelectionMemberOffset::RoadBuildPointX, 0);
		Write<std::uint16_t>(selection, NC2Layout::InputSelectionMemberOffset::WallBuildPointY, 0);
		Write<std::uint16_t>(selection, NC2Layout::InputSelectionMemberOffset::WallBuildPointX, 0);
	}

	// Original function: FUN_004b47bf; complete 300-point array.
	void ConstructInputPoints(void* storage)
	{
		for (std::uint32_t index = 0; index < NC2Runtime::InputTargetPointLimits::Points; ++index)
		{
			Write<std::uint16_t>(storage, NC2Layout::InputTargetPointListOffset::Points + index * NC2Layout::RecordSize::MapMigPointUnsigned + NC2Layout::MapMigPointOffset::MapYUnsigned, 0);
			Write<std::uint16_t>(storage, NC2Layout::InputTargetPointListOffset::Points + index * NC2Layout::RecordSize::MapMigPointUnsigned, 0);
		}
	}

	// Original function: FUN_004b479f.
	void ConstructInputPointState(void* storage)
	{
		Write<std::uint16_t>(storage, NC2Layout::InputRoadWallPointStateOffset::RoadStartPointY, 0);
		Write<std::uint16_t>(storage, NC2Layout::InputRoadWallPointStateOffset::RoadStartPointX, 0);
		Write<std::uint16_t>(storage, NC2Layout::InputRoadWallPointStateOffset::WallStartPointY, 0);
		Write<std::uint16_t>(storage, NC2Layout::InputRoadWallPointStateOffset::WallStartPointX, 0);
		ConstructInputPoints(static_cast<std::byte*>(storage) + NC2Layout::InputRoadWallPointStateOffset::TargetPoints);
	}

	// Original function: FUN_004b473b.
	void ConstructInputMembers(void* storage)
	{
		auto* bytes = static_cast<std::byte*>(storage);
		ConstructInputSelection(bytes + NC2Layout::InputManagerOffset::SelectionType);
		Write<std::uint16_t>(storage, NC2Layout::InputManagerOffset::CursorMapPointY, 0);
		Write<std::uint16_t>(storage, NC2Layout::InputManagerOffset::CursorMapPoint, 0);
		Write<std::uint16_t>(storage, NC2Layout::InputManagerOffset::AcceptedTargetMapPointY, 0);
		Write<std::uint16_t>(storage, NC2Layout::InputManagerOffset::AcceptedTargetMapPoint, 0);
		ConstructInputPointState(bytes + NC2Layout::InputManagerOffset::RoadStartPoint);
	}

	void RegisterCallbacks(NC2Logic::TCallbackFunction callback, std::initializer_list<std::uint32_t> types, std::int32_t priority)
	{
		for (auto type : types)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(type, callback, 0, priority);
		}
	}

	void UnregisterCallbacks(NC2Logic::TCallbackFunction callback, std::initializer_list<std::uint32_t> types)
	{
		for (auto type : types)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->UnRegisterCallback(type, callback, 0);
		}
	}
}

namespace NC2InGameGuiGroups
{
	// Original function: FUN_004a32c2.
	void CObjectGroupManager::Group_VerifyAllObjects()
	{
		for (std::uint32_t group = 0; group < NC2Runtime::ObjectGroupLimits::Groups; ++group)
		{
			l_Group_VerifyAllObjects(mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + group * NC2Layout::RecordSize::ObjectGroupUnsigned);
		}
	}

	// Original function: FUN_004a32e2.
	void CObjectGroupManager::Group_CreateNewGroup(std::uint32_t index)
	{
		if (index >= NC2Runtime::ObjectGroupLimits::Groups)
		{

			return;
		}

		Write<std::uint32_t>(mStorage, NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + index * NC2Layout::RecordSize::ObjectGroupUnsigned, 0);
		Group_AddSelectionToGroup(index);
	}

	// Original function: FUN_004a3301.
	void CObjectGroupManager::Group_AddSelectionToGroup(std::uint32_t index)
	{
		using namespace NC2InGameGuiInputManager;

		if (index >= NC2Runtime::ObjectGroupLimits::Groups)
		{

			return;
		}

		Group_VerifyAllObjects();
		void* group = mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + index * NC2Layout::RecordSize::ObjectGroupUnsigned;
		auto* input = gInputManager;

		switch (Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::SelectionType))
		{
			case SwitchIds::SelectionType::SingleHuman:
				l_Group_AddHuman(group, Selection_GetSelectedSingleHumanId(input));
				break;
			case SwitchIds::SelectionType::MultipleHumans:
				for (std::uint32_t i = 0; i < Selection_GetSelectedMultipleHumanNumber(input); ++i)
				{
					l_Group_AddHuman(group, Selection_GetSelectedMultipleHumanId(input, i));
				}

				break;
			case SwitchIds::SelectionType::SingleVehicle:
				l_Group_AddVehicle(group, Selection_GetSelectedSingleVehicleId(input));
				break;
			case SwitchIds::SelectionType::MultipleVehicles:
				for (std::uint32_t i = 0; i < Selection_GetSelectedMultipleVehiclesNumber(input); ++i)
				{
					l_Group_AddVehicle(group, Selection_GetSelectedMultipleVehicleId(input, i));
				}

				break;
			case SwitchIds::SelectionType::House:
				l_Group_AddHouse(group, Selection_GetSelectedHouseId(input));
				break;
			default:
				break;
		}
	}

	// Original function: FUN_004a340e.
	void CObjectGroupManager::Group_Choose(std::uint32_t index)
	{
		using namespace NC2InGameGuiInputManager;

		if (index >= NC2Runtime::ObjectGroupLimits::Groups)
			return;

		Group_VerifyAllObjects();
		const void* group = mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + index * NC2Layout::RecordSize::ObjectGroupUnsigned;

		if (l_Group_IsSelected(group))
		{
			std::uint32_t point = 0;

			if (l_Group_GetPosition(group, &point))
				NC2InGameGuiEntryPoints::C2IGG_EntryPoint_MainDisplay_SetToMapPosition(&point);

			return;
		}

		Selection_UnselectAll(gInputManager);
		InputMode_Cancel(gInputManager);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count); ++i)
		{
			const auto id = Read<std::int32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + i * NC2Layout::RecordSize::ObjectGroupObjectUnsigned);

			switch (Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + i * NC2Layout::RecordSize::ObjectGroupObjectUnsigned))
			{
				case SwitchIds::ObjectGroupType::Human:
					Selection_SelectAddSingleHuman(gInputManager, id);
					break;
				case SwitchIds::ObjectGroupType::Vehicle:
					Selection_SelectAddSingleVehicle(gInputManager, id);
					break;
				case SwitchIds::ObjectGroupType::House:
					Selection_SelectHouse(gInputManager, id);
					break;
				default:
					break;
			}
		}
	}

	// Original function: FUN_004a358d; an object belongs to at most one group.
	void CObjectGroupManager::l_Group_AddHuman(void* group, std::uint32_t id)
	{
		for (std::uint32_t i = 0; i < NC2Runtime::ObjectGroupLimits::Groups; ++i)
		{
			l_Group_RemoveHuman(mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + i * NC2Layout::RecordSize::ObjectGroupUnsigned, id);
		}

		if (Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count) == 0 || Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectType) != SwitchIds::ObjectGroupType::Human)
			Write<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count, 0);

		const auto count = Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count);

		if (count == NC2Runtime::ObjectGroupLimits::Objects)
		{

			return;
		}

		Write<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + count * NC2Layout::RecordSize::ObjectGroupObjectUnsigned, SwitchIds::ObjectGroupType::Human);
		Write(group, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + count * NC2Layout::RecordSize::ObjectGroupObjectUnsigned, id);
		Write(group, NC2Layout::ObjectGroupOffset::FirstObjectUniqueIdUnsigned + count * NC2Layout::RecordSize::ObjectGroupObjectUnsigned, Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars + id * NC2Logic::CHumanArray::ElementSize, NC2Layout::HumanOffset::UniqueId));
		Write(group, NC2Layout::ObjectGroupOffset::Count, count + 1);
	}

	// Original function: FUN_004a363a.
	void CObjectGroupManager::l_Group_AddVehicle(void* group, std::uint32_t id)
	{
		for (std::uint32_t i = 0; i < NC2Runtime::ObjectGroupLimits::Groups; ++i)
		{
			l_Group_RemoveVehicle(mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + i * NC2Layout::RecordSize::ObjectGroupUnsigned, id);
		}

		if (Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count) == 0 || Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectType) != SwitchIds::ObjectGroupType::Vehicle)
			Write<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count, 0);

		const auto count = Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count);

		if (count == NC2Runtime::ObjectGroupLimits::Objects)
		{

			return;
		}

		Write<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + count * NC2Layout::RecordSize::ObjectGroupObjectUnsigned, SwitchIds::ObjectGroupType::Vehicle);
		Write(group, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + count * NC2Layout::RecordSize::ObjectGroupObjectUnsigned, id);
		Write(group, NC2Layout::ObjectGroupOffset::FirstObjectUniqueIdUnsigned + count * NC2Layout::RecordSize::ObjectGroupObjectUnsigned, Read<std::uint32_t>(NC2Logic::CVehicleArray::mStaticVars + id * NC2Logic::CVehicleArray::ElementSize, NC2Layout::VehicleOffset::UniqueId));
		Write(group, NC2Layout::ObjectGroupOffset::Count, count + 1);
	}

	// Original function: FUN_004a36e7; houses replace the complete group.
	void CObjectGroupManager::l_Group_AddHouse(void* group, std::uint32_t id)
	{
		for (std::uint32_t i = 0; i < NC2Runtime::ObjectGroupLimits::Groups; ++i)
		{
			l_Group_RemoveHouse(mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + i * NC2Layout::RecordSize::ObjectGroupUnsigned, id);
		}

		Write<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count, 0);
		Write(group, NC2Layout::ObjectGroupOffset::FirstObjectId, id);
		Write<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectType, SwitchIds::ObjectGroupType::House);
		Write(group, NC2Layout::ObjectGroupOffset::FirstObjectUniqueId, Read<std::uint32_t>(NC2Logic::CHouseArray::mStaticVars + id * NC2Logic::CHouseArray::ElementSize, NC2Layout::HouseOffset::UniqueId));
		Write<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count, 1);
	}

	// Original function: FUN_004a3947; Windows tests membership, not equal counts.
	bool CObjectGroupManager::l_Group_IsSelected(const void* group) const
	{
		using namespace NC2InGameGuiInputManager;
		bool selected = false;

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count); ++i)
		{
			const auto id = Read<std::int32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + i * NC2Layout::RecordSize::ObjectGroupObjectUnsigned);

			switch (Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + i * NC2Layout::RecordSize::ObjectGroupObjectUnsigned))
			{
				case SwitchIds::ObjectGroupType::Human:
					selected = Selection_IsHumanSelected(gInputManager, id);
					break;
				case SwitchIds::ObjectGroupType::Vehicle:
					selected = Selection_IsVehicleSelected(gInputManager, id);
					break;
				case SwitchIds::ObjectGroupType::House:
					selected = Selection_IsHouseSelected(gInputManager, id);
					break;
				default:
					continue;
			}

			if (!selected)
				return false;
		}

		return selected;
	}

	// Original function: FUN_004a39a8; signed sums and division precede WORD narrowing.
	bool CObjectGroupManager::l_Group_GetPosition(const void* group, std::uint32_t* point) const
	{
		std::int32_t x = 0, y = 0, count = 0;

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count); ++i)
		{
			const auto id = Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + i * NC2Layout::RecordSize::ObjectGroupObjectUnsigned);
			const void* position;

			switch (Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + i * NC2Layout::RecordSize::ObjectGroupObjectUnsigned))
			{
				case SwitchIds::ObjectGroupType::Human:
					position = NC2Logic::Human_GetMapPositionRef(NC2Logic::CHumanArray::mStaticVars + id * NC2Logic::CHumanArray::ElementSize);
					break;
				case SwitchIds::ObjectGroupType::Vehicle:
					position = NC2Logic::Vehicle_GetMapPositionRef(NC2Logic::CVehicleArray::mStaticVars + id * NC2Logic::CVehicleArray::ElementSize);
					break;
				case SwitchIds::ObjectGroupType::House:
					position = NC2Logic::CHouseArray::mStaticVars + id * NC2Logic::CHouseArray::ElementSize + NC2Layout::HouseOffset::MapPosition;
					break;
				default:
					continue;
			}

			x += Read<std::int16_t>(position, NC2Layout::MapMigPointOffset::MapX);
			y += Read<std::int16_t>(position, NC2Layout::MapMigPointOffset::MapY);
			++count;
		}

		if (count == 0)
		{
			*point = 0;
			return false;
		}

		*point = static_cast<std::uint16_t>(x / count) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(y / count)) << 16);

		return true;
	}

	// Original function: FUN_004a34c8; removal compacts and revisits the slot.
	void CObjectGroupManager::l_Group_VerifyAllObjects(void* group)
	{
		std::uint32_t index = 0;

		while (index < NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count))
		{
			if (l_Group_IsObjectValid(static_cast<std::byte*>(group) + NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned))
			{
				++index;
			}
			else
			{
				l_Group_RemoveObject(group, index);
			}
		}
	}

	// Original function: FUN_004a3508.
	bool CObjectGroupManager::l_Group_IsObjectValid(const void* object) const
	{
		using NC2Lifetime::Read;
		const auto id = Read<std::uint32_t>(object, NC2Layout::ObjectGroupObjectOffset::ObjectId);
		const void* entity;
		std::uint32_t playerOffset;

		switch (Read<std::uint32_t>(object, NC2Layout::ObjectGroupObjectOffset::Type))
		{
			case SwitchIds::ObjectGroupType::Human:
				entity = NC2Logic::CHumanArray::mStaticVars + id * NC2Logic::CHumanArray::ElementSize;
				playerOffset = NC2Layout::HumanOffset::PlayerId;
				break;
			case SwitchIds::ObjectGroupType::Vehicle:
				entity = NC2Logic::CVehicleArray::mStaticVars + id * NC2Logic::CVehicleArray::ElementSize;
				playerOffset = NC2Layout::VehicleOffset::PlayerId;
				break;
			case SwitchIds::ObjectGroupType::House:
				entity = NC2Logic::CHouseArray::mStaticVars + id * NC2Logic::CHouseArray::ElementSize;
				playerOffset = NC2Layout::HouseOffset::PlayerId;
				break;
			default:
				return false;
		}

		return Read<std::uint8_t>(entity, NC2Layout::LogicActorHeaderOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(entity, NC2Layout::LogicActorHeaderOffset::UniqueId) == Read<std::uint32_t>(object, NC2Layout::ObjectGroupObjectOffset::UniqueId) && Read<std::int32_t>(entity, playerOffset) == NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
	}

	// Original function: FUN_004a3603; remove only the first matching object.
	void CObjectGroupManager::l_Group_RemoveHuman(void* group, std::uint32_t id)
	{
		const auto count = NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			if (NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == 1 && NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == id)
			{
				l_Group_RemoveObject(group, index);

				return;
			}
		}
	}

	// Original function: FUN_004a36b0.
	void CObjectGroupManager::l_Group_RemoveVehicle(void* group, std::uint32_t id)
	{
		const auto count = NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			if (NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == 2 && NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == id)
			{
				l_Group_RemoveObject(group, index);

				return;
			}
		}
	}

	// Original function: FUN_004a3741.
	void CObjectGroupManager::l_Group_RemoveHouse(void* group, std::uint32_t id)
	{
		const auto count = NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			if (NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == 3 && NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == id)
			{
				l_Group_RemoveObject(group, index);

				return;
			}
		}
	}

	// Original function: FUN_004a3778; unchecked slot and wrapped byte count.
	void CObjectGroupManager::l_Group_RemoveObject(void* group, std::uint32_t index)
	{
		auto* bytes = static_cast<std::byte*>(group);
		const auto count = NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count);
		std::memmove(bytes + NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned, bytes + NC2Layout::ObjectGroupOffset::SecondObjectTypeUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned, (count - index - 1u) * NC2Layout::RecordSize::ObjectGroupObjectUnsigned);
		NC2Lifetime::Write(group, NC2Layout::ObjectGroupOffset::Count, NC2Lifetime::Read<std::uint32_t>(group, NC2Layout::ObjectGroupOffset::Count) - 1u);
	}

	// Original function: FUN_004a37b1.
	bool __cdecl CObjectGroupManager::ls_Callback_Logic(std::uint32_t type, std::uintptr_t, std::uintptr_t callbackValueParam1, std::uintptr_t callbackValueParam2, std::uintptr_t)
	{
		std::uint32_t param1 = static_cast<std::uint32_t>(callbackValueParam1);
		std::uint32_t param2 = static_cast<std::uint32_t>(callbackValueParam2);

		if (type == SwitchIds::CallbackType::GameTick)
		{
			static_cast<CObjectGroupManager*>(gObjectGroupManager)->Group_VerifyAllObjects();
		}
		else if (type == SwitchIds::CallbackType::LoadDone && param1 != 0)
		{
			std::memset(static_cast<CObjectGroupManager*>(gObjectGroupManager)->mStorage + NC2Layout::ObjectGroupManagerOffset::Groups, 0, NC2Layout::RecordSize::ObjectGroupPool);
		}
		else if (param2 == 0)
		{
			auto* manager = static_cast<CObjectGroupManager*>(gObjectGroupManager);

			if (type == SwitchIds::CallbackType::HumanLifetime)
			{
				for (std::uint32_t group = 0; group < NC2Runtime::ObjectGroupLimits::Groups; ++group)
				{
					manager->l_Group_RemoveHuman(manager->mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + group * NC2Layout::RecordSize::ObjectGroupUnsigned, param1);
				}
			}
			else if (type == SwitchIds::CallbackType::VehicleLifetime)
			{
				for (std::uint32_t group = 0; group < NC2Runtime::ObjectGroupLimits::Groups; ++group)
				{
					manager->l_Group_RemoveVehicle(manager->mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + group * NC2Layout::RecordSize::ObjectGroupUnsigned, param1);
				}
			}
			else if (type == SwitchIds::CallbackType::HouseLifetime)
			{
				for (std::uint32_t group = 0; group < NC2Runtime::ObjectGroupLimits::Groups; ++group)
				{
					manager->l_Group_RemoveHouse(manager->mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + group * NC2Layout::RecordSize::ObjectGroupUnsigned, param1);
				}
			}
		}

		return true;
	}

	// Original function: FUN_004a388d.
	std::int32_t CObjectGroupManager::Misc_GetGroupIdOfHuman(std::uint32_t humanId) const
	{
		for (std::uint32_t group = 0; group < NC2Runtime::ObjectGroupLimits::Groups; ++group)
		{
			const auto* data = mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + group * NC2Layout::RecordSize::ObjectGroupUnsigned;

			for (std::uint32_t index = 0; index < NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::ObjectGroupOffset::Count); ++index)
			{
				if (NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == 1 && NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == humanId)
					return static_cast<std::int32_t>(group);
			}
		}

		return -1;
	}

	// Original function: FUN_004a38cb.
	std::int32_t CObjectGroupManager::Misc_GetGroupIdOfVehicle(std::uint32_t vehicleId) const
	{
		for (std::uint32_t group = 0; group < NC2Runtime::ObjectGroupLimits::Groups; ++group)
		{
			const auto* data = mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + group * NC2Layout::RecordSize::ObjectGroupUnsigned;

			for (std::uint32_t index = 0; index < NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::ObjectGroupOffset::Count); ++index)
			{
				if (NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == 2 && NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == vehicleId)
					return static_cast<std::int32_t>(group);
			}
		}

		return -1;
	}

	// Original function: FUN_004a3909.
	std::int32_t CObjectGroupManager::Misc_GetGroupIdOfHouse(std::uint32_t houseId) const
	{
		for (std::uint32_t group = 0; group < NC2Runtime::ObjectGroupLimits::Groups; ++group)
		{
			const auto* data = mStorage + NC2Layout::ObjectGroupManagerOffset::GroupsUnsigned + group * NC2Layout::RecordSize::ObjectGroupUnsigned;

			for (std::uint32_t index = 0; index < NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::ObjectGroupOffset::Count); ++index)
			{
				if (NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::ObjectGroupOffset::FirstObjectTypeUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == 3 && NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::ObjectGroupOffset::FirstObjectIdUnsigned + index * NC2Layout::RecordSize::ObjectGroupObjectUnsigned) == houseId)
					return static_cast<std::int32_t>(group);
			}
		}

		return -1;
	}

	// Original function: FUN_004a3179.
	CObjectGroupManager::CObjectGroupManager()
	{
		gObjectGroupManager = this;
		std::memset(this, 0, NC2Layout::ObjectSize::ObjectGroupManager);
		RegisterCallbacks(ls_Callback_Logic, { SwitchIds::CallbackType::GameTick, SwitchIds::CallbackType::LoadDone, SwitchIds::CallbackType::HumanLifetime, SwitchIds::CallbackType::VehicleLifetime, SwitchIds::CallbackType::HouseLifetime }, NC2Runtime::GuiCallbackPriority::StateManagers);
	}

	// Original function: FUN_004a31f4.
	CObjectGroupManager::~CObjectGroupManager()
	{
		UnregisterCallbacks(ls_Callback_Logic, { SwitchIds::CallbackType::HouseLifetime, SwitchIds::CallbackType::VehicleLifetime, SwitchIds::CallbackType::HumanLifetime, SwitchIds::CallbackType::LoadDone, SwitchIds::CallbackType::GameTick });
		gObjectGroupManager = nullptr;
	}
}

namespace NC2InGameGuiDeadHumanDataManager
{
	// Original function: FUN_004b3935; returns the first matching retained record.
	void* CDeadHumanDataManager::DeadHuman_GetDataPtrByHumanUniqueId(std::uint32_t uniqueId) const
	{
		const auto count = Read<std::uint32_t>(this, NC2Layout::DeadHumanManagerOffset::Count);
		auto* record = Read<std::byte*>(this, NC2Layout::DeadHumanManagerOffset::Records);

		for (std::uint32_t index = 0; index < count; ++index, record += NC2Layout::RecordSize::DeadHumanRecord)
		{
			if (Read<std::uint32_t>(record, NC2Layout::DeadHumanRecordOffset::HumanUniqueId) == uniqueId)
				return record;
		}

		return nullptr;
	}

	// Original function: FUN_004b3759.
	CDeadHumanDataManager::CDeadHumanDataManager()
	{
		gDeadHumanDataManager = this;
		std::memset(this, 0, NC2Layout::RecordSize::DeadHumanStatePrefix);
		std::memset(mStorage + NC2Layout::DeadHumanManagerOffset::Records, 0, NC2Layout::RecordSize::DeadHumanRecordPointer);
		RegisterCallbacks(ls_Callback_Logic, { SwitchIds::CallbackType::HumanLifetime, SwitchIds::CallbackType::HumanManagerLifetime }, NC2Runtime::GuiCallbackPriority::StateManagers);
		auto* records = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::RecordSize::DeadHumanPool, 1));

		if (records != nullptr)
		{
			for (std::uint32_t index = 0; index < NC2Runtime::DeadHumanLimits::Capacity; ++index)
			{
				Write<std::uint16_t>(records, index * NC2Layout::RecordSize::DeadHumanRecordUnsigned + NC2Layout::DeadHumanRecordOffset::MapY, 0);
				Write<std::uint16_t>(records, index * NC2Layout::RecordSize::DeadHumanRecordUnsigned + NC2Layout::DeadHumanRecordOffset::MapX, 0);
			}
		}

		Write(this, NC2Layout::DeadHumanManagerOffset::Records, records);
		std::memset(records, 0, NC2Layout::RecordSize::DeadHumanPool);
	}

	// Original function: FUN_004b37e0.
	CDeadHumanDataManager::~CDeadHumanDataManager()
	{
		NXBasics::Runtime_Free(Read<void*>(this, NC2Layout::DeadHumanManagerOffset::Records));
		UnregisterCallbacks(ls_Callback_Logic, { SwitchIds::CallbackType::HumanManagerLifetime, SwitchIds::CallbackType::HumanLifetime });
		gDeadHumanDataManager = nullptr;
	}

	// Original function: FUN_004b389e.
	void CDeadHumanDataManager::DeadHuman_Add(std::uint32_t humanId)
	{
		void* human = NC2Logic::CHumanArray::mStaticVars + humanId * NC2Layout::RecordSize::HumanUnsigned;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) != NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId())
		{

			return;
		}

		void* entry = static_cast<std::byte*>(Read<void*>(this, NC2Layout::DeadHumanManagerOffset::Records)) + Read<std::uint32_t>(this, NC2Layout::DeadHumanManagerOffset::NextRecordIndex) * NC2Layout::RecordSize::DeadHumanRecordUnsigned;
		std::memset(entry, 0, NC2Layout::RecordSize::DeadHumanRecord);
		Write<std::uint32_t>(entry, NC2Layout::DeadHumanRecordOffset::HumanUniqueId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
		NXBasics::Text_Copy(static_cast<char*>(entry) + NC2Layout::DeadHumanRecordOffset::Name, NC2Runtime::DeadHumanLimits::NameBytes, reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries + humanId * NC2Layout::RecordSize::DynamicHumanUnsigned + NC2Layout::DynamicHumanOffset::NameUnsigned));
		Write<std::uint32_t>(entry, NC2Layout::DeadHumanRecordOffset::DeathTick, NC2Logic::gGameTime.tick);
		Write<std::uint32_t>(entry, NC2Layout::DeadHumanRecordOffset::JobType, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType));
		Write<std::uint32_t>(entry, NC2Layout::DeadHumanRecordOffset::MapPosition, Read<std::uint32_t>(NC2Logic::Human_GetMapPositionRef(human), NC2Layout::MapMigPointOffset::Coordinates));
		const auto count = Read<std::uint32_t>(this, NC2Layout::DeadHumanManagerOffset::Count);

		if (count != NC2Runtime::DeadHumanLimits::Capacity)
			Write<std::uint32_t>(this, NC2Layout::DeadHumanManagerOffset::Count, count + 1u);

		const auto next = Read<std::uint32_t>(this, NC2Layout::DeadHumanManagerOffset::NextRecordIndex) + 1u;
		Write<std::uint32_t>(this, NC2Layout::DeadHumanManagerOffset::NextRecordIndex, next == NC2Runtime::DeadHumanLimits::Capacity ? 0 : next);
	}

	// Original callback range: 004b397b..004b3991; Mac System_HumanArrayDestructed.
	void CDeadHumanDataManager::System_HumanArrayDestructed()
	{
		std::memset(this, 0, NC2Layout::RecordSize::DeadHumanStatePrefix);
	}

	// Original function: FUN_004b3957.
	bool __cdecl CDeadHumanDataManager::ls_Callback_Logic(std::uint32_t type, std::uintptr_t /*userData*/, std::uintptr_t callbackValueParam1, std::uintptr_t callbackValueParam2, std::uintptr_t /*param3*/)
	{
		std::uint32_t param1 = static_cast<std::uint32_t>(callbackValueParam1);
		std::uint32_t param2 = static_cast<std::uint32_t>(callbackValueParam2);
		auto* manager = static_cast<CDeadHumanDataManager*>(gDeadHumanDataManager);

		if (type == SwitchIds::CallbackType::HumanManagerLifetime && param1 == 0)
		{
			manager->System_HumanArrayDestructed();
		}
		else if (type == SwitchIds::CallbackType::HumanLifetime && param2 == 0)
		{
			manager->DeadHuman_Add(param1);
		}

		return true;
	}
}

namespace NC2InGameGuiInputManager
{
	// Original function: FUN_004b4683.
	CInputManager::CInputManager()
	{
		ConstructInputMembers(this);
		gInputManager = this;
		std::memset(this, 0, NC2Layout::ObjectSize::InputManager);
		gInputManagerStaticVars = 0;
		RegisterCallbacks(InputManager_ls_Callback_Logic, { SwitchIds::CallbackType::GameTick, SwitchIds::CallbackType::HumanLifetime, SwitchIds::CallbackType::AnimalLifetime, SwitchIds::CallbackType::VehicleLifetime, SwitchIds::CallbackType::HouseLifetime, SwitchIds::CallbackType::GuideLifetime, SwitchIds::CallbackType::LoadDone }, NC2Runtime::GuiCallbackPriority::InputManager);
		Write<std::uint32_t>(this, NC2Layout::InputManagerOffset::SelectionChangeCount, 1);
	}

	// Original function: FUN_004b47d9.
	CInputManager::~CInputManager()
	{
		UnregisterCallbacks(InputManager_ls_Callback_Logic, { SwitchIds::CallbackType::LoadDone, SwitchIds::CallbackType::GuideLifetime, SwitchIds::CallbackType::HouseLifetime, SwitchIds::CallbackType::VehicleLifetime, SwitchIds::CallbackType::AnimalLifetime, SwitchIds::CallbackType::HumanLifetime, SwitchIds::CallbackType::GameTick });
		gInputManager = nullptr;
	}
}
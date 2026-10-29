#include "XPalette.h"
#include "C2DefaultInteraction.h"
#include "C2E2AmbientSoundManagerAccess.h"
#include "C2E2AnimalSoundManager.h"
#include "C2E2HumanSoundAccess.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2UserNetworkCommandAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiKeyAssignmentManager.h"
#include "InGameGuiManager.h"
#include "SwitchIdentifiers.h"
#include "WorldDisplayElementAccess.h"
#include "XBasicTypes.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XMKeyManager.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"
#include "XPaletteAccess.h"
#include "XPictureAccess.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <new>

extern void __fastcall Display_DE_MapMoveableToGlobalWorldPixelCoordinates(void* display, void* unusedEdx, std::uint32_t moveableId, std::int32_t* point);

namespace NC2InGameGuiWorldElement
{
	// GUI hit testing stays in reference coordinates; the main world uses native pixels.
	static void WorldGuiPointToRender(const void* element, std::int32_t* point)
	{
		auto* presentation = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay();

		if (presentation->IsMainWorld(element))
			presentation->HudToWorld(point);
	}

	template <class T>
	static T WorldDisplayElement_ReadField(const void* object, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));

		return value;
	}

	template <typename T>
	static void WorldDisplayElement_WriteField(void* object, std::size_t offset, T value)
	{
		std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
	}

	// Original function: FUN_004bab5e.
	static void WorldDisplayElement_MouseMoved(void* element, std::int32_t x, std::int32_t y)
	{
		using namespace NC2Lifetime;
		std::int32_t point[2]{ x, y };
		NC2InGameGuiBase::BaseElement_GlobalToLocalCoordinates(element, point);
		auto* display = Read<void*>(element, NC2Layout::WorldGuiElementOffset::Display);
		WorldGuiPointToRender(element, point);
		NC2E2::Display_LocalDisplayPixelToGlobalWorldPixelCoordinates(display, point, true);

		if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::AreaSelectionActiveFlag) != 0)
		{
			Write(element, NC2Layout::WorldGuiElementOffset::AreaEndX, point[0]);
			Write(element, NC2Layout::WorldGuiElementOffset::AreaEndY, point[1]);

			return;
		}

		NC2E2::DisplayDescription_MakeValidWorldPixelPosition(display, point);
		std::uint32_t mapPoint = 0;
		NC2E2::DisplayDescription_WorldPixelToMapMigCoordinates(display, point, &mapPoint);
		NC2InGameGuiInputManager::Cursor_SetPosition(NC2InGameGuiInputManager::GetInputManagerPtr(), &mapPoint);
	}

	// Original function: FUN_004babe0; complete single/group human and vehicle preparation.
	static void WorldDisplayElement_CheckObjectUnderMouse(void* element)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::InputEnabledFlag) == 0)
			return;

		auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr;
		std::int32_t point[2]{ Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::X), Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::Y) };
		NC2InGameGuiBase::BaseElement_GlobalToLocalCoordinates(element, point);
		auto* display = Read<void*>(element, NC2Layout::WorldGuiElementOffset::Display);
		WorldGuiPointToRender(element, point);
		NC2E2::Display_LocalDisplayPixelToGlobalWorldPixelCoordinates(display, point, true);
		auto* target = static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::ObjectUnderMouse;
		NC2E2::DE_SearchObject_FindOnWorldPosition(display, point, target);
		Write<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount, 0);
		const auto localPlayer = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();

		if (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type) == 1)
		{
			const auto* hoveredHuman = NC2Logic::CHumanArray::mStaticVars + Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId) * NC2Logic::CHumanArray::ElementSize;

			if (Read<std::int32_t>(hoveredHuman, NC2Layout::HumanOffset::PlayerId) == localPlayer)
			{
				for (const auto offset : { 0xB20u, 0xB18u, 0xB14u, 0xB24u })
				{
					const auto id = Read<std::uint32_t>(hoveredHuman, offset);

					if (id != NXBasics::IntegerRange::UInt32Maximum)
						NC2E2::Display_DE_ObjectsToMark_AddSecondaryHuman(display, 2, id);
				}

				for (const auto offset : { 0xB28u, 0xB34u })
				{
					const auto id = Read<std::uint32_t>(hoveredHuman, offset);

					if (id != NXBasics::IntegerRange::UInt32Maximum)
						NC2E2::Display_DE_ObjectsToMark_AddSecondaryHouse(display, 2, id);
				}
			}
		}
		else if (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type) == 4)
		{
			const auto* house = NC2Logic::CHouseArray::mStaticVars + Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HouseId) * NC2Logic::CHouseArray::ElementSize;

			if (Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId) == localPlayer && NC2Logic::House_Inhabitants_GetNumberOfAttachedHumans(house) != 0)
				for (std::uint32_t i = 0; i < 20; ++i)
				{
					const auto* slot = house + NC2Layout::HouseOffset::Attachments + i * 0x10u;

					if (Read<std::uint8_t>(slot, NC2Layout::HouseAttachmentOffset::RoleMask) != 0)
						NC2E2::Display_DE_ObjectsToMark_AddSecondaryHuman(display, 2, Read<std::uint32_t>(slot, NC2Layout::HouseAttachmentOffset::HumanId));
				}
		}

		Write<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::HumanInteractionAction, 0);
		Write<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::VehicleInteractionAction, 0);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::PrimaryCommand, 0, 20);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::SecondaryCommand, 0, 20);
		const auto* input = NC2InGameGuiInputManager::GetInputManagerPtr();
		auto* command = static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::PrimaryCommand;
		auto* secondary = static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::SecondaryCommand;
		const auto modifier = []()
		{

			return (Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags) & 2u) != 0;
		};
		const auto clearGroup = [&](std::size_t offset)
		{
			Write<std::uint32_t>(element, offset, 0);
			std::memset(command, 0, 20);
			std::memset(secondary, 0, 20);
		};
		using namespace NC2InGameGuiInputManager;

		switch (Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::SelectionType))
		{
			case SwitchIds::SelectionType::SingleHuman:
			{
				const auto id = Selection_GetSelectedSingleHumanId(input);

				if (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type) == 1 && Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId) == id)
					return;

				const auto* human = NC2Logic::CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CHumanArray::ElementSize;
				Write(element, NC2Layout::WorldGuiElementOffset::HumanInteractionAction, NC2Logic::Human_UserControl_GetDefaultInteractionCommand_SingleHuman(human, target, command, secondary, modifier()));
				return;
			}
			case SwitchIds::SelectionType::MultipleHumans:
			{
				const auto count = Selection_GetSelectedMultipleHumanNumber(input);

				for (std::uint32_t i = 0; i < count; ++i)
				{
					const auto id = Selection_GetSelectedMultipleHumanId(input, i);

					if (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type) == 1 && id == Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId))
						return;
				}

				for (std::uint32_t i = 0; i < count; ++i)
				{
					const auto id = Selection_GetSelectedMultipleHumanId(input, i);
					const auto* human = NC2Logic::CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CHumanArray::ElementSize;

					if (!NC2Logic::Human_UserControl_GetDefaultInteractionCommand_MultipleHuman(human, reinterpret_cast<std::uint32_t*>(static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::HumanInteractionAction), target, command, i, count, modifier()))
					{
						clearGroup(NC2Layout::WorldGuiElementOffset::HumanInteractionAction);
						return;
					}
				}

				return;
			}
			case SwitchIds::SelectionType::SingleVehicle:
			{
				const auto id = Selection_GetSelectedSingleVehicleId(input);

				if (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type) == 3 && Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::VehicleId) == id)
					return;

				const auto* vehicle = NC2Logic::CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CVehicleArray::ElementSize;
				Write(element, NC2Layout::WorldGuiElementOffset::VehicleInteractionAction, NC2Logic::Vehicle_UserControl_GetDefaultInteractionCommand_SingleVehicle(vehicle, target, command, secondary, modifier()));
				return;
			}
			case SwitchIds::SelectionType::MultipleVehicles:
			{
				const auto count = Selection_GetSelectedMultipleVehiclesNumber(input);

				for (std::uint32_t i = 0; i < count; ++i)
				{
					const auto id = Selection_GetSelectedMultipleVehicleId(input, i);

					if (Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type) == 3 && id == Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::VehicleId))
						return;
				}

				for (std::uint32_t i = 0; i < count; ++i)
				{
					const auto id = Selection_GetSelectedMultipleVehicleId(input, i);
					const auto* vehicle = NC2Logic::CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CVehicleArray::ElementSize;

					if (!NC2Logic::Vehicle_UserControl_GetDefaultInteractionCommand_MultipleVehicles(vehicle, reinterpret_cast<std::uint32_t*>(static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::VehicleInteractionAction), target, command, i, count, modifier()))
					{
						clearGroup(NC2Layout::WorldGuiElementOffset::VehicleInteractionAction);
						return;
					}
				}

				return;
			}
			default:

				return;
		}
	}

	// Original function: FUN_004bb0c1; complete prepared single/group human and vehicle dispatch.
	static bool WorldDisplayElement_DoDefaultAction(void* element)
	{
		using namespace NC2Lifetime;
		auto* command = static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::PrimaryCommand;

		if (Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode) == 0)
			return false;

		if (Read<std::uint32_t>(NC2InGameGuiInputManager::GetInputManagerPtr(), NC2Layout::InputManagerOffset::SelectionType) == 1)
		{
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, command, false, false);

			if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::SecondaryCommand) != 0)
				NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::SecondaryCommand, false, false);

			NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Read<std::int32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId));

			if (Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode) == 32)
			{
				const auto address = reinterpret_cast<std::uintptr_t>(NC2Logic::CHouseArray::mStaticVars) + Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1) * NC2Logic::CHouseArray::ElementSize;
				const auto* house = reinterpret_cast<const void*>(address);

				if (address != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 38)
					NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->MiscWindows_HumanLearnJobWindow_Open();
			}
		}
		else if (Read<std::uint32_t>(NC2InGameGuiInputManager::GetInputManagerPtr(), NC2Layout::InputManagerOffset::SelectionType) == 2)
		{
			auto* input = NC2InGameGuiInputManager::GetInputManagerPtr();
			const auto count = NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanNumber(input);

			for (std::uint32_t i = 0; i < count; ++i)
			{
				const auto id = NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanId(input, i);
				std::byte selectedCommand[20];
				std::memcpy(selectedCommand, command, sizeof(selectedCommand));
				Write(selectedCommand, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
				Write(selectedCommand, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CHumanArray::ElementSize, NC2Layout::HumanOffset::UniqueId));
				NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, selectedCommand, false, false);
				NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, id);
			}
		}
		else if (Read<std::uint32_t>(NC2InGameGuiInputManager::GetInputManagerPtr(), NC2Layout::InputManagerOffset::SelectionType) == 4)
		{
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, command, false, false);

			if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::SecondaryCommand) != 0)
				NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::SecondaryCommand, false, false);
		}
		else if (Read<std::uint32_t>(NC2InGameGuiInputManager::GetInputManagerPtr(), NC2Layout::InputManagerOffset::SelectionType) == 5)
		{
			auto* input = NC2InGameGuiInputManager::GetInputManagerPtr();
			const auto count = NC2InGameGuiInputManager::Selection_GetSelectedMultipleVehiclesNumber(input);

			for (std::uint32_t i = 0; i < count; ++i)
			{
				const auto id = NC2InGameGuiInputManager::Selection_GetSelectedMultipleVehicleId(input, i);
				std::byte selectedCommand[20];
				std::memcpy(selectedCommand, command, sizeof(selectedCommand));
				Write(selectedCommand, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
				Write(selectedCommand, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(NC2Logic::CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CVehicleArray::ElementSize, NC2Layout::VehicleOffset::UniqueId));
				NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, selectedCommand, false, false);
			}
		}

		return true;
	}

	// Original function: FUN_004bb288.
	bool WorldDisplayElement_l_TryToBringUpActionButtonsForObjectUnderMouse(void* element)
	{
		using namespace NC2Lifetime;
		auto* input = NC2InGameGuiInputManager::GetInputManagerPtr();
		const auto type = Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::ObjectUnderMouse), selection = Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::SelectionType);
		const bool matches = (type == 1 && selection == 1 && NC2InGameGuiInputManager::Selection_GetSelectedSingleHumanId(input) == Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::HoveredHumanId)) || (type == 3 && selection == 4 && NC2InGameGuiInputManager::Selection_GetSelectedSingleVehicleId(input) == Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::HoveredVehicleId)) || (type == 4 && selection == 6 && Read<std::int32_t>(input, NC2Layout::InputManagerOffset::HouseId) == Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::HoveredHouseId));

		if (!matches)
			return false;

		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Selection_ActionButtons_BringUp();

		return true;
	}

	// Original function: FUN_004bb2f2.
	bool WorldDisplayElement_l_TryToSelectAndBringUpActionButtonsForObjectUnderMouse(void* element)
	{
		using namespace NC2Lifetime;
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		const auto type = Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::ObjectUnderMouse);
		std::uint32_t player;

		if (type == 1)
		{
			player = Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars, Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::HoveredHumanId) * NC2Logic::CHumanArray::ElementSize + NC2Layout::HumanOffset::PlayerId);
		}
		else if (type == 3)
		{
			player = Read<std::uint32_t>(NC2Logic::CVehicleArray::mStaticVars, Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::HoveredVehicleId) * NC2Logic::CVehicleArray::ElementSize + NC2Layout::VehicleOffset::PlayerId);
		}
		else if (type == 4)
		{
			player = Read<std::uint32_t>(NC2Logic::CHouseArray::mStaticVars, Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::HoveredHouseId) * NC2Logic::CHouseArray::ElementSize + NC2Layout::HouseOffset::PlayerId);
		}
		else
		{
			return false;
		}

		if (player != static_cast<std::uint32_t>(manager->GetLocalPlayerId()) || !NC2InGameGuiInputManager::Action_DoNormalSelect(NC2InGameGuiInputManager::GetInputManagerPtr(), static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::ObjectUnderMouse, true, false, false))
			return false;

		manager->Selection_ActionButtons_BringUp();
		NC2GuiToolsBase::FXTool_ClickConfirm();

		return true;
	}

	// Original function: FUN_004bb035; Windows scales directional scrolling by the measured frame rate.
	static void l_DoTimeSyncedScrolling(void* element, std::uint32_t direction)
	{
		using namespace NC2Lifetime;
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		const auto speed = NXBasics::Runtime_TimeSigned32(manager->Options_ScrollSpeed_GetInPixelPerSecond());
		std::int32_t x = 0, y = 0;

		if (direction == 1)
		{
			x = speed;
		}
		else if (direction == 2)
		{
			y = speed;
		}
		else if (direction == 3)
		{
			x = NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(speed));
		}
		else if (direction == 4)
		{
			y = NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(speed));
		}

		const auto rate = Read<float>(manager, NC2Layout::InGameGuiManagerOffset::MeasuredFrameRate);
		const auto factor = 1.0 / (rate >= 1.0f ? static_cast<double>(rate) : 1.0);
		NC2E2::DE_SetWantedPositionAddDeltaPixelPoint(Read<void*>(element, NC2Layout::WorldGuiElementOffset::Display), static_cast<std::int32_t>(NXBasics::Runtime_TruncateDoubleToInt64(x * factor)), static_cast<std::int32_t>(NXBasics::Runtime_TruncateDoubleToInt64(y * factor)));
		WorldDisplayElement_FollowMode_End(element);
	}

	// Original function: FUN_004ba3be; complete native message branches, retaining the event-time compatibility adapter.
	static bool __fastcall WorldDisplayElement_MessageHandle(void* element, void*, std::int32_t message, std::int32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3)
	{
		using namespace NC2Lifetime;
		using namespace NC2InGameGuiInputManager;
		auto* input = gInputManager;
		auto* display = Read<void*>(element, NC2Layout::WorldGuiElementOffset::Display);
		const auto keys = Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags);
		auto* target = static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::ObjectUnderMouse;
		const auto mainMode = Read<std::uint32_t>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::MainMode);

		if (message == 1 || message == 2)
		{
			// Use the modern adapter's event time for the native 250 ms gate.
			const auto doubleTime = Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::LastDoubleClickEventTime);

			if (doubleTime != 0 && NXSysMouseManager::CMouseManager::sTheObjectPtr->GetInputMessageTime() - doubleTime < 250u)
				return true;
		}

		if (message == 1 || message == 0x20)
		{
			const auto offset = message == 1 ? NC2Layout::WorldGuiElementOffset::LeftClickHandlerUnsigned : NC2Layout::WorldGuiElementOffset::DoubleClickHandlerUnsigned;
			auto* overrideHandler = Read<void*>(element, offset);

			if (overrideHandler != nullptr)
			{
				using THandle = bool(__fastcall*)(void*, void*, std::int32_t, std::int32_t, std::uint32_t, std::uint32_t, std::uint32_t);
				const auto handle = Read<THandle>(Read<void*>(overrideHandler, NC2Layout::GuiBaseElementOffset::VirtualTable), NC2Layout::GuiVirtualTableOffset::MessageHandle);

				if (handle(overrideHandler, nullptr, 0x800, Read<std::int32_t>(element, offset + NC2Layout::WorldClickHandlerOffset::Command), 0, 0, 0))
					return true;
			}
		}

		if (message == 1)
		{
			if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::InputEnabledFlag) == 0 || Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) == 0 || Read<std::int32_t>(input, NC2Layout::InputManagerOffset::InputMode) != 0)
				return true;

			Write<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::AreaSelectionActiveFlag, 1);
			std::int32_t point[2]{ command, static_cast<std::int32_t>(param1) };
			WorldGuiPointToRender(element, point);
			NC2E2::Display_LocalDisplayPixelToGlobalWorldPixelCoordinates(display, point, true);
			Write(element, NC2Layout::WorldGuiElementOffset::AreaStartX, point[0]);
			Write(element, NC2Layout::WorldGuiElementOffset::AreaStartY, point[1]);
			Write(element, NC2Layout::WorldGuiElementOffset::AreaEndX, point[0]);
			Write(element, NC2Layout::WorldGuiElementOffset::AreaEndY, point[1]);
			return true;
		}

		if (message == 2)
		{
			if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::InputEnabledFlag) == 0)
				return true;

			bool selectedArea = false;

			if (WorldDisplayElement_MouseSelectingArea_IsAreaValid(element))
			{
				NXBasics::SRectangle rectangle(Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::AreaStartX), Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::AreaStartY), Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::AreaEndX) - Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::AreaStartX), Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::AreaEndY) - Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::AreaStartY));
				selectedArea = Selection_SelectHumansInArea(input, element, rectangle, 0, (keys & 1u) != 0, (keys & 2u) != 0);
			}

			Write<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::AreaSelectionActiveFlag, 0);
			WorldDisplayElement_MouseMoved(element, command, static_cast<std::int32_t>(param1));

			if (selectedArea)
				return true;

			if (mainMode == 1)
			{
				if (Read<std::int32_t>(input, NC2Layout::InputManagerOffset::InputMode) != 0 && Action_DoCancel(input, target))
				{
					NC2GuiToolsBase::FXTool_Fail();
					return true;
				}

				if (Action_DoNormalSelect(input, target, false, (keys & 1u) != 0, (keys & 2u) != 0))
					NC2GuiToolsBase::FXTool_ClickConfirm();
			}
			else if (mainMode == 0)
			{
				if (Action_DoInputSelect(input, target))
					return true;

				if (Read<std::int32_t>(input, NC2Layout::InputManagerOffset::InputMode) != 0)
					return true;

				if (WorldDisplayElement_DoDefaultAction(element) || Action_DoDefault(input, target))
					return true;

				if (Action_DoNormalSelect(input, target, true, (keys & 1u) != 0, (keys & 2u) != 0))
					NC2GuiToolsBase::FXTool_ClickConfirm();
			}

			return true;
		}

		if (message == 0x20)
		{
			if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::InputEnabledFlag) == 0)
				return true;

			WorldDisplayElement_MouseMoved(element, command, static_cast<std::int32_t>(param1));

			if (mainMode == 1)
			{
				if (Action_DoInputSelect(input, target))
				{
					NC2GuiToolsBase::FXTool_ClickConfirm();
					return true;
				}

				if (Read<std::int32_t>(input, NC2Layout::InputManagerOffset::InputMode) != 0)
					return true;

				if (WorldDisplayElement_DoDefaultAction(element) || Action_DoDefault(input, target))
				{
					NC2GuiToolsBase::FXTool_ClickConfirm();
					return true;
				}

				if (WorldDisplayElement_l_TryToSelectAndBringUpActionButtonsForObjectUnderMouse(element))
					NC2GuiToolsBase::FXTool_ClickConfirm();

				return true;
			}
			else if (mainMode == 0)
			{
				if (WorldDisplayElement_l_TryToBringUpActionButtonsForObjectUnderMouse(element))
					return true;

				if (Action_DoCancel(input, target))
					return true;
			}
			else
			{
				return true;
			}

			WorldDisplayElement_l_TryToSelectAndBringUpActionButtonsForObjectUnderMouse(element);
			return true;
		}

		if (message == 4)
		{
			Write(element, NC2Layout::WorldGuiElementOffset::LastDoubleClickEventTime, NXSysMouseManager::CMouseManager::sTheObjectPtr->GetInputMessageTime());

			if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::InputEnabledFlag) != 0 && Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) != 0 && Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type) == 1 && Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::PlayerId) == Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId) && Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::InputMode) == 0 && (Selection_IsHumanSelected(input, Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId)) || Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::SelectionType) == 2))
			{
				const auto* human = NC2Logic::CHumanArray::mStaticVars + Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId) * NC2Logic::CHumanArray::ElementSize;
				const auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr;
				std::int32_t point[2]{ Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::X), Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::Y) };
				NC2InGameGuiBase::BaseElement_GlobalToLocalCoordinates(element, point);
				WorldGuiPointToRender(element, point);
				NC2E2::Display_LocalDisplayPixelToGlobalWorldPixelCoordinates(display, point, true);
				Selection_SelectHumansInArea(input, element, NXBasics::SRectangle(point[0] - 400, point[1] - 300, 800, 600), Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), (keys & 1u) != 0, (keys & 2u) != 0);
			}

			Write<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::AreaSelectionActiveFlag, 0);
			return true;
		}

		if (message == 0x80)
		{
			if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::InputEnabledFlag) == 0)
				return true;

			const auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr;
			const auto callback = Read<bool(__cdecl*)()>(mouse, NC2Layout::MouseManagerOffset::PositionOverrideCallback);

			if (callback != nullptr && callback())
			{
				const auto speed = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Options_ScrollSpeed_GetInPixelPerSecond();
				NC2E2::DE_SetWantedPositionAddDeltaPixelPoint(display, static_cast<std::int32_t>(speed * param2) / 300, static_cast<std::int32_t>(speed * param3) / 300);
				WorldDisplayElement_FollowMode_End(element);
			}

			WorldDisplayElement_MouseMoved(element, command, static_cast<std::int32_t>(param1));
			return true;
		}

		if (message == 0x800)
		{
			if (command == 2030)
			{
				WorldDisplayElement_l_FollowMode_Update(element);
				WorldDisplayElement_CheckObjectUnderMouse(element);

				if (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) != 0)
				{
					auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

					if (Read<std::uint8_t>(manager, NC2Layout::InGameGuiManagerOffset::BorderScroll) != 0)
					{
						auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr;
						const auto x = Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::X), y = Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::Y);

						if (x >= 0 && x <= 1)
							l_DoTimeSyncedScrolling(element, 3);

						const auto width = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width);

						if (x >= width - 2 && x <= width - 1)
							l_DoTimeSyncedScrolling(element, 1);

						if (y >= 0 && y <= 1)
							l_DoTimeSyncedScrolling(element, 4);

						const auto height = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height);

						if (y >= height - 2 && y <= height - 1)
							l_DoTimeSyncedScrolling(element, 2);
					}

					auto* assignments = NC2InGameGuiKeyAssignmentManager::CKeyAssignmentManager::sTheObjectPtr;

					if (assignments->CheckDirectKeyVirtualOnly(26))
						l_DoTimeSyncedScrolling(element, 3);

					if (assignments->CheckDirectKeyVirtualOnly(27))
						l_DoTimeSyncedScrolling(element, 1);

					if (assignments->CheckDirectKeyVirtualOnly(24))
						l_DoTimeSyncedScrolling(element, 4);

					if (assignments->CheckDirectKeyVirtualOnly(25))
						l_DoTimeSyncedScrolling(element, 2);

					Write<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::TooltipOverlaysEnabled, assignments->CheckDirectKeyVirtualOnly(28));
				}
			}
			else if (command == 2031)
			{
				std::memset(target, 0, NC2Layout::RecordSize::DisplaySearchObject);
				NC2E2::DisplayObjectReference_Reset(target);
				Write<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::HumanInteractionAction, 0);
				Write<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::VehicleInteractionAction, 0);
				std::memset(static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::PrimaryCommand, 0, 20);
				std::memset(static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::SecondaryCommand, 0, 20);
				Write<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::AreaSelectionActiveFlag, 0);
			}
			else
			{
				return false;
			}

			return true;
		}

		if (message == 0x100)
		{
			void* display = WorldDisplayElement_ReadField<void*>(element, NC2Layout::WorldGuiElementOffset::Display);

			if (WorldDisplayElement_ReadField<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::InputEnabledFlag) != 0 && WorldDisplayElement_ReadField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) != 0)
			{
				const auto wheelDelta = NXBasics::Runtime_TimeSigned32(param2);

				if (wheelDelta > 0)
				{
					NC2E2::DE_Zoom_In(display, false);
				}
				else if (wheelDelta < 0)
				{
					NC2E2::DE_Zoom_Out(display);
				}
			}

			return true;
		}

		if (message != 0x1000)
			return false;

		if (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) == 0)
			return false;

		if (command == 2002)
		{
			float definition[3];
			const std::uint32_t bits[3]{ param1, param2, param3 };
			std::memcpy(definition, bits, sizeof(definition));
			NC2E2::Display_AssignNewDisplayDefinitionValues(display, definition[0], definition[1], definition[2], true);
			return false;
		}

		if (command == 2003)
		{
			NC2E2::DE_ToDraw_ToggleFlagset(display, param1);
			return false;
		}

		if (command == 2006)
		{
			l_DoTimeSyncedScrolling(element, param1);
			return false;
		}

		if (command == 2007)
		{
			NC2E2::DE_Zoom_ToggleLevel(display, false);
			return false;
		}

		if (command == 2005)
		{
			if ((Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit9Unsigned) == 0)
			{
				NC2E2::DE_SetWantedPositionAddDeltaPixelPoint(display, NXBasics::Runtime_TimeSigned32(param1), NXBasics::Runtime_TimeSigned32(param2));
				WorldDisplayElement_FollowMode_End(element);
			}

			return false;
		}

		if (command != 2004)
			return false;

		if (WorldDisplayElement_ReadField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) == 0 || (WorldDisplayElement_ReadField<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit9Unsigned) != 0)
			return false;

		std::int32_t point[2];
		NC2E2::Display_MapMigToLocalDisplayPixelCoordinates(display, &param1, point, false);
		NC2E2::Display_LocalDisplayPixelToGlobalWorldPixelCoordinates(display, point, false);
		NC2E2::DE_SetWantedPositionToPixelPoint(display, point);
		WorldDisplayElement_FollowMode_End(element);
		// The original command branch changes the camera and still returns false.

		return false;
	}

	// Original virtual entry: FUN_004ba124.
	static void* __fastcall WorldDisplayElement_DeletingDestructor(void* element, void* unusedEdx, std::uint32_t flags)
	{
		(void)unusedEdx;
		WorldDisplayElement_Destruct(element);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// ABI adapter for the reconstructed virtual draw method, FUN_004ba1ba.
	static void __fastcall WorldDisplayElement_VirtualDraw(void* element, void* unusedEdx, NXBasics::CBitmap* bitmap, NXBasics::SRectangle* rectangle)
	{
		(void)unusedEdx;
		WorldDisplayElement_Draw(element, bitmap, rectangle);
	}

	// Complete 15-slot table recovered from Game.exe at 004F6FAC..004F6FE7.
	// Destruction, drawing, wheel zoom and the overview camera command use reconstructed
	// bodies. Other world-input dispatcher branches remain unfinished.
	static const std::uintptr_t gWorldDisplayElementVirtualTable[] = {
		reinterpret_cast<std::uintptr_t>(&WorldDisplayElement_DeletingDestructor),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&WorldDisplayElement_MessageHandle),
		reinterpret_cast<std::uintptr_t>(&WorldDisplayElement_VirtualDraw),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Activate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce)
	};
	static_assert(sizeof(gWorldDisplayElementVirtualTable) == 60);

	// Original function: FUN_004ba140; initializes the state subobjects at +0x30.
	static void* WorldDisplayElement_ConstructState(void* state)
	{
		WorldDisplayElement_WriteField<std::uint16_t>(state, NC2Layout::WorldGuiStateOffset::HoveredMapPointY, 0);
		WorldDisplayElement_WriteField<std::uint16_t>(state, NC2Layout::WorldGuiStateOffset::HoveredMapPointX, 0);
		std::memset(static_cast<std::byte*>(state) + NC2Layout::WorldGuiStateOffset::PrimaryCommand, 0, 20);
		std::memset(static_cast<std::byte*>(state) + NC2Layout::WorldGuiStateOffset::SecondaryCommand, 0, 20);

		return state;
	}

	// Original functions: FUN_004baffa/FUN_004bb007.
	void WorldDisplayElement_DrawFrame_SetMode(void* element, std::uint32_t mode)
	{
		NC2Lifetime::Write(element, NC2Layout::WorldGuiElementOffset::FrameMode, mode);
	}

	void WorldDisplayElement_Misc_LeftMouseClick_HandlerSet(void* element, void* handler, std::uint32_t command)
	{
		NC2Lifetime::Write(element, NC2Layout::WorldGuiElementOffset::LeftClickHandler, handler);
		NC2Lifetime::Write(element, NC2Layout::WorldGuiElementOffset::LeftClickCommand, command);
	}

	// Original function: FUN_004bb01e.
	void WorldDisplayElement_Misc_LeftMouseDoubleClick_HandlerSet(void* element, void* handler, std::uint32_t command)
	{
		NC2Lifetime::Write(element, NC2Layout::WorldGuiElementOffset::DoubleClickHandler, handler);
		NC2Lifetime::Write(element, NC2Layout::WorldGuiElementOffset::DoubleClickCommand, command);
	}

	// Original function: FUN_004ba042.
	void* WorldDisplayElement_Construct(void* element, const NXBasics::SRectangle* rectangle, bool active, bool inputEnabled)
	{
		auto* desktopBitmap = NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap();
		const std::int32_t depth = WorldDisplayElement_ReadField<std::int32_t>(desktopBitmap, 0x04);
		NC2InGameGuiBase::BaseBitmapToolElement_Construct(element, rectangle, 1, 0x1A7, depth);
		auto* presentation = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay();

		if (active && presentation->RegisterMainWorld(element))
		{
			delete WorldDisplayElement_ReadField<NXBasics::CBitmap*>(element, NC2Layout::WorldGuiElementOffset::Bitmap);
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CBitmap));
			auto* nativeBitmap = memory != nullptr ? ::new (memory) NXBasics::CBitmap(presentation->GetWidth(), presentation->GetHeight(), depth) : nullptr;
			WorldDisplayElement_WriteField(element, NC2Layout::WorldGuiElementOffset::Bitmap, nativeBitmap);
			presentation->SetMainWorldBitmap(nativeBitmap);
		}

		auto* state = static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::Display;
		WorldDisplayElement_ConstructState(state);
		WorldDisplayElement_WriteField(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldDisplayElementVirtualTable + 0);
		std::memset(state, 0, NC2Layout::RecordSize::WorldGuiElementState);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::EngineDisplay, 1);
		void* display = nullptr;

		if (allocation != nullptr)
		{
			auto* font = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08;
			auto* bitmap = WorldDisplayElement_ReadField<NXBasics::CBitmap*>(element, NC2Layout::WorldGuiElementOffset::Bitmap);
			display = NC2E2::Display_Construct(allocation, bitmap, static_cast<std::uint8_t>(active), font);
		}

		WorldDisplayElement_WriteField(element, NC2Layout::WorldGuiElementOffset::Display, display);
		WorldDisplayElement_WriteField<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::InputEnabledFlag, static_cast<std::uint8_t>(inputEnabled));
		auto* interactionTarget = static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::ObjectUnderMouse;
		std::memset(interactionTarget, 0, NC2Layout::RecordSize::DisplaySearchObject);
		NC2E2::DisplayObjectReference_Reset(interactionTarget);

		if (active)
		{
			WorldDisplayElement_WriteField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::PrimaryReferenceEnabled, interactionTarget != nullptr ? 1 : 0);
			WorldDisplayElement_WriteField(display, NC2Layout::EngineDisplayOffset::PrimaryMarkedObject, interactionTarget);
		}
		else
		{
			const bool explored = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Exploration_GetFlag();
			NC2E2::DE_ToDraw_SetState(display, 4, explored);
		}

		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->EngineWorldDisplayGuiElement_Register(element);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_Register(element);
		WorldDisplayElement_WriteField<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::FrameMode, 0);

		return element;
	}

	// Original function: FUN_004ba174.
	void WorldDisplayElement_Destruct(void* element)
	{
		WorldDisplayElement_WriteField(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldDisplayElementVirtualTable + 0);
		WorldDisplayElement_DarkenBitmap_Exit(element);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_UnRegister(element);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->EngineWorldDisplayGuiElement_UnRegister(element);
		void* display = WorldDisplayElement_ReadField<void*>(element, NC2Layout::WorldGuiElementOffset::Display);

		if (display != nullptr)
		{
			NC2E2::Display_Destruct(display);
			NXBasics::Runtime_Free(display);
		}

		NC2InGameGuiBase::BaseBitmapToolElement_Destruct(element);
	}

	// Original function: FUN_004bb388.
	void WorldDisplayElement_FollowMode_FollowHuman(void* element, std::int32_t id)
	{
		using namespace NC2Lifetime;
		std::memset(static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::FollowEnabledFlag, 0, 24);
		Write(element, NC2Layout::WorldGuiElementOffset::FollowHumanId, id);
		Write<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowEnabledFlag, 1);
		Write<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowHumanFlag, 1);
		Write(element, NC2Layout::WorldGuiElementOffset::FollowHumanUniqueId, Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHumanArray::ElementSize, NC2Layout::HumanOffset::UniqueId));
		WorldDisplayElement_l_FollowMode_Update(element);
	}

	// Original function: FUN_004bb3d5; validate the revision before reporting the id.
	std::int32_t WorldDisplayElement_FollowMode_GetHumanId(void* element)
	{
		WorldDisplayElement_l_FollowMode_Validate(element);

		return NC2Lifetime::Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowEnabledFlag) != 0 && NC2Lifetime::Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowHumanFlag) != 0 ? NC2Lifetime::Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::FollowHumanId) : -1;
	}

	// Original function: FUN_004bb3fc.
	void WorldDisplayElement_FollowMode_FollowVehicle(void* element, std::int32_t id)
	{
		using namespace NC2Lifetime;
		std::memset(static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::FollowEnabledFlag, 0, 24);
		Write(element, NC2Layout::WorldGuiElementOffset::FollowVehicleId, id);
		Write<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowEnabledFlag, 1);
		Write<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowVehicleFlag, 1);
		Write(element, NC2Layout::WorldGuiElementOffset::FollowVehicleUniqueId, Read<std::uint32_t>(NC2Logic::CVehicleArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CVehicleArray::ElementSize, NC2Layout::VehicleOffset::UniqueId));
		WorldDisplayElement_l_FollowMode_Update(element);
	}

	// Original function: FUN_004bb449.
	std::int32_t WorldDisplayElement_FollowMode_GetVehicleId(void* element)
	{
		WorldDisplayElement_l_FollowMode_Validate(element);

		return NC2Lifetime::Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowEnabledFlag) != 0 && NC2Lifetime::Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowVehicleFlag) != 0 ? NC2Lifetime::Read<std::int32_t>(element, NC2Layout::WorldGuiElementOffset::FollowVehicleId) : -1;
	}

	// Original function: FUN_004bb470; CWorldDisplayElement::FollowMode_End.
	void WorldDisplayElement_FollowMode_End(void* element)
	{
		std::memset(static_cast<std::byte*>(element) + NC2Layout::WorldGuiElementOffset::FollowEnabledFlag, 0, 24);
	}

	// Original function: FUN_004bb484; both modes share the 0x18-byte state.
	void WorldDisplayElement_l_FollowMode_Validate(void* element)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowEnabledFlag) == 0)
		{

			return;
		}

		const void* object = nullptr;
		std::uint32_t revision = 0;

		if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowHumanFlag) != 0)
		{
			object = NC2Logic::CHumanArray::mStaticVars + Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::FollowHumanId) * NC2Logic::CHumanArray::ElementSize;
			revision = Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::FollowHumanUniqueId);
		}
		else if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowVehicleFlag) != 0)
		{
			object = NC2Logic::CVehicleArray::mStaticVars + Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::FollowVehicleId) * NC2Logic::CVehicleArray::ElementSize;
			revision = Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::FollowVehicleUniqueId);
		}

		if (object == nullptr || Read<std::uint8_t>(object, NC2Layout::ActorIdentityOffset::Allocated) == 0 || Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::UniqueId) != revision)
			WorldDisplayElement_FollowMode_End(element);
	}

	// Original function: FUN_004bb4f1; use the map-moveable id, including vehicle passengers.
	void WorldDisplayElement_l_FollowMode_Update(void* element)
	{
		using namespace NC2Lifetime;
		WorldDisplayElement_l_FollowMode_Validate(element);

		if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowEnabledFlag) == 0)
			return;

		std::uint32_t moveable;

		if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowHumanFlag) != 0)
		{
			moveable = Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars + Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::FollowHumanId) * NC2Logic::CHumanArray::ElementSize, NC2Layout::HumanOffset::MapMoveableId);
		}
		else if (Read<std::uint8_t>(element, NC2Layout::WorldGuiElementOffset::FollowVehicleFlag) != 0)
		{
			moveable = Read<std::uint32_t>(NC2Logic::CVehicleArray::mStaticVars + Read<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::FollowVehicleId) * NC2Logic::CVehicleArray::ElementSize, NC2Layout::VehicleOffset::MapMoveableId);
		}
		else
		{

			return;
		}

		std::int32_t point[2];
		void* display = Read<void*>(element, NC2Layout::WorldGuiElementOffset::Display);
		Display_DE_MapMoveableToGlobalWorldPixelCoordinates(display, nullptr, moveable, point);
		point[1] = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(point[1]) - 25u);
		NC2E2::DE_SetWantedPositionToPixelPoint(display, point);
	}

	// Original function: FUN_004bb57b; Windows maps the brightest channel through a PCX palette.
	void WorldDisplayElement_DarkenBitmap_Init(void* element, std::uint32_t variant)
	{
		using namespace NC2Lifetime;
		WorldDisplayElement_DarkenBitmap_Exit(element);
		Write(element, NC2Layout::WorldGuiElementOffset::DarkenVariant, variant);

		if (variant == 0)
		{

			return;
		}

		char path[200];
		NXBasics::Runtime_PrintString(path, "data\\gui\\palettes\\ingame_remap_%2.2d.pcx", variant);
		auto* palette = NXBasics::XB_PictureTool_LoadPaletteOutOfPicture(path);
		Write(element, NC2Layout::WorldGuiElementOffset::DarkenPalette, palette);
	}

	// Original function: FUN_004bb669.
	void WorldDisplayElement_DarkenBitmap_Exit(void* element)
	{
		void* palette = WorldDisplayElement_ReadField<void*>(element, NC2Layout::WorldGuiElementOffset::DarkenPalette);
		WorldDisplayElement_WriteField<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::DarkenVariant, 0);

		if (palette != nullptr)
		{
			using TDeletingDestructor = void*(__thiscall*)(void*, std::uint32_t);
			auto* methods = *static_cast<TDeletingDestructor**>(palette);
			methods[3](palette, 1);
		}

		WorldDisplayElement_WriteField<void*>(element, NC2Layout::WorldGuiElementOffset::DarkenPalette, nullptr);
	}

	// Original function: FUN_004bb6a6; CWorldDisplayElement::l_DarkenBitmap_Do.
	void WorldDisplayElement_DarkenBitmap_Do(const void* element, NXBasics::CBitmap* bitmap)
	{
		// +0x238 is the palette variant selected by DarkenBitmap_Init (004bb57b).

		if (WorldDisplayElement_ReadField<std::uint32_t>(element, NC2Layout::WorldGuiElementOffset::DarkenVariant) == 0)
			return;

		const std::uint32_t depth = WorldDisplayElement_ReadField<std::uint32_t>(bitmap, NXBasics::BitmapOffset::Depth);

		if (depth != 2u)
		{

			return;
		}

		const std::uint32_t width = WorldDisplayElement_ReadField<std::uint32_t>(bitmap, NXBasics::BitmapOffset::Width);
		const std::uint32_t height = WorldDisplayElement_ReadField<std::uint32_t>(bitmap, NXBasics::BitmapOffset::Height);
		const std::uint32_t rowPixels = WorldDisplayElement_ReadField<std::uint32_t>(bitmap, NXBasics::BitmapOffset::RowPixelCount);

		if (depth == 2u)
		{
			auto* pixels = WorldDisplayElement_ReadField<std::uint32_t*>(bitmap, NXBasics::BitmapOffset::TrueColorMemory);
			void* palette = WorldDisplayElement_ReadField<void*>(element, NC2Layout::WorldGuiElementOffset::DarkenPalette);
			const std::uint32_t* table = NXBasics::Palette_GetTrueColorTablePtr(palette);

			for (std::uint32_t y = 0; y < height; ++y)
			{
				for (std::uint32_t x = 0; x < width; ++x)
				{
					const std::uint32_t pixel = *pixels;
					std::uint32_t brightness = (pixel >> 16) & 255u;
					const std::uint32_t green = (pixel >> 8) & 255u;
					const std::uint32_t blue = pixel & 255u;

					if (brightness < green)
						brightness = green;

					if (brightness < blue)
						brightness = blue;

					*pixels++ = table[brightness];
				}

				const std::uint32_t address = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pixels)) + (rowPixels - width) * 4u;
				pixels = reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(address));
			}
		}
	}

	// Original function: FUN_004ba1ba
	// Original method: CWorldDisplayElement::XGui_BE_Element_Draw.
	void WorldDisplayElement_Draw(void* element, NXBasics::CBitmap* bitmap, NXBasics::SRectangle* rectangle)
	{
		static_assert(sizeof(void*) == 4);

		const auto readWord = [](const void* object, std::size_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));

			return value;
		};

		const auto readByte = [](const void* object, std::size_t offset) -> std::uint8_t
		{
			std::uint8_t value;
			std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));

			return value;
		};

		const auto readPointer = [&readWord](const void* object, std::size_t offset) -> void*
		{

			return reinterpret_cast<void*>(static_cast<std::uintptr_t>(readWord(object, offset)));
		};

		WorldDisplayElement_ValidateObjectUnderMouse(element);
		NC2E2::Display_UpdateDisplay(readPointer(element, NC2Layout::WorldGuiElementOffset::Display));
		auto* presentation = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay();
		const bool mainWorld = presentation->IsMainWorld(element);
		NXBasics::SRectangle nativeRectangle(0, 0, presentation->GetWidth(), presentation->GetHeight());

		if (mainWorld)
		{
			bitmap = presentation->GetWorldComposition();
			auto* worldBitmap = static_cast<NXBasics::CBitmap*>(readPointer(element, NC2Layout::WorldGuiElementOffset::Bitmap));
			worldBitmap->CopyIntoBitmap(bitmap, 0, 0);
			rectangle = &nativeRectangle;
		}
		else
		{
			NC2InGameGuiBase::BaseBitmapToolElement_Draw(element, bitmap, rectangle);
		}

		WorldDisplayElement_DarkenBitmap_Do(element, bitmap);

		if (readWord(element, NC2Layout::WorldGuiElementOffset::ObjectUnderMouse) == 4u)
		{
			void* display = readPointer(element, NC2Layout::WorldGuiElementOffset::Display);

			if (readWord(element, NC2Layout::WorldGuiElementOffset::HoveredPlayerId) == readWord(display, NC2Layout::EngineDisplayOffset::PlayerId))
			{
				const bool enabled = readByte(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, 0xE1) == 0;
				NC2E2::Display_DrawTooltipOverlay_External_House(display, readWord(element, NC2Layout::WorldGuiElementOffset::HoveredHouseId), bitmap, enabled);
			}
		}

		if (readWord(element, NC2Layout::WorldGuiElementOffset::ObjectUnderMouse) == 3u)
		{
			void* display = readPointer(element, NC2Layout::WorldGuiElementOffset::Display);

			if (readWord(element, NC2Layout::WorldGuiElementOffset::HoveredPlayerId) == readWord(display, NC2Layout::EngineDisplayOffset::PlayerId))
			{
				const bool enabled = readByte(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, 0xE1) == 0;
				NC2E2::Display_DrawTooltipOverlay_External_Vehicle(display, readWord(element, NC2Layout::WorldGuiElementOffset::HoveredVehicleId), bitmap, enabled);
			}
		}

		// The original branch also accepts an unordered comparison.

		if (!(NC2Logic::gGameTime.speed >= 0.01) || NC2Logic::gGameStaticVars.networkWait != 0)
			bitmap->Tool_Darken();

		if (WorldDisplayElement_MouseSelectingArea_IsAreaValid(element))
		{
			// Inline the two SPoint copies performed by 00439842.
			std::int32_t firstPoint[2];
			std::int32_t lastPoint[2];

			std::memcpy(firstPoint, static_cast<const std::byte*>(element) + NC2Layout::WorldGuiElementOffset::AreaStartX, sizeof(firstPoint));
			std::memcpy(lastPoint, static_cast<const std::byte*>(element) + NC2Layout::WorldGuiElementOffset::AreaEndX, sizeof(lastPoint));

			NC2E2::Display_GlobalWorldPixelToLocalDisplayPixelCoordinates(readPointer(element, NC2Layout::WorldGuiElementOffset::Display), firstPoint, true);
			NC2E2::Display_GlobalWorldPixelToLocalDisplayPixelCoordinates(readPointer(element, NC2Layout::WorldGuiElementOffset::Display), lastPoint, true);

			if (mainWorld)
			{
				presentation->WorldToHud(firstPoint);
				presentation->WorldToHud(lastPoint);
			}

			const std::uint32_t widthBits = static_cast<std::uint32_t>(lastPoint[0]) - static_cast<std::uint32_t>(firstPoint[0]);
			const std::uint32_t heightBits = static_cast<std::uint32_t>(lastPoint[1]) - static_cast<std::uint32_t>(firstPoint[1]);

			NXBasics::SRectangle selection(firstPoint[0], firstPoint[1], NXBasics::Runtime_TimeSigned32(widthBits), NXBasics::Runtime_TimeSigned32(heightBits));
			NXBasics::SColorRGB white(255, 255, 255);

			(mainWorld ? presentation->GetHudBitmap() : bitmap)->Draw_Rectangle(selection, white);
		}

		void* display = readPointer(element, NC2Layout::WorldGuiElementOffset::Display);

		// Original double at 004f6ff8: 120.0.

		if (readByte(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) != 0 && !(NC2Logic::gGameTime.speed >= 120.0) && NC2Logic::gGameTime.maximizeSpeed == 0)
		{
			NC2E2::AmbientSoundManager_UpdateAmbientSounds(NC2E2::gE2AmbientSoundManager, display);
			NC2E2::CE2AnimalSoundManager::sTheObjectPtr->PlayAnimalSounds(readPointer(element, NC2Layout::WorldGuiElementOffset::Display));
			NC2E2::WorldMusic_UpdateVisiblePlayerSelection();
		}

		display = readPointer(element, NC2Layout::WorldGuiElementOffset::Display);

		if (readByte(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) != 0 && !(NC2Logic::gGameTime.speed >= 0.01))
		{
			auto* textBitmap = mainWorld ? presentation->GetHudBitmap() : bitmap;
			char text[200];
			NXBasics::Runtime_PrintString(text, "* %s *", NC2GuiToolsBase::StringTool_GetUpdate103StringPtr(20));

			const auto& guiResources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;

			auto* font = guiResources.font10;
			const auto* colour = reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(guiResources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);

			const std::int32_t halfHeight = textBitmap->GetRectangle().height / 2;
			const std::int32_t y = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(halfHeight) + 180u);
			const std::int32_t textWidth = NXBasics::Font_GetPixelWidth(font, text);
			const std::uint32_t remainingWidth = static_cast<std::uint32_t>(textBitmap->GetRectangle().width) - static_cast<std::uint32_t>(textWidth);
			const std::int32_t x = NXBasics::Runtime_TimeSigned32(remainingWidth) / 2;

			NC2GuiToolsBase::DrawTool_PrintStringX(textBitmap, x, y, font, colour, text);
		}

		const std::uint32_t frameMode = readWord(element, NC2Layout::WorldGuiElementOffset::FrameMode);

		if (frameMode != 0)
			NC2GuiToolsBase::DrawTool_DrawFrame(nullptr, bitmap, rectangle, NXBasics::Runtime_TimeSigned32(frameMode));

		if (mainWorld)
			presentation->BitmapToScreen(bitmap, 0, 0);
	}

	// Original function: FUN_004baf71
	void WorldDisplayElement_ValidateObjectUnderMouse(void* element)
	{
		auto* data = static_cast<std::byte*>(element);

		if (!NC2E2::DisplaySearchObject_Validate(data + NC2Layout::WorldGuiElementOffset::ObjectUnderMouse))
		{

			return;
		}

		std::uint32_t displayAddress;
		std::memcpy(&displayAddress, data + NC2Layout::WorldGuiElementOffset::Display, sizeof(displayAddress));

		auto* display = reinterpret_cast<std::byte*>(static_cast<std::uintptr_t>(displayAddress));
		const std::uint32_t zero = 0;

		std::memcpy(display + NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount, &zero, sizeof(zero));
		std::memcpy(data + NC2Layout::WorldGuiElementOffset::HumanInteractionAction, &zero, sizeof(zero));
		std::memcpy(data + NC2Layout::WorldGuiElementOffset::VehicleInteractionAction, &zero, sizeof(zero));

		std::memset(data + NC2Layout::WorldGuiElementOffset::PrimaryCommand, 0, 20);
		std::memset(data + NC2Layout::WorldGuiElementOffset::SecondaryCommand, 0, 20);
	}

	// Original function: FUN_004bafc2
	bool WorldDisplayElement_MouseSelectingArea_IsAreaValid(const void* element)
	{
		const auto* data = static_cast<const std::byte*>(element);

		std::uint8_t active;
		std::memcpy(&active, data + NC2Layout::WorldGuiElementOffset::AreaSelectionActiveFlag, sizeof(active));

		if (active == 0)
			return false;

		const auto hasMinimumDistance = [data](std::size_t firstOffset, std::size_t lastOffset) -> bool
		{
			std::uint32_t first;
			std::uint32_t last;

			std::memcpy(&first, data + firstOffset, sizeof(first));
			std::memcpy(&last, data + lastOffset, sizeof(last));

			const std::uint32_t difference = first - last;
			const std::uint32_t magnitude = (difference & NXBasics::IntegerRange::Bit31) != 0 ? 0u - difference : difference;

			// Preserve wrapped subtraction and the signed comparison after NEG.

			return magnitude >= 5u && magnitude <= NXBasics::IntegerRange::Int32Maximum;
		};

		if (!hasMinimumDistance(NC2Layout::WorldGuiElementOffset::AreaStartX, NC2Layout::WorldGuiElementOffset::AreaEndX))
			return false;

		return hasMinimumDistance(NC2Layout::WorldGuiElementOffset::AreaStartY, NC2Layout::WorldGuiElementOffset::AreaEndY);
	}
}
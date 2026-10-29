#include "XGuiElementAccess.h"
#include "InGameGuiStorage.h"
#include "C2EngineDisplayAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiObservationWindow.h"
#include "WorldDisplayElementAccess.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include "XList.h"
#include <array>
#include <cstring>
#include <new>

extern void __fastcall Display_DE_MapMoveableToGlobalWorldPixelCoordinates(void* display, void* unusedEdx, std::uint32_t moveableId, std::int32_t* point);

namespace NC2InGameGuiWorldWindow
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2InGameGuiWorldElement;
	using namespace NC2InGameGuiInputManager;
	using namespace NC2Logic;

	static void* Display(void* window)
	{

		return Read<void*>(Read<void*>(window, NC2Layout::ObservationWindowOffset::WorldElement), NC2Layout::WorldGuiElementOffset::Display);
	}

	static void* __fastcall WorldObserveDisplayWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall WorldObserveDisplayWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t param1Payload, std::uintptr_t param2Payload, std::uintptr_t param3Payload);
	// Complete Windows virtual table 004F7080..004F70CB.
	static const std::array<std::uintptr_t, 19> gObservationWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&WorldObserveDisplayWindow_Delete>());
		t[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&WorldObserveDisplayWindow_Message>());

		return t;
	}();

	// Original function: FUN_004bd3c6; Windows CWorldObserveDisplayWindow allocation 0x8C.
	void* WorldObserveDisplayWindow_Construct(void* window)
	{
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		NXBasics::SRectangle area(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 260u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 200u) / 2, 260, 200);
		BaseToolWindow_Construct(window, &area);
		std::memset(static_cast<std::byte*>(window) + offsetof(SObservationWindowStorage, worldElement), 0, sizeof(SObservationWindowStorage) - offsetof(SObservationWindowStorage, worldElement));
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gObservationWindowVirtualTable.data());
		area = NXBasics::SRectangle(2, 2, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22, 14);
		auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiWindowMoverStorage), 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &area, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(240)));
		area = NXBasics::SRectangle(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18, 2, 16, 14);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiGfxButtonStorage), 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &area));
		area = NXBasics::SRectangle(4, 20, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 8, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Height) - 24);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiWorldElement::SWorldGuiElementStorage), 1);
		void* element = allocation == nullptr ? nullptr : WorldDisplayElement_Construct(allocation, &area, false, true);
		Write(window, NC2Layout::ObservationWindowOffset::WorldElement, element);
		NXBaseGui::BaseWindow_Element_Add(window, element);
		WorldDisplayElement_DrawFrame_SetMode(element, 2);
		std::uint32_t point = 0;
		NC2E2::DE_Misc_GetDisplayedCenterMapPosition(Read<void*>(manager->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), &point);
		NC2E2::DE_SetWantedPositionToMapPoint(Display(window), &point);
		NC2E2::DE_ControllingPlayer_SetId(Display(window), manager->GetLocalPlayerId());
		WorldDisplayElement_Misc_LeftMouseClick_HandlerSet(element, window, 20000);
		WorldDisplayElement_Misc_LeftMouseDoubleClick_HandlerSet(element, window, 20001);
		area = NXBasics::SRectangle(4, 20, 100, 14);

		for (std::uint32_t button = 0; button < 4; ++button)
		{
			allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
			NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : BaseToolTextButton_Construct(allocation, nullptr, &area, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(241 + button), 10001 + button, window));
			area.x += area.width;
		}

		WorldObserveDisplayWindow_l_Follow_Start(window);
		manager->ObservationWindow_Register(window);
		manager->FrameCall_Register(window);

		return window;
	}

	// Original function: FUN_004bd67e.
	static void WorldObserveDisplayWindow_Destruct(void* window)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gObservationWindowVirtualTable.data());
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		manager->FrameCall_UnRegister(window);
		manager->ObservationWindow_UnRegister(window);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
	}

	// Original function: FUN_004bd662.
	static void* __fastcall WorldObserveDisplayWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		WorldObserveDisplayWindow_Destruct(window);

		if (flags & 1u)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004bd9e4.
	void WorldObserveDisplayWindow_l_Follow_Stop(void* window)
	{
		Write<std::int32_t>(window, NC2Layout::ObservationWindowOffset::HumanId, -1);
		Write<std::int32_t>(window, NC2Layout::ObservationWindowOffset::VehicleId, -1);
		Write<std::int32_t>(window, NC2Layout::ObservationWindowOffset::HouseId, -1);
		Write<std::uint32_t>(window, NC2Layout::ObservationWindowOffset::HumanRevision, 0);
		Write<std::uint32_t>(window, NC2Layout::ObservationWindowOffset::VehicleRevision, 0);
		Write<std::uint32_t>(window, NC2Layout::ObservationWindowOffset::HouseRevision, 0);
		Write<std::uint8_t>(Read<void*>(window, NC2Layout::ObservationWindowOffset::WorldElement), NC2Layout::WorldGuiElementOffset::InputEnabledFlag, 1);
	}

	// Original function: FUN_004bd907.
	void WorldObserveDisplayWindow_l_Follow_Human(void* window, std::int32_t id)
	{
		WorldObserveDisplayWindow_l_Follow_Stop(window);
		Write(window, NC2Layout::ObservationWindowOffset::HumanId, id);
		Write(window, NC2Layout::ObservationWindowOffset::HumanRevision, Read<std::uint32_t>(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize, NC2Layout::HumanOffset::UniqueId));
		Write<std::uint8_t>(Read<void*>(window, NC2Layout::ObservationWindowOffset::WorldElement), NC2Layout::WorldGuiElementOffset::InputEnabledFlag, 0);
	}

	// Original function: FUN_004bd934.
	void WorldObserveDisplayWindow_l_Follow_Vehicle(void* window, std::int32_t id)
	{
		WorldObserveDisplayWindow_l_Follow_Stop(window);
		Write(window, NC2Layout::ObservationWindowOffset::VehicleId, id);
		Write(window, NC2Layout::ObservationWindowOffset::VehicleRevision, Read<std::uint32_t>(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize, NC2Layout::VehicleOffset::UniqueId));
		Write<std::uint8_t>(Read<void*>(window, NC2Layout::ObservationWindowOffset::WorldElement), NC2Layout::WorldGuiElementOffset::InputEnabledFlag, 0);
	}

	// Original function: FUN_004bd964.
	void WorldObserveDisplayWindow_l_Follow_House(void* window, std::int32_t id)
	{
		WorldObserveDisplayWindow_l_Follow_Stop(window);
		Write(window, NC2Layout::ObservationWindowOffset::HouseId, id);
		Write(window, NC2Layout::ObservationWindowOffset::HouseRevision, Read<std::uint32_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize, NC2Layout::HouseOffset::UniqueId));
		Write<std::uint8_t>(Read<void*>(window, NC2Layout::ObservationWindowOffset::WorldElement), NC2Layout::WorldGuiElementOffset::InputEnabledFlag, 0);
	}

	// Original function: FUN_004bd997.
	void WorldObserveDisplayWindow_l_Follow_Start(void* window)
	{
		WorldObserveDisplayWindow_l_Follow_Stop(window);
		const auto type = Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType);

		if (type == 1)
		{
			WorldObserveDisplayWindow_l_Follow_Human(window, Selection_GetSelectedSingleHumanId(gInputManager));
		}
		else if (type == 4)
		{
			WorldObserveDisplayWindow_l_Follow_Vehicle(window, Selection_GetSelectedSingleVehicleId(gInputManager));
		}
		else if (type == 6)
		{
			WorldObserveDisplayWindow_l_Follow_House(window, Selection_GetSelectedHouseId(gInputManager));
		}
	}

	// Original function: FUN_004bda0c.
	void WorldObserveDisplayWindow_l_Follow_Validate(void* window)
	{
		const std::byte* arrays[3] = { CHumanArray::mStaticVars, CVehicleArray::mStaticVars, CHouseArray::mStaticVars };
		constexpr std::uint32_t strides[3] = { 0xC58, 0x54C, 0x7E0 };

		for (std::uint32_t type = 0; type < 3; ++type)
		{
			const auto offset = 0x74u + type * 8u;
			const auto id = Read<std::int32_t>(window, offset);

			if (id == -1)
				continue;

			const auto* object = arrays[type] + static_cast<std::uint32_t>(id) * strides[type];

			if (Read<std::uint8_t>(object, NC2Layout::ActorIdentityOffset::Allocated) == 0 || Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::UniqueId) != Read<std::uint32_t>(window, offset + NC2Layout::ObjectReferenceOffset::Revision))
				WorldObserveDisplayWindow_l_Follow_Stop(window);
		}
	}

	static void SelectFollowed(void* window)
	{
		if (Read<std::int32_t>(window, NC2Layout::ObservationWindowOffset::HumanId) != -1)
		{
			InputMode_Cancel(gInputManager);
			Selection_SelectSingleHuman(gInputManager, Read<std::int32_t>(window, NC2Layout::ObservationWindowOffset::HumanId));
		}
		else if (Read<std::int32_t>(window, NC2Layout::ObservationWindowOffset::VehicleId) != -1)
		{
			InputMode_Cancel(gInputManager);
			Selection_SelectSingleVehicle(gInputManager, Read<std::int32_t>(window, NC2Layout::ObservationWindowOffset::VehicleId));
		}
		else if (Read<std::int32_t>(window, NC2Layout::ObservationWindowOffset::HouseId) != -1)
		{
			InputMode_Cancel(gInputManager);
			Selection_SelectHouse(gInputManager, Read<std::int32_t>(window, NC2Layout::ObservationWindowOffset::HouseId));
		}
	}

	// Original function: FUN_004bd6ae.
	static bool __fastcall WorldObserveDisplayWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t, std::uintptr_t)
	{
		if (message != 0x800)
			return false;

		if (command == 10001)
		{
			NC2E2::DE_Zoom_ToggleLevel(Display(window), false);
		}
		else if (command == 10002)
		{
			WorldObserveDisplayWindow_l_Follow_Start(window);
		}
		else if (command == 10003 || command == 20001)
		{
			WorldObserveDisplayWindow_l_Follow_Validate(window);
			SelectFollowed(window);

			if (command == 20001)
				NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Selection_ActionButtons_BringUp();
		}
		else if (command == 10004 || command == 20000)
		{
			std::uint32_t point = 0;
			NC2E2::DE_Misc_GetDisplayedCenterMapPosition(Display(window), &point);
			NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), &point);
		}
		else if (command == 0x7EE)
		{
			WorldObserveDisplayWindow_l_Follow_Validate(window);
			const auto human = Read<std::int32_t>(window, NC2Layout::ObservationWindowOffset::HumanId), vehicle = Read<std::int32_t>(window, NC2Layout::ObservationWindowOffset::VehicleId), house = Read<std::int32_t>(window, NC2Layout::ObservationWindowOffset::HouseId);

			if (human != -1 || vehicle != -1)
			{
				const auto moveable = human != -1 ? Read<std::uint32_t>(CHumanArray::mStaticVars + static_cast<std::uint32_t>(human) * CHumanArray::ElementSize, NC2Layout::HumanOffset::MapMoveableId) : Read<std::uint32_t>(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(vehicle) * CVehicleArray::ElementSize, NC2Layout::VehicleOffset::MapMoveableId);
				std::int32_t point[2];
				Display_DE_MapMoveableToGlobalWorldPixelCoordinates(Display(window), nullptr, moveable, point);
				point[1] -= 30;
				NC2E2::DE_SetWantedPositionToPixelPoint(Display(window), point);
			}
			else if (house != -1)
			{
				NC2E2::DE_SetWantedPositionToMapPoint(Display(window), reinterpret_cast<const std::uint32_t*>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(house) * CHouseArray::ElementSize + 0x1C));
			}
		}

		return true;
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a0bc5.
	void CInGameGuiManager::ActivateDesktop()
	{
		NXBaseGui::CDesktop::sTheObjectPtr->Element_Activate(static_cast<NXBaseGui::CBaseElement*>(NC2Lifetime::Read<void*>(this, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement)));
	}

	// Original function: FUN_004a2997.
	void* CInGameGuiManager::MiscWindows_OverviewWindow_GetPtr() const
	{

		return NC2Lifetime::Read<void*>(this, NC2Layout::InGameGuiManagerOffset::OverviewWindow);
	}

	// Original function: FUN_004a0bf2.
	void CInGameGuiManager::ObservationWindow_CreateNewOne()
	{
		auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiWorldWindow::SObservationWindowStorage), 1);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(reinterpret_cast<NXBaseGui::CBaseWindow*>(allocation == nullptr ? nullptr : NC2InGameGuiWorldWindow::WorldObserveDisplayWindow_Construct(allocation)), false);
	}

	// Original function: FUN_004a0c17.
	void CInGameGuiManager::ObservationWindow_Register(void* window)
	{
		std::launder(reinterpret_cast<NXBasics::CListBase*>(mLegacyState + NC2Layout::InGameGuiManagerOffset::ObservationWindowList))->InsertAtEnd(window);
	}

	// Original function: FUN_004a0c26.
	void CInGameGuiManager::ObservationWindow_UnRegister(void* window)
	{
		std::launder(reinterpret_cast<NXBasics::CListBase*>(mLegacyState + NC2Layout::InGameGuiManagerOffset::ObservationWindowList))->l_Base_RemoveElement(window);
	}

	// Original function: FUN_004a0c35.
	void CInGameGuiManager::ObservationWindow_CloseAll()
	{
		NXBasics::CListBase::Iterator iterator(*std::launder(reinterpret_cast<NXBasics::CListBase*>(mLegacyState + NC2Layout::InGameGuiManagerOffset::ObservationWindowList)));

		for (auto* window = iterator.First(); window != nullptr; window = iterator.Next())
		{
			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
	}

	// Original function: FUN_004a0cc2.
	void CInGameGuiManager::Exploration_Toggle()
	{
		Exploration_Set(!Exploration_GetFlag());
	}

	// Original function: FUN_004a290b.
	void CInGameGuiManager::MiscWindows_OverviewWindow_Toggle()
	{
		if (NC2Lifetime::Read<void*>(this, NC2Layout::InGameGuiManagerOffset::OverviewWindow) != nullptr)
		{
			MiscWindows_OverviewWindow_Close();
		}
		else
		{
			MiscWindows_OverviewWindow_Open();
		}
	}

	// Original function: FUN_004a28ec.
	void CInGameGuiManager::MiscWindows_OverviewWindow_Close()
	{
		auto* window = NC2Lifetime::Read<NXBaseGui::CBaseWindow*>(this, NC2Layout::InGameGuiManagerOffset::OverviewWindow);

		if (window != nullptr)
		{
			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(window);
			MiscWindows_OverviewWindow_Closed();
		}
	}
}
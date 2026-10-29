#include "InGameGuiStorage.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2UserNetworkCommandAccess.h"
#include "C2WallRoadConstruction.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "InGameGuiWallRoadWindows.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiWindowAccess.h"
#include <array>
#include <cstring>

namespace NC2InGameGuiSelection
{
	using namespace NC2Lifetime;
	using NXBasics::SRectangle;

	namespace
	{
		// Windows folds this inherited virtual into the existing unconditional true leaf.
		bool __fastcall SelectionBuildWindow_CanBeHidden(void*)
		{

			return true;
		}

		// Original function: FUN_004bdb84; camera and radius-four cancellation controls.
		bool __fastcall SelectionWallBuildWindow_MessageHandle(void*, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t, std::uintptr_t)
		{
			const auto* point = NC2InGameGuiInputManager::Selection_GetSelectedWallBuildPlacePositionRef(NC2InGameGuiInputManager::gInputManager);
			void* data = nullptr;
			const auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

			if (NC2Logic::MapMigPoint_IsInsideMap(point))
				data = NC2Logic::RoadBuildManager_GetRoadBuildDataPtr(NC2Logic::gWallBuildManager, manager->GetLocalPlayerId(), point);

			if (message == 0x1000)
				return command == 2001;

			if (message != 0x800)
				return false;

			if (command == 10100 && data != nullptr)
			{
				NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(Read<void*>(manager, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement), NC2Layout::WorldGuiElementOffset::Display), static_cast<const std::uint32_t*>(data));
			}
			else if (command == 10101 && data != nullptr)
			{
				std::byte packet[20]{ };
				Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, 0x79);
				Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(manager->GetLocalPlayerId()));
				Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, NC2Logic::MapMigPoint_GetLong(data));
				Write<std::uint32_t>(packet, NC2Layout::UserNetworkCommandOffset::ObjectRevision, 4);
				NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, packet, false, false);
			}

			return true;
		}

		// Original function: FUN_004bdc4d.
		void __fastcall SelectionWallBuildWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
		{
			NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
			const SRectangle title(0, 0, 200, 16);
			SelectionTool_FillRectangle(bitmap, title, 5);
			SelectionTool_PrintCenteredString(bitmap, title, true, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(310));
		}

		// Complete native nineteen-entry table at 004F70CC..004F7117.
		const std::array<std::uintptr_t, 19> gSelectionWallBuildWindowVirtualTable = []
		{
			std::array<std::uintptr_t, 19> table;
			std::memcpy(table.data(), NC2InGameGuiBase::gBaseToolWindowVirtualTable, sizeof(table));
			table[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionWallBuildWindow_MessageHandle>());
			table[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionWallBuildWindow_Draw>());
			table[15] = reinterpret_cast<std::uintptr_t>(&SelectionBuildWindow_CanBeHidden);

			return table;
		}();

		// Original function: FUN_004bdd8b; camera and radius-four cancellation controls.
		bool __fastcall SelectionRoadBuildWindow_MessageHandle(void*, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t, std::uintptr_t)
		{
			const auto* point = NC2InGameGuiInputManager::Selection_GetSelectedRoadBuildPlacePositionRef(NC2InGameGuiInputManager::gInputManager);
			void* data = nullptr;
			const auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

			if (NC2Logic::MapMigPoint_IsInsideMap(point))
				data = NC2Logic::RoadBuildManager_GetRoadBuildDataPtr(NC2Logic::gRoadBuildManager, manager->GetLocalPlayerId(), point);

			if (message == 0x1000)
				return command == 2001;

			if (message != 0x800)
				return false;

			if (command == 10100 && data != nullptr)
			{
				NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(Read<void*>(manager, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement), NC2Layout::WorldGuiElementOffset::Display), static_cast<const std::uint32_t*>(data));
			}
			else if (command == 10101 && data != nullptr)
			{
				std::byte packet[20]{ };
				Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, 0x77);
				Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(manager->GetLocalPlayerId()));
				Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, NC2Logic::MapMigPoint_GetLong(data));
				Write<std::uint32_t>(packet, NC2Layout::UserNetworkCommandOffset::ObjectRevision, 4);
				NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, packet, false, false);
			}

			return true;
		}

		// Original function: FUN_004bde54.
		void __fastcall SelectionRoadBuildWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
		{
			NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
			const SRectangle title(0, 0, 200, 16);
			SelectionTool_FillRectangle(bitmap, title, 5);
			SelectionTool_PrintCenteredString(bitmap, title, true, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(320));
		}

		// Complete native nineteen-entry table at 004F7118..004F7163.
		const std::array<std::uintptr_t, 19> gSelectionRoadBuildWindowVirtualTable = []
		{
			std::array<std::uintptr_t, 19> table;
			std::memcpy(table.data(), NC2InGameGuiBase::gBaseToolWindowVirtualTable, sizeof(table));
			table[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionRoadBuildWindow_MessageHandle>());
			table[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionRoadBuildWindow_Draw>());
			table[15] = reinterpret_cast<std::uintptr_t>(&SelectionBuildWindow_CanBeHidden);

			return table;
		}();
	}

	// Original function: FUN_004bda98; full 74h-byte native window allocation.
	void* SelectionWallBuildWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle rectangle(state.displayWidth - 200, state.displayHeight - 100, 200, 100);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		Write<std::uint8_t>(window, offsetof(SWallBuildWindowStorage, unresolved70), 0);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSelectionWallBuildWindowVirtualTable.data());
		const SRectangle camera(4, 82, 192, 14);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
		auto* button = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructWithToolTip(memory, nullptr, &camera, "->", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(313), 10100, window);
		NXBaseGui::BaseWindow_Element_Add(window, button);
		const SRectangle cancel(4, 66, 192, 14);
		memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
		button = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructWithToolTip(memory, nullptr, &cancel, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(311), NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(312), 10101, window);
		NXBaseGui::BaseWindow_Element_Add(window, button);

		return window;
	}

	// Original function: FUN_004bdc9f; full 74h-byte native window allocation.
	void* SelectionRoadBuildWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle rectangle(state.displayWidth - 200, state.displayHeight - 100, 200, 100);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		Write<std::uint8_t>(window, offsetof(SRoadBuildWindowStorage, unresolved70), 0);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSelectionRoadBuildWindowVirtualTable.data());
		const SRectangle camera(4, 82, 192, 14);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
		auto* button = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructWithToolTip(memory, nullptr, &camera, "->", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(323), 10100, window);
		NXBaseGui::BaseWindow_Element_Add(window, button);
		const SRectangle cancel(4, 66, 192, 14);
		memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
		button = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructWithToolTip(memory, nullptr, &cancel, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(321), NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(322), 10101, window);
		NXBaseGui::BaseWindow_Element_Add(window, button);

		return window;
	}
}
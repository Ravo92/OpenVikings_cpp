#include "XPalette.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2UserNetworkCommandAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiLandscapeSelectionWindow.h"
#include "InGameGuiManager.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiWindowAccess.h"
#include "XNumericConstants.h"
#include <array>
#include <cstring>

namespace NC2InGameGuiSelection
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiInputManager;
	using namespace NC2InGameGuiBase;
	using namespace NC2GuiToolsBase;
	using NXBasics::SRectangle;
	static void* __fastcall SelectionLandscapeTypeWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall SelectionLandscapeTypeWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t, std::uint32_t);
	static void __fastcall SelectionLandscapeTypeWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle);
	static bool SelectionLandscapeTypeWindow_l_Landscape_NeedToBeUpdatedFromMap(void* window);
	static std::int32_t SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(void* window);
	static std::uint32_t SelectionLandscapeTypeWindow_l_Landscape_GetValency(void* window);
	static std::uint32_t SelectionLandscapeTypeWindow_l_Gui_CalculateChecksum(void* window);
	static void SelectionLandscapeTypeWindow_l_Gui_UpdateElements(void* window);
	// Complete native virtual table 004F7164..004F71AF.
	static const std::array<std::uintptr_t, 19> gSelectionLandscapeTypeVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&SelectionLandscapeTypeWindow_Delete);
		t[2] = reinterpret_cast<std::uintptr_t>(&SelectionLandscapeTypeWindow_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&SelectionLandscapeTypeWindow_Draw);

		return t;
	}();

	static const void* Point(void* window)
	{

		return static_cast<std::byte*>(window) + 0x72;
	}

	static const char* Text(std::uint32_t id)
	{

		return StringTool_GetGameGuiMiscWindowStringPtr(id);
	}

	static const NXBasics::SColorRGB* White()
	{

		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
	}

	// Original function: FUN_004bdea6; native allocation 0x88, including the unaligned packed map point.
	void* SelectionLandscapeTypeWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(state.displayWidth - 160, state.displayHeight - 240, 160, 240);
		BaseToolWindow_Construct(window, &area);
		std::memset(static_cast<std::byte*>(window) + 0x70, 0, 24);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSelectionLandscapeTypeVirtualTable.data());
		Write<std::uint32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::MapPosition, NXBasics::IntegerRange::UInt32Maximum);

		if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) == 9)
		{
			Write<std::uint8_t>(window, NC2Layout::LandscapeSelectionWindowOffset::ValidSelection, 1);
			Write(window, NC2Layout::LandscapeSelectionWindowOffset::MapPosition, Read<std::uint32_t>(Selection_GetSelectedEngineLandscapeMapPointRef(gInputManager), NC2Layout::MapCoordinatePairOffset::Coordinates));
			Write(window, NC2Layout::LandscapeSelectionWindowOffset::PlayerId, Selection_GetSelectedEngineLandscapePlayerId(gInputManager));
			Write(window, NC2Layout::LandscapeSelectionWindowOffset::EngineType, Selection_GetSelectedEngineLandscapeTypeId(gInputManager));
			Write(window, NC2Layout::LandscapeSelectionWindowOffset::Valency, Selection_GetSelectedEngineLandscapeValency(gInputManager));
		}

		SelectionLandscapeTypeWindow_l_Gui_UpdateElements(window);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_Register(window);

		return window;
	}

	// Original function: FUN_004bdf75.
	static void* __fastcall SelectionLandscapeTypeWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSelectionLandscapeTypeVirtualTable.data());
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_UnRegister(window);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);

		if (flags & 1u)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004bdfab.
	static bool __fastcall SelectionLandscapeTypeWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t, std::uint32_t)
	{
		if (message != 0x800)
			return message == 0x1000 && command == 2001;

		if (command == 0x7EE)
		{
			if (Read<std::uint32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::StateChecksum) != SelectionLandscapeTypeWindow_l_Gui_CalculateChecksum(window))
				SelectionLandscapeTypeWindow_l_Gui_UpdateElements(window);
		}
		else if (command == 10100)
		{
			NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), static_cast<const std::uint32_t*>(Point(window)));
		}
		else if (command == 10200 || command == 10300 || command == 10301)
		{
			std::byte packet[20]{ };
			Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, command == 10200 ? 0x7A : 0x7C);
			Write(packet, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(window, NC2Layout::LandscapeSelectionWindowOffset::PlayerId));
			Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::MapPosition));
			Write<std::uint32_t>(packet, NC2Layout::UserNetworkCommandOffset::ObjectRevision, command == 10301 ? 1 : 0);
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, packet, false, false);

			if (command == 10200)
				Selection_UnselectAll(gInputManager);
		}

		return true;
	}

	// Original function: FUN_004be0f6.
	static void __fastcall SelectionLandscapeTypeWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const SRectangle title(0, 0, 160, 16);
		SelectionTool_FillRectangle(bitmap, title, 5);
		SelectionTool_PrintCenteredString(bitmap, title, true, Text(480));

		if (Read<std::uint8_t>(window, NC2Layout::LandscapeSelectionWindowOffset::ValidSelection) == 0 || SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(window) == -1)
		{

			return;
		}

		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		char caption[200];
		bool resource = false;
		Tool_LandscapeType_GetTooltipStringPtr(SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(window), SelectionLandscapeTypeWindow_l_Landscape_GetValency(window), caption, nullptr, &resource);

		if (caption[0] != 0)
		{
			bitmap->Text_Print(font, White(), 6, 20, caption);

			if (resource)
				bitmap->Text_PrintV(font, White(), 16, 32, "(%d)", SelectionLandscapeTypeWindow_l_Landscape_GetValency(window));
		}

		std::int32_t y = 200;
		const auto id = SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(window);

		if (id != -1 && Read<std::uint32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::PlayerId) == NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId())
		{
			const auto type = Read<std::uint32_t>(NC2LogicExternals::External_Landscape_GetLogicDataPtr(id), NC2Layout::EngineLandscapeLogicViewOffset::LogicType);

			if (type == 82)
				y -= 40;

			if (type == 83 || type == 84)
				y -= 60;
		}

		NC2E2::EngineGraphic_Draw(SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(window), SelectionLandscapeTypeWindow_l_Landscape_GetValency(window), bitmap, 80, y, NC2Logic::gGameTime.tick, 256, Read<std::int32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::PlayerId), false);
	}

	// Original function: FUN_004be257.
	static bool SelectionLandscapeTypeWindow_l_Landscape_NeedToBeUpdatedFromMap(void* window)
	{
		if (Read<std::uint8_t>(window, NC2Layout::LandscapeSelectionWindowOffset::ValidSelection) == 0 || !NC2Logic::MapMigPoint_IsInsideMap(Point(window)))
			return false;

		const auto type = Read<std::uint32_t>(NC2LogicExternals::External_Landscape_GetLogicDataPtr(Read<std::int32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::EngineType)), NC2Layout::EngineLandscapeLogicViewOffset::LogicType);

		return Read<std::uint32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::PlayerId) == NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId() && (type == 83 || type == 84);
	}

	// Original function: FUN_004be29b.
	static std::int32_t SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(void* window)
	{
		if (!SelectionLandscapeTypeWindow_l_Landscape_NeedToBeUpdatedFromMap(window))
			return Read<std::int32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::EngineType);

		const auto id = Read<std::uint16_t>(NC2E2::EngineMap_GetConstMapMicroPointPtr(Point(window)), NC2Layout::EngineMapMicroPointOffset::LandscapeId);

		return id == NXBasics::IntegerRange::UInt16Maximum ? -1 : id;
	}

	// Original function: FUN_004be2c5.
	static std::uint32_t SelectionLandscapeTypeWindow_l_Landscape_GetValency(void* window)
	{
		if (!SelectionLandscapeTypeWindow_l_Landscape_NeedToBeUpdatedFromMap(window))
			return Read<std::uint32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::Valency);

		const auto* data = NC2Logic::GameMap_GetConstMapMicroPointPtr(Point(window));

		return Read<std::uint8_t>(data, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(data, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0;
	}

	static void AddButton(void* window, const SRectangle& area, std::uint32_t string, std::uint32_t command, bool camera)
	{
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : camera ? BaseToolTextButton_ConstructWithToolTip(memory, nullptr, &area, "->", Text(string), command, window) : BaseToolTextButton_Construct(memory, nullptr, &area, Text(string), command, window));
	}

	// Original function: FUN_004be2f2.
	static void SelectionLandscapeTypeWindow_l_Gui_UpdateElements(void* window)
	{
		NXBaseGui::BaseWindow_Element_DeleteAll(window);
		Write(window, NC2Layout::LandscapeSelectionWindowOffset::StateChecksum, SelectionLandscapeTypeWindow_l_Gui_CalculateChecksum(window));

		if (Read<std::uint8_t>(window, NC2Layout::LandscapeSelectionWindowOffset::ValidSelection) == 0)
			return;

		SRectangle area(4, 220, 152, 16);

		if (NC2Logic::MapMigPoint_IsInsideMap(Point(window)))
		{
			AddButton(window, area, 481, 10100, true);
			area.y -= area.height + 2;
		}

		if (SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(window) == -1)
			return;

		const auto* data = NC2LogicExternals::External_Landscape_GetLogicDataPtr(SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(window));

		if (Read<std::uint32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::PlayerId) != NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId())
		{

			return;
		}

		const auto type = Read<std::uint32_t>(data, NC2Layout::EngineLandscapeLogicViewOffset::LogicType);

		if (type == 82 || type == 83 || type == 84)
		{
			AddButton(window, area, 482, 10200, false);
			area.y -= area.height + 2;
		}

		if (type == 84)
		{
			AddButton(window, area, 483, 10300, false);
			area.y -= area.height + 2;
		}

		if (type == 83)
			AddButton(window, area, 484, 10301, false);
	}

	// Original function: FUN_004be488; a missing engine graphic returns 0xFFFFFFFF instead of the accumulated flags.
	static std::uint32_t SelectionLandscapeTypeWindow_l_Gui_CalculateChecksum(void* window)
	{
		if (Read<std::uint8_t>(window, NC2Layout::LandscapeSelectionWindowOffset::ValidSelection) == 0)
			return 0;

		std::uint32_t flags = NC2Logic::MapMigPoint_IsInsideMap(Point(window)) ? 1 : 0;

		if (SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(window) == -1)
			return NXBasics::IntegerRange::UInt32Maximum;

		const auto* data = NC2LogicExternals::External_Landscape_GetLogicDataPtr(SelectionLandscapeTypeWindow_l_Landscape_GetEngineId(window));

		if (Read<std::uint32_t>(window, NC2Layout::LandscapeSelectionWindowOffset::PlayerId) == NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId())
		{
			const auto type = Read<std::uint32_t>(data, NC2Layout::EngineLandscapeLogicViewOffset::LogicType);

			if (type == 82 || type == 83 || type == 84)
				flags |= 2;

			if (type == 84)
				flags |= 4;

			if (type == 83)
				flags |= 8;
		}

		return flags;
	}
}
#include "XPalette.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDrawingAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2StringAccess.h"
#include "C2UserNetworkCommandAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "InGameGuiSimpleSelectionWindows.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiWindowAccess.h"
#include "XMMouseManager.h"
#include <array>
#include <cstring>
#include <new>

namespace NC2InGameGuiSelection
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2InGameGuiInputManager;
	using namespace NC2GuiToolsBase;
	using NXBasics::SRectangle;

	static const NXBasics::SColorRGB* White()
	{

		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
	}

	static const char* Text(std::uint32_t id)
	{

		return StringTool_GetGameGuiMiscWindowStringPtr(id);
	}

	static CStaticToolTipHandler* Tooltips(void* window)
	{

		return std::launder(reinterpret_cast<CStaticToolTipHandler*>(static_cast<std::byte*>(window) + 0x70));
	}

	static std::byte* SelectedGuide(std::int32_t id)
	{
		if (id == -1)
			return nullptr;

		auto* guide = NC2Logic::CGuideArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CGuideArray::ElementSize;

		return Read<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag) != 0 ? guide : nullptr;
	}

	// Folded original function: FUN_004b86ab; also used by the native mission-information window.
	static bool __fastcall SelectionSimpleWindow_Message(void*, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t, std::uint32_t)
	{

		return message == 0x1000 && command == 2001;
	}

	static void* __fastcall SelectionGuideWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall SelectionGuideWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t, std::uint32_t);
	static void __fastcall SelectionGuideWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle);
	static bool __fastcall SelectionGuideWindow_ToolTip_Available(void* window, void*, const char** output);
	static void __fastcall SelectionHouseTypeWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle);
	static void __fastcall SelectionSingleAnimalWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle);
	// Complete native tables 004F71B0..004F71FB, 004F71FC..004F7247 and 004F72DC..004F7327.
	static const std::array<std::uintptr_t, 19> gSelectionGuideVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&SelectionGuideWindow_Delete);
		t[2] = reinterpret_cast<std::uintptr_t>(&SelectionGuideWindow_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&SelectionGuideWindow_Draw);
		t[13] = reinterpret_cast<std::uintptr_t>(&SelectionGuideWindow_ToolTip_Available);
		t[14] = reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden);

		return t;
	}();
	static const std::array<std::uintptr_t, 19> gSelectionHouseTypeVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[2] = reinterpret_cast<std::uintptr_t>(&SelectionSimpleWindow_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&SelectionHouseTypeWindow_Draw);

		return t;
	}();
	static const std::array<std::uintptr_t, 19> gSelectionSingleAnimalVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[2] = reinterpret_cast<std::uintptr_t>(&SelectionSimpleWindow_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&SelectionSingleAnimalWindow_Draw);

		return t;
	}();

	// Original function: FUN_004be649; native allocation 0x2F54.
	void* SelectionGuideWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(state.displayWidth - 170, state.displayHeight - 250, 170, 250);
		BaseToolWindow_Construct(window, &area);
		new (static_cast<std::byte*>(window) + 0x70) CStaticToolTipHandler;
		std::memset(static_cast<std::byte*>(window) + 0x70, 0, 12004);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSelectionGuideVirtualTable.data());
		const SRectangle camera(4, 18, 80, 14);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : BaseToolTextButton_ConstructWithToolTip(memory, nullptr, &camera, "->", Text(271), 10100, window));
		const SRectangle remove(86, 18, 80, 14);
		memory = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : BaseToolTextButton_ConstructWithToolTip(memory, nullptr, &remove, Text(272), Text(273), 10101, window));

		return window;
	}

	// Original function: FUN_004be754.
	static void* __fastcall SelectionGuideWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSelectionGuideVirtualTable.data());
		Tooltips(window)->~CStaticToolTipHandler();
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);

		if (flags & 1u)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004be786.
	static bool __fastcall SelectionGuideWindow_Message(void*, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t, std::uint32_t)
	{
		const auto id = Selection_GetSelectedGuideId(gInputManager);
		const auto* guide = SelectedGuide(id);

		if (message != 0x800)
			return message == 0x1000 && command == 2001;

		if (command == 10100 && guide != nullptr)
		{
			NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), reinterpret_cast<const std::uint32_t*>(guide + 0x14));
		}
		else if (command == 10101 && guide != nullptr)
		{
			std::byte packet[20]{ };
			Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, 0x7E);
			Write(packet, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(guide, NC2Layout::GuideOffset::PlayerId));
			Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
			Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(guide, NC2Layout::GuideOffset::UniqueId));
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, packet, false, false);
		}

		return true;
	}

	// Original function: FUN_004be83f.
	static void __fastcall SelectionGuideWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const SRectangle title(0, 0, 170, 16);
		SelectionTool_FillRectangle(bitmap, title, 5);
		SelectionTool_PrintCenteredString(bitmap, title, true, Text(270));
		Tooltips(window)->ClearAll();
		const auto* guide = SelectedGuide(Selection_GetSelectedGuideId(gInputManager));

		if (guide == nullptr)
		{

			return;
		}

		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		bitmap->Text_PrintV(font, White(), 4, 40, Text(274));
		std::int32_t x = 4, y = 66;

		for (std::uint32_t good = 0; good < 66; ++good)
		{
			const auto amount = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::GoodsCounts + good * NC2Layout::RecordSize::GuideGoodCountUnsigned);

			if (amount == 0)
				continue;

			if (x + 26 > 170)
			{
				y += 22;
				x = 4;
			}

			DrawTool_PrintStringXV(bitmap, x, y, font, White(), "{gfxgood:%d,1}", good, amount);
			char caption[100];
			NXBasics::Runtime_PrintString(caption, "%d %s", amount, NC2Logic::StringAccess_GetGoodNamePtr(good, amount));
			Tooltips(window)->Add(SRectangle(x, y, 24, 20), caption);
			x += 26;
		}
	}

	// Original function: FUN_004be9a9.
	static bool __fastcall SelectionGuideWindow_ToolTip_Available(void* window, void*, const char** output)
	{
		std::int32_t point[2];
		std::memcpy(point, NXSysMouseManager::CMouseManager::sTheObjectPtr, sizeof(point));
		BaseElement_GlobalToLocalCoordinates(window, point);
		const auto* caption = Tooltips(window)->Get(point);

		if (caption == nullptr)
			return false;

		*output = caption;

		return true;
	}

	// Original function: FUN_004be9ea; native allocation 0x74.
	void* SelectionHouseTypeWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(state.displayWidth - 160, state.displayHeight - 100, 160, 100);
		BaseToolWindow_Construct(window, &area);
		Write<std::uint8_t>(window, 0x70, 0);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSelectionHouseTypeVirtualTable.data());

		return window;
	}

	// Original function: FUN_004bea39.
	static void __fastcall SelectionHouseTypeWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const SRectangle title(0, 0, 160, 16);
		SelectionTool_FillRectangle(bitmap, title, 5);
		SelectionTool_PrintCenteredString(bitmap, title, true, Text(520));
		const auto type = Selection_GetSelectedEngineHouseTypeId(gInputManager);

		if (type != -1)
			bitmap->Text_PrintV(CGuiBaseDataManager::mStaticVars.resources.font08, White(), 4, 20, NC2Logic::StringAccess_GetHouseNamePtr(type, Selection_GetSelectedEngineHouseExtensionLevel(gInputManager), 1));
	}

	// Original function: FUN_004bef73; native allocation 0x74.
	void* SelectionSingleAnimalWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(state.displayWidth - 140, state.displayHeight - 160, 140, 160);
		BaseToolWindow_Construct(window, &area);
		Write<std::uint8_t>(window, 0x70, 0);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSelectionSingleAnimalVirtualTable.data());

		return window;
	}

	// Original function: FUN_004befda.
	static void __fastcall SelectionSingleAnimalWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const SRectangle title(0, 0, 140, 16);
		SelectionTool_FillRectangle(bitmap, title, 5);
		SelectionTool_PrintCenteredString(bitmap, title, true, Text(530));
		const auto id = Selection_GetSelectedSingleAnimalId(gInputManager);

		if (id == -1)
			return;

		const auto* animal = NC2Logic::CAnimalArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CAnimalArray::ElementSize;

		if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) == 0)
		{

			return;
		}

		NC2E2::DrawAtom_MapMoveable_DoDraw(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId), bitmap, 70, 110, nullptr, true, Read<std::int32_t>(animal, NC2Layout::AnimalOffset::PlayerId));
		bitmap->Text_Print(CGuiBaseDataManager::mStaticVars.resources.font08, White(), 4, 16, NC2Logic::StringAccess_GetTribeNamePtr(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType), 1));
	}
}
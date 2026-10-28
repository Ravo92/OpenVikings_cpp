#include "XPalette.h"
#include "C2E2StaticDataPalettes.h"
#include "CivilizationBuildings.h"
#include "C2EngineDrawingAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2StaticTribeTypeData.h"
#include "C2StringAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "InGameGuiTechTreeData.h"
#include "InGameGuiTechTreeWindow.h"
#include "InGameGuiWindowState.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XGuiWindowAccess.h"
#include "XNumericConstants.h"
#include <array>
#include <cstring>
#include <new>

namespace NC2E2
{
	// Original function: FUN_0048f6e9; CE2StaticDataHousesManager::GetHouseId overload.
	std::int32_t StaticDataHouses_GetHouseId(std::uint32_t tribe, std::uint32_t logicType, std::uint32_t* stage)
	{
		if (const auto session = NRemaster::CivilizationBuildings::GetHouseId(tribe, logicType, stage); session != -1) return session;
		for (std::int32_t index = 0; index < gStaticHouseCount; ++index)
		{
			const auto* entry = static_cast<const std::byte*>(gStaticHouseEntries) + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::EngineHouseTypeUnsigned;
			if (NC2Lifetime::Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::TribeType) != tribe) continue;
			for (std::uint32_t variant = 0; variant < NC2Lifetime::Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount); ++variant)
			{
				if (NC2Lifetime::Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::LogicTypesUnsigned + variant * NC2Layout::RecordSize::HouseStageValueUnsigned) == logicType)
				{
					*stage = variant;
					return index;
				}
			}
		}
		return -1;
	}
}

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2GuiToolsBase;
	static std::uint32_t LocalTribe()
	{
		const auto player = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
		return player >= 20 ? 1u : Read<std::uint32_t>(NC2Logic::gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeType);
	}
	static const std::byte* PlayerFlags(std::uint32_t stride, std::uint32_t type)
	{
		const auto player = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
		return static_cast<const std::byte*>(NC2Logic::gGameControl) + static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + LocalTribe() * stride + type;
	}
	// Original functions: FUN_004a6d66, FUN_004a6db3, FUN_004a6e00.
	bool TechTreeWindow_IsHouseTypeFree(std::uint32_t type)
	{
		const auto* flags = PlayerFlags(55, type);
		return Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AllowedHouses) != 0 && Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AvailableHouses) != 0;
	}
	bool TechTreeWindow_IsGoodTypeFree(std::uint32_t type)
	{
		const auto* flags = PlayerFlags(66, type);
		return Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AllowedGoods) != 0 && Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AvailableGoods) != 0;
	}
	bool TechTreeWindow_IsJobTypeFree(std::uint32_t type)
	{
		const auto* flags = PlayerFlags(56, type);
		return Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AllowedJobs) != 0 && Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AvailableJobs) != 0;
	}
	// Original functions: FUN_004a6e4a, FUN_004a6e8d, FUN_004a6ed0.
	bool TechTreeWindow_IsHouseTypeForbidden(std::uint32_t type)
	{
		return Read<std::uint8_t>(PlayerFlags(55, type), NC2Layout::GameControlPlayerOffset::AllowedHouses) == 0;
	}
	bool TechTreeWindow_IsGoodTypeForbidden(std::uint32_t type)
	{
		return Read<std::uint8_t>(PlayerFlags(66, type), NC2Layout::GameControlPlayerOffset::AllowedGoods) == 0;
	}
	bool TechTreeWindow_IsJobTypeForbidden(std::uint32_t type)
	{
		return Read<std::uint8_t>(PlayerFlags(56, type), NC2Layout::GameControlPlayerOffset::AllowedJobs) == 0;
	}

	static void* __fastcall TechTreeHouseTypeElement_Delete(void* element, void*, std::uint32_t flags);
	static void* __fastcall TechTreeGoodTypeElement_Delete(void* element, void*, std::uint32_t flags);
	static void* __fastcall TechTreeJobTypeElement_Delete(void* element, void*, std::uint32_t flags);
	static void* __fastcall TechTreeLineElement_Delete(void* element, void*, std::uint32_t flags);
	static void __fastcall TechTreeHouseTypeElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static void __fastcall TechTreeGoodTypeElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static void __fastcall TechTreeJobTypeElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static void __fastcall TechTreeLineElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static bool __fastcall TechTreeGoodTypeElement_ToolTip(void* element, void*, const char** output);
	// Original folded function: FUN_004bf2f4.
	static bool __fastcall TechTreeGoodTypeElement_ToolTipAtOnce(void*)
	{
		return false;
	}
	static void* __fastcall TechTreeWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall TechTreeWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	static void __fastcall TechTreeWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);

	// Complete Windows tables 004F6078..004F61E3. Only the native overridden slots differ.
	static const std::array<std::uintptr_t, 18> gTechTreeHouseVirtualTable = []
	{
		std::array<std::uintptr_t, 18> t{ };
		std::memcpy(t.data(), gTextButtonVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&TechTreeHouseTypeElement_Delete);
		t[3] = reinterpret_cast<std::uintptr_t>(&TechTreeHouseTypeElement_Draw);
		return t;
	}();
	static const std::array<std::uintptr_t, 18> gTechTreeGoodVirtualTable = []
	{
		std::array<std::uintptr_t, 18> t{ };
		std::memcpy(t.data(), gTextButtonVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&TechTreeGoodTypeElement_Delete);
		t[3] = reinterpret_cast<std::uintptr_t>(&TechTreeGoodTypeElement_Draw);
		t[13] = reinterpret_cast<std::uintptr_t>(&TechTreeGoodTypeElement_ToolTip);
		t[14] = reinterpret_cast<std::uintptr_t>(&TechTreeGoodTypeElement_ToolTipAtOnce);
		return t;
	}();
	static const std::array<std::uintptr_t, 18> gTechTreeJobVirtualTable = []
	{
		std::array<std::uintptr_t, 18> t{ };
		std::memcpy(t.data(), gTextButtonVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&TechTreeJobTypeElement_Delete);
		t[3] = reinterpret_cast<std::uintptr_t>(&TechTreeJobTypeElement_Draw);
		return t;
	}();
	static const std::array<std::uintptr_t, 18> gTechTreeLineVirtualTable = []
	{
		std::array<std::uintptr_t, 18> t{ };
		std::memcpy(t.data(), NXBaseGui::gBaseElementVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&TechTreeLineElement_Delete);
		t[3] = reinterpret_cast<std::uintptr_t>(&TechTreeLineElement_Draw);
		return t;
	}();
	static const std::array<std::uintptr_t, 19> gTechTreeWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&TechTreeWindow_Delete);
		t[2] = reinterpret_cast<std::uintptr_t>(&TechTreeWindow_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&TechTreeWindow_Draw);
		return t;
	}();

	// Original function: FUN_004a5a70.
	static void* TechTreeHouseTypeElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t id, void* window)
	{
		BaseToolTextButton_Construct(element, nullptr, rectangle, NC2Logic::StringAccess_GetHouseNamePtr(id, 1), 10100, window);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::TechTreeTypeElementOffset::TypeId, 0, 8);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeHouseVirtualTable.data());
		Write(element, NC2Layout::TechTreeTypeElementOffset::TypeId, id);
		Write(element, NC2Layout::ToolTextButtonOffset::CommandData1, id);
		Write<std::uint32_t>(element, NC2Layout::ToolTextButtonOffset::CommandData2, 0);
		Write(element, NC2Layout::TechTreeTypeElementOffset::Window, window);
		return element;
	}
	// Original function: FUN_004a5aca.
	static void* __fastcall TechTreeHouseTypeElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeHouseVirtualTable.data());
		BaseToolTextButton_Destruct(element);
		if (flags & 1u) NXBasics::Runtime_Free(element);
		return element;
	}
	// Original function: FUN_004a5aec.
	static void __fastcall TechTreeHouseTypeElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		const auto id = Read<std::uint32_t>(element, NC2Layout::TechTreeTypeElementOffset::TypeId);
		const auto mode = Read<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedHouseType) == id ? 3 : (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) != 0 ? 4 : 2;
		DrawTool_FillArea(element, bitmap, rectangle, mode);
		DrawTool_DrawFrame(element, bitmap, rectangle, mode);
		auto* font = DrawTool_GetFont10Ptr();
		const auto* palette = CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette;
		NXBasics::SColorRGB color = Read<NXBasics::SColorRGB>(palette, NC2Layout::TechTreeFontPaletteOffset::NormalColor);
		if (!TechTreeWindow_IsHouseTypeFree(id)) color = Read<NXBasics::SColorRGB>(palette, NC2Layout::TechTreeFontPaletteOffset::UnavailableColor);
		if (TechTreeWindow_IsHouseTypeForbidden(id)) color = Read<NXBasics::SColorRGB>(palette, NC2Layout::TechTreeFontPaletteOffset::ForbiddenColor);
		const auto* text = Read<const char*>(element, NC2Layout::GuiTextButtonOffset::Caption);
		bitmap->Text_Print(font, &color, (Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) - NXBasics::Font_GetPixelWidth(font, text)) / 2 - rectangle->x, (Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - 12) / 2 - rectangle->y - 3, text);
	}
	// Original function: FUN_004a5bc0.
	static void* TechTreeGoodTypeElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t id, void* window)
	{
		BaseToolTextButton_Construct(element, nullptr, rectangle, NC2Logic::StringAccess_GetGoodNamePtr(id, 1), 10101, window);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::TechTreeTypeElementOffset::TypeId, 0, 8);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeGoodVirtualTable.data());
		Write(element, NC2Layout::TechTreeTypeElementOffset::TypeId, id);
		Write(element, NC2Layout::ToolTextButtonOffset::CommandData1, id);
		Write<std::uint32_t>(element, NC2Layout::ToolTextButtonOffset::CommandData2, 0);
		Write(element, NC2Layout::TechTreeTypeElementOffset::Window, window);
		return element;
	}
	// Original function: FUN_004a5c16.
	static void* __fastcall TechTreeGoodTypeElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeGoodVirtualTable.data());
		BaseToolTextButton_Destruct(element);
		if (flags & 1u) NXBasics::Runtime_Free(element);
		return element;
	}
	// Original function: FUN_004a5c38.
	static void __fastcall TechTreeGoodTypeElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		const auto id = Read<std::uint32_t>(element, NC2Layout::TechTreeTypeElementOffset::TypeId);
		const auto mode = Read<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedGoodType) == id ? 3 : (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) != 0 ? 4 : 2;
		DrawTool_FillArea(element, bitmap, rectangle, mode);
		DrawTool_DrawFrame(element, bitmap, rectangle, mode);
		const auto& resources = CGuiBaseDataManager::mStaticVars.resources;
		NXBasics::SColorRGB color = Read<NXBasics::SColorRGB>(resources.fontWhitePalette, NC2Layout::TechTreeFontPaletteOffset::NormalColor);
		const char* format = "{gfxgood:%d,1}";
		if (!TechTreeWindow_IsGoodTypeFree(id))
		{
			if (TechTreeWindow_IsGoodTypeForbidden(id))
			{
				color = Read<NXBasics::SColorRGB>(resources.fontWhitePalette, NC2Layout::TechTreeFontPaletteOffset::ForbiddenColor);
				format = "{setx:-4}[{gfxgood:%d,1}]";
			}
			else
			{
				color = Read<NXBasics::SColorRGB>(resources.fontWhitePalette, NC2Layout::TechTreeFontPaletteOffset::UnavailableColor);
				format = "{setx:-4}({gfxgood:%d,1})";
			}
		}
		DrawTool_PrintStringXV(bitmap, Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) / 2 - 12, Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) / 2 - 10, resources.font08, &color, format, id);
	}
	// Original function: FUN_004a5cf8.
	static bool __fastcall TechTreeGoodTypeElement_ToolTip(void* element, void*, const char** output)
	{
		*output = NC2Logic::StringAccess_GetGoodNamePtr(Read<std::uint32_t>(element, NC2Layout::TechTreeTypeElementOffset::TypeId), 1);
		return true;
	}
	// Original function: FUN_004a5d0f.
	static void* TechTreeJobTypeElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t id, void* window)
	{
		BaseToolTextButton_Construct(element, nullptr, rectangle, NC2Logic::StringAccess_GetJobNamePtr(id, 1), 10102, window);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::TechTreeTypeElementOffset::TypeId, 0, 8);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeJobVirtualTable.data());
		Write(element, NC2Layout::TechTreeTypeElementOffset::TypeId, id);
		Write(element, NC2Layout::ToolTextButtonOffset::CommandData1, id);
		Write<std::uint32_t>(element, NC2Layout::ToolTextButtonOffset::CommandData2, 0);
		Write(element, NC2Layout::TechTreeTypeElementOffset::Window, window);
		return element;
	}
	// Original function: FUN_004a5d65.
	static void* __fastcall TechTreeJobTypeElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeJobVirtualTable.data());
		BaseToolTextButton_Destruct(element);
		if (flags & 1u) NXBasics::Runtime_Free(element);
		return element;
	}
	// Original function: FUN_004a5d87.
	static void __fastcall TechTreeJobTypeElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		const auto id = Read<std::uint32_t>(element, NC2Layout::TechTreeTypeElementOffset::TypeId);
		const auto mode = Read<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedJobType) == id ? 3 : (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) != 0 ? 4 : 2;
		DrawTool_FillArea(element, bitmap, rectangle, mode);
		DrawTool_DrawFrame(element, bitmap, rectangle, mode);
		const auto& resources = CGuiBaseDataManager::mStaticVars.resources;
		auto* font = resources.font08;
		NXBasics::SColorRGB color = Read<NXBasics::SColorRGB>(resources.fontWhitePalette, NC2Layout::TechTreeFontPaletteOffset::NormalColor);
		if (!TechTreeWindow_IsJobTypeFree(id)) color = Read<NXBasics::SColorRGB>(resources.fontWhitePalette, NC2Layout::TechTreeFontPaletteOffset::UnavailableColor);
		if (TechTreeWindow_IsJobTypeForbidden(id)) color = Read<NXBasics::SColorRGB>(resources.fontWhitePalette, NC2Layout::TechTreeFontPaletteOffset::ForbiddenColor);
		const auto* text = Read<const char*>(element, NC2Layout::GuiTextButtonOffset::Caption);
		bitmap->Text_Print(font, &color, (Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) - NXBasics::Font_GetPixelWidth(font, text)) / 2 - rectangle->x, (Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - 12) / 2 - rectangle->y - 3, text);
	}
	// Original function: FUN_004a5e5a.
	static void* TechTreeLineElement_Construct(void* element, const NXBasics::SPoint* first, const NXBasics::SPoint* second)
	{
		const auto x = first->x < second->x ? first->x : second->x, y = first->y < second->y ? first->y : second->y;
		const auto dx = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(first->x) - static_cast<std::uint32_t>(second->x));
		const auto dy = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(first->y) - static_cast<std::uint32_t>(second->y));
		const NXBasics::SRectangle rectangle(x, y, NXBasics::Runtime_TimeSigned32((dx < 0 ? 0u - static_cast<std::uint32_t>(dx) : static_cast<std::uint32_t>(dx)) + 1u), NXBasics::Runtime_TimeSigned32((dy < 0 ? 0u - static_cast<std::uint32_t>(dy) : static_cast<std::uint32_t>(dy)) + 1u));
		NXBaseGui::BaseElement_Construct(element, &rectangle, 0, 0);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeLineVirtualTable.data());
		Write(element, NC2Layout::TechTreeLineElementOffset::FirstPointX, first->x - x);
		Write(element, NC2Layout::TechTreeLineElementOffset::FirstPointY, first->y - y);
		Write(element, NC2Layout::TechTreeLineElementOffset::SecondPointX, second->x - x);
		Write(element, NC2Layout::TechTreeLineElementOffset::SecondPointY, second->y - y);
		return element;
	}
	// Original function: FUN_004a5eea.
	static void* __fastcall TechTreeLineElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeLineVirtualTable.data());
		NXBaseGui::BaseElement_Destruct(element);
		if (flags & 1u) NXBasics::Runtime_Free(element);
		return element;
	}
	// Original function: FUN_004a5f0c.
	static void __fastcall TechTreeLineElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle*)
	{
		const NXBasics::SColorRGB color(100, 100, 100);
		bitmap->Draw_Line(Read<std::int32_t>(element, NC2Layout::TechTreeLineElementOffset::SecondPointX), Read<std::int32_t>(element, NC2Layout::TechTreeLineElementOffset::SecondPointY), Read<std::int32_t>(element, NC2Layout::TechTreeLineElementOffset::FirstPointX), Read<std::int32_t>(element, NC2Layout::TechTreeLineElementOffset::FirstPointY), color);
	}
	// Original function: FUN_004a5f3b; Windows CTechTreeWindow allocation 0xB64.
	void* TechTreeWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle rectangle(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 500u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 420u) / 2, 500, 420);
		BaseToolWindow_Construct(window, &rectangle);
		auto* buttons = new (static_cast<std::byte*>(window) + NC2Layout::TechTreeWindowOffset::DynamicButtons) CDynamicSubButtonsHandler();
		std::memset(static_cast<std::byte*>(window) + NC2Layout::TechTreeWindowOffset::DynamicButtons, 0, 2804);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeWindowVirtualTable.data());
		Write<std::uint32_t>(window, NC2Layout::BaseToolWindowOffset::BackgroundStyle, 1);
		buttons->SetWindow(window);
		TechTreeWindow_l_GuiButtons_Add(window);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Misc_GameTime_Stop_LargeWindowOpened();
		return window;
	}
	// Original function: FUN_004a5fc0.
	static void* __fastcall TechTreeWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gTechTreeWindowVirtualTable.data());
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Misc_GameTime_Continue_LargeWindowClosed();
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
		if (flags & 1u) NXBasics::Runtime_Free(window);
		return window;
	}
	// Original function: FUN_004a6022; the text-button command data is the second parameter.
	static bool __fastcall TechTreeWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t data, std::uint32_t)
	{
		if (message != 0x800) return message == 0x1000 && command == 0x7E7;
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		if (command >= 10000 && command <= 10099)
		{
			Write(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::Page, command - 10000);
			Write<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedHouseType, 0);
			Write<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedGoodType, 0);
			Write<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedJobType, 0);
			TechTreeWindow_l_GuiButtons_Update(window);
		}
		else if (command >= 10100 && command <= 10102)
		{
			Write<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedHouseType, 0);
			Write<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedGoodType, 0);
			Write<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedJobType, 0);
			Write(gTechTreeWindowStatics, (command - 10100) * NC2Layout::RecordSize::TechTreeSelectedHouseTypeUnsigned + NC2Layout::TechTreeWindowStateOffset::SelectedHouseTypeUnsigned, data);
			TechTreeWindow_l_GuiButtons_Update(window);
		}
		else if (command == 10103)
		{
			std::uint32_t stage = 0;
			const auto engine = NC2E2::StaticDataHouses_GetHouseId(LocalTribe(), data, &stage);
			manager->MiscWindows_HelpWindow_Open_HouseInfo(engine, stage);
		}
		else if (command == 10104) manager->MiscWindows_HelpWindow_Open_GoodInfo(data);
		return true;
	}
	// Original function: FUN_004a6128; native detail branches and dynamic cross-links.
	static void __fastcall TechTreeWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const auto& resources = CGuiBaseDataManager::mStaticVars.resources;
		auto* font = resources.font08;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(static_cast<const std::byte*>(static_cast<const void*>(resources.fontDarkPalette)) + NXBasics::PaletteOffset::Colors + 255u * 4u);
		auto* buttons = reinterpret_cast<CDynamicSubButtonsHandler*>(static_cast<std::byte*>(window) + NC2Layout::TechTreeWindowOffset::DynamicButtons);
		const auto misc = [] (std::uint32_t id)
		{
			return StringTool_GetGameGuiMiscWindowStringPtr(id);
		};
		std::int32_t y = 220;
		const auto house = Read<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedHouseType);
		if (house != 0)
		{
			const auto& tribe = NC2Logic::gStaticTribeTypeData[LocalTribe()];
			const auto* name = NC2Logic::StringAccess_GetHouseNamePtr(house, 1);
			const auto palette = NC2E2::E2StaticDataPalettesManager_GetPalette256Id("house_sepia");
			std::uint32_t stage = 0;
			const auto engine = NC2E2::StaticDataHouses_GetHouseId(LocalTribe(), house, &stage);
			NC2E2::DrawAtom_House_Tool_DoDrawEngineHouseType(engine, stage, bitmap, 340, 340, palette, false, false);
			bitmap->Text_Print(font, color, 206, y, name);
			buttons->UpdateGfxButton(10103, 22000 + house, 478, y, 143, misc(418));
			buttons->SetButtonMiscCommandData(10103, 22000 + house, house, 0);
			y += 20;
			const auto* flags = PlayerFlags(55, house);
			if (Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AllowedHouses) == 0)
			{
				bitmap->Text_Print(font, color, 206, y, misc(391));
				y += 14;
			}
			else if (Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AvailableHouses) != 0)
			{
				bitmap->Text_Print(font, color, 206, y, misc(392));
				y += 14;
			}
			else
			{
				bitmap->Text_PrintV(font, color, 206, y, "%s %s:", misc(393), name);
				y += 14;
				const auto need = tribe.houseNeeds[house];
				if (need.requiredExperience > 0)
				{
					bitmap->Text_PrintV(font, color, 206, y, "...%s %d", misc(394), need.requiredExperience);
					y += 14;
					for (std::uint32_t index = 0; index < need.jobCount; ++index)
					{
						bitmap->Text_PrintV(font, color, 206, y, "...%s %s: %s", misc(index == 0 ? 395 : 396), misc(397), NC2Logic::StringAccess_GetJobExperienceNamePtr(need.jobs[index]));
						y += 14;
					}
				}
				y += 2;
				const auto build = tribe.houseBuildNeeds[house];
				for (std::uint32_t index = 0; index < build.jobCount; ++index)
				{
					const auto job = static_cast<std::uint32_t>(build.jobs[index]);
					if (Read<std::uint8_t>(PlayerFlags(56, job), NC2Layout::GameControlPlayerOffset::AvailableJobs) != 0)
					{
						buttons->RemoveButton(10102, 21000 + job);
						continue;
					}
					bitmap->Text_PrintV(font, color, 206, y, "...%s: %s", misc(398), NC2Logic::StringAccess_GetJobNamePtr(job, 1));
					buttons->UpdateGfxButton(10102, 21000 + job, 478, y, 18, misc(417));
					buttons->SetButtonMiscCommandData(10102, 21000 + job, job, 0);
					y += 14;
				}
				y += 2;
				for (std::uint32_t index = 0; index < build.goodCount; ++index)
				{
					const auto good = static_cast<std::uint32_t>(build.goods[index]);
					if (Read<std::uint8_t>(PlayerFlags(66, good), NC2Layout::GameControlPlayerOffset::AvailableGoods) != 0)
					{
						buttons->RemoveButton(10101, 20000 + good);
						buttons->RemoveButton(10104, 20000 + good);
						continue;
					}
					bitmap->Text_PrintV(font, color, 206, y, "...%s: %s", misc(399), NC2Logic::StringAccess_GetGoodNamePtr(good, 1));
					buttons->UpdateGfxButton(10101, 20000 + good, 458, y, 18, misc(416));
					buttons->SetButtonMiscCommandData(10101, 20000 + good, good, 0);
					buttons->UpdateGfxButton(10104, 20000 + good, 478, y, 143, misc(419));
					buttons->SetButtonMiscCommandData(10104, 20000 + good, good, 0);
					y += 14;
				}
			}
		}
		const auto good = Read<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedGoodType);
		if (good != 0)
		{
			const auto& tribe = NC2Logic::gStaticTribeTypeData[LocalTribe()];
			const auto* name = NC2Logic::StringAccess_GetGoodNamePtr(good, 1);
			bitmap->Text_Print(font, color, 206, y, name);
			buttons->UpdateGfxButton(10104, 20000 + good, 478, y, 143, misc(419));
			buttons->SetButtonMiscCommandData(10104, 20000 + good, good, 0);
			y += 20;
			const auto* flags = PlayerFlags(66, good);
			if (Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AllowedGoods) == 0)
			{
				bitmap->Text_Print(font, color, 206, y, misc(400));
				y += 14;
			}
			else if (Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AvailableGoods) != 0)
			{
				bitmap->Text_Print(font, color, 206, y, misc(401));
				y += 14;
			}
			else
			{
				bitmap->Text_PrintV(font, color, 206, y, "%s %s", misc(402), name);
				y += 14;
				for (std::uint32_t job = 0; job < 56; ++job)
				{
					const auto* jobFlags = PlayerFlags(56, job);
					if (Read<std::uint8_t>(jobFlags, NC2Layout::GameControlPlayerOffset::AllowedJobs) == 0)
					{
						buttons->RemoveButton(10102, 21000 + job);
						continue;
					}
					const auto enables = tribe.jobEnables[job];
					bool found = false;
					for (std::uint32_t index = 0; index < enables.goodCount; ++index) if (static_cast<std::uint32_t>(enables.goods[index]) == good)
					{
						found = true;
						break;
					}
					if (!found) continue;
					bitmap->Text_PrintV(font, color, 206, y, "...%s: %s", misc(Read<std::uint8_t>(jobFlags, NC2Layout::GameControlPlayerOffset::AvailableJobs) != 0 ? 403 : 404), NC2Logic::StringAccess_GetJobNamePtr(job, 1));
					buttons->UpdateGfxButton(10102, 21000 + job, 478, y, 18, misc(417));
					buttons->SetButtonMiscCommandData(10102, 21000 + job, job, 0);
					y += 14;
					break;
				}
				const auto need = tribe.goodNeeds[good];
				if (need.requiredExperience > 0)
				{
					bitmap->Text_PrintV(font, color, 206, y, "...%s %d", misc(405), need.requiredExperience);
					y += 14;
					for (std::uint32_t index = 0; index < need.jobCount; ++index)
					{
						bitmap->Text_PrintV(font, color, 206, y, "...%s %s: %s", misc(index == 0 ? 406 : 407), misc(408), NC2Logic::StringAccess_GetJobExperienceNamePtr(need.jobs[index]));
						y += 14;
					}
				}
			}
		}
		const auto job = Read<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::SelectedJobType);
		if (job != 0)
		{
			const auto& tribe = NC2Logic::gStaticTribeTypeData[LocalTribe()];
			bitmap->Text_Print(font, color, 206, y, NC2Logic::StringAccess_GetJobNamePtr(job, 1));
			y += 20;
			const auto* flags = PlayerFlags(56, job);
			if (Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AllowedJobs) == 0) bitmap->Text_PrintV(font, color, 206, y, misc(409));
			else if (Read<std::uint8_t>(flags, NC2Layout::GameControlPlayerOffset::AvailableJobs) != 0) bitmap->Text_PrintV(font, color, 206, y, misc(410));
			else
			{
				const auto need = tribe.jobNeeds[job];
				if (need.requiredExperience > 0)
				{
					bitmap->Text_PrintV(font, color, 206, y, "...%s %d", misc(411), need.requiredExperience);
					y += 14;
					for (std::uint32_t index = 0; index < need.jobCount; ++index)
					{
						bitmap->Text_PrintV(font, color, 206, y, "...%s %s: %s", misc(index == 0 ? 412 : 413), misc(414), NC2Logic::StringAccess_GetJobExperienceNamePtr(need.jobs[index]));
						y += 14;
					}
				}
			}
		}
	}
	// Original function: FUN_004a6a74.
	void TechTreeWindow_l_GuiButtons_Update(void* window)
	{
		NXBaseGui::BaseWindow_Element_DeleteAll(window);
		reinterpret_cast<CDynamicSubButtonsHandler*>(static_cast<std::byte*>(window) + NC2Layout::TechTreeWindowOffset::DynamicButtons)->AllButtonsHaveBeenRemoved();
		TechTreeWindow_l_GuiButtons_Add(window);
	}
	// Original function: FUN_004a6a8d; the complete ten-page layout is sourced from Game.exe.
	void TechTreeWindow_l_GuiButtons_Add(void* window)
	{
		const NXBasics::SRectangle title(2, 2, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22, 14), close(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18, 2, 16, 14);
		auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(248, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &title, StringTool_GetGameGuiMiscWindowStringPtr(380)));
		allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &close));
		const auto page = Read<std::uint32_t>(gTechTreeWindowStatics, NC2Layout::TechTreeWindowStateOffset::Page);
		for (std::uint32_t index = 0; index < 10; ++index)
		{
			const NXBasics::SRectangle rectangle(4 + index * 49, 22, 47, 30);
			allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
			void* element = allocation == nullptr ? nullptr : BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &rectangle, gTechTreeTabs[index].graphic, StringTool_GetGameGuiMiscWindowStringPtr(gTechTreeTabs[index].tooltip), 10000 + index, window);
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Selected, index == page);
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::RectangularStyle, 1);
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::ImmediateTooltip, 1);
			NXBaseGui::BaseWindow_Element_Add(window, element);
		}
		if (page > 9) return;
		for (const auto* node = gTechTreePages[page]; node->type != 0; ++node)
		{
			void* element = nullptr;
			if (node->type >= 1 && node->type <= 3)
			{
				const NXBasics::SRectangle rectangle(node->x, node->y, node->width, node->height);
				allocation = NXBasics::Runtime_AllocateWithNewHandler(104, 1);
				if (allocation != nullptr)
				{
					if (node->type == 1) element = TechTreeHouseTypeElement_Construct(allocation, &rectangle, node->id, window);
					else if (node->type == 2) element = TechTreeGoodTypeElement_Construct(allocation, &rectangle, node->id, window);
					else element = TechTreeJobTypeElement_Construct(allocation, &rectangle, node->id, window);
				}
			}
			else if (node->type == 4)
			{
				const NXBasics::SPoint first{ node->x, node->y }, second{ node->width, node->height };
				allocation = NXBasics::Runtime_AllocateWithNewHandler(60, 1);
				if (allocation != nullptr) element = TechTreeLineElement_Construct(allocation, &first, &second);
			}
			else continue;
			NXBaseGui::BaseWindow_Element_Add(window, element);
		}
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a21a1.
	void CInGameGuiManager::MiscWindows_TechTreeWindow_Toggle()
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(0x7E7, 0, 0, 0);
		if (window != nullptr) desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		else MiscWindows_TechTreeWindow_Open();
	}
	// Original function: FUN_004a21d5.
	void CInGameGuiManager::MiscWindows_TechTreeWindow_Open()
	{
		l_Selection_RemoveAllGuiElements();
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7E7, 0, 0, 0) != nullptr) return;
		MiscWindows_CloseAllLargeOnes(true);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(2916, 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiMiscWindows::TechTreeWindow_Construct(allocation);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
	}
}

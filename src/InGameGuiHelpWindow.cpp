#include "XGuiElementAccess.h"
#include "InGameGuiStorage.h"
#include "XPalette.h"
#include "C2E2StaticDataPalettes.h"
#include "C2EngineDrawingAccess.h"
#include "C2HouseAccess.h"
#include "C2LanguageTools.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2StaticHouseTypeData.h"
#include "C2StringAccess.h"
#include "C2StringIO.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "HyperLinkTextSystem.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiHelpWindow.h"
#include "InGameGuiManager.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include "XNumericConstants.h"
#include "XStringArray.h"
#include <array>
#include <cstring>
#include <new>

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	static void HyperLinkTextWindow_Destruct(void* window);
	static void* __fastcall HyperLinkTextWindow_Delete(void* window, void* unusedEdx, std::uint32_t flags);
	// Complete Windows table 004F74B8..004F7503.
	static const std::array<std::uintptr_t, 19> gHyperLinkTextWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> table{ };
		std::memcpy(table.data(), gBaseToolWindowVirtualTable, sizeof(table));
		table[0] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&HyperLinkTextWindow_Delete>());
		table[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&HyperLinkTextWindow_XGui_BE_Message_Handle>());
		table[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&HyperLinkTextWindow_XGui_BE_Element_Draw>());

		return table;
	}();

	// Original function: FUN_004c6fec; equal names use the native nonzero stage tie result.
	std::int32_t __cdecl lsSortHouseTypeCompareFunction(const void* first, const void* second)
	{
		const auto firstStage = Read<std::uint32_t>(first, NC2Layout::HelpHouseEntryOffset::DevelopmentStage), secondStage = Read<std::uint32_t>(second, NC2Layout::HelpHouseEntryOffset::DevelopmentStage);
		const auto compare = NC2GuiToolsBase::StringToolMisc_StringSortCompare(NC2Logic::StringAccess_GetHouseNamePtr(Read<std::uint32_t>(first, NC2Layout::HelpHouseEntryOffset::EngineType), firstStage, 1), NC2Logic::StringAccess_GetHouseNamePtr(Read<std::uint32_t>(second, NC2Layout::HelpHouseEntryOffset::EngineType), secondStage, 1));

		return compare != 0 ? compare : secondStage < firstStage ? 1
		                                                         : -1;
	}

	// Original function: FUN_004c340e.
	static std::int32_t __cdecl lsSortGoodTypeCompareFunction(const void* first, const void* second)
	{

		return NC2GuiToolsBase::StringToolMisc_StringSortCompare(NC2Logic::StringAccess_GetGoodNamePtr(Read<std::uint32_t>(first, NC2Layout::HelpGoodEntryOffset::GoodType), 1), NC2Logic::StringAccess_GetGoodNamePtr(Read<std::uint32_t>(second, NC2Layout::HelpGoodEntryOffset::GoodType), 1));
	}

	// Original function: FUN_004c31ca; Windows CHyperLinkTextWindow allocation 0x1050.
	void* HyperLinkTextWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle rectangle(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 500u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 420u) / 2, 500, 420);
		BaseToolWindow_Construct(window, &rectangle);
		auto* buttons = new (static_cast<std::byte*>(window) + NC2Layout::HelpWindowOffset::DynamicButtons) CDynamicSubButtonsHandler();
		std::memset(static_cast<std::byte*>(window) + NC2Layout::HelpWindowOffset::Page, 0, NC2Layout::RecordSize::HelpWindowVars);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gHyperLinkTextWindowVirtualTable.data());
		Write<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedHouseIndex, -1);
		buttons->SetWindow(window);
		Write<std::uint32_t>(window, NC2Layout::BaseToolWindowOffset::BackgroundStyle, 1);
		const auto player = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
		const auto tribe = player >= 20 ? 1u : Read<std::uint32_t>(NC2Logic::gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeType);
		bool full = false;

		for (std::int32_t type = 0; type < NC2E2::gStaticHouseCount && !full; ++type)
		{
			const auto* engine = static_cast<const std::byte*>(NC2E2::gStaticHouseEntries) + static_cast<std::uint32_t>(type) * NC2Layout::RecordSize::EngineHouseTypeUnsigned;

			if (Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::TribeType) != tribe)
				continue;

			for (std::uint32_t stage = 0; stage < Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount); ++stage)
			{
				const auto logic = Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::LogicTypesUnsigned + stage * NC2Layout::RecordSize::HouseStageValueUnsigned);

				if (NC2Logic::gStaticHouseTypeStorage.records[logic].mainType == 6)
					continue;

				const auto index = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseCount);
				Write(window, NC2Layout::HelpWindowOffset::HouseTypesUnsigned + index * NC2Layout::RecordSize::HelpHouseEntryUnsigned, type);
				Write(window, NC2Layout::HelpWindowOffset::HouseDevelopmentStagesUnsigned + index * NC2Layout::RecordSize::HelpHouseEntryUnsigned, stage);
				Write(window, NC2Layout::HelpWindowOffset::HouseCount, index + 1);

				if (index + 1 == 100)
				{
					full = true;
					break;
				}
			}
		}

		const auto count = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseCount);

		if (count > 1)
			NXBasics::Runtime_QuickSort(static_cast<std::byte*>(window) + NC2Layout::HelpWindowOffset::HouseTypes, count, 8, lsSortHouseTypeCompareFunction);

		char filename[260];
		NC2Logic::LanguageTool_GetFilename("data\\text\\%s\\strings\\help\\houseshelp.ini", filename, true);
		Write(window, NC2Layout::HelpWindowOffset::HouseHelpStrings, NC2Logic::StringIO_CreateOutOfIniFile(filename));
		Write<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedGoodIndex, -1);

		for (std::uint32_t good = 1; good <= 55; ++good)
		{
			if (good == 17 || good == 23)
				continue;

			const auto index = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::GoodCount);
			Write(window, NC2Layout::HelpWindowOffset::GoodsUnsigned + index * NC2Layout::RecordSize::HelpGoodIdUnsigned, good);
			Write(window, NC2Layout::HelpWindowOffset::GoodCount, index + 1);

			if (index + 1 == 100)
				break;
		}

		const auto goods = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::GoodCount);

		if (goods > 1)
			NXBasics::Runtime_QuickSort(static_cast<std::byte*>(window) + NC2Layout::HelpWindowOffset::Goods, goods, 4, lsSortGoodTypeCompareFunction);

		NC2Logic::LanguageTool_GetFilename("data\\text\\%s\\strings\\help\\goodshelp.ini", filename, true);
		Write(window, NC2Layout::HelpWindowOffset::GoodHelpStrings, NC2Logic::StringIO_CreateOutOfIniFile(filename));
		Write<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedMiscIndex, -1);
		NC2Logic::LanguageTool_GetFilename("data\\text\\%s\\strings\\help\\mischelp.ini", filename, true);
		auto* strings = NC2Logic::StringIO_CreateOutOfIniFile(filename);
		Write(window, NC2Layout::HelpWindowOffset::MiscHelpStrings, strings);
		Write(window, NC2Layout::HelpWindowOffset::MiscCount, strings->GetNumberOfStrings() / 2);
		HyperLinkTextWindow_l_GuiButtons_Add(window);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Misc_GameTime_Stop_LargeWindowOpened();

		return window;
	}

	// Original function: FUN_004c3436.
	static void* __fastcall HyperLinkTextWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		HyperLinkTextWindow_Destruct(window);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004c3452; string arrays are owned by the window.
	static void HyperLinkTextWindow_Destruct(void* window)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gHyperLinkTextWindowVirtualTable.data());
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Misc_GameTime_Continue_LargeWindowClosed();
		delete Read<NXBasics::CStringArray*>(window, NC2Layout::HelpWindowOffset::MiscHelpStrings);
		delete Read<NXBasics::CStringArray*>(window, NC2Layout::HelpWindowOffset::GoodHelpStrings);
		delete Read<NXBasics::CStringArray*>(window, NC2Layout::HelpWindowOffset::HouseHelpStrings);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
	}

	// Original function: FUN_004c34a8; selection cycling returns to the list on either boundary.
	bool __fastcall HyperLinkTextWindow_XGui_BE_Message_Handle(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t param2, std::uintptr_t param3)
	{
		if (message == 0x1000 && command == 0x7E8)
			return true;

		if (message != 0x800)
			return false;

		if (command >= 10000 && command < 10010)
		{
			Write<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedHouseIndex, -1);
			Write<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedGoodIndex, -1);
			Write<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedMiscIndex, -1);
			Write(window, NC2Layout::HelpWindowOffset::Page, command - 10000);
			HyperLinkTextWindow_l_GuiButtons_Update(window);
			return true;
		}

		if (command == 10024 || command == 10025)
		{
			if (void* element = Read<void*>(window, NC2Layout::HelpWindowOffset::HyperTextElement))
			{
				HyperLinkTextGuiElementBase_SetAutoScrollSpeed(element, nullptr, 0.0f, 0);
				Read<NHyperLinkTextSystem::CDisplayManager*>(element, NC2Layout::HypertextGuiElementOffset::Display)->ScrollY(command == 10024 ? -60 : 60);
			}

			return true;
		}

		std::uint32_t field = 0, count = 0;

		switch (command)
		{
			case SwitchIds::HelpWindowCommand::SelectHouse:
				Write<std::uint32_t>(window, NC2Layout::HelpWindowOffset::SelectedHouseIndex, static_cast<std::uint32_t>(param2));
				break;
			case SwitchIds::HelpWindowCommand::ClearHouseSelection:
				Write<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedHouseIndex, -1);
				break;
			case SwitchIds::HelpWindowCommand::PreviousHouse:
				field = NC2Layout::HelpWindowOffset::SelectedHouseIndex;
				count = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseCount);
				break;
			case SwitchIds::HelpWindowCommand::NextHouse:
				field = NC2Layout::HelpWindowOffset::SelectedHouseIndex;
				count = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseCount);
				break;
			case SwitchIds::HelpWindowCommand::ShowGoodForHouse:
			case SwitchIds::HelpWindowCommand::OpenGoodInfo:
			{
				Write<std::uint32_t>(window, NC2Layout::HelpWindowOffset::Page, 2);
				std::int32_t slot = -1;

				for (std::uint32_t index = 0; index < Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::GoodCount); ++index)
				{
					if (Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::GoodsUnsigned + index * NC2Layout::RecordSize::HelpGoodIdUnsigned) == param2)
					{
						slot = index;
						break;
					}
				}

				Write(window, NC2Layout::HelpWindowOffset::SelectedGoodIndex, slot);
				break;
			}
			case SwitchIds::HelpWindowCommand::SelectGood:
				Write<std::uint32_t>(window, NC2Layout::HelpWindowOffset::SelectedGoodIndex, static_cast<std::uint32_t>(param2));
				break;
			case SwitchIds::HelpWindowCommand::ClearGoodSelection:
				Write<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedGoodIndex, -1);
				break;
			case SwitchIds::HelpWindowCommand::PreviousGood:
				field = NC2Layout::HelpWindowOffset::SelectedGoodIndex;
				count = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::GoodCount);
				break;
			case SwitchIds::HelpWindowCommand::NextGood:
				field = NC2Layout::HelpWindowOffset::SelectedGoodIndex;
				count = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::GoodCount);
				break;
			case SwitchIds::HelpWindowCommand::SelectMisc:
				Write<std::uint32_t>(window, NC2Layout::HelpWindowOffset::SelectedMiscIndex, static_cast<std::uint32_t>(param2));
				break;
			case SwitchIds::HelpWindowCommand::ClearMiscSelection:
				Write<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedMiscIndex, -1);
				break;
			case SwitchIds::HelpWindowCommand::PreviousMisc:
				field = NC2Layout::HelpWindowOffset::SelectedMiscIndex;
				count = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::MiscCount);
				break;
			case SwitchIds::HelpWindowCommand::NextMisc:
				field = NC2Layout::HelpWindowOffset::SelectedMiscIndex;
				count = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::MiscCount);
				break;
			case SwitchIds::HelpWindowCommand::OpenHouseInfo:
			{
				Write<std::uint32_t>(window, NC2Layout::HelpWindowOffset::Page, 1);
				std::int32_t slot = -1;

				for (std::uint32_t index = 0; index < Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseCount); ++index)
				{
					if (Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseTypesUnsigned + index * NC2Layout::RecordSize::HelpHouseEntryUnsigned) == param2 && Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseDevelopmentStagesUnsigned + index * NC2Layout::RecordSize::HelpHouseEntryUnsigned) == param3)
					{
						slot = index;
						break;
					}
				}

				Write(window, NC2Layout::HelpWindowOffset::SelectedHouseIndex, slot);
				break;
			}
			default:
				return true;
		}

		if (field != 0)
		{
			const bool previous = command == 10013 || command == 10018 || command == 10022;
			const auto value = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(window, field) + (previous ? NXBasics::IntegerRange::UInt32Maximum : 1u));
			Write(window, field, value < 0 || (!previous && value >= NXBasics::Runtime_TimeSigned32(count)) ? -1 : value);
		}

		HyperLinkTextWindow_l_GuiButtons_Update(window);

		return true;
	}

	// Original function: FUN_004c3f86; remove child widgets before resetting their references.
	void __fastcall HyperLinkTextWindow_l_GuiButtons_Update(void* window)
	{
		NXBaseGui::BaseWindow_Element_DeleteAll(window);
		reinterpret_cast<CDynamicSubButtonsHandler*>(static_cast<std::byte*>(window) + NC2Layout::HelpWindowOffset::DynamicButtons)->AllButtonsHaveBeenRemoved();
		HyperLinkTextWindow_l_GuiButtons_Add(window);
	}

	// Original function: FUN_004c4853; the original user-bitmap callback always returns null.
	static NXBasics::CBitmap* __cdecl ls_GetUserBitmapCallback(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
	{

		return nullptr;
	}

	// Original function: FUN_004c3fa2; four native tabs, detail navigation and index lists.
	void __fastcall HyperLinkTextWindow_l_GuiButtons_Add(void* window)
	{
		Write<void*>(window, NC2Layout::HelpWindowOffset::HyperTextElement, nullptr);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiWindowMoverStorage), 1);
		NXBasics::SRectangle rectangle(2, 2, NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22u), 14);
		void* element = allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(30));
		NXBaseGui::BaseWindow_Element_Add(window, element);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiGfxButtonStorage), 1);
		rectangle = NXBasics::SRectangle(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18u), 2, 16, 14);
		element = allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &rectangle);
		NXBaseGui::BaseWindow_Element_Add(window, element);
		const auto button = [window](const NXBasics::SRectangle& area, const char* caption, std::uint32_t command, bool selected) -> void*
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);

			return memory == nullptr ? nullptr : BaseToolTextButton_ConstructToggle(memory, nullptr, &area, caption, command, window, selected);
		};
		const auto page = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::Page);
		rectangle = NXBasics::SRectangle(4, 22, 121, 22);

		for (std::uint32_t tab = 0; tab < 4; ++tab)
		{
			NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(31 + tab), 10000 + tab, page == tab));
			rectangle.x += rectangle.width + 2;
		}

		rectangle = NXBasics::SRectangle(4, 59, 492, 22);

		if (page >= 1 && page <= 3)
		{
			const std::uint32_t selections[3]{ NC2Layout::HelpWindowOffset::SelectedHouseIndex, NC2Layout::HelpWindowOffset::SelectedGoodIndex, NC2Layout::HelpWindowOffset::SelectedMiscIndex };
			const std::uint32_t counts[3]{ NC2Layout::HelpWindowOffset::HouseCount, NC2Layout::HelpWindowOffset::GoodCount, NC2Layout::HelpWindowOffset::MiscCount };
			const std::uint32_t selectCommands[3]{ 10011, 10016, 10020 };
			const std::uint32_t previousCommands[3]{ 10012, 10017, 10021 };

			if (Read<std::int32_t>(window, selections[page - 1]) != -1)
			{
				rectangle = NXBasics::SRectangle(4, 394, 162, 22);

				for (std::uint32_t index = 0; index < 3; ++index)
				{
					const auto command = previousCommands[page - 1] + (index == 0 ? 1 : index == 1 ? 0
					                                                                               : 2);
					NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(35 + index), command, false));
					rectangle.x += rectangle.width + 2;
				}

				return;
			}

			rectangle.width = 468;
			rectangle.height = 416 - rectangle.y;
			allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBaseListBox::SListBoxStorage), 1);
			void* list = allocation == nullptr ? nullptr : ToolListBoxElement_Construct(allocation, nullptr, &rectangle);
			NXBasics::SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(window, counts[page - 1]); ++index)
			{
				const char* caption = page == 1 ? NC2Logic::StringAccess_GetHouseNamePtr(Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseTypesUnsigned + index * NC2Layout::RecordSize::HelpHouseEntryUnsigned), Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseDevelopmentStagesUnsigned + index * NC2Layout::RecordSize::HelpHouseEntryUnsigned), 1) : page == 2 ? NC2Logic::StringAccess_GetGoodNamePtr(Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::GoodsUnsigned + index * NC2Layout::RecordSize::HelpGoodIdUnsigned), 1)
				                                                                                                                                                                                                                                                                                                                                                                                : Read<NXBasics::CStringArray*>(window, NC2Layout::HelpWindowOffset::MiscHelpStrings)->GetStringPtr(index * 2u);
				element = button(row, caption, selectCommands[page - 1], false);
				Write<std::uintptr_t>(element, NC2Layout::ToolTextButtonOffset::CommandData1, index);
				Write<std::uintptr_t>(element, NC2Layout::ToolTextButtonOffset::CommandData2, 0);
				BaseListBoxElement_Element_Add(list, nullptr, element);
				row.y += row.height + 2;
			}

			NXBaseGui::BaseWindow_Element_Add(window, list);
			rectangle.x = 476;
			rectangle.width = 20;
			allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBaseListBox::SListBoxControllerStorage), 1);
			element = allocation == nullptr ? nullptr : ToolListBoxControllerElement_Construct(allocation, nullptr, &rectangle, list, false, true);
			NXBaseGui::BaseWindow_Element_Add(window, element);
		}
		else if (page == 0)
		{
			rectangle.height = 354 - rectangle.y;
			Write(window, NC2Layout::HelpWindowOffset::HyperTextFrame, rectangle);
			rectangle.Inflate(-5, -5);
			char directory[260];
			NC2Logic::LanguageTool_GetDirectoryName("data\\text\\%s\\hypertext\\ingamehelp", directory, "index.hlt");
			allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NHyperLinkTextSystem::SHypertextGuiElementStorage), 1);
			element = allocation == nullptr ? nullptr : HyperLinkTextGuiElementBase_Construct(allocation, nullptr, &rectangle, 3, 0x100, directory, "index.hlt", nullptr, reinterpret_cast<void*>(&ls_GetUserBitmapCallback));
			Write(window, NC2Layout::HelpWindowOffset::HyperTextElement, element);
			Write<std::uint8_t>(element, NC2Layout::HypertextGuiElementOffset::FillBackground, 0);
			NXBaseGui::BaseWindow_Element_Add(window, element);
			const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;

			for (std::uint32_t index = 0; index < 2; ++index)
			{
				rectangle = NXBasics::SRectangle(212 + index * 34, 358, 42, 58);
				allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiGfxButtonStorage), 1);
				element = allocation == nullptr ? nullptr : BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &rectangle, 138 + index, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(67 + index), 10024 + index, window);
				ToolGraphics_SetPalettePtr(element, resources.iconsLeftPalette);
				NXBaseGui::BaseWindow_Element_Add(window, element);
			}
		}
	}

	// Original function: FUN_004c3730; complete four-page drawing and interactive cross-references.
	void __fastcall HyperLinkTextWindow_XGui_BE_Element_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* fontSmall = resources.font08;
		auto* fontMedium = resources.font10;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(resources.fontDarkPalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
		auto* buttons = reinterpret_cast<CDynamicSubButtonsHandler*>(static_cast<std::byte*>(window) + NC2Layout::HelpWindowOffset::DynamicButtons);
		const auto page = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::Page);

		if (page == 0)
		{
			NXBasics::SRectangle frame(0, 0, 0, 0);
			std::memcpy(&frame, static_cast<std::byte*>(window) + NC2Layout::HelpWindowOffset::HyperTextFrame, sizeof(frame));
			NC2GuiToolsBase::DrawTool_DrawFrame(bitmap, frame, 2);
			return;
		}

		if (page == 3)
		{
			const auto index = Read<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedMiscIndex);

			if (index == -1)
				return;

			auto* strings = Read<NXBasics::CStringArray*>(window, NC2Layout::HelpWindowOffset::MiscHelpStrings);
			NC2GuiToolsBase::DrawTool_PrintStringX(bitmap, 20, 60, fontMedium, color, strings->GetStringPtr(static_cast<std::uint32_t>(index) * 2u));

			if (Read<void*>(window, NC2Layout::HelpWindowOffset::GoodHelpStrings) != nullptr)
			{
				const auto* text = strings->GetStringPtr(static_cast<std::uint32_t>(index) * 2u + 1);

				if (text != nullptr)
					NC2GuiToolsBase::DrawTool_PrintStringWrapping(bitmap, 20, 90, 500, fontSmall, color, text);
			}

			return;
		}

		if (page == 2)
		{
			const auto index = Read<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedGoodIndex);

			if (index == -1)
				return;

			const auto good = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::GoodsUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::HelpGoodIdUnsigned);
			const auto* name = NC2Logic::StringAccess_GetGoodNamePtr(good, 1);
			NC2GuiToolsBase::DrawTool_PrintStringXV(bitmap, 20, 60, fontMedium, color, "{gfxgood:%d,1}  %s", good, name);
			std::int32_t y = 90;

			if (auto* strings = Read<NXBasics::CStringArray*>(window, NC2Layout::HelpWindowOffset::GoodHelpStrings))
			{
				if (const char* text = strings->GetStringPtr(good))
					y = NC2GuiToolsBase::DrawTool_PrintStringWrapping(bitmap, 20, y, 500, resources.font12, color, text) + 100;
			}

			y += 30;
			std::uint32_t producers[10]{ }, producerCount = 0;

			for (std::uint32_t houseIndex = 0; houseIndex < Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseCount); ++houseIndex)
			{
				const auto type = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseTypesUnsigned + houseIndex * NC2Layout::RecordSize::HelpHouseEntryUnsigned), stage = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseDevelopmentStagesUnsigned + houseIndex * NC2Layout::RecordSize::HelpHouseEntryUnsigned);
				const auto* engine = static_cast<const std::byte*>(NC2E2::gStaticHouseEntries) + type * NC2Layout::RecordSize::EngineHouseTypeUnsigned;
				const auto& house = NC2Logic::gStaticHouseTypeStorage.records[Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::LogicTypesUnsigned + stage * NC2Layout::RecordSize::HouseStageValueUnsigned)];

				for (std::uint32_t production = 0; production < house.productionCount; ++production)
				{
					if (static_cast<std::uint32_t>(house.productionGoods[production]) != good)
						continue;

					producers[producerCount++] = houseIndex;
					break;
				}

				if (producerCount == 10)
					break;
			}

			if (producerCount == 0)
				return;

			char caption[300];
			NXBasics::Runtime_PrintString(caption, "%s %s:", name, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(43));
			y += NC2GuiToolsBase::DrawTool_PrintStringWrapping(bitmap, 20, y, 500, fontMedium, color, caption) + 6;

			for (std::uint32_t producer = 0; producer < producerCount; ++producer)
			{
				const auto slot = producers[producer];
				const auto type = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseTypesUnsigned + slot * NC2Layout::RecordSize::HelpHouseEntryUnsigned), stage = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseDevelopmentStagesUnsigned + slot * NC2Layout::RecordSize::HelpHouseEntryUnsigned);
				const auto height = NC2GuiToolsBase::DrawTool_PrintStringWrapping(bitmap, 20, y, 500, resources.font12, color, NC2Logic::StringAccess_GetHouseNamePtr(type, stage, 1));
				buttons->UpdateGfxButton(20000, producer, 478, y, 143, nullptr);
				buttons->SetButtonMiscCommandData(20000, producer, type, stage);
				y += height + 2;
			}

			return;
		}

		if (page != 1)
			return;

		const auto index = Read<std::int32_t>(window, NC2Layout::HelpWindowOffset::SelectedHouseIndex);

		if (index == -1)
			return;

		const auto type = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseTypesUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::HelpHouseEntryUnsigned), stage = Read<std::uint32_t>(window, NC2Layout::HelpWindowOffset::HouseDevelopmentStagesUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::HelpHouseEntryUnsigned);
		const auto* engine = static_cast<const std::byte*>(NC2E2::gStaticHouseEntries) + type * NC2Layout::RecordSize::EngineHouseTypeUnsigned;
		const auto logic = Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::LogicTypesUnsigned + stage * NC2Layout::RecordSize::HouseStageValueUnsigned);
		const auto& house = NC2Logic::gStaticHouseTypeStorage.records[logic];
		NC2E2::DrawAtom_House_Tool_DoDrawEngineHouseType(static_cast<std::int32_t>(type), stage, bitmap, 250, 300, NC2E2::E2StaticDataPalettesManager_GetPalette256Id("house_sepia"), false, false);
		NC2GuiToolsBase::DrawTool_PrintStringX(bitmap, 20, 60, fontMedium, color, NC2Logic::StringAccess_GetHouseNamePtr(type, stage, 1));
		std::int32_t y = 86;

		if (auto* strings = Read<NXBasics::CStringArray*>(window, NC2Layout::HelpWindowOffset::HouseHelpStrings))
		{
			if (const char* text = strings->GetStringPtr(logic))
				y = NC2GuiToolsBase::DrawTool_PrintStringWrapping(bitmap, 20, y, 500, fontSmall, color, text) + 96;
		}

		NC2GuiToolsBase::DrawTool_PrintStringXV(bitmap, 20, y, fontSmall, color, "%s:", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(38));
		y += 16;
		const auto drawCosts = [&](std::uint32_t developmentStage, bool onlyCurrentStage, std::uint32_t firstId)
		{
			NC2Logic::SHouseConstructionGoods goods{ };
			NC2Logic::House_Construction_CalculateGoodsNeeded(type, developmentStage, onlyCurrentStage, &goods);
			std::int32_t x = 40;

			for (std::int32_t slot = 0; slot < goods.count; ++slot)
			{
				const auto good = goods.entries[slot].good, amount = goods.entries[slot].requiredAmount;
				char graphic[32], tooltip[100];
				NXBasics::Runtime_PrintString(graphic, "{setx:10}{gfxgood:%d,%d}", good, amount);
				NXBasics::Runtime_PrintString(tooltip, "%d %s", amount, NC2Logic::StringAccess_GetGoodNamePtr(good, amount));
				const auto id = firstId + static_cast<std::uint32_t>(slot);
				buttons->UpdateTextButton(10015, id, x, y, 50, 24, graphic, false, tooltip);
				buttons->SetButtonMiscCommandData(10015, id, good, 0);
				x += 52;
			}
		};
		drawCosts(stage, false, 0);
		y += 30;

		if (Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount) > stage + 1)
		{
			NC2GuiToolsBase::DrawTool_PrintStringXV(bitmap, 20, y, fontSmall, color, "%s:", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(39));
			y += 16;
			drawCosts(stage + 1, true, 1000);
			y += 30;
		}

		if (house.mainType == 1 || house.mainType == 3 || house.mainType == 5)
		{
			NC2GuiToolsBase::DrawTool_PrintStringXV(bitmap, 20, y, fontSmall, color, "%s:", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(40));
			y += 16;
			char text[300], worker[100];
			text[0] = '\0';

			for (std::int32_t slot = 0; slot < house.workerCount; ++slot)
			{
				const auto amount = house.workers[slot].maximumWorkers;
				NXBasics::Runtime_PrintString(worker, "%d %s", amount, NC2Logic::StringAccess_GetJobNamePtr(house.workers[slot].jobType, amount));

				if (slot != 0)
					std::strcat(text, ", ");

				std::strcat(text, worker);
			}

			NC2GuiToolsBase::DrawTool_PrintStringX(bitmap, 40, y, fontSmall, color, text);
			y += 22;
		}

		NC2GuiToolsBase::DrawTool_PrintStringXV(bitmap, 20, y, fontSmall, color, "%s:", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(41));
		y += 16;
		const auto difficulty = Read<std::uint32_t>(NC2Logic::gGameControl, NC2Layout::GameControlOffset::DifficultyLevel);
		const auto multiplier = difficulty == 1 ? 2u : difficulty == 2 ? 5u
		                                                               : 1u;

		if (house.mainType == 1)
		{
			NC2GuiToolsBase::DrawTool_PrintStringXV(bitmap, 40, y, fontSmall, color, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(42), static_cast<std::uint32_t>(house.stock[0].maximumAmount) * multiplier);

			return;
		}

		std::uint32_t column = 0;

		for (std::uint32_t stock = 0; stock < house.stockCount && stock < 20; ++stock)
		{
			const auto good = static_cast<std::uint32_t>(house.stock[stock].goodType), amount = static_cast<std::uint32_t>(house.stock[stock].maximumAmount) * multiplier;
			const auto x = 20 + static_cast<std::int32_t>(column) * 220;
			NC2GuiToolsBase::DrawTool_PrintStringXV(bitmap, x + 20, y, fontSmall, color, "%d", amount);
			char graphic[28];
			NXBasics::Runtime_PrintString(graphic, "{setx:10}{gfxgood:%d,1}", good);
			buttons->UpdateTextButton(10015, 2000 + stock, x + 40, y, 40, 18, graphic, false, NC2Logic::StringAccess_GetGoodNamePtr(good, 1));
			buttons->SetButtonMiscCommandData(10015, 2000 + stock, good, 0);
			NC2GuiToolsBase::DrawTool_PrintStringXV(bitmap, x + 84, y, fontSmall, color, "%s %s", NC2Logic::StringAccess_GetGoodNamePtr(good, amount), house.stock[stock].isOutputStock != 0 ? "<-" : "<>");

			if (++column == 2)
			{
				column = 0;
				y += 20;
			}
		}
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a2226.
	void CInGameGuiManager::MiscWindows_HelpWindow_Toggle()
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(0x7E8, 0, 0, 0);

		if (window != nullptr)
		{
			desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
		else
		{
			MiscWindows_HelpWindow_Open();
		}
	}

	// Original function: FUN_004a225a.
	void CInGameGuiManager::MiscWindows_HelpWindow_Open()
	{
		l_Selection_RemoveAllGuiElements();

		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7E8, 0, 0, 0) != nullptr)
		{

			return;
		}

		MiscWindows_CloseAllLargeOnes(true);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiMiscWindows::SHelpWindowStorage), 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiMiscWindows::HyperLinkTextWindow_Construct(allocation);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
	}

	// Original function: FUN_004a22ab.
	void CInGameGuiManager::MiscWindows_HelpWindow_Open_HouseInfo(std::uint32_t type, std::uint32_t developmentStage)
	{
		MiscWindows_HelpWindow_Open();

		if (void* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7E8, 0, 0, 0))
		{
			using TMessage = NXBaseGui::TGuiMessage;
			NC2Lifetime::Read<TMessage*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](window, 0x800, 20000, 0, type, developmentStage);
		}
	}

	// Original function: FUN_004a22e8.
	void CInGameGuiManager::MiscWindows_HelpWindow_Open_GoodInfo(std::uint32_t good)
	{
		MiscWindows_HelpWindow_Open();

		if (void* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7E8, 0, 0, 0))
		{
			using TMessage = NXBaseGui::TGuiMessage;
			NC2Lifetime::Read<TMessage*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](window, 0x800, 20001, 0, good, 0);
		}
	}
}
#include "XPalette.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2StringAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiStatisticsData.h"
#include "InGameGuiStatisticsWindow.h"
#include "InGameGuiSupportAccess.h"
#include "InGameGuiWindowState.h"
#include "MainMenuGuiManager_Access.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"

#include <array>
#include <cstring>
#include <Windows.h>

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2GuiToolsBase;
	using namespace NC2Logic;
	// Complete Windows layout region 00569200..0056921F, initialized by 004B1B49/004B1B61.
	static const NXBasics::SRectangle gEntityControllerArea(4, 50, 124, 24), gTimeScaleArea(4, 400, 100, 16);

	struct SJobCount
	{
		std::uint32_t job;
		std::int32_t count;
	};

	static SJobCount gStatisticsJobCounts[56]{ }; // Complete scratch array 00569230..005693EF.

	// Complete lazy color region 005693F0..005693F7.
	struct SStatisticsShadowState
	{
		std::uint32_t initialized = 0;
		NXBasics::SColorRGB color{ 0, 0, 0 };
	};

	static SStatisticsShadowState gStatisticsShadow;
	static_assert(sizeof(SStatisticsShadowState) == 8);

	// Original function: FUN_004b2f88; the native color destructor is empty.
	static int __cdecl StatisticsShadow_Exit()
	{

		return 0;
	}

	static std::uint32_t Page()
	{

		return Read<std::uint32_t>(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::Page);
	}

	static std::uint32_t LocalPlayer()
	{

		return NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
	}

	static const NXBasics::SColorRGB* White()
	{

		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
	}

	static void* __fastcall StatisticsWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall StatisticsWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	static void __fastcall StatisticsWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	// Complete Windows virtual table 004F6CA8..004F6CF3.
	static const std::array<std::uintptr_t, 19> gStatisticsWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&StatisticsWindow_Delete);
		t[2] = reinterpret_cast<std::uintptr_t>(&StatisticsWindow_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&StatisticsWindow_Draw);

		return t;
	}();

	// Original function: FUN_004b1b77.
	std::int32_t SStatisticsEntityDescription::GetNumber(const SPlayerStatisticSampleData* sample) const
	{
		std::uint32_t result = 0;

		for (std::uint32_t index = 0; index < count; ++index)
		{
			result += static_cast<std::uint32_t>(sample->GetNumber(ids[index]));
		}

		return static_cast<std::int32_t>(result);
	}

	// Original function: FUN_004b1ba6.
	const char* SStatisticsEntityDescription::GetNamePtr() const
	{

		return good != 0 ? StringAccess_GetGoodNamePtr(good, 2) : CGuiBaseStringsManager::GetStringPtr(textArray, textId);
	}

	// Original function: FUN_004b1d91; the embedded receiver starts at window +0x88.
	static void* StatisticsWindow_Vars_Construct(void* vars)
	{
		for (std::uint32_t index = 0; index < 12; ++index)
		{
			::new (static_cast<std::byte*>(vars) + NC2Layout::StatisticsDiagramStateOffset::Colors + index * 4) NXBasics::SColorRGB;
		}

		return vars;
	}

	// Original function: FUN_004b1bc5; Windows CStatisticsWindow allocation 0x26C.
	void* StatisticsWindow_Construct(void* window)
	{
		const auto& display = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle area(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(display.displayWidth) - 500u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(display.displayHeight) - 420u) / 2, 500, 420);
		BaseToolWindow_Construct(window, &area);
		StatisticsWindow_Vars_Construct(static_cast<std::byte*>(window) + NC2Layout::StatisticsWindowOffset::DiagramState);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::StatisticsWindowOffset::JobsList, 0, NC2Layout::RecordSize::StatisticsWindowVars);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gStatisticsWindowVirtualTable.data());
		const NXBasics::SColorRGB colors[12] = { { 255, 49, 56 }, { 248, 155, 44 }, { 252, 241, 43 }, { 89, 211, 106 }, { 0, 153, 211 }, { 0, 114, 188 }, { 46, 49, 146 }, { 146, 39, 143 }, { 0, 0, 0 }, { 255, 255, 255 }, { 128, 128, 128 }, { 64, 28, 0 } };
		std::memcpy(static_cast<std::byte*>(window) + NC2Layout::StatisticsWindowOffset::DiagramColors, colors, sizeof(colors));
		StatisticsWindow_l_GuiButtons_Add(window);
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		manager->FrameCall_Register(window);
		manager->Misc_GameTime_Stop_LargeWindowOpened();

		return window;
	}

	// Original function: FUN_004b1daf.
	static void StatisticsWindow_Destruct(void* window)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gStatisticsWindowVirtualTable.data());
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		manager->Misc_GameTime_Continue_LargeWindowClosed();
		manager->FrameCall_UnRegister(window);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
	}

	// Original function: FUN_004b1d75.
	static void* __fastcall StatisticsWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		StatisticsWindow_Destruct(window);

		if (flags & 1u)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004b1e71.
	static bool __fastcall StatisticsWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t data, std::uint32_t)
	{
		if (message != 0x800)
			return message == 0x1000 && command == 0x7E5;

		if (command >= 10000 && command < 10010)
		{
			Write(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::Page, command - 10000);
			StatisticsWindow_l_GuiButtons_Update(window);
		}
		else if (command >= 10010 && command < 10020)
		{
			Write(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::GoodGroup, command - 10010);
			StatisticsWindow_l_GuiButtons_Update(window);
		}
		else if (command == 10100 || command == 10200)
		{
			if (command == 10100)
			{
				reinterpret_cast<SStatisticsEntityDescription*>(data)->enabled ^= 1u;
			}
			else
			{
				Write(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::TimeScaleHours, data);
			}

			StatisticsWindow_l_GuiButtons_Update(window);
			FXTool_ClickConfirm();
		}
		else if (command == 11000)
		{
			NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->MiscWindows_HumanListWindow_Open(data, 0);
			FXTool_ClickConfirm();
		}
		else if (command == 0x7EE)
		{
			if (Page() == 1 && timeGetTime() > Read<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::JobsListUpdateTick) + 2000u)
				StatisticsWindow_l_GuiButtons_UpdateJobsListBox(window);

			if (Page() == 3 && timeGetTime() > Read<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::HousesListUpdateTick) + 2000u)
				StatisticsWindow_l_GuiButtons_UpdateHousesListBox(window);

			if (Page() == 6 && timeGetTime() > Read<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::GraveyardListUpdateTick) + 2000u)
				StatisticsWindow_l_GuiButtons_UpdateGraveyardListBox(window);
		}

		return true;
	}

	// Original function: FUN_004b1f99.
	void StatisticsWindow_l_GuiButtons_Update(void* window)
	{
		NXBaseGui::BaseWindow_Element_DeleteAll(window);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::StatisticsWindowOffset::JobsList, 0, NC2Layout::RecordSize::StatisticsListPointers);
		StatisticsWindow_l_GuiButtons_Add(window);
	}

	// Native entity dispatch is inlined at multiple Windows call sites.
	static SStatisticsEntityDescription* StatisticsWindow_l_Misc_GetFirstStatisticsEntityPtr()
	{
		if (Page() == 0)
			return gStatisticsEntities0;

		if (Page() == 2)
			return gStatisticsEntities1;

		if (Page() == 5)
			return gStatisticsEntities7;

		if (Page() != 4)
			return nullptr;

		static SStatisticsEntityDescription* const goods[5] = { gStatisticsEntities2, gStatisticsEntities3, gStatisticsEntities4, gStatisticsEntities5, gStatisticsEntities6 };
		const auto subpage = Read<std::uint32_t>(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::GoodGroup);

		return subpage < 5 ? goods[subpage] : nullptr;
	}

	// Original function: FUN_004b31fa.
	static NXBasics::SRectangle StatisticsWindow_l_Tool_GetEntityControllerButtonArea(std::uint32_t index)
	{
		auto area = gEntityControllerArea;
		area.y += static_cast<std::int32_t>(index * 26u);

		if (Page() == 4)
			area.y += 26;

		return area;
	}

	// Original function: FUN_004b1fba.
	void StatisticsWindow_l_GuiButtons_Add(void* window)
	{
		const NXBasics::SRectangle title(2, 2, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22, 14), close(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18, 2, 16, 14);
		auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(248, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &title, StringTool_GetGameGuiMiscWindowStringPtr(180)));
		allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &close));
		constexpr std::uint32_t graphics[7] = { 162, 158, 161, 160, 164, 163, 159 };
		NXBasics::SRectangle area(4, 22, 68, 24);

		for (std::uint32_t index = 0; index < 7; ++index)
		{
			allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
			void* element = allocation == nullptr ? nullptr : BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &area, graphics[index], StringTool_GetGameGuiMiscWindowStringPtr(181 + index), 10000 + index, window);
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Selected, Page() == index);
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::RectangularStyle, 1);
			NXBaseGui::BaseWindow_Element_Add(window, element);
			area.x += area.width + 2;
		}

		auto* entities = StatisticsWindow_l_Misc_GetFirstStatisticsEntityPtr();
		std::uint32_t index = 0;

		if (entities != nullptr)
			for (auto* entity = entities; entity->count != 0; ++entity, ++index)
			{
				area = StatisticsWindow_l_Tool_GetEntityControllerButtonArea(index);
				allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
				void* element = allocation == nullptr ? nullptr : BaseToolTextButton_Construct(allocation, nullptr, &area, entity->GetNamePtr(), 10100, window);
				Write(element, NC2Layout::ToolTextButtonOffset::CommandData1, entity);
				Write<std::uint32_t>(element, NC2Layout::ToolTextButtonOffset::CommandData2, 0);
				Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SmallFontFlag, 1);
				Write(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, entity->enabled);

				if (Page() == 4)
				{
					Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::CenteredFlag, 0);
					NC2InGameGuiBase::BaseToolTextButton_SetToolTipString(element, StringAccess_GetGoodNamePtr(entity->good, 1));
					Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::ImmediateTooltipFlag, 1);
					char caption[200];
					NXBasics::Runtime_PrintString(caption, "{setx:50}{gfxgood:%d,1}", entity->good);
					NC2InGameGuiBase::BaseToolTextButton_SetCaptionString(element, caption);
				}

				NXBaseGui::BaseWindow_Element_Add(window, element);
			}

		if (entities != nullptr)
		{
			area = gTimeScaleArea;

			for (auto scale : gStatisticsTimeScales)
			{
				char caption[32];

				if (scale == 1)
				{
					std::strcpy(caption, StringTool_GetGameGuiMiscWindowStringPtr(201));
				}
				else
				{
					NXBasics::Runtime_PrintString(caption, StringTool_GetGameGuiMiscWindowStringPtr(202), scale);
				}

				allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
				void* element = allocation == nullptr ? nullptr : BaseToolTextButton_Construct(allocation, nullptr, &area, caption, 10200, window);
				Write(element, NC2Layout::ToolTextButtonOffset::CommandData1, scale);
				Write<std::uint32_t>(element, NC2Layout::ToolTextButtonOffset::CommandData2, 0);
				Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, scale == Read<std::uint32_t>(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::TimeScaleHours));
				NXBaseGui::BaseWindow_Element_Add(window, element);
				area.x += area.width + 3;
			}
		}

		if (Page() == 4)
		{
			constexpr std::uint32_t goodGraphics[5] = { 73, 72, 75, 76, 71 };
			area = NXBasics::SRectangle(4, 50, 28, 20);

			for (std::uint32_t group = 0; group < 5; ++group)
			{
				allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
				void* element = allocation == nullptr ? nullptr : BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &area, goodGraphics[group], CGuiBaseStringsManager::GetStringPtr(8, 31 + group), 10010 + group, window);
				Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Selected, Read<std::uint32_t>(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::GoodGroup) == group);
				Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::RectangularStyle, 1);
				Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::ImmediateTooltip, 1);
				NXBaseGui::BaseWindow_Element_Add(window, element);
				area.x += area.width + 2;
			}
		}

		if (Page() == 1 || Page() == 3 || Page() == 6)
		{
			area = NXBasics::SRectangle(4, 54, 468, 360);
			allocation = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
			void* list = allocation == nullptr ? nullptr : ToolListBoxElement_Construct(allocation, nullptr, &area);
			Write(window, Page() == 1 ? NC2Layout::StatisticsWindowOffset::JobsList : Page() == 3 ? NC2Layout::StatisticsWindowOffset::HousesList : NC2Layout::StatisticsWindowOffset::GraveyardList, list);
			NXBaseGui::BaseWindow_Element_Add(window, list);

			if (Page() == 1)
			{
				StatisticsWindow_l_GuiButtons_UpdateJobsListBox(window);
			}
			else if (Page() == 3)
			{
				StatisticsWindow_l_GuiButtons_UpdateHousesListBox(window);
			}
			else
			{
				StatisticsWindow_l_GuiButtons_UpdateGraveyardListBox(window);
			}

			area.x = 476;
			area.width = 20;
			allocation = NXBasics::Runtime_AllocateWithNewHandler(60, 1);
			NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : ToolListBoxControllerElement_Construct(allocation, nullptr, &area, list, false, true));
		}
	}

	// Original function: FUN_004b289e.
	static std::int32_t __cdecl StatisticsWindow_ls_SortJobs(const void* first, const void* second)
	{
		const auto* a = static_cast<const SJobCount*>(first);
		const auto* b = static_cast<const SJobCount*>(second);

		return StringToolMisc_StringSortCompare(StringAccess_GetJobNamePtr(a->job, a->count), StringAccess_GetJobNamePtr(b->job, b->count));
	}

	// Original function: FUN_004b2795.
	void StatisticsWindow_l_GuiButtons_UpdateJobsListBox(void* window)
	{
		auto* list = Read<void*>(window, NC2Layout::StatisticsWindowOffset::JobsList);

		if (list == nullptr)
		{

			return;
		}

		const auto scroll = Read<std::int32_t>(list, NC2Layout::ListBoxOffset::ScrollY);
		NC2InGameGuiBaseListBox::BaseListBoxElement_Element_DeleteAll(list);
		std::uint32_t count = 0;

		for (std::uint32_t job = 0; job < 56; ++job)
		{
			const auto amount = Tool_PlayerStatistics_CountHumansOfJobExecuting(LocalPlayer(), job);

			if (amount > 0)
				gStatisticsJobCounts[count++] = { job, amount };
		}

		if (count != 0)
			NXBasics::Runtime_QuickSort(gStatisticsJobCounts, count, 8, StatisticsWindow_ls_SortJobs);

		NXBasics::SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			const auto& job = gStatisticsJobCounts[index];
			char caption[200];
			NXBasics::Runtime_PrintString(caption, "%d %s", job.count, StringAccess_GetJobNamePtr(job.job, job.count));
			auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
			void* element = allocation == nullptr ? nullptr : BaseToolTextButton_Construct(allocation, nullptr, &row, caption, 11000, window);
			Write(element, NC2Layout::ToolTextButtonOffset::CommandData1, job.job);
			Write<std::uint32_t>(element, NC2Layout::ToolTextButtonOffset::CommandData2, 0);
			BaseListBoxElement_Element_Add(list, nullptr, element);
			row.y += row.height + 2;
		}

		NC2InGameGuiBaseListBox::BaseListBoxElement_ScrollOffset_Set(list, 0, scroll);
		Write<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::JobsListUpdateTick, timeGetTime());
	}

	// Original function: FUN_004b2a1d.
	static std::int32_t __cdecl StatisticsWindow_ls_SortHouses(const void* first, const void* second)
	{

		return StringToolMisc_StringSortCompare(StringAccess_GetHouseNamePtr(*static_cast<const std::uint32_t*>(first), 1), StringAccess_GetHouseNamePtr(*static_cast<const std::uint32_t*>(second), 1));
	}

	// Original function: FUN_004b28c8.
	void StatisticsWindow_l_GuiButtons_UpdateHousesListBox(void* window)
	{
		auto* list = Read<void*>(window, NC2Layout::StatisticsWindowOffset::HousesList);

		if (list == nullptr)
		{

			return;
		}

		const auto scroll = Read<std::int32_t>(list, NC2Layout::ListBoxOffset::ScrollY);
		NC2InGameGuiBaseListBox::BaseListBoxElement_Element_DeleteAll(list);
		auto* types = static_cast<std::uint32_t*>(NXBasics::Runtime_AllocateWithNewHandler(220, 1));

		for (std::uint32_t type = 0; type < 55; ++type)
		{
			types[type] = type;
		}

		NXBasics::Runtime_QuickSort(types, 55, 4, StatisticsWindow_ls_SortHouses);
		NXBasics::SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22);

		for (std::uint32_t index = 0; index < 55; ++index)
		{
			std::int32_t unfinished = 0;
			const auto total = Tool_PlayerStatistics_CountHousesOfLogicType(LocalPlayer(), types[index], &unfinished);

			if (total == 0 && unfinished == 0)
				continue;

			char caption[200];

			if (unfinished != 0)
			{
				NXBasics::Runtime_PrintString(caption, "%d+%d ", total - unfinished, unfinished);
			}
			else
			{
				NXBasics::Runtime_PrintString(caption, "%d ", total);
			}

			std::strcat(caption, StringAccess_GetHouseNamePtr(types[index], total + unfinished));
			auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(256, 1);
			void* element = allocation == nullptr ? nullptr : BaseToolTextElement_Construct(allocation, nullptr, &row, caption, true, false);
			Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::FillBackgroundFlag, 0);
			BaseListBoxElement_Element_Add(list, nullptr, element);
			row.y += row.height + 2;
		}

		NXBasics::Runtime_Free(types);
		NC2InGameGuiBaseListBox::BaseListBoxElement_ScrollOffset_Set(list, 0, scroll);
		Write<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::HousesListUpdateTick, timeGetTime());
	}

	// Original function: FUN_004b2a49.
	void StatisticsWindow_l_GuiButtons_UpdateGraveyardListBox(void* window)
	{
		auto* list = Read<void*>(window, NC2Layout::StatisticsWindowOffset::GraveyardList);

		if (list == nullptr)
		{

			return;
		}

		const auto scroll = Read<std::int32_t>(list, NC2Layout::ListBoxOffset::ScrollY);
		NC2InGameGuiBaseListBox::BaseListBoxElement_Element_DeleteAll(list);
		const auto* dead = NC2InGameGuiDeadHumanDataManager::gDeadHumanDataManager;
		const auto* records = Read<const std::byte*>(dead, NC2Layout::DeadHumanDataManagerOffset::Records);
		NXBasics::SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22);

		for (std::uint32_t index = 0; index < Read<std::uint32_t>(dead, NC2Layout::DeadHumanDataManagerOffset::Count); ++index)
		{
			const auto* record = records + index * 0x3Cu;
			char caption[200];
			NXBasics::Runtime_PrintString(caption, "%s - %s", reinterpret_cast<const char*>(record + 4), StringAccess_GetJobNamePtr(Read<std::uint32_t>(record, NC2Layout::DeadHumanRecordOffset::JobType), 1));
			auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(256, 1);
			void* element = allocation == nullptr ? nullptr : BaseToolTextElement_Construct(allocation, nullptr, &row, caption, true, false);
			Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::FillBackgroundFlag, 0);
			BaseListBoxElement_Element_Add(list, nullptr, element);
			row.y += row.height + 2;
		}

		NC2InGameGuiBaseListBox::BaseListBoxElement_ScrollOffset_Set(list, 0, scroll);
		Write<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::GraveyardListUpdateTick, timeGetTime());
	}

	// Original function: FUN_004b2f89.
	static void StatisticsWindow_l_DrawDiagram_Start(void* window)
	{
		const auto minimum = Read<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::ObservedMinimum), maximum = Read<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::ObservedMaximum);
		Write<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::ObservedMinimum, 0);
		Write<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::ObservedMaximum, 0);
		Write(window, NC2Layout::StatisticsWindowOffset::DisplayMinimum, minimum - 1u);
		Write(window, NC2Layout::StatisticsWindowOffset::DisplayMaximum, maximum + 1u);
	}

	// Original function: FUN_004b2fc0. Numeric helpers replace x87 truncation; intermediate precision can differ.
	static std::int32_t StatisticsWindow_l_DrawDiagram_GetDisplayY(void* window, std::int32_t value)
	{
		if (value < Read<std::int32_t>(window, NC2Layout::StatisticsWindowOffset::ObservedMinimum))
			Write(window, NC2Layout::StatisticsWindowOffset::ObservedMinimum, value);

		if (value > Read<std::int32_t>(window, NC2Layout::StatisticsWindowOffset::ObservedMaximum))
			Write(window, NC2Layout::StatisticsWindowOffset::ObservedMaximum, value);

		const auto maximum = Read<std::int32_t>(window, NC2Layout::StatisticsWindowOffset::DisplayMaximum);
		const auto span = static_cast<std::int32_t>(Read<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::DisplayMinimum) - Read<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::DisplayMaximum));

		return static_cast<std::int32_t>(NXBasics::Runtime_TruncateDoubleToInt64((static_cast<double>(value) - maximum) / span * Read<std::int32_t>(window, NC2Layout::StatisticsWindowOffset::DiagramHeight)));
	}

	// Original function: FUN_004b3013.
	static void StatisticsWindow_l_DrawDiagram_Do(void* window, const SStatisticsEntityDescription* entity, const NXBasics::SColorRGB* color)
	{
		const auto width = Read<std::int32_t>(window, NC2Layout::StatisticsWindowOffset::DiagramWidth);
		const auto step = static_cast<float>(static_cast<double>(Read<std::uint32_t>(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::TimeScaleHours)) * 3600.0 / width);
		auto time = static_cast<float>(static_cast<double>(gGameTime.tick) * static_cast<double>(0.0833333358168602f));
		const auto player = Read<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::DiagramPlayerId);
		auto* sample = PlayerStatisticManager_GetStatistics(gPlayerStatisticManager, static_cast<std::int32_t>(NXBasics::Runtime_TruncateDoubleToInt64(time)), player);

		if (sample == nullptr)
		{

			return;
		}

		SPlayerStatisticSampleData copy;
		std::memcpy(&copy, sample, sizeof(copy));
		const auto value = entity->GetNumber(&copy);
		auto previousY = StatisticsWindow_l_DrawDiagram_GetDisplayY(window, value);
		auto x = width - 1;
		char caption[16];
		NXBasics::Runtime_PrintString(caption, "%d", value);
		auto* bitmap = Read<NXBasics::CBitmap*>(window, NC2Layout::StatisticsWindowOffset::DiagramState);
		auto* diagram = Read<NXBasics::CBitmap*>(window, NC2Layout::StatisticsWindowOffset::DiagramBitmap);
		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		bitmap->Text_Print(font, White(), 498 - NXBasics::Font_GetPixelWidth(font, caption), previousY + 52 < 60 ? 60 : previousY + 52, caption);

		while (x > 0)
		{
			time = static_cast<float>(static_cast<double>(time) - static_cast<double>(step));
			sample = PlayerStatisticManager_GetStatistics(gPlayerStatisticManager, static_cast<std::int32_t>(NXBasics::Runtime_TruncateDoubleToInt64(time)), player);
			const auto nextX = x - 1;
			const auto y = StatisticsWindow_l_DrawDiagram_GetDisplayY(window, sample == nullptr ? 0 : entity->GetNumber(sample));

			if (color != nullptr)
				diagram->Draw_Line(nextX, y, x, previousY, *color);

			if (sample == nullptr)
				break;

			std::memcpy(&copy, sample, sizeof(copy));
			x = nextX;
			previousY = y;
		}
	}

	// Original function: FUN_004b31c5.
	static void StatisticsWindow_l_DrawDiagram_GetMaximumValues(void* window, const SStatisticsEntityDescription* entities)
	{
		StatisticsWindow_l_DrawDiagram_Start(window);
		StatisticsWindow_l_DrawDiagram_GetDisplayY(window, 0);

		for (auto* entity = entities; entity->count != 0; ++entity)
		{
			if (entity->enabled != 0)
				StatisticsWindow_l_DrawDiagram_Do(window, entity, nullptr);
		}
	}

	// Original function: FUN_004b2b28.
	static void __fastcall StatisticsWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		const auto* current = PlayerStatisticManager_GetStatistics(gPlayerStatisticManager, static_cast<std::int32_t>(gGameTime.tick / 12u), Read<std::uint32_t>(window, NC2Layout::StatisticsWindowOffset::DiagramPlayerId));
		auto* entities = StatisticsWindow_l_Misc_GetFirstStatisticsEntityPtr();
		const auto* colors = reinterpret_cast<const NXBasics::SColorRGB*>(static_cast<const std::byte*>(window) + NC2Layout::StatisticsWindowOffset::DiagramColors);

		if (current != nullptr && entities != nullptr)
		{
			std::uint32_t index = 0;

			for (auto* entity = entities; entity->count != 0; ++entity, ++index)
			{
				auto area = StatisticsWindow_l_Tool_GetEntityControllerButtonArea(index);
				area.x += area.width + 2;
				area.width = 24;

				if (entity->enabled != 0)
					bitmap->Draw_Box(area, colors[index % 12]);

				char caption[20];
				NXBasics::Runtime_PrintString(caption, "%d", entity->GetNumber(current));
				const auto x = area.x + 22 - NXBasics::Font_GetPixelWidth(font, caption);
				const auto y = area.y + 3;

				if ((gStatisticsShadow.initialized & 1u) == 0)
				{
					gStatisticsShadow.initialized |= 1u;
					gStatisticsShadow.color.SetColors(0, 0, 0);
					NXBasics::Runtime_AtExit(StatisticsShadow_Exit);
				}

				constexpr std::int32_t shadowX[4] = { -1, 1, 0, 0 }, shadowY[4] = { 0, 0, -1, 1 };

				for (std::uint32_t shadow = 0; shadow < 4; ++shadow)
				{
					bitmap->Text_Print(font, &gStatisticsShadow.color, x + shadowX[shadow], y + shadowY[shadow], caption);
				}

				bitmap->Text_Print(font, White(), x, y, caption);
			}
		}

		if (entities == nullptr)
		{

			return;
		}

		NXBasics::CBitmap diagram(bitmap, NXBasics::SRectangle(160, 60, 320, 320));
		const auto scale = Read<std::uint32_t>(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::TimeScaleHours);
		const NXBasics::SColorRGB grid(70, 70, 70), zero(100, 100, 100);

		for (std::uint32_t index = 1; index < scale; ++index)
		{
			const auto x = 320 - 320u * index / scale;
			diagram.Draw_VerticalLine(x, 0, 320, grid);
			bitmap->Text_PrintV(font, White(), x + 150, 380, "-%d h", index);
		}

		Write(window, NC2Layout::StatisticsWindowOffset::DiagramState, bitmap);
		Write(window, NC2Layout::StatisticsWindowOffset::DiagramBitmap, &diagram);
		Write<std::int32_t>(window, NC2Layout::StatisticsWindowOffset::DiagramWidth, 320);
		Write<std::int32_t>(window, NC2Layout::StatisticsWindowOffset::DiagramHeight, 320);
		Write(window, NC2Layout::StatisticsWindowOffset::DiagramPlayerId, LocalPlayer());
		DrawTool_DrawFrame(&diagram, diagram.GetRectangle(), 2);
		StatisticsWindow_l_DrawDiagram_GetMaximumValues(window, entities);
		StatisticsWindow_l_DrawDiagram_Start(window);
		auto zeroY = StatisticsWindow_l_DrawDiagram_GetDisplayY(window, 0);

		if (zeroY >= 318)
			zeroY = 318;

		diagram.Draw_HorizontalLine(2, zeroY, 316, zero);
		bitmap->Text_Print(font, White(), 498 - NXBasics::Font_GetPixelWidth(font, "0"), zeroY + 52, "0");
		std::uint32_t index = 0;

		for (auto* entity = entities; entity->count != 0; ++entity, ++index)
		{
			if (entity->enabled != 0)
				StatisticsWindow_l_DrawDiagram_Do(window, entity, colors + index % 12);
		}

		const auto seconds = gGameTime.tick / 12u;
		bitmap->Text_PrintV(font, White(), 163, 59, "%d:%2.2d:%2.2d", seconds / 3600u, seconds / 60u % 60u, seconds % 60u);
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a202f.
	void CInGameGuiManager::MiscWindows_StatisticsWindow_Toggle()
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(0x7E5, 0, 0, 0);

		if (window != nullptr)
		{
			desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
		else
		{
			MiscWindows_StatisticsWindow_Open();
		}
	}

	// Original function: FUN_004a2063.
	void CInGameGuiManager::MiscWindows_StatisticsWindow_Open()
	{
		l_Selection_RemoveAllGuiElements();

		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7E5, 0, 0, 0) != nullptr)
		{

			return;
		}

		MiscWindows_CloseAllLargeOnes(true);
		auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(620, 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiMiscWindows::StatisticsWindow_Construct(allocation);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(reinterpret_cast<NXBaseGui::CBaseWindow*>(window), false);
	}
}
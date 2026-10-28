#include "C2E2HumanSoundAccess.h"
#include "XTextEncoding.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineMoveableDrawing.h"
#include "C2EngineObjectData.h"
#include "C2HumanEquipment.h"
#include "C2HumanJobAccess.h"
#include "C2HumanUserControl.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2PlayerCenterPosition.h"
#include "C2RuntimeConstants.h"
#include "C2StaticTribeTypeData.h"
#include "C2StringAccess.h"
#include "C2VehicleAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "InGameGuiTraderConfigWindow.h"
#include "InGameGuiWindowState.h"
#include "SwitchIdentifiers.h"
#include "WorldDisplayElementAccess.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include "XMKeyManager.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"
#include "XPalette.h"

#include <cstdarg>
#include <cstring>
#include <new>

extern void __fastcall Display_DE_MapMoveableToGlobalWorldPixelCoordinates(void* display, void* unusedEdx, std::uint32_t moveableId, std::int32_t* point);

namespace NC2InGameGuiBase
{
	// Original function: FUN_004b9d0c; full Windows storage, including text padding.
	CStaticToolTipHandler::CStaticToolTipHandler()
	{
		std::memset(this, 0, sizeof(*this));
	}
	// Original function: FUN_004b9d23.
	CStaticToolTipHandler::~CStaticToolTipHandler()
	{}
	// Original function: FUN_004b9d24.
	void CStaticToolTipHandler::ClearAll()
	{
		mCount = 0;
	}
	// Original function: FUN_004b9d28; the terminator follows the 100 copied bytes.
	bool CStaticToolTipHandler::Add(const NXBasics::SRectangle& rectangle, const char* text)
	{
		if (mCount == NC2Runtime::SelectionTooltipLimits::Entries) return false;
		auto& entry = mEntries[mCount];
		entry.rectangle = rectangle;
		NXBasics::Text_Copy(entry.text, NC2Runtime::SelectionTooltipLimits::CopiedTextBytes + 1u, text);
		entry.text[NC2Runtime::SelectionTooltipLimits::StoredTerminator] = '\0';
		++mCount;
		return true;
	}
	// Original function: FUN_004b9d73; the native temporary buffer holds 100 bytes.
	bool CStaticToolTipHandler::AddV(const NXBasics::SRectangle& rectangle, const char* format, ...)
	{
		char text[NC2Runtime::SelectionTooltipLimits::FormatBufferBytes];
		va_list arguments;
		va_start(arguments, format);
		NXBasics::Runtime_FormatString(text, NC2Runtime::SelectionTooltipLimits::FormatBufferBytes, format, arguments);
		va_end(arguments);
		text[NC2Runtime::SelectionTooltipLimits::FormatTerminator] = '\0';
		return Add(rectangle, text);
	}
	// Original function: FUN_004b9da3; later rectangles take precedence.
	const char* CStaticToolTipHandler::Get(const std::int32_t* point) const
	{
		for (std::uint32_t index = mCount; index != 0; --index)
		{
			const auto& entry = mEntries[index - 1];
			const auto right = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(entry.rectangle.x) + static_cast<std::uint32_t>(entry.rectangle.width) - 1u);
			const auto bottom = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(entry.rectangle.y) + static_cast<std::uint32_t>(entry.rectangle.height) - 1u);
			if (point[0] >= entry.rectangle.x && point[0] <= right && point[1] >= entry.rectangle.y && point[1] <= bottom) return entry.text;
		}
		return nullptr;
	}

	// Original function: FUN_004b9ddd; complete 100-entry Windows storage.
	CDynamicSubButtonsHandler::CDynamicSubButtonsHandler()
	{
		std::memset(this, 0, sizeof(*this));
	}
	// Original function: FUN_004b9df4; the owning window survives child deletion.
	void CDynamicSubButtonsHandler::AllButtonsHaveBeenRemoved()
	{
		std::memset(mEntries, 0, sizeof(mEntries));
	}
	// Original function: FUN_004b9e08; unchanged coordinates retain the existing control.
	void CDynamicSubButtonsHandler::UpdateGfxButton(std::uint32_t command, std::uint32_t id, std::int32_t x, std::int32_t y, std::uint32_t graphic, const char* tooltip)
	{
		if (mWindow == nullptr) return;
		auto* entry = l_GetButtonDataPtr(command, id);
		if (entry != nullptr)
		{
			if (entry->x == x && entry->y == y) return;
			NXBaseGui::BaseWindow_Element_Delete(mWindow, entry->graphic);
		}
		else entry = l_GetFreeButtonDataPtr();
		if (entry == nullptr) return;
		const NXBasics::SRectangle rectangle(x, y, 18, 16);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiGfxButton, 1);
		entry->graphic = allocation == nullptr ? nullptr : BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &rectangle, graphic, tooltip, command, mWindow);
		NXBaseGui::BaseWindow_Element_Add(mWindow, entry->graphic);
		entry->x = x;
		entry->y = y;
		entry->command = command;
		entry->id = id;
		NC2Lifetime::Write<std::uint8_t>(entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag, 1);
	}
	// Original function: FUN_004b9eb7.
	void CDynamicSubButtonsHandler::UpdateTextButton(std::uint32_t command, std::uint32_t id, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, const char* text, bool centered, const char* tooltip)
	{
		if (mWindow == nullptr) return;
		auto* entry = l_GetButtonDataPtr(command, id);
		if (entry != nullptr)
		{
			if (entry->x == x && entry->y == y) return;
			NXBaseGui::BaseWindow_Element_Delete(mWindow, entry->text);
		}
		else entry = l_GetFreeButtonDataPtr();
		if (entry == nullptr) return;
		const NXBasics::SRectangle rectangle(x, y, width, height);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
		entry->text = allocation == nullptr ? nullptr : BaseToolTextButton_ConstructWithToolTip(allocation, nullptr, &rectangle, text, tooltip, command, mWindow);
		NC2Lifetime::Write<std::uint8_t>(entry->text, NC2Layout::ToolTextButtonOffset::CenteredFlag, centered);
		NXBaseGui::BaseWindow_Element_Add(mWindow, entry->text);
		entry->x = x;
		entry->y = y;
		entry->command = command;
		entry->id = id;
		NC2Lifetime::Write<std::uint8_t>(entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag, 1);
	}
	// Original function: FUN_004b9f70; the two control classes have distinct offsets.
	void CDynamicSubButtonsHandler::SetButtonMiscCommandData(std::uint32_t command, std::uint32_t id, std::uint32_t first, std::uint32_t second)
	{
		if (mWindow == nullptr) return;
		auto* entry = l_GetButtonDataPtr(command, id);
		if (entry == nullptr) return;
		if (entry->graphic != nullptr)
		{
			NC2Lifetime::Write(entry->graphic, NC2Layout::ToolGfxButtonOffset::CommandData1, first);
			NC2Lifetime::Write(entry->graphic, NC2Layout::ToolGfxButtonOffset::CommandData2, second);
		}
		if (entry->text != nullptr)
		{
			NC2Lifetime::Write(entry->text, NC2Layout::ToolTextButtonOffset::CommandData1, first);
			NC2Lifetime::Write(entry->text, NC2Layout::ToolTextButtonOffset::CommandData2, second);
		}
	}
	// Original function: FUN_004b9fad; inactive entries retain their unused fields.
	void CDynamicSubButtonsHandler::RemoveButton(std::uint32_t command, std::uint32_t id)
	{
		if (mWindow == nullptr) return;
		auto* entry = l_GetButtonDataPtr(command, id);
		if (entry == nullptr) return;
		if (entry->graphic != nullptr) NXBaseGui::BaseWindow_Element_Delete(mWindow, entry->graphic);
		if (entry->text != nullptr) NXBaseGui::BaseWindow_Element_Delete(mWindow, entry->text);
		NC2Lifetime::Write<std::uint8_t>(entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag, 0);
	}
	// Original function: FUN_004b9fef.
	CDynamicSubButtonsHandler::SEntry* CDynamicSubButtonsHandler::l_GetButtonDataPtr(std::uint32_t command, std::uint32_t id)
	{
		for (auto& entry : mEntries) if (NC2Lifetime::Read<std::uint8_t>(&entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag) != 0 && entry.command == command && entry.id == id) return &entry;
		return nullptr;
	}
	// Original function: FUN_004ba024.
	CDynamicSubButtonsHandler::SEntry* CDynamicSubButtonsHandler::l_GetFreeButtonDataPtr()
	{
		for (auto& entry : mEntries) if (NC2Lifetime::Read<std::uint8_t>(&entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag) == 0) return &entry;
		return nullptr;
	}
}

namespace NC2InGameGuiSelection
{
	using namespace NC2Lifetime;
	using NXBasics::SRectangle;

	// Original function: FUN_004cbf94; the bitmap view owns no source pixels.
	void SelectionTool_FillRectangle(NXBasics::CBitmap* bitmap, const SRectangle& rectangle, std::int32_t mode)
	{
		NXBasics::CBitmap view(bitmap, rectangle);
		NC2GuiToolsBase::DrawTool_FillArea(nullptr, &view, &view.GetRectangle(), mode);
		NC2GuiToolsBase::DrawTool_DrawFrame(nullptr, &view, &view.GetRectangle(), mode);
	}
	// Original function: FUN_004cbfd9.
	void SelectionTool_PrintCenteredString(NXBasics::CBitmap* bitmap, const SRectangle& rectangle, bool largeFont, const char* text)
	{
		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* font = largeFont ? resources.font10 : resources.font08;
		bitmap->Text_Print(font, reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(resources.fontWhitePalette) + NC2Layout::PaletteOffset::LastColor), rectangle.x + (rectangle.width - NXBasics::Font_GetPixelWidth(font, text)) / 2, rectangle.y + (rectangle.height - NXBasics::Font_GetPixelHeight(font, text)) / 2 - 1, text);
	}

	namespace
	{
		// Complete Windows descriptor region 004F6770..004F67CF. The final field is zero.
		struct SSubTabDescriptor
		{
			std::int32_t width, height;
			std::uint32_t title, field0C;
		};
		constexpr SSubTabDescriptor gSubTabs[6] = { { 200,136,1,0 },{ 240,100,2,0 },{ 260,100,3,0 },{ 260,80,5,0 },{ 260,80,4,0 },{ 200,84,6,0 } };
		static_assert(sizeof(gSubTabs) == 96);
		// Complete Windows experience-button/job-page region 004F67D0..004F6A8F.
		struct SExperiencePageButton
		{
			std::uint32_t graphic, tooltip;
		};
		constexpr SExperiencePageButton gExperiencePageButtons[11] = { { 151,130 },{ 149,131 },{ 146,132 },{ 155,133 },{ 147,134 },{ 153,135 },{ 152,136 },{ 148,137 },{ 154,138 },{ 150,139 },{ 156,140 } };
		constexpr std::uint32_t gExperiencePages[11][14] = {
			{ },{ 45,47,49,50,51,52,65 },{ 37,38,39,40,41,42,43,44 },{ 3,9,10,11,12,13,14,15,16,17,18,19,20 },
			{ 4,21,22,23,24 },{ 5,25,26,27 },{ 6,7,28,29,30,31,32,33,34,35,36 },{ 8,54,55,56,57,58,59,60,61 },
			{ 62,63 },{ 70,69,71,72,73,74,75,76 },{ 1,67,68,77 }
		};
		static_assert(sizeof(gExperiencePageButtons) + sizeof(gExperiencePages) == 704);
		struct SSelectionSingleHumanWindowMostExperiencesData
		{
			std::uint32_t count;
			struct SEntry
			{
				std::uint32_t job, amount;
			} entries[78];
		};
		const char* HumanString(std::uint32_t id)
		{
			return NC2GuiToolsBase::StringTool_GetGameGuiHumanSelectedWindowStringPtr(id);
		}
		NC2InGameGuiBase::CStaticToolTipHandler& Tooltips(void* window)
		{
			return *std::launder(reinterpret_cast<NC2InGameGuiBase::CStaticToolTipHandler*>(static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::Tooltips));
		}

		NC2InGameGuiBase::CDynamicSubButtonsHandler& Buttons(void* window)
		{
			return *std::launder(reinterpret_cast<NC2InGameGuiBase::CDynamicSubButtonsHandler*>(static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::DynamicButtons));
		}
		SRectangle Section(void* window, std::uint32_t index)
		{
			SRectangle rectangle(0, 0, 0, 0);
			std::memcpy(&rectangle, static_cast<const std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::TabRectangles + index * NC2Layout::RecordSize::SingleHumanTabRectangleUnsigned, sizeof(rectangle));
			return rectangle;
		}
		// Original function: FUN_004aed3f.
		void l_SubTabs_OverwriteHeight(void* window, std::uint32_t index, std::int32_t height)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags + index) == 0 || Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags + index) != 0) return;
			if (height < 16) height = 16;
			if (Read<std::int32_t>(window, NC2Layout::SingleHumanWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::SingleHumanTabHeightUnsigned) == height) return;
			Write(window, NC2Layout::SingleHumanWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::SingleHumanTabHeightUnsigned, height);
			Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::LayoutInvalidFlag, 1);
		}
		// Original function: FUN_004aeaf8.
		std::uint32_t l_SubTabs_GetEnabledChecksum(void* window)
		{
			std::uint32_t checksum = 0;
			for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::Tabs; ++index)
			{
				if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags + index) == 0) continue;
				checksum |= 1u << (index * 2u);
				if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags + index) != 0) checksum |= 2u << (index * 2u);
			}
			return checksum;
		}

		// Original function: FUN_004ae231.
		void* l_Misc_GetSelectedHumanPtr()
		{
			const auto id = NC2InGameGuiInputManager::Selection_GetSelectedSingleHumanId(NC2InGameGuiInputManager::gInputManager);
			if (id == -1) return nullptr;
			auto* human = NC2Logic::CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CHumanArray::ElementSize;
			return Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 ? human : nullptr;
		}
		// Original function: FUN_004aee9c; unsigned experience order and name tie-breaking.
		void l_SubTabs_Experiences_GetMostExperiences(SSelectionSingleHumanWindowMostExperiencesData* output)
		{
			output->count = 0;
			const void* human = l_Misc_GetSelectedHumanPtr();
			if (human == nullptr) return;
			for (std::uint32_t job = 0; job < NC2Runtime::SingleHumanWindowLimits::ExperienceJobs; ++job)
			{
				if (job == 2 || job == 46 || job == 48 || job == 53) continue;
				const auto amount = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperience + job * NC2Layout::RecordSize::HumanJobExperienceUnsigned);
				if (amount != 0) output->entries[output->count++] = { job, amount };
			}
			for (std::uint32_t pass = 0; pass + 1 < output->count; ++pass)
				for (std::uint32_t index = 0; index + 1 < output->count; ++index)
				{
					auto& first = output->entries[index];
					auto& second = output->entries[index + 1];
					if (first.amount > second.amount) continue;
					if (first.amount == second.amount && NC2GuiToolsBase::StringToolMisc_StringSortCompare(NC2Logic::StringAccess_GetJobExperienceNamePtr(first.job), NC2Logic::StringAccess_GetJobExperienceNamePtr(second.job)) <= 0) continue;
					const auto temporary = first;
					first = second;
					second = temporary;
				}
		}
		// Original function: FUN_004aee77.
		std::int32_t __cdecl lsSortExperiencesCompareFunction(const void* first, const void* second)
		{
			return NC2GuiToolsBase::StringToolMisc_StringSortCompare(NC2Logic::StringAccess_GetJobExperienceNamePtr(*static_cast<const std::uint32_t*>(first)), NC2Logic::StringAccess_GetJobExperienceNamePtr(*static_cast<const std::uint32_t*>(second)));
		}
		// Original function: FUN_004aed79; eleven pages of fourteen Windows job IDs.
		void l_SubTabs_Experiences_Init(void* window)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceTabEnabled) == 0 || l_Misc_GetSelectedHumanPtr() == nullptr) return;
			for (std::uint32_t page = 0; page < NC2Runtime::SingleHumanWindowLimits::ExperiencePages; ++page)
			{
				std::uint32_t count = 0;
				for (auto job : gExperiencePages[page])
				{
					if (job == 2 || job == 46 || job == 48 || job == 53) continue;
					Write(window, NC2Layout::SingleHumanWindowOffset::ExperienceJobsUnsigned + (page * NC2Runtime::SingleHumanWindowLimits::ExperienceJobsPerPageUnsigned + count++) * NC2Layout::RecordSize::SingleHumanExperienceJobIdUnsigned, job);
				}
			}
			SSelectionSingleHumanWindowMostExperiencesData most;
			l_SubTabs_Experiences_GetMostExperiences(&most);
			const auto count = most.count < NC2Runtime::SingleHumanWindowLimits::ExperienceJobsPerPage ? most.count : NC2Runtime::SingleHumanWindowLimits::ExperienceJobsPerPageUnsigned;
			for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::ExperienceJobsPerPage; ++index) Write(window, NC2Layout::SingleHumanWindowOffset::ExperienceJobsUnsigned + index * NC2Layout::RecordSize::SingleHumanExperienceJobIdUnsigned, index < count ? most.entries[index].job : 0u);
			for (std::uint32_t page = 1; page < NC2Runtime::SingleHumanWindowLimits::ExperiencePages; ++page)
			{
				auto* jobs = reinterpret_cast<std::uint32_t*>(static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::ExperienceJobsUnsigned + page * NC2Layout::RecordSize::SingleHumanExperiencePageUnsigned);
				std::uint32_t number = 0;
				for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::ExperienceJobsPerPage; ++index) if (jobs[index] != 0) ++number;
				if (number > 1) NXBasics::Runtime_QuickSort(jobs, number, NC2Layout::RecordSize::SingleHumanExperienceJobId, lsSortExperiencesCompareFunction);
			}
		}
		// Original function: FUN_004ae25f.
		void l_Misc_CenterWorldDisplay(void* window)
		{
			void* human = l_Misc_GetSelectedHumanPtr();
			void* preview = Read<void*>(window, NC2Layout::SingleHumanWindowOffset::PreviewElement);
			if (human == nullptr || preview == nullptr) return;
			void* display = Read<void*>(preview, NC2Layout::WorldGuiElementOffset::Display);
			std::int32_t point[2];
			Display_DE_MapMoveableToGlobalWorldPixelCoordinates(display, nullptr, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId), point);
			point[1] -= 25;
			NC2E2::DE_SetWantedPositionToPixelPoint(display, point);
		}
		// Windows outlined helper 00436B16; its separate original name is unresolved.
		void* l_Misc_GetSelectedMerchantDataPtr()
		{
			const void* human = l_Misc_GetSelectedHumanPtr();
			return human != nullptr ? NC2Logic::Human_GetMerchantDataPtr(human) : nullptr;
		}
		// Original function: FUN_004aeb29; compact valid routes into two GUI rows.
		void l_SubTabs_Trader_UpdateButtons(void* window)
		{
			if (Read<void*>(window, NC2Layout::SingleHumanWindowOffset::TraderConfigButton) == nullptr) return;
			auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
			const auto hide = [&] (std::uint32_t offset)
			{
				void* element = Read<void*>(window, offset);
				desktop->Element_Hide(static_cast<NXBaseGui::CBaseElement*>(element));
				Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Disabled, 1);
			};
			const auto show = [&] (std::uint32_t offset)
			{
				void* element = Read<void*>(window, offset);
				desktop->Element_UnHide(static_cast<NXBaseGui::CBaseElement*>(element));
				Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Disabled, 0);
			};
			hide(NC2Layout::SingleHumanWindowOffset::RouteSelectButtons);
			hide(NC2Layout::SingleHumanWindowOffset::SecondRouteSelectButton);
			show(NC2Layout::SingleHumanWindowOffset::RouteAssignButtons);
			show(NC2Layout::SingleHumanWindowOffset::SecondRouteAssignButton);
			hide(NC2Layout::SingleHumanWindowOffset::RouteActionButtons);
			hide(NC2Layout::SingleHumanWindowOffset::SecondRouteActionButton);
			NC2InGameGuiBase::BaseToolTextElement_SetCaptionString(Read<void*>(window, NC2Layout::SingleHumanWindowOffset::RouteLabels), HumanString(112));
			NC2InGameGuiBase::BaseToolTextElement_SetCaptionString(Read<void*>(window, NC2Layout::SingleHumanWindowOffset::SecondRouteLabel), HumanString(113));
			Write<std::uint8_t>(Read<void*>(window, NC2Layout::SingleHumanWindowOffset::TraderConfigButton), NC2Layout::GuiTextButtonOffset::DisabledFlag, 1);
			void* merchant = l_Misc_GetSelectedMerchantDataPtr();
			if (merchant == nullptr) return;
			std::uint32_t row = 0;
			for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::TraderRoutes; ++index)
			{
				const void* houseData = NC2Logic::MerchantData_GetHouseDataPtr(merchant, index);
				if (!NC2Logic::MerchantHouseData_IsValid(houseData)) continue;
				const auto* house = NC2Logic::CHouseArray::mStaticVars + Read<std::uint32_t>(houseData, NC2Layout::MerchantHouseOffset::HouseId) * NC2Logic::CHouseArray::ElementSize;
				NC2InGameGuiBase::BaseToolTextElement_SetCaptionString(Read<void*>(window, NC2Layout::SingleHumanWindowOffset::RouteLabelsUnsigned + row * NC2Layout::RecordSize::SingleHumanRouteControlsUnsigned), NC2Logic::StringAccess_GetHouseNamePtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType), 1));
				for (auto offset : { NC2Layout::SingleHumanWindowOffset::RouteSelectButtonsUnsigned, NC2Layout::SingleHumanWindowOffset::RouteActionButtonsUnsigned })
				{
					void* element = Read<void*>(window, offset + row * NC2Layout::RecordSize::SingleHumanRouteControlsUnsigned);
					Write(element, NC2Layout::GuiGfxButtonOffset::MessageParameter1, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
					Write(element, NC2Layout::GuiGfxButtonOffset::MessageParameter2, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
					show(offset + row * NC2Layout::RecordSize::SingleHumanRouteControlsUnsigned);
					if (offset == NC2Layout::SingleHumanWindowOffset::RouteSelectButtons) hide(NC2Layout::SingleHumanWindowOffset::RouteAssignButtonsUnsigned + row * NC2Layout::RecordSize::SingleHumanRouteControlsUnsigned);
				}
				if (++row == NC2Runtime::SingleHumanWindowLimits::TraderRoutes) break;
			}
			Write<std::uint8_t>(Read<void*>(window, NC2Layout::SingleHumanWindowOffset::TraderConfigButton), NC2Layout::GuiTextButtonOffset::DisabledFlag, row < NC2Runtime::SingleHumanWindowLimits::TraderRoutes);
		}
		void* AddGraphicButton(void* window, const SRectangle& rectangle, std::uint32_t graphic, std::uint32_t tooltip, std::uint32_t command)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiGfxButton, 1);
			void* element = allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &rectangle, graphic, HumanString(tooltip), command, window);
			NXBaseGui::BaseWindow_Element_Add(window, element);
			return element;
		}

		// Original function: FUN_004ae2a2; all native section children and desktop area clamping.
		void l_SubTabs_UpdateAll(void* window, bool force)
		{
			const auto checksum = l_SubTabs_GetEnabledChecksum(window);
			if (!force && Read<std::uint32_t>(window, NC2Layout::SingleHumanWindowOffset::TabStateChecksum) == checksum && Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::LayoutInvalidFlag) == 0) return;
			Write(window, NC2Layout::SingleHumanWindowOffset::TabStateChecksum, checksum);
			Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::LayoutInvalidFlag, 0);
			std::int32_t width = 0, height = 0;
			for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::Tabs; ++index)
			{
				if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags + index) == 0) continue;
				if (width < gSubTabs[index].width) width = gSubTabs[index].width;
				const auto overrideHeight = Read<std::int32_t>(window, NC2Layout::SingleHumanWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::SingleHumanTabHeightUnsigned);
				height += Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags + index) != 0 ? 16 : 16 + (overrideHeight == 0 ? gSubTabs[index].height : overrideHeight);
			}
			Write(window, NC2Layout::SingleHumanWindowOffset::ComputedWidth, width);
			Write(window, NC2Layout::SingleHumanWindowOffset::ComputedHeight, height);
			std::int32_t y = 0;
			for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::Tabs; ++index)
			{
				SRectangle rectangle(-1, -1, 0, 0);
				if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags + index) != 0)
				{
					const auto overrideHeight = Read<std::int32_t>(window, NC2Layout::SingleHumanWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::SingleHumanTabHeightUnsigned);
					rectangle = SRectangle(width - gSubTabs[index].width, y, gSubTabs[index].width, Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags + index) != 0 ? 16 : 16 + (overrideHeight == 0 ? gSubTabs[index].height : overrideHeight));
					y += rectangle.height;
				}
				std::memcpy(static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::TabRectangles + index * NC2Layout::RecordSize::SingleHumanTabRectangleUnsigned, &rectangle, sizeof(rectangle));
			}
			const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
			const SRectangle rectangle(NC2InGameGuiBase::GetDisplayWidth() - width, NC2InGameGuiBase::GetDisplayHeight() - height, width, height);
			NC2InGameGuiBase::GetDesktopPtr()->Window_ChangeAreaRectangle(static_cast<NXBaseGui::CBaseWindow*>(window), rectangle);
			NXBaseGui::BaseWindow_Element_DeleteAll(window);
			std::memset(static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::PreviewElement, 0, NC2Layout::RecordSize::SingleHumanChildControlPointers);
			Buttons(window).AllButtonsHaveBeenRemoved();
			for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::Tabs; ++index)
			{
				if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags + index) == 0) continue;
				const auto section = Section(window, index);
				const auto collapsed = Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags + index) != 0;
				void* toggle = AddGraphicButton(window, SRectangle(section.x + section.width - 19, section.y + 2, 16, 12), collapsed ? 19u : 20u, collapsed ? 90u : 91u, 10000);
				if (toggle != nullptr)
				{
					Write(toggle, NC2Layout::GuiGfxButtonOffset::MessageParameter1, index);
					Write<std::uint32_t>(toggle, NC2Layout::GuiGfxButtonOffset::MessageParameter2, 0);
				}
			}
			const auto basic = Section(window, 0);
			const auto collapsed = Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags) != 0;
			void* human = l_Misc_GetSelectedHumanPtr();
			if (!collapsed && human != nullptr)
			{
				const SRectangle previewRect(basic.x + 4, basic.y + 20, 60, 80);
				void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::WorldDisplayElement, 1);
				void* preview = allocation == nullptr ? nullptr : NC2InGameGuiWorldElement::WorldDisplayElement_Construct(allocation, &previewRect, false, false);
				Write(window, NC2Layout::SingleHumanWindowOffset::PreviewElement, preview);
				if (preview != nullptr)
				{
					Write<std::uint32_t>(preview, NC2Layout::WorldGuiElementOffset::FrameMode, 2);
					Write(preview, NC2Layout::WorldGuiElementOffset::LeftClickHandler, window);
					Write<std::uint32_t>(preview, NC2Layout::WorldGuiElementOffset::LeftClickCommand, 10001);
					Write(preview, NC2Layout::WorldGuiElementOffset::DoubleClickHandler, window);
					Write<std::uint32_t>(preview, NC2Layout::WorldGuiElementOffset::DoubleClickCommand, 10002);
					NXBaseGui::BaseWindow_Element_Add(window, preview);
					NC2E2::DE_ControllingPlayer_SetId(Read<void*>(preview, NC2Layout::WorldGuiElementOffset::Display), NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId());
					l_Misc_CenterWorldDisplay(window);
				}
				AddGraphicButton(window, SRectangle(basic.x + 6, basic.y + 22, 18, 16), 18, 100, 10003);
				AddGraphicButton(window, SRectangle(basic.x + 44, basic.y + 22, 18, 16), 28, 101, 10004);
				AddGraphicButton(window, SRectangle(basic.x + basic.width - 20, basic.y + basic.height - 20, 18, 18), 142, 102, 10005);
				const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId);
				const char* name = reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries) + id * NC2Layout::RecordSize::DynamicHumanUnsigned + NC2Layout::DynamicHumanOffset::Name;
				const SRectangle nameRect(basic.x + 68, basic.y + 20, basic.width - 72, 14);
				allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
				void* nameElement = allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructWithToolTip(allocation, nullptr, &nameRect, name, HumanString(103), 10006, window);
				if (nameElement != nullptr) Write<std::uint8_t>(nameElement, NC2Layout::GuiTextButtonOffset::DisabledFlag, NC2Logic::Human_IsJobHero(human) || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit7) != 0);
				Write(window, NC2Layout::SingleHumanWindowOffset::NameElement, nameElement);
				NXBaseGui::BaseWindow_Element_Add(window, nameElement);
			}
			if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceTabEnabled) != 0 && Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceTabCollapsed) == 0)
			{
				const auto experience = Section(window, 3);
				for (std::uint32_t page = 0; page < NC2Runtime::SingleHumanWindowLimits::ExperiencePages; ++page)
				{
					void* button = AddGraphicButton(window, SRectangle(experience.x + 8 + static_cast<std::int32_t>(page) * 22, experience.y + 24, 20, 18), gExperiencePageButtons[page].graphic, gExperiencePageButtons[page].tooltip, 10042);
					Write(button, NC2Layout::GuiGfxButtonOffset::MessageParameter1, page);
					Write<std::uint32_t>(button, NC2Layout::GuiGfxButtonOffset::MessageParameter2, 0);
					Write<std::uint8_t>(button, NC2Layout::GuiGfxButtonOffset::Selected, page == Read<std::uint32_t>(window, NC2Layout::SingleHumanWindowOffset::ExperiencePage));
					Write<std::uint8_t>(button, NC2Layout::GuiGfxButtonOffset::RectangularStyle, 1);
					Write<std::uint8_t>(button, NC2Layout::GuiGfxButtonOffset::ImmediateTooltip, 1);
				}
			}
			if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TraderTabEnabled) != 0 && Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TraderTabCollapsed) == 0 && l_Misc_GetSelectedMerchantDataPtr() != nullptr)
			{
				const auto trader = Section(window, 5);
				void* allocation;
				for (std::uint32_t row = 0; row < NC2Runtime::SingleHumanWindowLimits::TraderRoutes; ++row)
				{
					const auto y = trader.y + 24 + static_cast<std::int32_t>(row) * 24;
					Write(window, NC2Layout::SingleHumanWindowOffset::RouteSelectButtonsUnsigned + row * NC2Layout::RecordSize::SingleHumanRouteControlsUnsigned, AddGraphicButton(window, SRectangle(trader.x, y, 20, 18), 18, 117, 10007));
					Write(window, NC2Layout::SingleHumanWindowOffset::RouteAssignButtonsUnsigned + row * NC2Layout::RecordSize::SingleHumanRouteControlsUnsigned, AddGraphicButton(window, SRectangle(trader.x, y, 20, 30), 30, 118, 10008));
					Write(window, NC2Layout::SingleHumanWindowOffset::RouteActionButtonsUnsigned + row * NC2Layout::RecordSize::SingleHumanRouteControlsUnsigned, AddGraphicButton(window, SRectangle(trader.x + 20, y, 20, 30), 29, 119, 10009));
					allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextElement, 1);
					const SRectangle textRect(trader.x + 38, y, 154, 20);
					void* label = allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextElement_Construct(allocation, nullptr, &textRect, "", false, false);
					Write(window, NC2Layout::SingleHumanWindowOffset::RouteLabelsUnsigned + row * NC2Layout::RecordSize::SingleHumanRouteControlsUnsigned, label);
					Write<std::uint8_t>(label, NC2Layout::GuiTextElementOffset::SmallFontFlag, 1);
					NXBaseGui::BaseWindow_Element_Add(window, label);
				}
				allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
				const SRectangle config(trader.x + 8, trader.y + 76, 184, 20);
				void* button = allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_Construct(allocation, nullptr, &config, HumanString(110), 10010, window);
				Write(window, NC2Layout::SingleHumanWindowOffset::TraderConfigButton, button);
				NXBaseGui::BaseWindow_Element_Add(window, button);
				l_SubTabs_Trader_UpdateButtons(window);
			}
		}

		// The five native inlined bars in 004AC4E8..004AC96B share presentation only.
		void DrawBar(void* window, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t current, std::int32_t maximum, std::uint32_t icon, std::uint32_t label, NXBasics::CPalette* palette, bool hitpoints)
		{
			std::int32_t percent = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(current) * 100u) / maximum;
			if (hitpoints)
			{
				if (percent < 0) percent = 0;
				if (percent > 100) percent = 100;
			}
			NC2GuiToolsBase::DrawTool_DrawIcon(icon, bitmap, x + 6, y + 6);
			const auto width = Section(window, 0).width - 72;
			SRectangle bar(x + 16, y + 2, width - 16, 8);
			NC2GuiToolsBase::DrawTool_DrawFrame(bitmap, bar, 2);
			bar.Inflate(-2, -2);
			const auto fillPercent = static_cast<std::uint32_t>(percent) > 100u ? 100u : static_cast<std::uint32_t>(percent);
			bar.width = static_cast<std::int32_t>(static_cast<std::uint32_t>(bar.width) * fillPercent / 100u);
			auto colorIndex = static_cast<std::uint32_t>(percent) * 255u / 100u;
			if (colorIndex > 255u) colorIndex = 255u;
			bitmap->Draw_Box(bar, *reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(palette) + NC2Layout::PaletteOffset::Colors + colorIndex * NC2Layout::RecordSize::PaletteColorUnsigned));
			if (hitpoints) Tooltips(window).AddV(SRectangle(x, y, width, 12), "%s %d/%d", HumanString(label), current, maximum);
			else Tooltips(window).AddV(SRectangle(x, y, width, 12), "%s %d%%", HumanString(label), percent);
		}

		// Inlined equipment presentation in 004ADAAB..004AE0A7.
		void DrawEquipment(void* window, NXBasics::CBitmap* bitmap, const void* human)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::JobTabEnabled) == 0 || Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::JobTabCollapsed) != 0)
			{
				for (std::uint32_t command : {10029u, 10030u, 10032u, 10031u, 10033u, 10034u, 10035u, 10036u}) Buttons(window).RemoveButton(command, 0);
				return;
			}
			const auto section = Section(window, 4);
			const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
			const auto* color = NC2GuiToolsBase::DrawTool_GetFontColorRef();
			const auto x = section.x + 6;
			auto y = section.y + 20;
			const auto printLabel = [&] (std::uint32_t label)
			{
				const char* text = HumanString(label);
				NC2GuiToolsBase::DrawTool_PrintStringX(bitmap, x + 78 - NXBasics::Font_GetPixelWidth(resources.font08, text), y, resources.font08, color, text);
			};
			const auto printGood = [&] (std::uint32_t good, std::int32_t percent, bool condition, const char* name)
			{
				SelectionTool_FillRectangle(bitmap, SRectangle(x + 97, y - 2, 28, 20), 2);
				if (good == 0) return;
				char text[100];
				if (condition) NXBasics::Runtime_PrintStringN(text, sizeof(text), "{gfxgood:%d,1} (%d%%)", good, percent);
				else NXBasics::Runtime_PrintStringN(text, sizeof(text), "{gfxgood:%d,1}", good);
				text[sizeof(text) - 1] = '\0';
				NC2GuiToolsBase::DrawTool_PrintStringX(bitmap, x + 100, y, resources.font08, color, text);
				if (condition) NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s (%d%%)", name, percent);
				else NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s", name);
				text[sizeof(text) - 1] = '\0';
				Tooltips(window).Add(SRectangle(x + 100, y, 22, 16), text);
			};
			if (NC2Logic::Human_IsJobSoldier(human) || NC2Logic::Human_IsJobHero(human))
			{
				printLabel(60);
				const auto good = NC2Logic::Human_GetEquipmentType_Weapon(human);
				printGood(good, 0, false, good != 0 ? NC2Logic::StringAccess_GetGoodNamePtr(good, 1) : nullptr);
				if (!NC2Logic::Human_IsJobHero(human)) Buttons(window).UpdateGfxButton(10029, 0, x + 78, y, 30, HumanString(62));
				y += 20;
				printLabel(63);
				const auto armor = NC2Logic::Human_GetEquipmentType_Armor(human);
				printGood(armor, 0, false, armor != 0 ? NC2Logic::StringAccess_GetGoodNamePtr(armor, 1) : nullptr);
				if (!NC2Logic::Human_IsJobHero(human)) Buttons(window).UpdateGfxButton(10030, 0, x + 78, y, 30, HumanString(65));
				y += 20;
			}
			else
			{
				Buttons(window).RemoveButton(10029, 0);
				Buttons(window).RemoveButton(10030, 0);
			}
			if (NC2Logic::Human_IsAgeAdult(human) && NC2Logic::Human_IsSexMale(human) && !NC2Logic::Human_IsJobHero(human))
			{
				printLabel(66);
				const auto shoes = NC2Logic::Human_GetEquipmentType_Shoes(human);
				const auto shoePercent = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ShoeCondition) * 100u) / 10000;
				printGood(shoes, shoePercent, true, shoes != 0 ? NC2Logic::Human_GetEquipmentType_Shoes_GetGuiNamePtr(human) : nullptr);
				Buttons(window).UpdateGfxButton(10032, 0, x + 78, y, 30, HumanString(68));
				y += 20;
				if (!NC2Logic::Human_IsJobSoldier(human))
				{
					printLabel(69);
					const auto tool = NC2Logic::Human_GetEquipmentType_Tool(human);
					// Native 00438CAB uses separate, equal 10000 maxima for goods 31/32.
					const auto percent = tool == 31 || tool == 32 ? NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition) * 100u) / 10000 : 0;
					printGood(tool, percent, true, tool != 0 ? NC2Logic::Human_GetEquipmentType_Tool_GetGuiNamePtr(human) : nullptr);
					Buttons(window).UpdateGfxButton(10031, 0, x + 78, y, 30, HumanString(71));
					y += 20;
				}
				else Buttons(window).RemoveButton(10031, 0);
			}
			else
			{
				Buttons(window).RemoveButton(10032, 0);
				Buttons(window).RemoveButton(10031, 0);
			}
			if (!NC2Logic::Human_IsJobHero(human))
			{
				printLabel(72);
				for (std::uint32_t slot = 0; slot < 4; ++slot)
				{
					const auto itemX = x + 100 + static_cast<std::int32_t>(slot) * 42;
					SelectionTool_FillRectangle(bitmap, SRectangle(itemX - 3, y - 2, 25, 20), 2);
					const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::InventorySlotsSigned + slot * NC2Layout::RecordSize::HumanInventorySlot);
					if (good != 0 && Read<std::int32_t>(human, NC2Layout::HumanOffset::InventoryConditionsSigned + slot * NC2Layout::RecordSize::HumanInventorySlot) > 0)
					{
						char text[100];
						NXBasics::Runtime_PrintStringN(text, sizeof(text), "{gfxgood:%d,1}", good);
						text[sizeof(text) - 1] = '\0';
						NC2GuiToolsBase::DrawTool_PrintStringX(bitmap, itemX, y, resources.font08, color, text);
						const auto percent = NC2Logic::Human_Inventory_GetPercentageConditionOfItemInSlot(human, static_cast<std::int32_t>(slot));
						const auto* name = NC2Logic::StringAccess_GetGoodNamePtr(good, 1);
						NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s (%d%%)", name, percent);
						text[sizeof(text) - 1] = '\0';
						Tooltips(window).Add(SRectangle(itemX, y, 22, 16), text);
					}
					Buttons(window).UpdateGfxButton(10033 + slot, 0, itemX - 20, y, 30, HumanString(73));
				}
				y += 20;
			}
			l_SubTabs_OverwriteHeight(window, 4, y - section.y - 10);
		}

		// Inlined military presentation in 004AD173..004AD257.
		void DrawMilitary(void* window, NXBasics::CBitmap* bitmap, const void* human)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::MilitaryTabEnabled) == 0 || Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::MilitaryTabCollapsed) != 0) return;
			const auto section = Section(window, 1);
			auto y = section.y + 20;
			const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
			const auto* color = NC2GuiToolsBase::DrawTool_GetFontColorRef();
			if (NC2Logic::Human_IsJobSoldier(human) || NC2Logic::Human_IsJobHero(human))
			{
				char text[140];
				NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s: %s", HumanString(80), NC2Logic::StringAccess_GetMilitaryModeNamePtr(NC2Logic::Human_GetMilitaryMode(human)));
				text[sizeof(text) - 1] = '\0';
				bitmap->Text_Print(resources.font08, color, section.x + 8, y, text);
				y += 14;
				bitmap->Text_Print(resources.font08, color, section.x + 8, y, HumanString(Read<std::uint8_t>(human, NC2Layout::HumanOffset::MilitaryWorldRefreshmentFlag) != 0 ? 81 : 82));
				y += 14;
				NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s: %d%%", HumanString(83), NC2Logic::Human_Tool_Gui_GetExperienceForEquippedWeapon(human));
				text[sizeof(text) - 1] = '\0';
				bitmap->Text_Print(resources.font08, color, section.x + 8, y, text);
				y += 14;
			}
			l_SubTabs_OverwriteHeight(window, 1, y - section.y - 8);
		}

		// Inlined production presentation in 004AD257..004ADAAB; native counters/actions.
		// Native per-good help commands share the existing help window.
		void DrawProduction(void* window, NXBasics::CBitmap* bitmap, const void* human)
		{
			const auto removeCounterButtons = [&] (std::uint32_t good)
			{
				Buttons(window).RemoveButton(10039, good);
				Buttons(window).RemoveButton(10040, good | NXBasics::BitMask::Bit16);
				Buttons(window).RemoveButton(10040, good | NXBasics::BitMask::Bit17);
				Buttons(window).RemoveButton(10040, good | NXBasics::BitMask::Bits16Through17);
				Buttons(window).RemoveButton(10040, good | NXBasics::BitMask::Bit18);
				Buttons(window).RemoveButton(10041, good);
			};
			if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::WorkTabEnabled) == 0 || Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::WorkTabCollapsed) != 0)
			{
				for (auto command : { 10017u, 10018u, 10019u, 10020u, 10021u, 10037u, 10038u }) Buttons(window).RemoveButton(command, 0);
				return;
			}
			const auto section = Section(window, 2);
			const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
			const auto* color = NC2GuiToolsBase::DrawTool_GetFontColorRef();
			auto y = section.y + 20;
			NC2GuiToolsBase::DrawTool_DrawIcon(40, bitmap, section.x + 10, y + 6);
			auto textX = section.x + 24;
			const auto house = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);
			if (house != NXBasics::IntegerRange::UInt32Maximum)
			{
				Buttons(window).UpdateGfxButton(SwitchIds::SingleHumanWindowCommand::SelectWorkHouse, 0, textX, y, 18, HumanString(30));
				textX += 20;
			}
			else Buttons(window).RemoveButton(SwitchIds::SingleHumanWindowCommand::SelectWorkHouse, 0);
			NC2Logic::SUserCommandsForHumanHandler available;
			const std::int32_t actions[4]{ 16, 17, 20, 21 };
			for (std::uint32_t index = 0; index < 4; ++index)
			{
				const auto command = 10018u + index;
				if (NC2Logic::Logic_UserControl_GetHumanCommands(Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId), &available, false, false) && available.IsCommandAllowed(actions[index]))
				{
					Buttons(window).UpdateGfxButton(command, 0, textX, y, (index & 1u) != 0 ? 29 : 30, HumanString(31u + index));
					textX += 20;
				}
				else Buttons(window).RemoveButton(command, 0);
			}
			if (house != NXBasics::IntegerRange::UInt32Maximum)
			{
				const auto type = Read<std::uint32_t>(NC2Logic::CHouseArray::mStaticVars + house * NC2Logic::CHouseArray::ElementSize, NC2Layout::HouseOffset::LogicType);
				if (type != 0) bitmap->Text_Print(resources.font08, color, textX, y, NC2Logic::StringAccess_GetHouseNamePtr(type, 1));
			}
			y += 14;
			// Windows 004AD49C..004AD59E: marker placement (28) and work-area display (29).
			const bool hasCommands = NC2Logic::Logic_UserControl_GetHumanCommands(Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId), &available, false, false);
			if (hasCommands && (available.IsCommandAllowed(28) || available.IsCommandAllowed(29)))
			{
				NC2GuiToolsBase::DrawTool_DrawIcon(39, bitmap, section.x + 10, y + 6);
				auto markerX = section.x + 24;
				if (available.IsCommandAllowed(28))
				{
					Buttons(window).UpdateGfxButton(10037, 0, markerX, y, 30, HumanString(35));
					markerX += 20;
				}
				else Buttons(window).RemoveButton(10037, 0);
				if (available.IsCommandAllowed(29)) Buttons(window).UpdateGfxButton(10038, 0, markerX, y, 28, HumanString(36));
				else Buttons(window).RemoveButton(10038, 0);
				y += 14;
			}
			else
			{
				Buttons(window).RemoveButton(10037, 0);
				Buttons(window).RemoveButton(10038, 0);
			}
			bool first = true;
			for (std::uint32_t good = 1; good < 66; ++good)
			{
				const auto counter = NC2Logic::Human_Production_GetCounter(human, good);
				if (counter == 12 || counter == 13)
				{
					removeCounterButtons(good);
					continue;
				}
				if (first)
				{
					y += 8;
					first = false;
				}
				const char* name = NC2Logic::StringAccess_GetGoodNamePtr(good, 1);
				char text[160];
				if (counter == 14)
				{
					NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s [%s]", name, HumanString(house != NXBasics::IntegerRange::UInt32Maximum ? 40 : 41));
					text[sizeof(text) - 1] = '\0';
					bitmap->Text_Print(resources.font08, color, section.x + 11, y, text);
					y += 12;
					removeCounterButtons(good);
					continue;
				}
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == good) bitmap->Text_Print(resources.font08, color, section.x + 4, y, ">");
				if (counter == 11)
				{
					bitmap->Text_Print(resources.font08, color, section.x + 11, y, name);
					NXBasics::BobManager_PrintBob_UsingShadedAlpha(resources.windowBobManager, 180, bitmap, section.x + 15 + NXBasics::Font_GetPixelWidth(resources.font08, name), y - 2, resources.fontWhitePalette, 128);
				}
				else
				{
					NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s [%d]", name, counter);
					text[sizeof(text) - 1] = '\0';
					bitmap->Text_Print(resources.font08, color, section.x + 11, y, text);
				}
				const auto buttonY = y + 2;
				NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s %s", HumanString(50), name);
				text[sizeof(text) - 1] = '\0';
				Buttons(window).UpdateGfxButton(10039, good, section.x + 189, buttonY, 30, text);
				Buttons(window).SetButtonMiscCommandData(10039, good, good, 0);
				NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s %s", HumanString(52), name);
				text[sizeof(text) - 1] = '\0';
				Buttons(window).UpdateGfxButton(10040, good | NXBasics::BitMask::Bit17, section.x + 206, buttonY, 27, text);
				Buttons(window).SetButtonMiscCommandData(10040, good | NXBasics::BitMask::Bit17, good, NXBasics::IntegerRange::UInt32Maximum);
				NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s %s", HumanString(53), name);
				text[sizeof(text) - 1] = '\0';
				Buttons(window).UpdateGfxButton(10040, good | NXBasics::BitMask::Bits16Through17, section.x + 223, buttonY, 26, text);
				Buttons(window).SetButtonMiscCommandData(10040, good | NXBasics::BitMask::Bits16Through17, good, 1);
				NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s %s", HumanString(55), name);
				text[sizeof(text) - 1] = '\0';
				Buttons(window).UpdateGfxButton(10041, good, section.x + 241, buttonY, 143, text);
				Buttons(window).SetButtonMiscCommandData(10041, good, good, 0);
				y += 12;
			}
			const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
			const auto& enables = NC2Logic::gStaticTribeTypeData[tribe].jobEnables[Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType)];
			const auto* player = static_cast<const std::byte*>(NC2Logic::gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;
			bool firstLocked = true;
			for (std::uint32_t index = 0; index < enables.goodCount; ++index)
			{
				const auto good = static_cast<std::uint32_t>(enables.goods[index]);
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) != 0 || Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good) == 0 || NC2Logic::Human_DoesEducationAllowGoodProduction(human, good)) continue;
				const auto needed = NC2Logic::Human_GetExperienceNeededYetForGoodProduction_Value(human, good);
				if (needed <= 0) continue;
				if (firstLocked)
				{
					y += 8;
					firstLocked = false;
				}
				char text[180];
				NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s - %d %s", NC2Logic::StringAccess_GetGoodNamePtr(good, 1), needed, NC2Logic::StringAccess_GetJobExperienceNamePtr(NC2Logic::Human_GetExperienceNeededYetForGoodProduction_Type(human, good)));
				text[sizeof(text) - 1] = '\0';
				bitmap->Text_Print(resources.font08, reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(resources.fontWhitePalette) + NC2Layout::PaletteOffset::MissingProductionExperienceColor), section.x + 4, y, text);
				y += 12;
			}
			l_SubTabs_OverwriteHeight(window, 2, y - section.y - 10);
		}

		// Inlined experience presentation in 004AE104..004AE1AF.
		void DrawExperiences(void* window, NXBasics::CBitmap* bitmap, const void* human)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceTabEnabled) == 0 || Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceTabCollapsed) != 0) return;
			const auto section = Section(window, 3);
			const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
			auto y = section.y + 46;
			const auto page = Read<std::uint32_t>(window, NC2Layout::SingleHumanWindowOffset::ExperiencePage);
			for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::ExperienceJobsPerPage; ++index)
			{
				const auto job = Read<std::uint32_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceJobsUnsigned + (page * NC2Runtime::SingleHumanWindowLimits::ExperienceJobsPerPageUnsigned + index) * NC2Layout::RecordSize::SingleHumanExperienceJobIdUnsigned);
				if (job == 0) break;
				const auto amount = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperience + job * NC2Layout::RecordSize::HumanJobExperienceUnsigned);
				if (amount == 0) continue;
				char text[160];
				NXBasics::Runtime_PrintStringN(text, sizeof(text), "%s: %d", NC2Logic::StringAccess_GetJobExperienceNamePtr(job), amount);
				text[sizeof(text) - 1] = '\0';
				bitmap->Text_Print(resources.font08, NC2GuiToolsBase::DrawTool_GetFontColorRef(), section.x + 8, y, text);
				y += 12;
			}
			l_SubTabs_OverwriteHeight(window, 3, y - section.y - 8);
		}

		// Original function: FUN_004ac26d; complete six-section Windows presentation.
		void __fastcall SelectionSingleHumanWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* clip)
		{
			Tooltips(window).ClearAll();
			for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::Tabs; ++index)
			{
				if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags + index) == 0) continue;
				auto section = Section(window, index);
				section.x += clip->x;
				section.y += clip->y;
				const auto collapsed = Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags + index) != 0;
				const SRectangle title(section.x + (collapsed ? section.width - 200 : 0), section.y, collapsed ? 200 : section.width, 16);
				SelectionTool_FillRectangle(bitmap, title, 5);
				SelectionTool_PrintCenteredString(bitmap, title, true, HumanString(gSubTabs[index].title));
				if (!collapsed) SelectionTool_FillRectangle(bitmap, SRectangle(section.x, section.y + 16, section.width, section.height - 16), 1);
			}
			const void* human = l_Misc_GetSelectedHumanPtr();
			if (human == nullptr) return;
			const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
			const auto basic = Section(window, 0);
			if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags) != 0 && Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags) == 0)
			{
				char job[140];
				const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), type = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
				const char* jobName = NC2Logic::Human_IsJobHero(human) ? (tribe == 2 && type == 47 ? NC2GuiToolsBase::StringTool_GetOdin001StringPtr(31) : HumanString(120)) : NC2Logic::StringAccess_GetJobNamePtr(type, 1);
				// Original 00512584 has only read references into zero-initialized storage.
				if (tribe == 3 && type == 32 && !NC2Logic::Human_IsJobHero(human)) jobName = "";
				if (NC2Logic::Human_IsAgeAdult(human)) NXBasics::Runtime_PrintStringN(job, sizeof(job), "%s", jobName);
				else NXBasics::Runtime_PrintStringN(job, sizeof(job), "%s - %d %s", jobName, Read<std::int32_t>(human, NC2Layout::HumanOffset::AgeCounter) / 270, HumanString(10));
				job[139] = '\0';
				bitmap->Text_Print(resources.font08, reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(resources.fontWhitePalette) + NC2Layout::PaletteOffset::LastColor), basic.x + 68 + (basic.width - 72 - NXBasics::Font_GetPixelWidth(resources.font08, job)) / 2, basic.y + 34, job);
				DrawBar(window, bitmap, basic.x + 68, basic.y + 48, Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints), static_cast<std::int32_t>(NC2Logic::Human_GetHumanAttributeMaximum_Hitpoints(human)), 34, 11, resources.hitpointsBarPalette, true);
				// Windows helper 00463AD2: children and adult behavior-flag bit 0 omit needs.
				if (NC2Logic::Human_IsAgeAdult(human) && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 1u) == 0)
				{
					DrawBar(window, bitmap, basic.x + 68, basic.y + 59, Read<std::int32_t>(human, NC2Layout::HumanOffset::Food), 10000, 38, 12, resources.standardBarPalette, false);
					DrawBar(window, bitmap, basic.x + 68, basic.y + 70, Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina), 10000, 35, 13, resources.standardBarPalette, false);
					DrawBar(window, bitmap, basic.x + 68, basic.y + 81, Read<std::int32_t>(human, NC2Layout::HumanOffset::Social), 10000, 36, 14, resources.standardBarPalette, false);
					DrawBar(window, bitmap, basic.x + 68, basic.y + 92, Read<std::int32_t>(human, NC2Layout::HumanOffset::Religion), 10000, 37, 15, NC2Logic::Human_IsJobNeedingReligion(human) ? resources.standardBarPalette : resources.disabledBarPalette, false);
				}
				std::int32_t rowY = basic.y + (NC2Logic::Human_IsAgeAdult(human) && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 1u) == 0 ? 103 : 100);
				NC2Logic::SUserCommandsForHumanHandler available;
				const auto allowed = [&] (std::int32_t action)
				{
					return NC2Logic::Logic_UserControl_GetHumanCommands(Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId), &available, false, false) && available.IsCommandAllowed(action);
				};
				const auto relative = [&] (std::size_t field, std::uint32_t command, std::uint32_t label, std::uint32_t icon)
				{
					const auto id = Read<std::uint32_t>(human, field);
					if (id == NXBasics::IntegerRange::UInt32Maximum)
					{
						Buttons(window).RemoveButton(command, 0);
						return;
					}
					NC2GuiToolsBase::DrawTool_DrawIcon(icon, bitmap, basic.x + 10, rowY + 6);
					bitmap->Text_Print(resources.font08, NC2GuiToolsBase::DrawTool_GetFontColorRef(), basic.x + 44, rowY, reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries) + id * NC2Layout::RecordSize::DynamicHumanUnsigned + NC2Layout::DynamicHumanOffset::Name);
					Buttons(window).UpdateGfxButton(command, 0, basic.x + 24, rowY, 18, HumanString(label));
					rowY += 14;
				};
				relative(NC2Layout::HumanOffset::SpouseId, 10022, 20, 33);
				if (allowed(7))
				{
					NC2GuiToolsBase::DrawTool_DrawIcon(33, bitmap, basic.x + 10, rowY + 6);
					Buttons(window).UpdateGfxButton(10026, 0, basic.x + 24, rowY, 30, HumanString(21));
					rowY += 14;
				}
				else Buttons(window).RemoveButton(10026, 0);
				relative(NC2Layout::HumanOffset::ChildId, 10025, 22, 31);
				const bool canGetPregnant = NC2Logic::Logic_UserControl_GetHumanCommands(Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId), &available, false, false) && (available.IsCommandAllowed(8) || available.IsCommandAllowed(9));
				if (canGetPregnant)
				{
					NC2GuiToolsBase::DrawTool_DrawIcon(31, bitmap, basic.x + 10, rowY + 6);
					auto x = basic.x + 24;
					if (available.IsCommandAllowed(9))
					{
						Buttons(window).UpdateGfxButton(10028, 0, x, rowY, 30, HumanString(24));
						x += 20;
					}
					else Buttons(window).RemoveButton(10028, 0);
					if (available.IsCommandAllowed(8)) Buttons(window).UpdateGfxButton(10027, 0, x, rowY, 30, HumanString(23));
					else Buttons(window).RemoveButton(10027, 0);
					rowY += 14;
				}
				else
				{
					Buttons(window).RemoveButton(10027, 0);
					Buttons(window).RemoveButton(10028, 0);
				}
				relative(NC2Layout::HumanOffset::MotherId, 10023, 25, 31);
				relative(NC2Layout::HumanOffset::FatherId, 10024, 26, 31);
				if (!NC2Logic::Human_IsJobHero(human))
				{
					NC2GuiToolsBase::DrawTool_DrawIcon(32, bitmap, basic.x + 10, rowY + 6);
					auto x = basic.x + 24;
					const auto house = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId);
					std::uint32_t type = 0;
					if (house != NXBasics::IntegerRange::UInt32Maximum)
					{
						Buttons(window).UpdateGfxButton(10011, 0, x, rowY, 18, HumanString(27));
						x += 20;
						type = Read<std::uint32_t>(NC2Logic::CHouseArray::mStaticVars + house * NC2Logic::CHouseArray::ElementSize, NC2Layout::HouseOffset::LogicType);
					}
					else Buttons(window).RemoveButton(10011, 0);
					if (allowed(10))
					{
						Buttons(window).UpdateGfxButton(10012, 0, x, rowY, 30, HumanString(28));
						x += 20;
					}
					else Buttons(window).RemoveButton(10012, 0);
					if (allowed(11))
					{
						Buttons(window).UpdateGfxButton(10013, 0, x, rowY, 29, HumanString(29));
						x += 20;
					}
					else Buttons(window).RemoveButton(10013, 0);
					if (type != 0) bitmap->Text_Print(resources.font08, NC2GuiToolsBase::DrawTool_GetFontColorRef(), x, rowY, NC2Logic::StringAccess_GetHouseNamePtr(type, 1));
					rowY += 12;
				}
				else for (auto command : { 10011u, 10012u, 10013u }) Buttons(window).RemoveButton(command, 0);
				const bool hasCommands = NC2Logic::Logic_UserControl_GetHumanCommands(Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId), &available, false, false);
				const bool attached = NC2Logic::Human_IsAttachedToVehicle(human);
				if (hasCommands && (attached || available.IsCommandAllowed(12) || available.IsCommandAllowed(13)))
				{
					rowY += 6;
					NC2GuiToolsBase::DrawTool_DrawIcon(144, bitmap, basic.x + 10, rowY + 6);
					auto x = basic.x + 24;
					if (attached)
					{
						Buttons(window).UpdateGfxButton(10014, 0, x, rowY, 18, HumanString(37));
						x += 20;
					}
					else Buttons(window).RemoveButton(10014, 0);
					if (hasCommands && available.IsCommandAllowed(12))
					{
						Buttons(window).UpdateGfxButton(10015, 0, x, rowY, 30, HumanString(38));
						x += 20;
					}
					else Buttons(window).RemoveButton(10015, 0);
					if (hasCommands && available.IsCommandAllowed(13))
					{
						Buttons(window).UpdateGfxButton(10016, 0, x, rowY, 29, HumanString(39));
						x += 20;
					}
					else Buttons(window).RemoveButton(10016, 0);
					if (attached)
					{
						const auto vehicle = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId);
						bitmap->Text_Print(resources.font08, NC2GuiToolsBase::DrawTool_GetFontColorRef(), x, rowY, NC2Logic::StringAccess_GetVehicleTypeNamePtr(Read<std::uint32_t>(NC2Logic::CVehicleArray::mStaticVars + vehicle * NC2Logic::CVehicleArray::ElementSize, NC2Layout::VehicleOffset::LogicType)));
					}
					rowY += 12;
				}
				else if (hasCommands) for (auto command : { 10014u, 10015u, 10016u }) Buttons(window).RemoveButton(command, 0);
				l_SubTabs_OverwriteHeight(window, 0, rowY - basic.y - 10);
			}
			else for (std::uint32_t command : {10022u, 10023u, 10024u, 10025u, 10026u, 10027u, 10028u, 10011u, 10012u, 10013u, 10014u, 10015u, 10016u}) Buttons(window).RemoveButton(command, 0);
			DrawMilitary(window, bitmap, human);
			DrawProduction(window, bitmap, human);
			DrawEquipment(window, bitmap, human);
			DrawExperiences(window, bitmap, human);

		}

		// Original function: FUN_004aba34; complete native command dispatcher and frame refresh.
		bool __fastcall SelectionSingleHumanWindow_MessageHandle(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3)
		{
			const void* selectedHuman = l_Misc_GetSelectedHumanPtr();
			if (message == 0x1000) return command == 2001;
			if (message != 0x800) return false;
			if (command == 10000 && Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags + param2) != 0)
			{
				Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags + param2, Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags + param2) == 0);
				l_SubTabs_UpdateAll(window, false);
			}
			else if (command == 10001 || command == 10003)
			{
				if (const void* human = selectedHuman)
				{
					void* world = Read<void*>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement);
					NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(world, NC2Layout::WorldGuiElementOffset::Display), reinterpret_cast<const std::uint32_t*>(NC2Logic::Human_GetMapPositionRef(human)));
					NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(world);
				}
			}
			else if (command == 10002) NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Selection_ActionButtons_BringUp();
			else if (command == 10005)
			{
				if (const void* human = selectedHuman)
				{
					void* world = Read<void*>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement);
					const auto id = Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId);
					if (NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_GetHumanId(world) == id) NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(world);
					else NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_FollowHuman(world, id);
				}
			}
			else if (command == 10004) NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->ObservationWindow_CreateNewOne();
			else if (command == 10006) NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->MiscWindows_HumanChangeNameWindow_Open();
			else if (command >= 10022 && command <= 10025)
			{
				if (const void* human = selectedHuman)
				{
					const std::size_t fields[4] = { NC2Layout::HumanOffset::SpouseId, NC2Layout::HumanOffset::MotherId, NC2Layout::HumanOffset::FatherId, NC2Layout::HumanOffset::ChildId };
					const auto id = Read<std::uint32_t>(human, fields[command - SwitchIds::SingleHumanWindowCommand::SelectSpouse]);
					if (id != NXBasics::IntegerRange::UInt32Maximum) NC2InGameGuiInputManager::Selection_SelectSingleHuman(NC2InGameGuiInputManager::gInputManager, static_cast<std::int32_t>(id));
				}
			}
			else if (command == SwitchIds::SingleHumanWindowCommand::SelectWorkHouse || command == 10011)
			{
				if (const void* human = selectedHuman)
				{
					const auto id = Read<std::uint32_t>(human, command == SwitchIds::SingleHumanWindowCommand::SelectWorkHouse ? NC2Layout::HumanOffset::WorkAttachmentHouseId : NC2Layout::HumanOffset::HomeHouseId);
					if (id != NXBasics::IntegerRange::UInt32Maximum) NC2InGameGuiInputManager::Selection_SelectHouse(NC2InGameGuiInputManager::gInputManager, static_cast<std::int32_t>(id));
				}
			}
			else if (command == 10012 || command == 10018 || command == 10020)
			{
				if (selectedHuman != nullptr) NC2InGameGuiInputManager::InputMode_Set(NC2InGameGuiInputManager::gInputManager, command == 10012 ? 4 : command == 10018 ? 5 : 7);
			}
			else if (command == 10015)
			{
				NC2InGameGuiInputManager::InputMode_Set(NC2InGameGuiInputManager::gInputManager, 27);
			}
			else if (command == 10013 || command == 10016 || command == 10019 || command == 10021 || (command >= 10026 && command <= 10028))
			{
				if (const void* human = selectedHuman)
				{
					if (command != 10016 || NC2Logic::Human_IsAttachedToVehicle(human))
					{
						std::byte networkCommand[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
						const auto opcode = command == 10013 ? 27 : command == 10016 ? 39 : command == 10019 ? 29 : command == 10021 ? 31 : command - 10012;
						Write<std::uint8_t>(networkCommand, NC2Layout::UserNetworkCommandOffset::Opcode, static_cast<std::uint8_t>(opcode));
						Write(networkCommand, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
						NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, networkCommand);
						NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId));
					}
				}
			}
			else if (command == 10014)
			{
				if (const void* human = selectedHuman) if (NC2Logic::Human_IsAttachedToVehicle(human)) NC2InGameGuiInputManager::Selection_SelectSingleVehicle(NC2InGameGuiInputManager::gInputManager, static_cast<std::int32_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId)));
			}
			else if (command == 10007)
			{
				const auto* house = NC2Logic::CHouseArray::mStaticVars + param2 * NC2Logic::CHouseArray::ElementSize;
				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == param3) NC2InGameGuiInputManager::Selection_SelectHouse(NC2InGameGuiInputManager::gInputManager, static_cast<std::int32_t>(param2));
			}
			else if (command == 10008)
			{
				NC2InGameGuiInputManager::InputMode_Set(NC2InGameGuiInputManager::gInputManager, 8);
			}
			else if (command == 10009)
			{
				const auto* house = NC2Logic::CHouseArray::mStaticVars + param2 * NC2Logic::CHouseArray::ElementSize;
				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == param3)
					if (const void* human = selectedHuman)
					{
						std::byte networkCommand[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
						Write<std::uint8_t>(networkCommand, NC2Layout::UserNetworkCommandOffset::Opcode, 37);
						Write(networkCommand, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
						Write(networkCommand, NC2Layout::UserNetworkCommandOffset::Parameter1, param2);
						Write(networkCommand, NC2Layout::UserNetworkCommandOffset::Parameter2, param3);
						NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, networkCommand);
					}
			}
			else if (command == 10010)
			{
				if (Read<void*>(window, NC2Layout::SingleHumanWindowOffset::TraderConfigWindow) == nullptr)
				{
					void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::TraderConfigWindow, 1);
					void* popup = allocation == nullptr ? nullptr : NC2InGameGuiSelectedAction::SelectedTraderConfigWindow_Construct(allocation, window, 10043);
					Write(window, NC2Layout::SingleHumanWindowOffset::TraderConfigWindow, popup);
					NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(popup), false);
				}
			}
			else if (command == 10043)
			{
				Write<void*>(window, NC2Layout::SingleHumanWindowOffset::TraderConfigWindow, nullptr);
			}
			else if (command == 10037 || command == 10038) NC2InGameGuiInputManager::InputMode_Set(NC2InGameGuiInputManager::gInputManager, command == 10037 ? 2 : 3);
			else if (command == 10039 || command == 10040)
			{
				if (const void* human = selectedHuman)
				{
					std::byte networkCommand[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
					Write<std::uint8_t>(networkCommand, NC2Layout::UserNetworkCommandOffset::Opcode, command == 10039 ? 0x16 : 0x17);
					Write(networkCommand, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
					Write(networkCommand, NC2Layout::UserNetworkCommandOffset::Parameter1, param2);
					if (command == 10040)
					{
						auto amount = param3;
						if ((Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags) & 2u) != 0) amount *= 11u;
						if (NC2Logic::Human_Production_GetCounter(human, param2) == 0 && NXBasics::Runtime_TimeSigned32(amount) < 0) amount = 11;
						if (NC2Logic::Human_Production_GetCounter(human, param2) == 11 && NXBasics::Runtime_TimeSigned32(amount) > 0) amount = 0u - 11u;
						Write(networkCommand, NC2Layout::UserNetworkCommandOffset::Parameter2, amount);
					}
					NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, networkCommand);
					NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, static_cast<std::int32_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)));
				}
			}
			else if (command >= 10029 && command <= 10036)
			{
				if (selectedHuman != nullptr)
				{
					const auto type = command <= 10032 ? command - 10028 : 5u;
					const auto slot = command <= 10032 ? -1 : static_cast<std::int32_t>(command - 10033);
					NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->MiscWindows_HumanChangeEquippedGoodsWindow_Open(type, slot);
				}
			}
			else if (command == 10041) NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->MiscWindows_HelpWindow_Open_GoodInfo(param2);
			else if (command == 10042)
			{
				Write(window, NC2Layout::SingleHumanWindowOffset::ExperiencePage, param2);
				{
					NXBasics::CBitmap scratch(10, 10, 1);
					const SRectangle bounds(0, 0, 10, 10);
					using TDraw = void(__thiscall*)(void*, NXBasics::CBitmap*, const SRectangle*);
					Read<TDraw*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[3](window, &scratch, &bounds);
				}
				l_SubTabs_UpdateAll(window, true);
			}
			else if (command == 2030)
			{
				const void* human = selectedHuman;
				const void* nameElement = Read<void*>(window, NC2Layout::SingleHumanWindowOffset::NameElement);
				if (human != nullptr && nameElement != nullptr)
				{
					const auto* name = reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) * NC2Layout::RecordSize::DynamicHumanUnsigned + NC2Layout::DynamicHumanOffset::Name;
					if (std::strcmp(name, Read<const char*>(nameElement, NC2Layout::GuiTextButtonOffset::Caption)) != 0) l_SubTabs_UpdateAll(window, true);
				}
				if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceTabEnabled) != 0 && Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceTabCollapsed) == 0 && Read<std::uint32_t>(window, NC2Layout::SingleHumanWindowOffset::ExperiencePage) == 0)
				{
					SSelectionSingleHumanWindowMostExperiencesData most;
					l_SubTabs_Experiences_GetMostExperiences(&most);
					const auto count = most.count < NC2Runtime::SingleHumanWindowLimits::ExperienceJobsPerPage ? most.count : NC2Runtime::SingleHumanWindowLimits::ExperienceJobsPerPageUnsigned;
					for (std::uint32_t index = 0; index < count; ++index)
					{
						if (Read<std::uint32_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceJobsUnsigned + index * NC2Layout::RecordSize::SingleHumanExperienceJobIdUnsigned) == most.entries[index].job) continue;
						l_SubTabs_Experiences_Init(window);
						l_SubTabs_UpdateAll(window, true);
						break;
					}
				}
				l_Misc_CenterWorldDisplay(window);
				if (human != nullptr)
				{
					Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::WorkTabEnabled, NC2Logic::Human_IsSexMale(human) && NC2Logic::Human_IsAgeAdult(human) && !NC2Logic::Human_IsJobHero(human) && !NC2Logic::Human_IsJobScout(human) && !NC2Logic::Human_IsJobTrader(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != 6);
					// Native 00436A97 has no established semantic name yet.
					Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::JobTabEnabled, NC2Logic::Human_UserControl_CanChooseJob(human) && !NC2Logic::Human_IsJobHero(human));
					Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TraderTabEnabled, NC2Logic::Human_IsJobTrader(human));
				}
				else
				{
					Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::WorkTabEnabled, 0);
					Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::JobTabEnabled, 0);
					Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TraderTabEnabled, 0);
				}
				if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TraderTabEnabled) != 0 && Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TraderTabCollapsed) == 0) l_SubTabs_Trader_UpdateButtons(window);
				l_SubTabs_UpdateAll(window, false);
			}
			return true;
		}
		// Original function: FUN_004ae1b6.
		bool __fastcall SelectionSingleHumanWindow_IsHit(void* window, void*, const std::int32_t* point)
		{
			for (std::uint32_t index = 0; index < NC2Runtime::SingleHumanWindowLimits::Tabs; ++index)
			{
				if (Read<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags + index) == 0) continue;
				SRectangle rectangle(0, 0, 0, 0);
				std::memcpy(&rectangle, static_cast<const std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::TabRectangles + index * NC2Layout::RecordSize::SingleHumanTabRectangleUnsigned, sizeof(rectangle));
				const auto right = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(rectangle.x) + static_cast<std::uint32_t>(rectangle.width) - 1u);
				const auto bottom = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.height) - 1u);
				if (point[0] >= rectangle.x && point[0] <= right && point[1] >= rectangle.y && point[1] <= bottom) return true;
			}
			return false;
		}
		// Original function: FUN_004ae1ed.
		bool __fastcall SelectionSingleHumanWindow_ToolTipAvailable(void* window, void*, const char** output)
		{
			const auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr;
			std::int32_t point[2] = { Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::X), Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::Y) };
			NC2InGameGuiBase::BaseElement_GlobalToLocalCoordinates(window, point);
			const char* tooltip = Tooltips(window).Get(point);
			if (tooltip == nullptr) return false;
			*output = tooltip;
			return true;
		}
		// Original function: FUN_004bf2f4.
		bool __fastcall SelectionSingleHumanWindow_True(void*)
		{
			return true;
		}
		extern const std::uintptr_t gSingleHumanWindowVirtualTable[19];
		// Original function: FUN_004ab9b7; close the owned popup before unregistering.
		void SelectionSingleHumanWindow_Destruct(void* window)
		{
			Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSingleHumanWindowVirtualTable + 0);
			auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
			if (void* popup = Read<void*>(window, NC2Layout::SingleHumanWindowOffset::TraderConfigWindow)) NXBaseGui::CDesktop::sTheObjectPtr->Window_Close(static_cast<NXBaseGui::CBaseWindow*>(popup));
			NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(manager->GetDesktop());
			manager->FrameCall_UnRegister(window);
			gSelectionSingleHumanWindowStatics[0] = std::byte{ 1 };
			std::memcpy(gSelectionSingleHumanWindowStatics + 1, static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags, 6);
			Tooltips(window).~CStaticToolTipHandler();
			Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NC2InGameGuiBase::gBaseToolWindowVirtualTable + 0);
			NXBaseGui::BaseWindow_Destruct(window);
		}
		// Original function: FUN_004ab989.
		void* __fastcall SelectionSingleHumanWindow_Delete(void* window, void*, std::uint32_t flags)
		{
			SelectionSingleHumanWindow_Destruct(window);
			if ((flags & 1u) != 0) NXBasics::Runtime_Free(window);
			return window;
		}
		// Complete Windows virtual table 004F6A90..004F6ADB.
		const std::uintptr_t gSingleHumanWindowVirtualTable[19] = {
			reinterpret_cast<std::uintptr_t>(&SelectionSingleHumanWindow_Delete),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
			reinterpret_cast<std::uintptr_t>(&SelectionSingleHumanWindow_MessageHandle),
			reinterpret_cast<std::uintptr_t>(&SelectionSingleHumanWindow_Draw),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_Activate),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_DeActivate),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_ButtonPressed),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_ButtonReleased),
			reinterpret_cast<std::uintptr_t>(&SelectionSingleHumanWindow_IsHit),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_GetRealActiveElementPtr),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
			reinterpret_cast<std::uintptr_t>(&SelectionSingleHumanWindow_ToolTipAvailable),
			reinterpret_cast<std::uintptr_t>(&SelectionSingleHumanWindow_True),
			reinterpret_cast<std::uintptr_t>(&SelectionSingleHumanWindow_True),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_UnresolvedVirtual40),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
			reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_DrawOverlays)
		};
		static_assert(sizeof(gSingleHumanWindowVirtualTable) == 76);
	}

	// Original function: FUN_004ab9a5; the receiver is the embedded SVars at window +0x70.
	static void* SelectionSingleHumanWindow_Vars_Construct(void* vars)
	{
		::new(static_cast<std::byte*>(vars) + NC2Layout::SingleHumanWindowVarsOffset::Tooltips) NC2InGameGuiBase::CStaticToolTipHandler;
		return vars;
	}
	// Original function: FUN_004ab7f1; native embedded constructors precede the storage reset.
	void* SelectionSingleHumanWindow_Construct(void* window)
	{
		const SRectangle rectangle(0, 0, 1, 1);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		SelectionSingleHumanWindow_Vars_Construct(static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::TabEnabledFlags);
		::new(static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::DynamicButtons) NC2InGameGuiBase::CDynamicSubButtonsHandler;
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSingleHumanWindowVirtualTable + 0);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::TabEnabledFlags, 0, NC2Layout::RecordSize::SingleHumanWindowVars);
		Buttons(window).SetWindow(window);
		Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TabEnabledFlags, 1);
		if (const void* human = l_Misc_GetSelectedHumanPtr())
		{
			Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::MilitaryTabEnabled, NC2Logic::Human_IsSexMale(human) && NC2Logic::Human_IsAgeAdult(human) && (NC2Logic::Human_IsJobHero(human) || NC2Logic::Human_IsJobSoldier(human)));
			Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::WorkTabEnabled, NC2Logic::Human_IsSexMale(human) && NC2Logic::Human_IsAgeAdult(human) && !NC2Logic::Human_IsJobHero(human) && !NC2Logic::Human_IsJobScout(human) && !NC2Logic::Human_IsJobTrader(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != 6);
			Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::ExperienceTabEnabled, NC2Logic::Human_IsSexMale(human) && NC2Logic::Human_IsAgeAdult(human) && !NC2Logic::Human_IsJobHero(human));
			// Native 00436A97: inspect the enable byte and the Odin exception.
			Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::JobTabEnabled, NC2Logic::Human_UserControl_CanChooseJob(human) && !NC2Logic::Human_IsJobHero(human));
			Write<std::uint8_t>(window, NC2Layout::SingleHumanWindowOffset::TraderTabEnabled, NC2Logic::Human_IsJobTrader(human));
		}
		if (gSelectionSingleHumanWindowStatics[0] != std::byte{ 0 }) std::memcpy(static_cast<std::byte*>(window) + NC2Layout::SingleHumanWindowOffset::TabCollapsedFlags, gSelectionSingleHumanWindowStatics + 1, 6);
		l_SubTabs_Experiences_Init(window);
		{
			NXBasics::CBitmap scratch(10, 10, 1);
			const SRectangle bounds(0, 0, 10, 10);
			SelectionSingleHumanWindow_Draw(window, nullptr, &scratch, &bounds);
		}
		l_SubTabs_UpdateAll(window, true);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_Register(window);
		return window;
	}
}

namespace NC2InGameGuiSelection
{
	// Original function: FUN_004be4fb; complete selector region 004BE610..004BE648.
	void Tool_LandscapeType_GetTooltipStringPtr(std::int32_t graphicId, std::int32_t amount, char* output, bool* isGood, bool* isResource)
	{
		output[0] = 0;
		if (isGood != nullptr) *isGood = false;
		if (isResource != nullptr) *isResource = false;
		const auto type = NC2Lifetime::Read<std::int32_t>(NC2E2::gStaticLandscapeEntries, static_cast<std::size_t>(static_cast<std::uint32_t>(graphicId)) * NC2Layout::RecordSize::StaticEngineLandscapeUnsigned + NC2Layout::EngineLandscapeTypeOffset::LogicTypeUnsigned);
		const auto good = NC2Logic::gStaticDependencyGoodByLandscape[type];
		if (good != 0)
		{
			const auto* name = NC2Logic::StringAccess_GetGoodNamePtr(good, amount);
			NXBasics::Runtime_PrintString(output, "%d %s", amount, name);
			if (isGood != nullptr) *isGood = true;
			return;
		}
		std::uint32_t stringId = 0;
		constexpr std::uint8_t strings[25]{ 100,0,0,0,0,101,101,101,102,102,0,103,103,0,104,104,0,105,105,0,0,0,0,106,106 };
		if (type >= 4 && type <= 28) stringId = strings[type - 4];
		else if (type == 33 || type == 34) stringId = 107;
		else if (type == 36) stringId = 108;
		else if (type == 79 || type == 80) stringId = 109;
		else if (type == 82) stringId = 110;
		else if (type == 83 || type == 84) stringId = 111;
		else if (type == 85) stringId = 112;
		else if (type == 86) stringId = 113;
		if (stringId != 0) std::strcpy(output, NC2GuiToolsBase::StringTool_GetMiscMenuStringPtr(stringId));
		if (isResource != nullptr && (stringId == 100 || (stringId >= 102 && stringId <= 105))) *isResource = true;
	}
}

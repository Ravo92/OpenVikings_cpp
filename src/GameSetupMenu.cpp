#include "GameSetupMenu.h"
#include "GameSetupSettings.h"
#include "SkirmishMenu.h"
#include "NetworkLobbySettings.h"
#include "C2EngineDisplayAccess.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StringAccess.h"
#include "GameProfile.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "MainMenuGuiManager.h"
#include "MainMenuGuiManager_Access.h"
#include "NetworkShutdown.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace NRemaster::GameSetup::Menu
{
	using namespace NC2Lifetime;
	using NC2MainMenuGuiManager::CGuiManager;

	namespace
	{
		std::uint32_t page = 0;
		enum class EDropdown
		{
			Tribe,
			Control
		};
		EDropdown dropdownKind = EDropdown::Tribe;
		std::uint32_t dropdownSlot = UINT32_MAX, dropdownHighlight = 0;
		constexpr std::array<std::uint32_t, 6> TribeChoices{ 0, 1, 2, 3, 4, 7 };
		constexpr std::array<std::uint32_t, 3> ControlChoices{ 1, 2, 0 };

		std::uint32_t ChoiceCount()
		{

			return dropdownKind == EDropdown::Control ? 3u : NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2 ? 5u
			                                                                                                               : 6u;
		}

		std::uint32_t ChoiceValue(std::uint32_t index)
		{

			return dropdownKind == EDropdown::Control ? ControlChoices[index] : TribeChoices[index];
		}

		bool CanConfigure(const CGuiManager& gui)
		{

			return IsLocal(gui) || NNetwork::Network_IsHost();
		}

		const char* ControlName(std::uint32_t kind)
		{

			return kind == 1 ? MenuText("Human", "Mensch") : kind == 2 ? MenuText("Computer", "Computer")
			                                                           : MenuText("Off", "Aus");
		}

		bool ChoiceEnabled(const CGuiManager& gui, std::uint32_t index)
		{
			if (!CanConfigure(gui) || dropdownSlot >= 20 || index >= ChoiceCount())
				return false;

			if (dropdownKind == EDropdown::Control)
				return (Allowed(gui, dropdownSlot) & (1u << ChoiceValue(index))) != 0;

			return CanChooseTribe(gui, dropdownSlot);
		}

		const char* TribeName(std::uint32_t tribe)
		{
			const auto* name = tribe > 0 && tribe <= 7 ? NC2Logic::StringAccess_GetTribeNamePtr(tribe, 0) : nullptr;

			return name != nullptr ? name : MenuText("Map default", "Kartenvorgabe");
		}

		void __fastcall DropdownField_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
		{
			// Retain the native button background/frame; inset the caption within the text area.
			const bool disabled = Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag) != 0;
			const bool open = Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag) != 0;
			const auto mode = (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) != 0 && !disabled ? 4 : open ? 3
			                                                                                                                                                : 2;
			NC2GuiToolsBase::DrawTool_FillArea(element, bitmap, rectangle, mode);
			NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, mode);
			const auto width = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width), height = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height);
			const auto* palette = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette;
			auto color = Read<NXBasics::SColorRGB>(palette, NC2Layout::PaletteOffset::LastColor);
			const auto& fonts = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
			auto* font = Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SmallFontFlag) != 0 ? fonts.font08 : fonts.font10;
			const auto fontHeight = Read<std::int32_t>(font, NC2Layout::FontOffset::Height);
			bitmap->Text_Print(font, &color, 12 - rectangle->x, (height - fontHeight) / 2 - rectangle->y - 3, Read<const char*>(element, NC2Layout::GuiTextButtonOffset::Caption));
			auto separator = color;
			separator.red /= 3;
			separator.green /= 3;
			separator.blue /= 3;

			for (std::int32_t y = 3; y < height - 3; ++y)
			{
				bitmap->Draw_SetPixel(width - 23 - rectangle->x, y - rectangle->y, separator);
			}

			for (std::int32_t row = 0; row < 4; ++row)
			{
				for (std::int32_t x = row - 3; x <= 3 - row; ++x)
				{
					bitmap->Draw_SetPixel(width - 12 + x - rectangle->x, height / 2 + (open ? 2 - row : row - 2) - rectangle->y, color);
				}
			}

			if (disabled)
			{
				auto area = bitmap->GetRectangle();
				area.Inflate(-1, -1);
				bitmap->Tool_Darken(area, 127, 127, 127);
			}
		}

		void __fastcall ColorField_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
		{
			// A locked color is still a color indicator; keep input disabled without dimming its swatch.
			const NXBasics::SRectangle area(-rectangle->x, -rectangle->y, Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width), Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height));
			bitmap->Draw_Box(area, Read<NXBasics::SColorRGB>(element, NC2Layout::GuiTextButtonOffset::SolidColor));
			NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, 2);
		}

		void AddOverlay(void* element)
		{
			if (element == nullptr)
			{

				return;
			}

			auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
			// Keep the existing menu ownership list; move only the desktop's borrowed draw/input entries.
			desktop->Element_Remove(static_cast<NXBaseGui::CBaseElement*>(element));
			desktop->Element_AddOverlay(static_cast<NXBaseGui::CBaseElement*>(element), true);
		}

		void BuildDropdown(CGuiManager& gui, const NXBasics::SRectangle& anchor, void* owner)
		{
			void* backdrop = Button(gui, { 0, 0, CGuiManager::mStaticVars.displayWidth, CGuiManager::mStaticVars.displayHeight }, "", nullptr, CloseDropdown, 0, owner, true);

			if (backdrop == nullptr)
			{
				dropdownSlot = UINT32_MAX;

				return;
			}
			// Reuse native button hit testing, action and destruction with an empty base renderer.
			static const auto backdropMethods = [backdrop]
			{
				std::array<std::uintptr_t, 18> methods;
				std::copy_n(Read<const std::uintptr_t*>(backdrop, NC2Layout::GuiBaseElementOffset::VirtualTable), methods.size(), methods.begin());
				methods[NC2Runtime::GuiVirtualSlot::Draw] = reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Draw);

				return methods;
			}();
			Write(backdrop, NC2Layout::GuiBaseElementOffset::VirtualTable, backdropMethods.data());
			AddOverlay(backdrop);
			const auto count = ChoiceCount();
			const auto height = static_cast<std::int32_t>(count) * 24;
			const auto bottom = CGuiManager::mStaticVars.displayHeight / 2 + 76;
			const auto width = (std::max)(220, anchor.width);
			const auto x = (std::min)(anchor.x, CGuiManager::mStaticVars.displayWidth - width - 24);
			const auto y = anchor.y + anchor.height + height <= bottom ? anchor.y + anchor.height : (std::max)(4, anchor.y - height);

			for (std::uint32_t index = 0; index < count; ++index)
			{
				const auto value = ChoiceValue(index);
				const auto label = dropdownKind == EDropdown::Control ? std::string(ControlName(value)) : value == 0 ? std::string(MenuText("Map default: ", "Kartenvorgabe: ")) + TribeName(GetChoices().mapTribes[dropdownSlot])
				                                                                                                     : std::string(TribeName(value));
				const bool enabled = ChoiceEnabled(gui, index);
				const auto* tooltip = !enabled ? MenuText("This choice is not allowed by the map or savegame.", "Diese Auswahl ist durch Karte oder Spielstand gesperrt.") : dropdownKind == EDropdown::Control && value == 1 ? (IsLocal(gui) ? MenuText("Play from this start slot.", "Von diesem Startplatz aus spielen.") : MenuText("Assign the player selected on the left.", "Den links ausgewählten Spieler zuweisen."))
				                                                                                                                                                                                                              : label.c_str();
				void* choice = Button(gui, { x, y + static_cast<std::int32_t>(index) * 24, width, 24 }, label.c_str(), tooltip, dropdownKind == EDropdown::Control ? SelectControl : SelectTribe, dropdownSlot * (dropdownKind == EDropdown::Control ? 4u : 16u) + value, owner, enabled, index == dropdownHighlight);
				AddOverlay(choice);
			}
		}

		void __fastcall Panel_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
		{
			bitmap->Tool_Darken();
			NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, 2);
		}

		void OpenDropdown(CGuiManager& gui, std::uint32_t slot, EDropdown kind)
		{
			const bool closing = dropdownSlot == slot && dropdownKind == kind;
			dropdownKind = kind;
			dropdownSlot = closing ? UINT32_MAX : slot;
			dropdownHighlight = 0;

			if (!closing)
			{
				const auto current = kind == EDropdown::Control ? Read<std::uint32_t>(&gui, Row(slot) + 4) : GetChoices().tribes[slot];
				bool first = true;

				for (std::uint32_t index = 0; index < ChoiceCount(); ++index)
				{
					if (!ChoiceEnabled(gui, index))
						continue;

					if (first)
					{
						dropdownHighlight = index;
						first = false;
					}

					if (ChoiceValue(index) == current)
					{
						dropdownHighlight = index;
						break;
					}
				}
			}

			gui.Screen_UpdateGui();
		}

		void SetControl(CGuiManager& gui, std::uint32_t slot, std::uint32_t kind)
		{
			if (IsLocal(gui))
			{
				Skirmish::SetControl(gui, slot, kind);
			}
			else
			{
				NNetwork::Lobby::SetControl(gui, slot, kind);
			}
		}
	}

	void Reset()
	{
		page = 0;
		dropdownSlot = UINT32_MAX;
		dropdownHighlight = 0;
	}

	void* Button(CGuiManager& gui, const NXBasics::SRectangle& rectangle, const char* caption, const char* tooltip, std::uint32_t command, std::uint32_t value, void* owner, bool enabled, bool selected, bool dropdownField, bool registerElement)
	{
		std::string label(caption != nullptr ? caption : "");
		const auto& fonts = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* font = IsLocal(gui) ? fonts.font10 : fonts.font08;
		const auto textWidth = rectangle.width - (dropdownField ? 42 : 10);
		const bool truncated = font != nullptr && NXBasics::Font_GetPixelWidth(font, label.c_str()) > textWidth;

		if (truncated)
		{
			while (!label.empty() && NXBasics::Font_GetPixelWidth(font, (label + "...").c_str()) > textWidth)
			{
				auto end = label.size() - 1;

				while (end != 0 && (static_cast<unsigned char>(label[end]) & 0xC0u) == 0x80u)
					--end;

				label.resize(end);
			}

			label += "...";
		}

		void* memory = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
		void* element = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructToggleWithToolTip(memory, nullptr, &rectangle, label.c_str(), truncated && tooltip == nullptr ? caption : tooltip, command, owner, selected);

		if (element != nullptr)
		{
			Write(element, NC2Layout::GuiTextButtonOffset::MessageParameter1, value);
			Write<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter2, 0);
			Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, !enabled);
			Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SmallFontFlag, !IsLocal(gui));

			if (dropdownField)
			{
				static const auto methods = []
				{
					std::array<std::uintptr_t, 18> table;
					std::copy_n(NC2InGameGuiBase::gTextButtonVirtualTable, table.size(), table.begin());
					table[NC2Runtime::GuiVirtualSlot::Draw] = reinterpret_cast<std::uintptr_t>(&DropdownField_Draw);

					return table;
				}();
				Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::CenteredFlag, 0);
				Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, methods.data());
			}
		}

		if (element != nullptr && registerElement)
			gui.l_ScreenGuiElement_Add(element);

		return element;
	}

	void* Label(CGuiManager& gui, const NXBasics::SRectangle& rectangle, const char* text, bool registerElement)
	{
		void* memory = NXBasics::Runtime_AllocateWithNewHandler(256, 1);
		void* element = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextElement_Construct(memory, nullptr, &rectangle, text, false, true);

		if (element != nullptr)
		{
			Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::CenteredFlag, 0);
			Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::SmallFontFlag, !IsLocal(gui));
		}

		if (element != nullptr && registerElement)
			gui.l_ScreenGuiElement_Add(element);

		return element;
	}

	void Panel(CGuiManager& gui, const NXBasics::SRectangle& rectangle)
	{
		void* element = Label(gui, rectangle, "");

		if (element != nullptr)
		{
			static const auto methods = [element]
			{
				std::array<std::uintptr_t, 18> table;
				std::copy_n(Read<const std::uintptr_t*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable), table.size(), table.begin());
				table[NC2Runtime::GuiVirtualSlot::Draw] = reinterpret_cast<std::uintptr_t>(&Panel_Draw);

				return table;
			}();
			Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, methods.data());
		}
	}

	void* List(CGuiManager& gui, const NXBasics::SRectangle& rectangle)
	{
		void* memory = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
		void* list = memory == nullptr ? nullptr : NC2InGameGuiBaseListBox::BaseListBoxElement_Construct(memory, &rectangle, 0, 0);

		if (list == nullptr)
			return nullptr;

		gui.l_ScreenGuiElement_Add(list);

		return list;
	}

	void Scrollbar(CGuiManager& gui, void* list, const NXBasics::SRectangle& rectangle)
	{
		if (Read<std::int32_t>(list, NC2Layout::ListBoxOffset::ChildBoundsHeight) <= rectangle.height)
		{

			return;
		}

		const NXBasics::SRectangle scrollbar(rectangle.x + rectangle.width + 4, rectangle.y, 20, rectangle.height);
		void* memory = NXBasics::Runtime_AllocateWithNewHandler(60, 1);

		if (memory != nullptr)
			gui.l_ScreenGuiElement_Add(ToolListBoxControllerElement_Construct(memory, nullptr, &scrollbar, list, false, true));
	}

	void WrappedLabel(CGuiManager& gui, NXBasics::SRectangle rectangle, const char* caption, std::uint32_t lines, void* list)
	{
		std::string remaining(caption == nullptr ? "" : caption);
		const auto& fonts = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* font = IsLocal(gui) ? fonts.font10 : fonts.font08;

		for (std::uint32_t line = 0; line < lines && !remaining.empty(); ++line)
		{
			std::size_t count = 0, split = 0;

			while (count < remaining.size() && remaining[count] != '\n' && remaining[count] != '\r')
			{
				auto next = count + 1;

				while (next < remaining.size() && (static_cast<unsigned char>(remaining[next]) & 0xC0u) == 0x80u)
					++next;

				if (NXBasics::Font_GetPixelWidth(font, remaining.substr(0, next).c_str()) > rectangle.width - 8)
					break;

				if (remaining[count] == ' ')
					split = count;

				count = next;
			}

			if (count == 0)
				count = 1;

			if (count < remaining.size() && remaining[count] != '\n' && remaining[count] != '\r' && split != 0)
				count = split;

			auto text = remaining.substr(0, count);
			remaining.erase(0, count);

			while (!remaining.empty() && (remaining.front() == ' ' || remaining.front() == '\n' || remaining.front() == '\r'))
				remaining.erase(0, 1);

			if (line + 1 == lines && !remaining.empty())
				text += "...";

			void* element = Label(gui, rectangle, text.c_str(), list == nullptr);

			if (list != nullptr && element != nullptr)
				BaseListBoxElement_Element_Add(list, nullptr, element);

			rectangle.y += IsLocal(gui) ? 18 : 16;
		}
	}

	bool HandleCommand(CGuiManager& gui, std::uint32_t command, std::uint32_t value)
	{
		if (command != SwitchIds::MainMenuCommand::AssignPlayerSlot && (command < AddAi || command > SelectControl))
			return false;

		if (gui.currentScreen != SwitchIds::MainMenuScreen::Lobby && !IsLocal(gui))
			return true;

		if (command == CloseDropdown)
		{
			dropdownSlot = UINT32_MAX;
			gui.Screen_UpdateGui();
			return true;
		}

		if (command == ChangePage)
		{
			dropdownSlot = UINT32_MAX;
			page = value;
			gui.Screen_UpdateGui();
			return true;
		}

		if (!CanConfigure(gui))
			return true;

		if (command == SelectControl)
		{
			const auto slot = value / 4, kind = value % 4;

			if (dropdownKind == EDropdown::Control && slot == dropdownSlot && kind <= 2 && (Allowed(gui, slot) & (1u << kind)) != 0)
			{
				dropdownSlot = UINT32_MAX;

				if (kind != 1 && Read<std::uint32_t>(&gui, Row(slot) + 4) == kind)
				{
					gui.Screen_UpdateGui();
				}
				else
				{
					SetControl(gui, slot, kind);
				}
			}

			return true;
		}

		if (command == SwitchIds::MainMenuCommand::AssignPlayerSlot)
		{
			SetControl(gui, value, 1);
			return true;
		}

		if (command == FillAi)
		{
			for (std::uint32_t slot = 0; slot < 20; ++slot)
			{
				if (Read<std::uint32_t>(&gui, Row(slot)) == 0 && Read<std::uint32_t>(&gui, Row(slot) + 4) != 2 && (Allowed(gui, slot) & 4) != 0)
					SetControl(gui, slot, 2);
			}

			return true;
		}

		if (command == SelectTribe)
		{
			const auto slot = value / 16, tribe = value % 16;

			if (dropdownKind == EDropdown::Tribe && slot == dropdownSlot && CanChooseTribe(gui, slot) && (tribe == 0 || Playable(tribe)))
			{
				SetSelectedTribe(slot, tribe);
				dropdownSlot = UINT32_MAX;
				gui.Screen_UpdateGui();
				gui.Network_SendLobbyData();
			}

			return true;
		}

		if (value >= 20)
			return true;

		if (command == AddAi)
			SetControl(gui, value, 2);

		if (command == CloseSlot)
			SetControl(gui, value, 0);

		if (command == OpenControl && Allowed(gui, value) != 0)
			OpenDropdown(gui, value, EDropdown::Control);

		if (command == OpenTribes && CanChooseTribe(gui, value))
			OpenDropdown(gui, value, EDropdown::Tribe);

		return true;
	}

	bool HandleDropdownKey(CGuiManager& gui, std::uint32_t key)
	{
		if (dropdownSlot == UINT32_MAX && IsLocal(gui))
			return Skirmish::HandleKey(gui, key);

		if (dropdownSlot == UINT32_MAX)
			return false;

		if ((gui.currentScreen != SwitchIds::MainMenuScreen::Lobby && !IsLocal(gui)) || gui.lobbyValue != 0)
		{
			dropdownSlot = UINT32_MAX;
			return false;
		}

		bool eligible = false;

		for (std::uint32_t index = 0; index < ChoiceCount(); ++index)
		{
			if (ChoiceEnabled(gui, index))
				eligible = true;
		}

		if (!eligible)
		{
			HandleCommand(gui, CloseDropdown, 0);
			return true;
		}

		const auto count = ChoiceCount();

		switch (key)
		{
			case SwitchIds::VirtualKey::Escape:
			case 0x09:
				HandleCommand(gui, CloseDropdown, 0);
				return true;
			case SwitchIds::VirtualKey::Return:
			case 0x20:
				if (ChoiceEnabled(gui, dropdownHighlight))
					HandleCommand(gui, dropdownKind == EDropdown::Control ? SelectControl : SelectTribe, dropdownSlot * (dropdownKind == EDropdown::Control ? 4u : 16u) + ChoiceValue(dropdownHighlight));

				return true;
			case 0x26:
			case 0x28:
				for (std::uint32_t step = 0; step < count; ++step)
				{
					dropdownHighlight = (dropdownHighlight + (key == 0x26 ? count - 1 : 1)) % count;

					if (ChoiceEnabled(gui, dropdownHighlight))
						break;
				}

				break;
			case SwitchIds::VirtualKey::Home:
			case SwitchIds::VirtualKey::End:
				for (std::uint32_t index = 0; index < count; ++index)
				{
					dropdownHighlight = key == SwitchIds::VirtualKey::Home ? index : count - 1 - index;

					if (ChoiceEnabled(gui, dropdownHighlight))
						break;
				}

				break;
			default:
				return true;
		}

		gui.Screen_UpdateGui();

		return true;
	}

	void BuildSlots(CGuiManager& gui, const NXBasics::SRectangle& rectangle, void* owner)
	{
		const bool host = CanConfigure(gui);
		const bool local = IsLocal(gui);
		const auto width = local ? rectangle.width : (std::min)(820, CGuiManager::mStaticVars.displayWidth - rectangle.x - 24);
		const bool compact = !local && width < 550;
		const auto personWidth = local ? 0 : (std::max)(80, width - (compact ? 260 : 424));
		const auto controlWidth = local ? (std::min)(220, width - 72 - 160) : 168;
		const auto tribeWidth = local ? width - controlWidth - 72 : 150;
		const auto personX = local ? 32 : 48, controlX = local ? 32 : personX + personWidth + 6, tribeX = controlX + controlWidth + 6;
		const auto colorX = compact ? controlX + controlWidth + 6 : tribeX + tribeWidth + 6;
		const auto rowHeight = local ? 24 : 22;
		const auto rowsPerPage = compact ? 4u : 7u;
		auto area = rectangle;
		area.width = width;
		area.height = 20;
		Label(gui, area, MenuText("Player slots", "Spielerplätze"));
		area.y += local ? 22 : 23;
		area.height = 18;
		Label(gui, area, local ? MenuText("Choose your start slot.", "Eigenen Startplatz wählen.") : MenuText("Assign players, add computers, and choose civilizations.", "Spieler zuweisen, Computer hinzufügen und Volk auswählen."));
		area.y += local ? 22 : 23;
		Label(gui, { area.x, area.y, personX - 6, 18 }, local ? "#" : MenuText("Slot", "Platz"));

		if (!local)
			Label(gui, { area.x + personX, area.y, personWidth, 18 }, MenuText("Player", "Spieler"));

		Label(gui, { area.x + controlX, area.y, controlWidth, 18 }, MenuText("Control", "Steuerung"));

		if (!compact)
			Label(gui, { area.x + tribeX, area.y, tribeWidth, 18 }, MenuText("Civilization", "Volk"));

		area.y += 22;
		area.height = rowHeight;
		std::array<std::uint32_t, 20> visible{ };
		std::uint32_t count = 0;

		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			if (Allowed(gui, slot) != 0)
				visible[count++] = slot;
		}

		const auto pages = (std::max)(1u, (count + rowsPerPage - 1) / rowsPerPage);
		page = (std::min)(page, pages - 1);
		NXBasics::SRectangle dropdownAnchor(0, 0, 0, 0);

		for (auto i = page * rowsPerPage; i < (std::min)(count, (page + 1) * rowsPerPage); ++i)
		{
			const auto slot = visible[i], kind = Read<std::uint32_t>(&gui, Row(slot) + 4), flags = Allowed(gui, slot), id = Read<std::uint32_t>(&gui, Row(slot));
			char number[12];
			std::snprintf(number, sizeof(number), "%u", slot + 1);
			Label(gui, { area.x, area.y + 2, local ? 26 : 42, 18 }, number);

			if (!local)
			{
				const auto* name = id == 0 ? nullptr : gui.Network_GetPlayerName(id);
				const auto* occupant = name != nullptr ? name : kind == 2 ? (Customizable(gui, slot) ? MenuText("Computer", "Computer") : MenuText("Map computer", "Karten-Computer"))
				                                                          : MenuText("Open slot", "Freier Platz");
				Button(gui, { area.x + personX, area.y, personWidth, rowHeight }, occupant, MenuText("Assign the player selected on the left.", "Den links ausgewählten Spieler zuweisen."), 0x13B3, slot, owner, host && (flags & 2) != 0);
			}

			const NXBasics::SRectangle control(area.x + controlX, area.y, controlWidth, rowHeight);
			const bool controlOpen = dropdownSlot == slot && dropdownKind == EDropdown::Control;
			Button(gui, control, kind == 0 && (flags & 1) == 0 ? MenuText("Unassigned", "Unbesetzt") : ControlName(kind), MenuText("Choose Human, Computer or Off. Map restrictions still apply.", "Mensch, Computer oder Aus wählen. Kartenvorgaben bleiben gültig."), OpenControl, slot, owner, host, controlOpen, true);

			if (host && controlOpen)
				dropdownAnchor = control;

			const auto* tribe = TribeName(GetChoices().tribes[slot] != 0 && CanChooseTribe(gui, slot) ? GetChoices().tribes[slot] : GetChoices().mapTribes[slot]);
			const bool chooseTribe = host && CanChooseTribe(gui, slot);
			const bool tribeOpen = dropdownSlot == slot && dropdownKind == EDropdown::Tribe;
			char tribeLabel[120];
			std::snprintf(tribeLabel, sizeof(tribeLabel), "%s%s", compact ? MenuText("Civilization: ", "Volk: ") : "", tribe);
			const NXBasics::SRectangle tribeArea(compact ? area.x + personX : area.x + tribeX, compact ? area.y + rowHeight + 2 : area.y, compact ? width - personX - (local ? 0 : 12) : tribeWidth, rowHeight);
			Button(gui, tribeArea, tribeLabel, Customizable(gui, slot) ? MenuText("Choose a civilization. Matching starting buildings are adapted where space permits.", "Volk auswählen. Passende Startgebäude werden bei genügend Platz angepasst.") : MenuText("Fixed by the map or savegame.", "Durch Karte oder Spielstand vorgegeben."), OpenTribes, slot, owner, chooseTribe, tribeOpen, chooseTribe);

			if (chooseTribe && tribeOpen)
				dropdownAnchor = tribeArea;

			if (kind == 1 || kind == 2)
			{
				const bool fixedColors = Read<std::uint8_t>(&gui, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0;
				void* color = Button(gui, { area.x + colorX, area.y, 28, rowHeight }, " ", fixedColors ? MenuText("Color fixed by the map.", "Farbe durch Karte vorgegeben.") : MenuText("Change player color.", "Spielerfarbe ändern."), 0x13B5, slot, owner, host && Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::LobbySavegameId) == 0 && !fixedColors);

				if (color != nullptr)
				{
					NC2InGameGuiBase::BaseToolTextButton_SetBackgroundColor(color, NC2E2::StaticHumanMisc_ColorIds_GetColorRef(Read<std::uint32_t>(&gui, Row(slot) + 12)));
					static const auto colorMethods = []
					{
						std::array<std::uintptr_t, 18> table;
						std::copy_n(NC2InGameGuiBase::gTextButtonVirtualTable, table.size(), table.begin());
						table[NC2Runtime::GuiVirtualSlot::Draw] = reinterpret_cast<std::uintptr_t>(&ColorField_Draw);

						return table;
					}();
					Write(color, NC2Layout::GuiBaseElementOffset::VirtualTable, colorMethods.data());
				}
			}

			area.y += compact ? (rowHeight + 2) * 2 : rowHeight + 4;
		}

		area.y = local ? rectangle.y + rectangle.height - rowHeight : area.y + 4;
		area.width = local ? (std::min)(300, width - (pages > 1 ? 212 : 0)) : compact ? 160
		                                                                              : 240;
		Button(gui, area, MenuText("Fill with computers", "Mit Computern füllen"), MenuText("Fill open slots with computer players.", "Freie Plätze mit Computerspielern füllen."), FillAi, 0, owner, host);

		if (pages > 1)
		{
			auto nav = area;
			nav.x += local ? width - 200 : compact ? 170
			                                       : 250;
			nav.width = local || compact ? 94 : 100;
			Button(gui, nav, MenuText("Previous", "Zurück"), nullptr, ChangePage, page == 0 ? 0 : page - 1, owner, page != 0);
			nav.x += local || compact ? 100 : 106;
			Button(gui, nav, MenuText("Next", "Weiter"), nullptr, ChangePage, page + 1, owner, page + 1 < pages);
		}

		if (dropdownAnchor.width != 0)
		{
			BuildDropdown(gui, dropdownAnchor, owner);
		}
		else
		{
			dropdownSlot = UINT32_MAX;
		}
	}
}
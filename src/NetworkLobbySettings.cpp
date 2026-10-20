#include "NetworkLobbySettings.h"
#include "NetworkCivilizationBuildings.h"
#include "C2CleanMapsManager.h"
#include "C2ControlStartup.h"
#include "C2MapPathfinderSearch.h"
#include "C2EngineDisplayAccess.h"
#include "C2E2StaticDataHouses.h"
#include "C2HouseBuildPosition.h"
#include "C2LanguageTools.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StringAccess.h"
#include "C2StaticTribeTypeData.h"
#include "C2StaticHouseTypeData.h"
#include "GameProfile.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "MainMenuGuiManager.h"
#include "MainMenuGuiManager_Access.h"
#include "MasterControlProgram.h"
#include "NetworkShutdown.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>

namespace NNetwork::Lobby
{
	using namespace NC2Lifetime;
	using NC2MainMenuGuiManager::CGuiManager;
	namespace
	{
		constexpr std::uint32_t ExtensionMagic = 0x59424F4C;
		std::array<std::uint32_t, 20> mapTribes{}, tribes{}, startupMapTribes{}, startupTribes{};
		std::array<std::byte, 16> startupMap{};
		bool startupPending = false, loading = false;
		std::uint32_t page = 0;
		enum class EDropdown { Tribe, Control };
		EDropdown dropdownKind = EDropdown::Tribe;
		std::uint32_t dropdownSlot = UINT32_MAX, dropdownHighlight = 0;
		constexpr std::array<std::uint32_t, 6> TribeChoices{ 0, 1, 2, 3, 4, 7 };
		constexpr std::array<std::uint32_t, 3> ControlChoices{ 1, 2, 0 };
		std::uint32_t ChoiceCount() { return dropdownKind == EDropdown::Control ? 3u : NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2 ? 5u : 6u; }
		std::uint32_t ChoiceValue(std::uint32_t index) { return dropdownKind == EDropdown::Control ? ControlChoices[index] : TribeChoices[index]; }
		const char* Text(const char* english, const char* german)
		{
			return NC2Logic::LanguageTool_GetCurrentLanguage() == 0 ? german : english;
		}
		bool CanConfigure(const CGuiManager& gui) { return IsLocal(gui) || Network_IsHost(); }
		const char* ControlName(std::uint32_t kind)
		{
			return kind == 1 ? Text("Human", "Mensch") : kind == 2 ? Text("AI", "KI") : Text("Off", "Aus");
		}
		std::uint32_t Row(std::uint32_t slot) { return NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned; }
		bool Playable(std::uint32_t tribe)
		{
			return (tribe >= 1 && tribe <= 4) || (tribe == 7 && NRemaster::gGameVariant != NRemaster::EGameVariant::Cultures2);
		}
		bool Customizable(const CGuiManager& gui, std::uint32_t slot)
		{
			if (slot >= 20 || Read<std::uint32_t>(gui.lobbyData, 0) != 1) return false;
			const auto flags = Read<std::uint32_t>(&gui, Row(slot) + 8);
			return (flags & 2) != 0 && (flags & 0x80000000u) == 0 && Playable(mapTribes[slot]);
		}
		std::uint32_t Allowed(const CGuiManager& gui, std::uint32_t slot)
		{
			if (slot >= 20) return 0;
			const auto flags = Read<std::uint32_t>(&gui, Row(slot) + 8);
			if ((flags & 0x80000000u) != 0) return 0;
			return (flags & 7u) | (Customizable(gui, slot) ? 4u : 0u);
		}
		bool CanChooseTribe(const CGuiManager& gui, std::uint32_t slot)
		{
			const auto kind = slot < 20 ? Read<std::uint32_t>(&gui, Row(slot) + 4) : 0;
			return Customizable(gui, slot) && (kind == 2 || (kind == 1 && IsLocal(gui)));
		}
		bool ChoiceEnabled(const CGuiManager& gui, std::uint32_t index)
		{
			if (!CanConfigure(gui) || dropdownSlot >= 20 || index >= ChoiceCount()) return false;
			if (dropdownKind == EDropdown::Control) return (Allowed(gui, dropdownSlot) & (1u << ChoiceValue(index))) != 0;
			return CanChooseTribe(gui, dropdownSlot);
		}
		const char* TribeName(std::uint32_t tribe)
		{
			const auto* name = tribe > 0 && tribe <= 7 ? NC2Logic::StringAccess_GetTribeNamePtr(tribe, 0) : nullptr;
			return name != nullptr ? name : Text("Map default", "Kartenvorgabe");
		}
		void __fastcall DropdownField_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
		{
			// Retain the native button background/frame; inset the caption within the text area.
			const bool disabled = Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag) != 0;
			const bool open = Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag) != 0;
			const auto mode = (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) != 0 && !disabled ? 4 : open ? 3 : 2;
			NC2GuiToolsBase::DrawTool_FillArea(element, bitmap, rectangle, mode);
			NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, mode);
			const auto width = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width), height = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height);
			const auto* palette = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette;
			auto color = Read<NXBasics::SColorRGB>(palette, NC2Layout::PaletteOffset::LastColor);
			auto* font = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08;
			const auto fontHeight = Read<std::int32_t>(font, NC2Layout::FontOffset::Height);
			bitmap->Text_Print(font, &color, 12 - rectangle->x, (height - fontHeight) / 2 - rectangle->y - 3, Read<const char*>(element, NC2Layout::GuiTextButtonOffset::Caption));
			auto separator = color; separator.red /= 3; separator.green /= 3; separator.blue /= 3;
			for (std::int32_t y = 3; y < height - 3; ++y) bitmap->Draw_SetPixel(width - 23 - rectangle->x, y - rectangle->y, separator);
			for (std::int32_t row = 0; row < 4; ++row)
				for (std::int32_t x = row - 3; x <= 3 - row; ++x) bitmap->Draw_SetPixel(width - 12 + x - rectangle->x, height / 2 + (open ? 2 - row : row - 2) - rectangle->y, color);
			if (disabled)
			{
				auto area = bitmap->GetRectangle(); area.Inflate(-1, -1);
				bitmap->Tool_Darken(area, 127, 127, 127);
			}
		}
		void* Button(CGuiManager& gui, const NXBasics::SRectangle& rectangle, const char* caption, const char* tooltip, std::uint32_t command, std::uint32_t value, void* owner, bool enabled, bool selected = false, bool dropdownField = false)
		{
			std::string label(caption != nullptr ? caption : "");
			auto* font = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08;
			const auto textWidth = rectangle.width - (dropdownField ? 42 : 10);
			const bool truncated = font != nullptr && NXBasics::Font_GetPixelWidth(font, label.c_str()) > textWidth;
			if (truncated)
			{
				while (!label.empty() && NXBasics::Font_GetPixelWidth(font, (label + "...").c_str()) > textWidth)
				{
					auto end = label.size() - 1;
					while (end != 0 && (static_cast<unsigned char>(label[end]) & 0xC0u) == 0x80u) --end;
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
				Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SmallFontFlag, 1);
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
			if (element != nullptr) gui.l_ScreenGuiElement_Add(element);
			return element;
		}
		void AddOverlay(void* element)
		{
			if (element == nullptr) return;
			auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
			// Keep the existing menu ownership list; move only the desktop's borrowed draw/input entries.
			desktop->Element_Remove(static_cast<NXBaseGui::CBaseElement*>(element));
			desktop->Element_AddOverlay(static_cast<NXBaseGui::CBaseElement*>(element), true);
		}
		void BuildDropdown(CGuiManager& gui, const NXBasics::SRectangle& anchor, void* owner)
		{
			void* backdrop = Button(gui, { 0, 0, CGuiManager::mStaticVars.displayWidth, CGuiManager::mStaticVars.displayHeight }, "", nullptr, CloseDropdown, 0, owner, true);
			if (backdrop == nullptr) { dropdownSlot = UINT32_MAX; return; }
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
				const auto label = dropdownKind == EDropdown::Control ? std::string(ControlName(value)) : value == 0 ? std::string(Text("Map default: ", "Kartenvorgabe: ")) + TribeName(mapTribes[dropdownSlot]) : std::string(TribeName(value));
				const bool enabled = ChoiceEnabled(gui, index);
				const auto* tooltip = !enabled ? Text("This choice is not allowed by the map or savegame.", "Diese Auswahl ist durch Karte oder Spielstand gesperrt.") : dropdownKind == EDropdown::Control && value == 1 ? (IsLocal(gui) ? Text("Play from this start slot.", "Von diesem Startplatz aus spielen.") : Text("Assign the player selected on the left.", "Den links ausgewählten Spieler zuweisen.")) : label.c_str();
				void* choice = Button(gui, { x, y + static_cast<std::int32_t>(index) * 24, width, 24 }, label.c_str(), tooltip, dropdownKind == EDropdown::Control ? SelectControl : SelectTribe, dropdownSlot * (dropdownKind == EDropdown::Control ? 4u : 16u) + value, owner, enabled, index == dropdownHighlight);
				AddOverlay(choice);
			}
		}
		void Label(CGuiManager& gui, const NXBasics::SRectangle& rectangle, const char* text)
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(256, 1);
			void* element = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextElement_Construct(memory, nullptr, &rectangle, text, false, true);
			if (element != nullptr)
			{
				Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::CenteredFlag, 0);
				Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::SmallFontFlag, 1);
			}
			gui.l_ScreenGuiElement_Add(element);
		}
		const char* LocalStartError(const CGuiManager& gui)
		{
			const auto* map = NC2Logic::gCleanMapsManager == nullptr ? nullptr : NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(Read<NC2Logic::SCleanMapGuid>(gui.lobbyData, NC2Layout::MenuLobbyOffset::MapGuid));
			if (map == nullptr || !IsSkirmishMap(*map)) return Text("Select a skirmish map.", "Eine Gefechtskarte auswählen.");
			std::uint32_t humans = 0;
			for (std::uint32_t slot = 0; slot < 20; ++slot)
			{
				const auto flags = Allowed(gui, slot), kind = Read<std::uint32_t>(&gui, Row(slot) + 4);
				if (kind == 1) ++humans;
				if (flags != 0 && (kind > 2 || (flags & (1u << kind)) == 0)) return Text("Assign all required player slots.", "Alle erforderlichen Spielerplätze besetzen.");
			}
			return humans == 1 ? nullptr : Text("Assign exactly one slot to yourself.", "Genau einen Platz als Mensch besetzen.");
		}
		bool CanStartLocal(const CGuiManager& gui) { return LocalStartError(gui) == nullptr; }
		void WrappedLabel(CGuiManager& gui, NXBasics::SRectangle rectangle, const char* caption, std::uint32_t lines)
		{
			std::string remaining(caption == nullptr ? "" : caption);
			auto* font = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08;
			for (std::uint32_t line = 0; line < lines && !remaining.empty(); ++line)
			{
				std::size_t count = 0, split = 0;
				while (count < remaining.size() && remaining[count] != '\n' && remaining[count] != '\r')
				{
					auto next = count + 1;
					while (next < remaining.size() && (static_cast<unsigned char>(remaining[next]) & 0xC0u) == 0x80u) ++next;
					if (NXBasics::Font_GetPixelWidth(font, remaining.substr(0, next).c_str()) > rectangle.width - 8) break;
					if (remaining[count] == ' ') split = count;
					count = next;
				}
				if (count == 0) count = 1;
				if (count < remaining.size() && remaining[count] != '\n' && remaining[count] != '\r' && split != 0) count = split;
				auto text = remaining.substr(0, count); remaining.erase(0, count);
				while (!remaining.empty() && (remaining.front() == ' ' || remaining.front() == '\n' || remaining.front() == '\r')) remaining.erase(0, 1);
				if (line + 1 == lines && !remaining.empty()) text += "...";
				Label(gui, rectangle, text.c_str()); rectangle.y += 16;
			}
		}
		void SetControl(CGuiManager& gui, std::uint32_t slot, std::uint32_t kind)
		{
			if (slot >= 20 || kind > 2 || (Allowed(gui, slot) & (1u << kind)) == 0) return;
			if (IsLocal(gui))
			{
				if (kind == 1)
					for (std::uint32_t old = 0; old < 20; ++old)
						if (old != slot && Read<std::uint32_t>(&gui, Row(old) + 4) == 1)
						{
							Write<std::uint32_t>(&gui, Row(old), 0);
							Write<std::uint32_t>(&gui, Row(old) + 4, (Allowed(gui, old) & 4) != 0 ? 2 : 0);
						}
				if (kind != 0 && Read<std::uint32_t>(&gui, Row(slot) + 4) == 0) Write(&gui, Row(slot) + 12, Read<std::uint8_t>(&gui, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? (std::min)(slot, 9u) : gui.Network_FindNextPlayerColor(UINT32_MAX));
				Write<std::uint32_t>(&gui, Row(slot), kind == 1 ? 1 : 0);
				Write(&gui, Row(slot) + 4, kind);
				if (kind == 0) tribes[slot] = 0;
				gui.Screen_UpdateGui();
				return;
			}
			if (kind == 1)
			{
				if (Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId) == 0) Write(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId, Network_GetLocalPlayerId());
				tribes[slot] = 0;
				gui.Network_AssignSelectedPlayer(slot);
				return;
			}
			if (kind == 2 && Read<std::uint32_t>(&gui, Row(slot) + 4) == 0 && Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::LobbySavegameId) == 0) Write(&gui, Row(slot) + 12, Read<std::uint8_t>(&gui, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? (std::min)(slot, 9u) : gui.Network_FindNextPlayerColor(UINT32_MAX));
			Write(&gui, Row(slot), std::uint32_t{0});
			Write(&gui, Row(slot) + 4, kind);
			if (kind == 0) tribes[slot] = 0;
			gui.Screen_UpdateGui();
			gui.Network_SendLobbyData();
		}
		void OpenDropdown(CGuiManager& gui, std::uint32_t slot, EDropdown kind)
		{
			const bool closing = dropdownSlot == slot && dropdownKind == kind;
			dropdownKind = kind; dropdownSlot = closing ? UINT32_MAX : slot; dropdownHighlight = 0;
			if (!closing)
			{
				const auto current = kind == EDropdown::Control ? Read<std::uint32_t>(&gui, Row(slot) + 4) : tribes[slot];
				bool first = true;
				for (std::uint32_t index = 0; index < ChoiceCount(); ++index)
				{
					if (!ChoiceEnabled(gui, index)) continue;
					if (first) { dropdownHighlight = index; first = false; }
					if (ChoiceValue(index) == current) { dropdownHighlight = index; break; }
				}
			}
			gui.Screen_UpdateGui();
		}
	}

	void Reset()
	{
		mapTribes.fill(0); tribes.fill(0); startupMapTribes.fill(0); startupTribes.fill(0);
		startupMap.fill(std::byte{}); startupPending = loading = false; page = 0;
		dropdownSlot = UINT32_MAX; dropdownHighlight = 0;
	}
	bool IsLocal(const CGuiManager& gui) { return gui.currentScreen == SwitchIds::MainMenuScreen::Skirmish; }
	const char* MenuText(const char* english, const char* german) { return Text(english, german); }
	const char* DifficultyName(std::uint32_t difficulty) { return difficulty == 0 ? Text("Normal", "Normal") : difficulty == 1 ? Text("Easy", "Leicht") : Text("Very easy", "Sehr leicht"); }
	bool HasTutorialMaps()
	{
		if (NC2Logic::gCleanMapsManager == nullptr) return false;
		NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);
		for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
			if (map->hiddenFromSelection == 0 && (map->mapTypes & 2) != 0 && map->campaignId == 100) return true;
		return false;
	}
	bool IsSkirmishMap(const NC2Logic::SCleanMapInfoData& map)
	{
		return map.hiddenFromSelection == 0 && (map.multiplayerOnly != 0 || (map.mapTypes & ((1u << 4) | (1u << 6))) != 0);
	}
	void InitLocal(CGuiManager& gui)
	{
		Reset(); std::memset(gui.lobbyData, 0, sizeof(gui.lobbyData)); gui.lobbyValue = 0;
		if (NC2Logic::gCleanMapsManager == nullptr) return;
		NC2Logic::gCleanMapsManager->CleanMapList_Update();
		NC2Logic::gCleanMapsManager->CleanMapList_Tool_SortByNames();
		NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);
		for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
			if (IsSkirmishMap(*map)) { gui.Network_SelectMap(map->guid); return; }
	}
	void BuildLocal(CGuiManager& gui, void* owner)
	{
		const auto half = CGuiManager::mStaticVars.displayHeight / 2;
		Label(gui, { 30, half - 245, 500, 22 }, Text("Skirmish", "Gefecht"));
		Label(gui, { 56, half - 210, 280, 20 }, Text("Map", "Karte"));
		const auto* map = NC2Logic::gCleanMapsManager == nullptr ? nullptr : NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(Read<NC2Logic::SCleanMapGuid>(gui.lobbyData, NC2Layout::MenuLobbyOffset::MapGuid));
		char name[200]{};
		if (map != nullptr) map->Tool_GetMapName(name);
		Button(gui, { 56, half - 186, 280, 24 }, map != nullptr ? name : Text("Choose map", "Karte auswählen"), nullptr, SwitchIds::MainMenuCommand::LobbySelectMap, 0, owner, true);
		if (map != nullptr)
		{
			char size[80]; std::snprintf(size, sizeof(size), Text("Map size: %u x %u", "Kartengröße: %u x %u"), map->width, map->height);
			Label(gui, { 56, half - 152, 280, 18 }, size);
			WrappedLabel(gui, { 56, half + 18, 280, 18 }, map->Tool_GetMapDescriptionPtr(), 8);
		}
		Label(gui, { 56, half - 122, 280, 20 }, Text("Difficulty", "Schwierigkeit"));
		for (std::uint32_t difficulty = 0; difficulty < 3; ++difficulty)
			Button(gui, { 56, half - 98 + static_cast<std::int32_t>(difficulty) * 26, 280, 22 }, DifficultyName(difficulty), nullptr, LocalDifficulty, difficulty, owner, true, Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::LobbyDifficulty) == difficulty);
		Label(gui, { 56, half - 12, 280, 20 }, Text("Map description", "Kartenbeschreibung"));
		Label(gui, { 56, half + 155, 280, 18 }, Text("Objectives: map rules", "Spielziele: Kartenvorgabe"));
		const auto* error = LocalStartError(gui);
		if (error != nullptr) WrappedLabel(gui, { 360, half + 142, CGuiManager::mStaticVars.displayWidth - 384, 18 }, error, 2);
		Button(gui, { CGuiManager::mStaticVars.displayWidth - 244, half + 220, 220, 30 }, Text("Start game!", "Spiel starten!"), error, StartLocal, 0, owner, error == nullptr);
		Button(gui, { 30, half + 220, 100, 30 }, Text("Back", "Zurück"), nullptr, LocalBack, 0, owner, true);
		// Build popups last so their borrowed overlay entries stay above all ordinary controls.
		BuildSlots(gui, { 360, half - 210, 280, 20 }, owner);
	}
	void ReadMapMetadata(NXBasics::CIniFileReader& reader)
	{
		if (!reader.SearchGroup("playerdata")) return;
		while (reader.GetNextCommandLineInGroup() != nullptr)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(reader.GetCurrentName(), "player") != 0) continue;
			const auto slot = static_cast<std::uint32_t>(reader.GetInt());
			const auto type = reader.GetInt();
			const auto tribe = static_cast<std::uint32_t>(reader.GetInt());
			const auto color = static_cast<std::uint32_t>(reader.GetInt());
			if (slot < 20 && (type == 1 || type == 2) && tribe > 0 && tribe <= 7 && color < 10) mapTribes[slot] = tribe;
		}
	}
	void AutoAssignPlayers(CGuiManager& gui)
	{
		if (IsLocal(gui))
		{
			for (std::uint32_t slot = 0; slot < 20; ++slot)
				if ((Allowed(gui, slot) & 2) != 0)
				{
					SetControl(gui, slot, 1);
					break;
				}
			for (std::uint32_t slot = 0; slot < 20; ++slot)
				if (Read<std::uint32_t>(&gui, Row(slot) + 4) != 1 && (Allowed(gui, slot) & 4) != 0) SetControl(gui, slot, 2);
			return;
		}
		if (!Network_IsHost() || Read<std::uint32_t>(gui.lobbyData, 0) == 0) return;
		const auto oldSelection = Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId);
		const auto count = (std::min)(10u, Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::PlayerCount));
		for (std::uint32_t index = 0; index < count; ++index)
		{
			const auto id = Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * NC2Layout::RecordSize::MenuNetworkPlayerUnsigned);
			if (id == 0) continue;
			bool assigned = false;
			for (std::uint32_t slot = 0; slot < 20; ++slot) if (Read<std::uint32_t>(&gui, Row(slot)) == id) assigned = true;
			if (assigned) continue;
			for (unsigned pass = 0; pass < 2 && !assigned; ++pass)
				for (std::uint32_t slot = 0; slot < 20; ++slot)
				{
					if ((Allowed(gui, slot) & 2) == 0 || Read<std::uint32_t>(&gui, Row(slot)) != 0 || (pass == 0 && Read<std::uint32_t>(&gui, Row(slot) + 4) == 2)) continue;
					Write(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId, id);
					tribes[slot] = 0; gui.Network_AssignSelectedPlayer(slot); assigned = true; break;
				}
		}
		Write(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId, oldSelection != 0 && gui.Network_GetPlayerListIndex(oldSelection) != UINT32_MAX ? oldSelection : Network_GetLocalPlayerId());
		gui.Screen_UpdateGui();
	}

	Message MakeMessage(const CGuiManager& gui, bool start)
	{
		Message message{};
		message[0] = 1; message[1] = start ? 1u : 0u;
		std::memcpy(reinterpret_cast<std::byte*>(message.data()) + 8, gui.lobbyData, sizeof(gui.lobbyData));
		message[90] = ExtensionMagic;
		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			message[91 + slot] = mapTribes[slot];
			message[111 + slot] = Customizable(gui, slot) && Read<std::uint32_t>(&gui, Row(slot) + 4) == 2 ? tribes[slot] : 0;
		}
		return message;
	}
	bool ReadMessage(CGuiManager& gui, const void* data, std::uint32_t bytes, bool& start)
	{
		if (data == nullptr || bytes != MessageSize) return false;
		Message message; std::memcpy(message.data(), data, sizeof(message));
		if (message[0] != 1 || message[1] > 1 || message[2] > 2 || message[90] != ExtensionMagic) return false;
		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			const auto tribe = message[111 + slot], base = message[91 + slot];
			const auto flags = message[8 + slot * 4 + 2], kind = message[8 + slot * 4 + 1];
			if (base > 7 || kind > 2 || message[8 + slot * 4 + 3] >= 10 || (tribe != 0 && (!Playable(tribe) || !Playable(base) || message[2] != 1 || kind != 2 || (flags & 2) == 0 || (flags & 0x80000000u) != 0))) return false;
		}
		if (!Network_IsHost())
		{
			std::memcpy(gui.lobbyData, message.data() + 2, sizeof(gui.lobbyData));
			for (std::uint32_t slot = 0; slot < 20; ++slot) { mapTribes[slot] = message[91 + slot]; tribes[slot] = message[111 + slot]; }
		}
		start = message[1] != 0;
		if (start)
		{
			for (std::uint32_t slot = 0; slot < 20; ++slot) { startupMapTribes[slot] = message[91 + slot]; startupTribes[slot] = message[111 + slot]; }
			std::memcpy(startupMap.data(), message.data() + 3, 16);
			startupPending = message[2] == 1;
		}
		return true;
	}
	CStartupScope::CStartupScope(const void* mapGuid)
	{
		loading = startupPending && mapGuid != nullptr && std::memcmp(startupMap.data(), mapGuid, 16) == 0;
	}
	CStartupScope::~CStartupScope() { loading = false; startupPending = false; }
	std::uint32_t ResolveStartupTribe(std::uint32_t player, std::uint32_t originalTribe)
	{
		return loading && player < 20 && startupTribes[player] != 0 && startupMapTribes[player] == originalTribe ? startupTribes[player] : originalTribe;
	}
	void ResolveStartupHouse(std::uint32_t player, std::uint32_t& engineType, std::uint32_t& stage, std::uint32_t& point)
	{
		if (!loading || player >= 20 || NC2E2::gStaticHouseStorage.entries == nullptr || engineType >= static_cast<std::uint32_t>(NC2E2::gStaticHouseStorage.count)) return;
		const auto* original = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(engineType);
		if (stage >= 5 || stage >= Read<std::uint32_t>(original, NC2Layout::EngineHouseLogicViewOffset::DevelopmentStageCount)) return;
		const auto originalTribe = Read<std::uint32_t>(original, NC2Layout::EngineHouseLogicViewOffset::TribeType);
		const bool expanded = CivilizationBuildings::IsExpandedPlayer(player);
		const auto selected = expanded ? Read<std::uint32_t>(NC2Logic::gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeType) : ResolveStartupTribe(player, originalTribe);
		if (selected == originalTribe && !expanded) return;
		const auto logic = Read<std::uint32_t>(original, stage * NC2Layout::RecordSize::HouseStageValueUnsigned);
		if (logic >= 55 || NC2Logic::gStaticHouseTypeStorage.records == nullptr) return;
		const auto tryType = [&] (std::uint32_t targetLogic)
		{
			std::uint32_t targetStage = 0;
			auto target = NC2E2::Engine2d_EntryPoint_GetHouseId(selected, targetLogic, &targetStage);
			while (target != -1)
			{
				if (NC2Logic::Tool_House_IsAllowedToBeBuild(player, target, targetStage, point, NC2Logic::gStaticHouseTypeStorage.records[targetLogic].ignoreContinents != 0, false))
				{
					engineType = static_cast<std::uint32_t>(target); stage = targetStage; return true;
				}
				if (expanded)
				{
					// A foreign footprint may need a nearby anchor; retain native terrain/occupancy checks.
					std::uint32_t search[4] = { player, static_cast<std::uint32_t>(target), targetStage, 0 };
					if (NC2Logic::CMapPathfinderElement::SearchOverMap(&point, 12, false, NC2Logic::Tool_HouseBuildPointSearchEvaluationFunction, search, false, player))
					{
						engineType = static_cast<std::uint32_t>(target); stage = targetStage; point = search[3]; return true;
					}
				}
				target = NC2E2::Engine2d_EntryPoint_GetNextHouseId(target, selected, targetLogic, &targetStage);
			}
			return false;
		};
		if (tryType(logic)) return;
		// Foreign main houses use warehouse roles rather than Viking headquarters type 1.
		if (NC2Logic::gStaticHouseTypeStorage.records[logic].mainType == 1)
			for (const auto stock : { 9u, 8u, 7u }) if (stock != logic && tryType(stock)) return;
	}

	bool HandleCommand(CGuiManager& gui, std::uint32_t command, std::uint32_t value)
	{
		if (command == ShowSkirmish) { gui.Screen_ChangeTo(SwitchIds::MainMenuScreen::Skirmish, false); return true; }
		if (IsLocal(gui))
		{
			if (command == LocalBack) { gui.Screen_ChangeTo(SwitchIds::MainMenuScreen::SinglePlayerMenu, false); return true; }
			if (command == LocalDifficulty) { if (value <= 2) gui.Network_SetLobbyDifficulty(value); return true; }
			if (command == SwitchIds::MainMenuCommand::ChangePlayerColor) { if (value < 20 && Allowed(gui, value) != 0) gui.Network_ChangePlayerColor(value); return true; }
			if (command == StartLocal)
			{
				if (!CanStartLocal(gui)) return true;
				auto* master = NMasterControlProgram::CMasterControlProgram::sTheObjectPtr;
				auto* startup = reinterpret_cast<NC2Logic::SGameControlStartupData*>(reinterpret_cast<std::byte*>(master) + NC2Layout::MasterControlProgramOffset::StartupData);
				startup->Reset(); startup->mMode = SwitchIds::GameStartupMode::SkirmishMap; startup->mLocalNetworkPlayerId = 1;
				std::memcpy(startup->mMultiplayerMapGuid, gui.lobbyData + NC2Layout::MenuLobbyOffset::MapGuid, 16);
				startup->mDifficultyLevel = Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::LobbyDifficulty);
				for (std::uint32_t slot = 0; slot < 20; ++slot)
				{
					startup->mPlayers[slot] = { Read<std::uint32_t>(&gui, Row(slot) + 4), Read<std::uint32_t>(&gui, Row(slot) + 12), Read<std::uint32_t>(&gui, Row(slot)) };
					startupMapTribes[slot] = mapTribes[slot]; startupTribes[slot] = CanChooseTribe(gui, slot) ? tribes[slot] : 0;
				}
				std::memcpy(startupMap.data(), startup->mMultiplayerMapGuid, 16); startupPending = true;
				master->System_ProgramState_ChangeNextTimePossible(NMasterControlProgram::TProgramState::Game, false);
				return true;
			}
		}
		if (command != SwitchIds::MainMenuCommand::AssignPlayerSlot && (command < AddAi || command > SelectControl)) return false;
		if (gui.currentScreen != SwitchIds::MainMenuScreen::Lobby && !IsLocal(gui)) return true;
		if (command == CloseDropdown) { dropdownSlot = UINT32_MAX; gui.Screen_UpdateGui(); return true; }
		if (command == ChangePage) { dropdownSlot = UINT32_MAX; page = value; gui.Screen_UpdateGui(); return true; }
		if (!CanConfigure(gui)) return true;
		if (command == SelectControl)
		{
			const auto slot = value / 4, kind = value % 4;
			if (dropdownKind == EDropdown::Control && slot == dropdownSlot && kind <= 2 && (Allowed(gui, slot) & (1u << kind)) != 0)
			{
				dropdownSlot = UINT32_MAX;
				if (kind != 1 && Read<std::uint32_t>(&gui, Row(slot) + 4) == kind) gui.Screen_UpdateGui();
				else SetControl(gui, slot, kind);
			}
			return true;
		}
		if (command == SwitchIds::MainMenuCommand::AssignPlayerSlot) { SetControl(gui, value, 1); return true; }
		if (command == FillAi)
		{
			for (std::uint32_t slot = 0; slot < 20; ++slot) if (Read<std::uint32_t>(&gui, Row(slot)) == 0 && Read<std::uint32_t>(&gui, Row(slot) + 4) != 2 && (Allowed(gui, slot) & 4) != 0) SetControl(gui, slot, 2);
			return true;
		}
		if (command == SelectTribe)
		{
			const auto slot = value / 16, tribe = value % 16;
			if (dropdownKind == EDropdown::Tribe && slot == dropdownSlot && CanChooseTribe(gui, slot) && (tribe == 0 || Playable(tribe)))
			{
				tribes[slot] = tribe; dropdownSlot = UINT32_MAX;
				gui.Screen_UpdateGui(); gui.Network_SendLobbyData();
			}
			return true;
		}
		if (value >= 20) return true;
		if (command == AddAi) SetControl(gui, value, 2);
		if (command == CloseSlot) SetControl(gui, value, 0);
		if (command == OpenControl && Allowed(gui, value) != 0) OpenDropdown(gui, value, EDropdown::Control);
		if (command == OpenTribes && CanChooseTribe(gui, value)) OpenDropdown(gui, value, EDropdown::Tribe);
		return true;
	}
	bool HandleDropdownKey(CGuiManager& gui, std::uint32_t key)
	{
		if (dropdownSlot == UINT32_MAX && IsLocal(gui))
		{
			if (key == SwitchIds::VirtualKey::Escape)
			{
				if (gui.lobbyValue != 0) gui.Network_SetLobbySelection(0, true);
				else HandleCommand(gui, LocalBack, 0);
				return true;
			}
			if (key == SwitchIds::VirtualKey::Return && gui.lobbyValue == 0) { HandleCommand(gui, StartLocal, 0); return true; }
		}
		if (dropdownSlot == UINT32_MAX) return false;
		if ((gui.currentScreen != SwitchIds::MainMenuScreen::Lobby && !IsLocal(gui)) || gui.lobbyValue != 0) { dropdownSlot = UINT32_MAX; return false; }
		bool eligible = false;
		for (std::uint32_t index = 0; index < ChoiceCount(); ++index) if (ChoiceEnabled(gui, index)) eligible = true;
		if (!eligible) { HandleCommand(gui, CloseDropdown, 0); return true; }
		const auto count = ChoiceCount();
		switch (key)
		{
			case SwitchIds::VirtualKey::Escape: case 0x09: HandleCommand(gui, CloseDropdown, 0); return true;
			case SwitchIds::VirtualKey::Return: case 0x20:
				if (ChoiceEnabled(gui, dropdownHighlight)) HandleCommand(gui, dropdownKind == EDropdown::Control ? SelectControl : SelectTribe, dropdownSlot * (dropdownKind == EDropdown::Control ? 4u : 16u) + ChoiceValue(dropdownHighlight));
				return true;
			case 0x26: case 0x28:
				for (std::uint32_t step = 0; step < count; ++step) { dropdownHighlight = (dropdownHighlight + (key == 0x26 ? count - 1 : 1)) % count; if (ChoiceEnabled(gui, dropdownHighlight)) break; }
				break;
			case SwitchIds::VirtualKey::Home: case SwitchIds::VirtualKey::End:
				for (std::uint32_t index = 0; index < count; ++index) { dropdownHighlight = key == SwitchIds::VirtualKey::Home ? index : count - 1 - index; if (ChoiceEnabled(gui, dropdownHighlight)) break; }
				break;
			default: return true;
		}
		gui.Screen_UpdateGui();
		return true;
	}
	void BuildSlots(CGuiManager& gui, const NXBasics::SRectangle& rectangle, void* owner)
	{
		const bool host = CanConfigure(gui);
		const auto width = (std::min)(820, CGuiManager::mStaticVars.displayWidth - rectangle.x - 24);
		const bool compact = width < 550;
		const auto personWidth = (std::max)(80, width - (compact ? 260 : 424));
		const auto rowsPerPage = compact ? 4u : 7u;
		auto area = rectangle; area.width = width;
		Label(gui, area, Text("Player slots", "Spielerplätze")); area.y += 23; area.height = 18;
		Label(gui, area, IsLocal(gui) ? Text("Choose your start slot and configure the AI.", "Startplatz wählen und KI konfigurieren.") : Text("Assign a player, add AI, or choose its civilization.", "Spieler zuweisen, KI hinzufügen und Volk auswählen.")); area.y += 23;
		Label(gui, {area.x, area.y, 42, 18}, Text("Slot", "Platz"));
		Label(gui, {area.x + 48, area.y, personWidth, 18}, Text("Player", "Spieler"));
		Label(gui, {area.x + 54 + personWidth, area.y, 168, 18}, Text("Control", "Steuerung"));
		if (!compact) Label(gui, {area.x + 228 + personWidth, area.y, 150, 18}, Text("Civilization", "Volk"));
		area.y += 22; area.height = 22;
		std::array<std::uint32_t, 20> visible{}; std::uint32_t count = 0;
		for (std::uint32_t slot = 0; slot < 20; ++slot) if (Allowed(gui, slot) != 0) visible[count++] = slot;
		const auto pages = (std::max)(1u, (count + rowsPerPage - 1) / rowsPerPage);
		page = (std::min)(page, pages - 1);
		NXBasics::SRectangle dropdownAnchor(0, 0, 0, 0);
		for (auto i = page * rowsPerPage; i < (std::min)(count, (page + 1) * rowsPerPage); ++i)
		{
			const auto slot = visible[i], kind = Read<std::uint32_t>(&gui, Row(slot) + 4), flags = Allowed(gui, slot), id = Read<std::uint32_t>(&gui, Row(slot));
			char number[12]; std::snprintf(number, sizeof(number), "%u", slot + 1);
			Label(gui, {area.x, area.y + 2, 42, 18}, number);
			const auto* name = id == 0 ? nullptr : IsLocal(gui) ? Text("You", "Du") : gui.Network_GetPlayerName(id);
			const auto* occupant = name != nullptr ? name : kind == 2 ? (Customizable(gui, slot) ? Text("Computer", "Computer") : Text("Map AI", "Karten-KI")) : Text("Open slot", "Freier Platz");
			Button(gui, {area.x + 48, area.y, personWidth, 22}, occupant, IsLocal(gui) ? Text("Play from this start slot.", "Von diesem Startplatz aus spielen.") : Text("Assign the player selected on the left.", "Den links ausgewählten Spieler zuweisen."), 0x13B3, slot, owner, host && (flags & 2) != 0);
			const NXBasics::SRectangle control(area.x + 54 + personWidth, area.y, 168, 22);
			const bool controlOpen = dropdownSlot == slot && dropdownKind == EDropdown::Control;
			Button(gui, control, kind == 0 && (flags & 1) == 0 ? Text("Unassigned", "Unbesetzt") : ControlName(kind), Text("Choose Human, AI or Off. Map restrictions still apply.", "Mensch, KI oder Aus wählen. Kartenvorgaben bleiben gültig."), OpenControl, slot, owner, host, controlOpen, true);
			if (host && controlOpen) dropdownAnchor = control;
			const auto* tribe = TribeName(tribes[slot] != 0 && CanChooseTribe(gui, slot) ? tribes[slot] : mapTribes[slot]);
			const bool chooseTribe = host && CanChooseTribe(gui, slot);
			const bool tribeOpen = dropdownSlot == slot && dropdownKind == EDropdown::Tribe;
			char tribeLabel[120]; std::snprintf(tribeLabel, sizeof(tribeLabel), "%s%s", compact ? Text("Civilization: ", "Volk: ") : "", tribe);
			const NXBasics::SRectangle tribeArea(compact ? area.x + 48 : area.x + 228 + personWidth, compact ? area.y + 24 : area.y, compact ? width - 60 : 150, 22);
			Button(gui, tribeArea, tribeLabel, Customizable(gui, slot) ? Text("Choose a civilization. Matching starting buildings are adapted where space permits.", "Volk auswählen. Passende Startgebäude werden bei genügend Platz angepasst.") : Text("Fixed by the map or savegame.", "Durch Karte oder Spielstand vorgegeben."), OpenTribes, slot, owner, chooseTribe, tribeOpen, chooseTribe);
			if (chooseTribe && tribeOpen) dropdownAnchor = tribeArea;
			if (kind == 1 || kind == 2)
			{
				void* color = Button(gui, {area.x + (compact ? 228 : 384) + personWidth, area.y, 28, 22}, " ", nullptr, 0x13B5, slot, owner, host && Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::LobbySavegameId) == 0 && Read<std::uint8_t>(&gui, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) == 0);
				if (color != nullptr) NC2InGameGuiBase::BaseToolTextButton_SetBackgroundColor(color, NC2E2::StaticHumanMisc_ColorIds_GetColorRef(Read<std::uint32_t>(&gui, Row(slot) + 12)));
			}
			area.y += compact ? 48 : 26;
		}
		area.y += 4; area.width = compact ? 160 : 240;
		Button(gui, area, compact ? Text("Fill with AI", "Mit KI füllen") : Text("Fill open slots with AI", "Freie Plätze mit KI füllen"), nullptr, FillAi, 0, owner, host);
		if (pages > 1)
		{
			auto nav = area; nav.x += compact ? 170 : 250; nav.width = compact ? 94 : 100;
			Button(gui, nav, Text("Previous", "Zurück"), nullptr, ChangePage, page == 0 ? 0 : page - 1, owner, page != 0);
			nav.x += compact ? 100 : 106;
			Button(gui, nav, Text("Next", "Weiter"), nullptr, ChangePage, page + 1, owner, page + 1 < pages);
		}
		if (dropdownAnchor.width != 0) BuildDropdown(gui, dropdownAnchor, owner);
		else dropdownSlot = UINT32_MAX;
	}
}

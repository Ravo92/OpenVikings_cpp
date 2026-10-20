#include "SkirmishMenu.h"
#include "GameSetupSettings.h"
#include "GameSetupMenu.h"
#include "GameSetupStartup.h"
#include "C2CleanMapsManager.h"
#include "C2ControlStartup.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "GuiToolsBase.h"
#include "GuiListBoxElementAccess.h"
#include "MainMenuGuiManager.h"
#include "MasterControlProgram.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XFileAccess.h"
#include "XFontMetrics.h"
#include "XPicture.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

namespace NRemaster::GameSetup::Skirmish
{
	using namespace NC2Lifetime;
	using NC2MainMenuGuiManager::CGuiManager;

	using namespace Menu;
	namespace
	{
		std::unique_ptr<NXBasics::CPicture> localPreview;
		std::unique_ptr<NXBasics::CBitmap> scaledPreview;
		bool previewAttempted = false;
		void __fastcall Preview_Draw(void*, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
		{
			if (scaledPreview != nullptr) scaledPreview->CopyIntoBitmap_ColorKeyed(bitmap, -rectangle->x, -rectangle->y, 0);
		}
		const char* LocalStartError(const CGuiManager& gui)
		{
			const auto* map = NC2Logic::gCleanMapsManager == nullptr ? nullptr : NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(Read<NC2Logic::SCleanMapGuid>(gui.lobbyData, NC2Layout::MenuLobbyOffset::MapGuid));
			if (map == nullptr || !IsSkirmishMap(*map)) return MenuText("Select a skirmish map.", "Eine Gefechtskarte auswählen.");
			std::uint32_t humans = 0;
			for (std::uint32_t slot = 0; slot < 20; ++slot)
			{
				const auto flags = Allowed(gui, slot), kind = Read<std::uint32_t>(&gui, Row(slot) + 4);
				if (kind == 1) ++humans;
				if (flags != 0 && (kind > 2 || (flags & (1u << kind)) == 0)) return MenuText("Assign all required player slots.", "Alle erforderlichen Spielerplätze besetzen.");
			}
			return humans == 1 ? nullptr : MenuText("Assign exactly one slot to yourself.", "Genau einen Platz als Mensch besetzen.");
		}
		bool CanStartLocal(const CGuiManager& gui) { return LocalStartError(gui) == nullptr; }
		void MapList(CGuiManager& gui, const NXBasics::SRectangle& rectangle, void* owner)
		{
			Panel(gui, rectangle);
			Label(gui, { rectangle.x + 8, rectangle.y + 6, rectangle.width - 16, 22 }, MenuText("Maps", "Karten"));
			const NXBasics::SRectangle bounds(rectangle.x + 8, rectangle.y + 32, rectangle.width - 40, rectangle.height - 40);
			void* list = List(gui, bounds);
			if (list == nullptr || NC2Logic::gCleanMapsManager == nullptr) return;
			const auto selected = Read<NC2Logic::SCleanMapGuid>(gui.lobbyData, NC2Layout::MenuLobbyOffset::MapGuid);
			NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);
			std::uint32_t index = 0; std::int32_t selectedY = 0;
			for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
			{
				if (!IsSkirmishMap(*map)) continue;
				char name[200]; map->Tool_GetMapName(name);
				char dimensions[40]; std::snprintf(dimensions, sizeof(dimensions), " - %u x %u", map->width, map->height);
				const auto fullTitle = std::string(name) + dimensions;
				std::string title(name);
				auto* font = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font10;
				if (font != nullptr && NXBasics::Font_GetPixelWidth(font, fullTitle.c_str()) > bounds.width - 10)
				{
					while (!title.empty() && NXBasics::Font_GetPixelWidth(font, (title + "..." + dimensions).c_str()) > bounds.width - 10)
					{
						auto end = title.size() - 1;
						while (end != 0 && (static_cast<unsigned char>(title[end]) & 0xC0u) == 0x80u) --end;
						title.resize(end);
					}
					title += "...";
				}
				title += dimensions;
				const NXBasics::SRectangle row(0, static_cast<std::int32_t>(index) * 26, bounds.width, 24);
				const bool active = selected.IsEqualTo(map->guid);
				void* element = Button(gui, row, title.c_str(), fullTitle.c_str(), LocalSelectMap, index++, owner, true, active, false, false);
				if (element != nullptr) BaseListBoxElement_Element_Add(list, nullptr, element);
				if (active) selectedY = row.y;
			}
			NC2InGameGuiBaseListBox::BaseListBoxElement_ScrollOffset_Set(list, 0, -(std::max)(0, selectedY - bounds.height / 2));
			Scrollbar(gui, list, bounds);
		}
		void MapPreview(CGuiManager& gui, const NXBasics::SRectangle& rectangle, const NC2Logic::SCleanMapInfoData* map)
		{
			Panel(gui, rectangle);
			Label(gui, { rectangle.x + 8, rectangle.y + 6, rectangle.width - 16, 22 }, MenuText("Map preview", "Kartenvorschau"));
			if (!previewAttempted && map != nullptr)
			{
				previewAttempted = true;
				void* archive = nullptr;
				if (map->userMap != 0)
				{
					void* memory = NXBasics::Runtime_AllocateWithNewHandler(20, 1);
					if (memory != nullptr) { archive = NXBasics::Archive_Initialize(memory, map->archiveFilename); NXBasics::CFile::FileSystem_FileLibrary_Add(archive); }
				}
				const std::string filename = std::string(map->mapRoot) + "minimap\\minimap.pcx";
				if (NXBasics::CFile::FileSystem_Tool_FileExists(filename.c_str(), false)) localPreview = std::make_unique<NXBasics::CPicture>(filename.c_str());
				if (archive != nullptr) NXBasics::CFile::FileSystem_FileLibrary_Remove(archive);
			}
			if (localPreview == nullptr || localPreview->GetBitmap() == nullptr)
			{
				WrappedLabel(gui, { rectangle.x + 10, rectangle.y + 34, rectangle.width - 20, 18 }, MenuText("No map preview available.", "Keine Kartenvorschau verfügbar."), 3);
				return;
			}
			const auto source = localPreview->GetBitmap()->GetRectangle();
			if (source.width <= 0 || source.height <= 0) return;
			const auto width = (std::min)(rectangle.width - 16, source.width * (rectangle.height - 40) / source.height);
			const auto height = source.height * width / source.width;
			const NXBasics::SRectangle bounds(rectangle.x + (rectangle.width - width) / 2, rectangle.y + 32 + (rectangle.height - 40 - height) / 2, width, height);
			if (scaledPreview == nullptr || scaledPreview->GetRectangle().width != width || scaledPreview->GetRectangle().height != height)
			{
				scaledPreview = std::make_unique<NXBasics::CBitmap>(width, height, 0);
				localPreview->GetBitmap()->CopyIntoBitmap_FitIn(scaledPreview.get());
				scaledPreview->SetPalettePtr(localPreview->GetPalette());
			}
			void* element = Label(gui, bounds, "");
			if (element != nullptr)
			{
				static const auto methods = [element]
				{
					std::array<std::uintptr_t, 18> table;
					std::copy_n(Read<const std::uintptr_t*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable), table.size(), table.begin());
					table[NC2Runtime::GuiVirtualSlot::Draw] = reinterpret_cast<std::uintptr_t>(&Preview_Draw);
					return table;
				}();
				Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, methods.data());
			}
		}
		void MapDescription(CGuiManager& gui, const NXBasics::SRectangle& rectangle, const NC2Logic::SCleanMapInfoData* map)
		{
			Panel(gui, rectangle);
			Label(gui, { rectangle.x + 8, rectangle.y + 6, rectangle.width - 16, 22 }, MenuText("Description", "Beschreibung"));
			const NXBasics::SRectangle bounds(rectangle.x + 8, rectangle.y + 32, rectangle.width - 40, rectangle.height - 40);
			void* list = List(gui, bounds);
			if (list != nullptr && map != nullptr)
			{
				WrappedLabel(gui, { 0, 0, bounds.width, 18 }, map->Tool_GetMapDescriptionPtr(), 400, list);
				Scrollbar(gui, list, bounds);
			}
		}
	}
	void ClearPreview() { scaledPreview.reset(); localPreview.reset(); previewAttempted = false; }
	void Init(CGuiManager& gui)
	{
		GameSetup::Reset(); std::memset(gui.lobbyData, 0, sizeof(gui.lobbyData)); gui.lobbyValue = 0;
		if (NC2Logic::gCleanMapsManager == nullptr) return;
		NC2Logic::gCleanMapsManager->CleanMapList_Update();
		NC2Logic::gCleanMapsManager->CleanMapList_Tool_SortByNames();
		NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);
		for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
			if (IsSkirmishMap(*map)) { gui.Network_SelectMap(map->guid); return; }
	}
	void Build(CGuiManager& gui, void* owner)
	{
	const auto half = CGuiManager::mStaticVars.displayHeight / 2;
	const auto width = CGuiManager::mStaticVars.displayWidth;
	const auto bodyWidth = (std::min)(1280, width - 60), left = (width - bodyWidth) / 2;
	const auto top = half - 215, bodyHeight = 417;
	const bool wide = width >= 1120;
	const auto mapWidth = wide ? (width >= 1280 ? 260 : 230) : (std::min)(280, width / 3);
	const auto previewWidth = wide ? (width >= 1280 ? 320 : 260) : mapWidth;
	const auto slotsX = left + mapWidth + 14 + (wide ? previewWidth + 14 : 0);
	const auto slotsWidth = left + bodyWidth - slotsX;
	Label(gui, { left, half - 245, 500, 22 }, MenuText("Skirmish", "Gefecht"));
	const auto* map = NC2Logic::gCleanMapsManager == nullptr ? nullptr : NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(Read<NC2Logic::SCleanMapGuid>(gui.lobbyData, NC2Layout::MenuLobbyOffset::MapGuid));
	MapList(gui, { left, top, mapWidth, wide ? bodyHeight : 158 }, owner);
	MapPreview(gui, { wide ? left + mapWidth + 14 : left, wide ? top : top + 170, previewWidth, wide ? 190 : 130 }, map);
	MapDescription(gui, { wide ? left + mapWidth + 14 : left, top + (wide ? 202 : 312), previewWidth, bodyHeight - (wide ? 202 : 312) }, map);
	Panel(gui, { slotsX, top, slotsWidth, 310 });
	Panel(gui, { slotsX, top + 322, slotsWidth, 95 });
	Label(gui, { slotsX + 8, top + 328, slotsWidth - 16, 20 }, MenuText("Difficulty", "Schwierigkeit"));
	const auto difficultyWidth = (slotsWidth - 28) / 3;
	for (std::uint32_t difficulty = 0; difficulty < 3; ++difficulty)
		Button(gui, { slotsX + 8 + static_cast<std::int32_t>(difficulty) * (difficultyWidth + 6), top + 352, difficultyWidth, 24 }, DifficultyName(difficulty), nullptr, LocalDifficulty, difficulty, owner, true, Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::LobbyDifficulty) == difficulty);
	const bool fixedColors = Read<std::uint8_t>(&gui, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0;
	Label(gui, { slotsX + 8, top + 384, slotsWidth - 16, 22 }, fixedColors ? MenuText("Colors fixed by the map.", "Farben durch Karte vorgegeben.") : MenuText("Click a color to change it.", "Farbe zum Ändern anklicken."));
	const auto* error = LocalStartError(gui);
	if (error != nullptr) WrappedLabel(gui, { left + 110, half + 220, bodyWidth - 344, 18 }, error, 2);
	Button(gui, { left + bodyWidth - 220, half + 220, 220, 30 }, MenuText("Start game!", "Spiel starten!"), error, StartLocal, 0, owner, error == nullptr);
	Button(gui, { left, half + 220, 100, 30 }, MenuText("Back", "Zurück"), nullptr, LocalBack, 0, owner, true);
	// Build popups last so their borrowed overlay entries stay above all ordinary controls.
	BuildSlots(gui, { slotsX + 8, top + 6, slotsWidth - 16, 298 }, owner);
	}
	void SetControl(CGuiManager& gui, std::uint32_t slot, std::uint32_t kind)
	{
		if (slot >= 20 || kind > 2 || (Allowed(gui, slot) & (1u << kind)) == 0) return;
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
		if (kind == 0) SetSelectedTribe(slot, 0);
		gui.Screen_UpdateGui();
		return;
	}
	void AutoAssignPlayers(CGuiManager& gui)
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
	bool HandleCommand(CGuiManager& gui, std::uint32_t command, std::uint32_t value)
	{
		if (command == ShowSkirmish) { gui.Screen_ChangeTo(SwitchIds::MainMenuScreen::Skirmish, false); return true; }
		if (IsLocal(gui))
		{
			if (command == LocalBack) { ClearPreview(); gui.Screen_ChangeTo(SwitchIds::MainMenuScreen::SinglePlayerMenu, false); return true; }
			if (command == LocalSelectMap)
			{
				if (NC2Logic::gCleanMapsManager == nullptr) return true;
				NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);
				for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
					if (IsSkirmishMap(*map) && value-- == 0)
					{
						if (!Read<NC2Logic::SCleanMapGuid>(gui.lobbyData, NC2Layout::MenuLobbyOffset::MapGuid).IsEqualTo(map->guid)) gui.Network_SelectMap(map->guid);
						break;
					}
				return true;
			}
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
				SPlayerChoices startupChoices;
				for (std::uint32_t slot = 0; slot < 20; ++slot)
				{
					startup->mPlayers[slot] = { Read<std::uint32_t>(&gui, Row(slot) + 4), Read<std::uint32_t>(&gui, Row(slot) + 12), Read<std::uint32_t>(&gui, Row(slot)) };
					startupChoices.mapTribes[slot] = GetChoices().mapTribes[slot]; startupChoices.tribes[slot] = CanChooseTribe(gui, slot) ? GetChoices().tribes[slot] : 0;
				}
				Startup::Prepare(startup->mMultiplayerMapGuid, startupChoices, true);
				ClearPreview();
				master->System_ProgramState_ChangeNextTimePossible(NMasterControlProgram::TProgramState::Game, false);
				return true;
			}
		}
		return false;
	}
	bool HandleKey(CGuiManager& gui, std::uint32_t key)
	{
		if (IsLocal(gui))
		{
			if (key == SwitchIds::VirtualKey::Escape)
			{
				if (gui.lobbyValue != 0) gui.Network_SetLobbySelection(0, true);
				else HandleCommand(gui, LocalBack, 0);
				return true;
			}
			if (key == SwitchIds::VirtualKey::Return && gui.lobbyValue == 0) { HandleCommand(gui, StartLocal, 0); return true; }
		}
		return false;
	}
}

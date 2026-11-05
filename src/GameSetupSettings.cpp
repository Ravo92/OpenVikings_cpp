#include "GameSetupSettings.h"
#include "GameSetupMenu.h"
#include "GameSetupStartup.h"
#include "SkirmishMenu.h"
#include "NetworkLobbySettings.h"
#include "C2LanguageTools.h"
#include "C2CleanMapsManager.h"
#include "C2LifetimeGlobals.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "GameProfile.h"
#include "MainMenuGuiManager.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"

namespace NRemaster::GameSetup
{
	using namespace NC2Lifetime;
	using NC2MainMenuGuiManager::CGuiManager;

	namespace
	{
		SPlayerChoices playerChoices;
		std::array<std::uint32_t, 20> mapPlayerColors;
		std::array<std::array<std::uint8_t, 20>, 20> mapDiplomacy{ };
	}

	const SPlayerChoices& GetChoices()
	{

		return playerChoices;
	}

	void SetMapTribe(std::uint32_t slot, std::uint32_t tribe)
	{
		if (slot < 20)
			playerChoices.mapTribes[slot] = tribe;
	}

	void SetSelectedTribe(std::uint32_t slot, std::uint32_t tribe)
	{
		if (slot < 20)
			playerChoices.tribes[slot] = tribe;
	}

	void Reset()
	{
		playerChoices = { };
		mapPlayerColors.fill(UINT32_MAX);
		mapDiplomacy = { };
		Startup::Reset();
		Menu::Reset();
		Skirmish::ClearPreview();
	}

	std::uint32_t GetMapPlayerColor(std::uint32_t slot)
	{

		return slot < mapPlayerColors.size() && playerChoices.mapTribes[slot] != 0 ? mapPlayerColors[slot] : UINT32_MAX;
	}

	bool AreMapPlayersHostile(std::uint32_t player, std::uint32_t other)
	{

		return player < 20 && other < 20 && (mapDiplomacy[player][other] == SwitchIds::DiplomacyState::Enemy || mapDiplomacy[other][player] == SwitchIds::DiplomacyState::Enemy);
	}

	bool IsLocal(const CGuiManager& gui)
	{

		return gui.currentScreen == SwitchIds::MainMenuScreen::Skirmish;
	}

	const char* MenuText(const char* english, const char* german)
	{

		return NC2Logic::LanguageTool_GetCurrentLanguage() == 0 ? german : english;
	}

	const char* DifficultyName(std::uint32_t difficulty)
	{

		return difficulty == 0 ? MenuText("Normal", "Normal") : difficulty == 1 ? MenuText("Easy", "Leicht")
		                                                                        : MenuText("Very easy", "Sehr leicht");
	}

	bool HasTutorialMaps()
	{
		if (NC2Logic::gCleanMapsManager == nullptr)
			return false;

		NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);

		for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
		{
			if (map->hiddenFromSelection == 0 && (map->mapTypes & 2) != 0 && map->campaignId == 100)
				return true;
		}

		return false;
	}

	bool IsSkirmishMap(const NC2Logic::SCleanMapInfoData& map)
	{

		return map.hiddenFromSelection == 0 && (map.multiplayerOnly != 0 || (map.mapTypes & ((1u << 4) | (1u << 6))) != 0);
	}

	std::uint32_t Row(std::uint32_t slot)
	{

		return NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;
	}

	bool Playable(std::uint32_t tribe)
	{

		return (tribe >= 1 && tribe <= 4) || (tribe == 7 && NRemaster::gGameVariant != NRemaster::EGameVariant::Cultures2);
	}

	bool Customizable(const CGuiManager& gui, std::uint32_t slot)
	{
		if (slot >= 20 || Read<std::uint32_t>(gui.lobbyData, 0) != 1)
			return false;

		const auto flags = Read<std::uint32_t>(&gui, Row(slot) + 8);

		return (flags & 2) != 0 && (flags & 0x80000000u) == 0 && Playable(GetChoices().mapTribes[slot]);
	}

	std::uint32_t Allowed(const CGuiManager& gui, std::uint32_t slot)
	{
		if (slot >= 20)
			return 0;

		const auto flags = Read<std::uint32_t>(&gui, Row(slot) + 8);

		if ((flags & 0x80000000u) != 0)
			return 0;

		return (flags & 7u) | (Customizable(gui, slot) ? 4u : 0u);
	}

	bool CanChooseTribe(const CGuiManager& gui, std::uint32_t slot)
	{
		const auto kind = slot < 20 ? Read<std::uint32_t>(&gui, Row(slot) + 4) : 0;

		return Customizable(gui, slot) && (kind == 2 || (kind == 1 && IsLocal(gui)));
	}

	void ReadMapMetadata(NXBasics::CIniFileReader& reader)
	{
		if (!reader.SearchGroup("playerdata"))
		{

			return;
		}

		while (reader.GetNextCommandLineInGroup() != nullptr)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(reader.GetCurrentName(), "player") == 0)
			{
				const auto slot = static_cast<std::uint32_t>(reader.GetInt());
				const auto type = reader.GetInt();
				const auto tribe = static_cast<std::uint32_t>(reader.GetInt());
				const auto color = static_cast<std::uint32_t>(reader.GetInt());

				if (slot < 20 && (type == 1 || type == 2) && tribe > 0 && tribe <= 7 && color < 10)
				{
					SetMapTribe(slot, tribe);
					mapPlayerColors[slot] = color;
				}
			}
			else if (NXBasics::Runtime_CompareStringsIgnoreCase(reader.GetCurrentName(), "diplomacy") == 0)
			{
				const auto player = static_cast<std::uint32_t>(reader.GetInt());
				const auto other = static_cast<std::uint32_t>(reader.GetInt());
				const auto state = static_cast<std::uint32_t>(reader.GetInt());

				// Match the Windows constructor's bounds, active-player check and line order.

				if (player < 20 && other < 20 && state >= 1 && state <= 3 && playerChoices.mapTribes[player] != 0 && playerChoices.mapTribes[other] != 0)
					mapDiplomacy[player][other] = static_cast<std::uint8_t>(state);
			}
		}
	}

	void AutoAssignPlayers(CGuiManager& gui)
	{
		if (IsLocal(gui))
		{
			Skirmish::AutoAssignPlayers(gui);
		}
		else
		{
			NNetwork::Lobby::AutoAssignPlayers(gui);
		}
	}
}
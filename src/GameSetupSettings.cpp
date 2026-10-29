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
		Startup::Reset();
		Menu::Reset();
		Skirmish::ClearPreview();
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
			if (NXBasics::Runtime_CompareStringsIgnoreCase(reader.GetCurrentName(), "player") != 0)
				continue;

			const auto slot = static_cast<std::uint32_t>(reader.GetInt());
			const auto type = reader.GetInt();
			const auto tribe = static_cast<std::uint32_t>(reader.GetInt());
			const auto color = static_cast<std::uint32_t>(reader.GetInt());

			if (slot < 20 && (type == 1 || type == 2) && tribe > 0 && tribe <= 7 && color < 10)
				SetMapTribe(slot, tribe);
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
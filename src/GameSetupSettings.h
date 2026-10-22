#pragma once

#include <array>
#include <cstdint>

namespace NXBasics { class CIniFileReader; }
namespace NC2MainMenuGuiManager { class CGuiManager; }
namespace NC2Logic { struct SCleanMapInfoData; }

namespace NRemaster::GameSetup
{
	// Session-side choices only; this is not a serialized game or GUI object.
	struct SPlayerChoices
	{
		std::array<std::uint32_t, 20> mapTribes{}, tribes{};
	};
	const SPlayerChoices& GetChoices();
	void SetMapTribe(std::uint32_t slot, std::uint32_t tribe);
	void SetSelectedTribe(std::uint32_t slot, std::uint32_t tribe);
	void Reset();
	bool IsLocal(const NC2MainMenuGuiManager::CGuiManager& gui);
	const char* MenuText(const char* english, const char* german);
	const char* DifficultyName(std::uint32_t difficulty);
	bool HasTutorialMaps();
	bool IsSkirmishMap(const NC2Logic::SCleanMapInfoData& map);
	std::uint32_t Row(std::uint32_t slot);
	bool Playable(std::uint32_t tribe);
	bool Customizable(const NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t slot);
	std::uint32_t Allowed(const NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t slot);
	bool CanChooseTribe(const NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t slot);
	void ReadMapMetadata(NXBasics::CIniFileReader& reader);
	void AutoAssignPlayers(NC2MainMenuGuiManager::CGuiManager& gui);
}

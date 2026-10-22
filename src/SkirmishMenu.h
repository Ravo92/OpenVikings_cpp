#pragma once

#include <cstdint>

namespace NC2MainMenuGuiManager { class CGuiManager; }

namespace NRemaster::GameSetup::Skirmish
{
	inline constexpr std::uint32_t ShowSkirmish = 0x7210, LocalBack = 0x7211, StartLocal = 0x7212, LocalDifficulty = 0x7213, LocalSelectMap = 0x7214;
	void ClearPreview();
	void Init(NC2MainMenuGuiManager::CGuiManager& gui);
	void Build(NC2MainMenuGuiManager::CGuiManager& gui, void* owner);
	void SetControl(NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t slot, std::uint32_t kind);
	void AutoAssignPlayers(NC2MainMenuGuiManager::CGuiManager& gui);
	bool HandleCommand(NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t command, std::uint32_t value);
	bool HandleKey(NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t key);
}

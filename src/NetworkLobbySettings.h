#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace NXBasics { class CIniFileReader; struct SRectangle; }
namespace NC2MainMenuGuiManager { class CGuiManager; }
namespace NC2Logic { struct SCleanMapInfoData; struct SGameControlStartupData; }

// Remaster-only lobby configuration; native GUI, startup and save layouts stay fixed.
namespace NNetwork::Lobby
{
	inline constexpr std::uint32_t AddAi = 0x7200, CloseSlot = 0x7201, OpenTribes = 0x7202, SelectTribe = 0x7203, FillAi = 0x7204, ChangePage = 0x7205, CloseDropdown = 0x7206, OpenControl = 0x7207, SelectControl = 0x7208;
	inline constexpr std::size_t MessageSize = 524;
	inline constexpr std::uint32_t ShowSkirmish = 0x7210, LocalBack = 0x7211, StartLocal = 0x7212, LocalDifficulty = 0x7213;
	using Message = std::array<std::uint32_t, MessageSize / 4>;
	static_assert(sizeof(Message) == MessageSize);
	void Reset();
	bool IsLocal(const NC2MainMenuGuiManager::CGuiManager& gui);
	bool IsSkirmishMap(const NC2Logic::SCleanMapInfoData& map);
	const char* MenuText(const char* english, const char* german);
	const char* DifficultyName(std::uint32_t difficulty);
	bool HasTutorialMaps();
	void InitLocal(NC2MainMenuGuiManager::CGuiManager& gui);
	void BuildLocal(NC2MainMenuGuiManager::CGuiManager& gui, void* owner);
	void ReadMapMetadata(NXBasics::CIniFileReader& reader);
	void AutoAssignPlayers(NC2MainMenuGuiManager::CGuiManager& gui);
	void BuildSlots(NC2MainMenuGuiManager::CGuiManager& gui, const NXBasics::SRectangle& rectangle, void* owner);
	bool HandleCommand(NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t command, std::uint32_t value);
	bool HandleDropdownKey(NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t key);
	Message MakeMessage(const NC2MainMenuGuiManager::CGuiManager& gui, bool start);
	bool ReadMessage(NC2MainMenuGuiManager::CGuiManager& gui, const void* data, std::uint32_t bytes, bool& start);
	std::uint32_t ResolveStartupTribe(std::uint32_t player, std::uint32_t originalTribe);
	void ResolveStartupHouse(std::uint32_t player, std::uint32_t& engineType, std::uint32_t& stage, std::uint32_t& point);
	class CStartupScope
	{
	public:
		explicit CStartupScope(const void* mapGuid);
		~CStartupScope();
		CStartupScope(const CStartupScope&) = delete;
		CStartupScope& operator=(const CStartupScope&) = delete;
	};
}

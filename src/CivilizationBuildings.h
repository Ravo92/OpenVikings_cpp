#pragma once

#include <cstdint>

namespace NC2Logic { class CIoHelper; }

namespace NRemaster::CivilizationBuildings
{
	inline constexpr std::uint32_t SaveChunk = 0x6862776Eu; // nwbh: optional session building-profile manifest; retain its existing identifier.
	void Begin();
	void End();
	bool IsActive();
	bool IsExpandedTribe(std::uint32_t tribe);
	bool IsExpandedPlayer(std::uint32_t player);
	std::int32_t GetHouseId(std::uint32_t tribe, std::uint32_t logic, std::uint32_t* stage);
	void WriteSaveManifest(NC2Logic::CIoHelper* io);
	void ReadSaveManifest(NC2Logic::CIoHelper* io);
}

#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SGameControlStartupPlayerStorage
	{
		std::uint32_t type, colorId, networkPlayerId;
	};

	struct alignas(4) SGameControlStartupData
	{
		SGameControlStartupData();
		void Reset();

		// GUIDs remain fixed byte arrays so no member constructor changes the native reset sequence.
		std::uint32_t mMode;
		std::byte mSingleMapGuid[16];
		std::uint32_t mSavegameId;
		std::byte mMultiplayerMapGuid[16];
		std::uint32_t mLocalNetworkPlayerId;
		SGameControlStartupPlayerStorage mPlayers[20];
		std::uint32_t mDifficultyLevel;
	};

	static_assert(sizeof(SGameControlStartupData) == 288);
	static_assert(alignof(SGameControlStartupData) == 4);
}
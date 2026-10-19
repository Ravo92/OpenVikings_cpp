#pragma once

#include <cstdint>

// Remaster selection; original object layouts and saved data remain unchanged.
namespace NRemaster
{
	enum class EGameVariant
	{
		EighthWonder,
		Cultures2,
		Northland,
		DieSaga
	};

	inline EGameVariant gGameVariant = EGameVariant::EighthWonder;

	inline bool HasDifficultySelection()
	{
		return gGameVariant != EGameVariant::Cultures2;
	}

	inline std::uint32_t GetSoundPlaybackBaseRate()
	{
		// Cultures2.exe requests 44100 Hz on both preload and transient play paths.
		return gGameVariant == EGameVariant::Cultures2 ? 44100u : 22050u;
	}

	inline unsigned int GetFirstAnimalTribe()
	{
		// The native 34-entry animal table starts at tribe 7 in Cultures 2, 8 later.
		// Die Saga keeps the same 8..41 range as Weltwunder.
		return gGameVariant == EGameVariant::Cultures2 ? 7u : 8u;
	}

	inline const char* GetMusicDirectorySuffix()
	{
		return gGameVariant == EGameVariant::Cultures2 ? "\\datax\\dm\\" : "\\datax\\dm2\\";
	}

	inline unsigned int GetGuiStringTableCount()
	{
		// Cultures2.exe 004CCD66 loads eleven tables; Odin/Wonders add two later.
		// Die Saga appends a fourteenth table for its own campaign strings
		// (Game.exe 004E1521 loops while the index is below 14).
		return gGameVariant == EGameVariant::Cultures2 ? 11u : gGameVariant == EGameVariant::DieSaga ? 14u : 13u;
	}

	inline const char* GetGameWindowTitle()
	{
		switch (gGameVariant)
		{
			case EGameVariant::Cultures2: return "OpenVikings - Cultures 2";
			case EGameVariant::Northland: return "OpenVikings - Northland";
			case EGameVariant::DieSaga: return "OpenVikings - Die Saga";
			default: return "Weltwunder";
		}
	}

	inline const char* GetGameMutexName()
	{
		switch (gGameVariant)
		{
			case EGameVariant::Cultures2: return "OpenVikings.Cultures2";
			case EGameVariant::Northland: return "OpenVikings.Northland";
			// Original: 0047A0D1 receives "saga" from 004FE320; Weltwunder uses "weltwunder".
			case EGameVariant::DieSaga: return "saga";
			default: return "weltwunder";
		}
	}

	// Original: 004FE2F4 "weltwunder" (Weltwunder) and 004FE320 "saga" (Die Saga);
	// both use "autorun.exe" as the disc marker file.
	inline const char* GetGameCDVolumeName()
	{
		return gGameVariant == EGameVariant::DieSaga ? "saga" : "weltwunder";
	}

	inline bool UsesVertexColors()
	{
		// Weltwunder 00480E39 and Die Saga 00480D35 both append the vertex-color load;
		// Cultures 2 ends the palette constructor before it.
		// Northland is gated before engine startup until its loading is established.
		return gGameVariant == EGameVariant::EighthWonder || gGameVariant == EGameVariant::DieSaga;
	}

	// Original layout: the campaign screens are not one screen but three sibling handlers,
	// each pinned to one campaign id and its own node table:
	//   screen 5  004D3167 cmp ecx,2  nodes 005089F8  (Nordland campaign)
	//   screen 6  004D32F2 cmp ecx,3  nodes 00508A68  (Wunder campaign)
	//   screen 7  004D347D cmp ecx,4  nodes 00508AD8  (Saga campaign)
	// Die Saga keeps the same three screens, so the id follows the screen number for every
	// variant that ships more than one campaign.
	//
	// Die Saga additionally offers user campaigns:
	//   screen 8  lists campaign00 / campaign01 (saga table strings 16..19)
	//   screen 9  004D380C test byte [map+120h], 80h   -> campaign 7 (campaign00)
	//   screen 10 004D39C4 test byte [map+120h], 100h  -> campaign 8 (campaign01)
	inline bool HasMultipleCampaigns()
	{
		return gGameVariant == EGameVariant::DieSaga;
	}

	// Die Saga presents campaigns 2/3/4 on its three screens. Other profiles retain
	// the existing single campaign 3 (Weltwunder 004D4624 cmp ecx,3).
	inline std::uint32_t GetCampaignIdForCampaignScreen(std::uint32_t campaignScreenIndex)
	{
		// Index 0 = first campaign screen, 1 = second, 2 = third.
		if (!HasMultipleCampaigns()) return campaignScreenIndex == 0u ? 3u : 0u;
		return campaignScreenIndex < 3u ? campaignScreenIndex + 2u : 0u;
	}

	// User campaign packages: the first user campaign screen lists campaign00 (id 7)
	// and the second lists campaign01 (id 8); both are chosen on the user campaign menu.
	inline std::uint32_t GetUserCampaignIdForUserScreen(std::uint32_t userScreenIndex)
	{
		return userScreenIndex < 2u ? userScreenIndex + 7u : 0u;
	}

	// Engine screen numbers for the three campaign screens and the user campaign screens.
	// These mirror SwitchIds::MainMenuScreen; the profile owns the grouping so the drawing
	// and menu code do not repeat the numbers.
	inline bool IsCampaignMapScreen(std::uint32_t engineScreen)
	{
		return engineScreen == 5u || engineScreen == 24u || engineScreen == 25u;
	}

	// 0 for the first campaign screen, 1 for the second, 2 for the third.
	inline std::uint32_t GetCampaignScreenIndex(std::uint32_t engineScreen)
	{
		return engineScreen == 5u ? 0u : engineScreen == 24u ? 1u : engineScreen == 25u ? 2u : 0u;
	}

	inline bool IsUserCampaignScreen(std::uint32_t engineScreen)
	{
		return engineScreen == 26u || engineScreen == 27u || engineScreen == 28u;
	}
}

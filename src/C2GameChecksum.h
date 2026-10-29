#pragma once
#include <cstdint>

namespace NC2Logic
{
	struct SGameChecksum
	{
		std::uint32_t checksum;
		std::uint32_t components[42];
	};

	static_assert(sizeof(SGameChecksum) == 172);
	std::uint32_t Game_Checksum_Calculate();
	void Game_Checksum_Calculate(SGameChecksum* output);
	std::uint32_t __fastcall GameControl_Checksum_Calculate(void* control, void* unusedEdx);
	std::uint32_t __fastcall Array_Checksum_Calculate(void* array, void* unusedEdx);
	std::uint32_t __fastcall PlayerStatisticManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	std::uint32_t __fastcall EmbeddedSavegameManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	std::uint32_t __fastcall HappinessManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	std::uint32_t __fastcall TributeManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	std::uint32_t __fastcall MissionOnscreenInfoManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	std::uint32_t __fastcall GuideManager_Checksum_Calculate(void* manager, void* unusedEdx);
	std::uint32_t __fastcall MissionManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	std::uint32_t __fastcall AIManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	std::uint32_t __fastcall AIHighLevelHandler_Checksum_Calculate(const void* handler, void* unusedEdx);
	std::uint32_t __fastcall AIGuideManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	std::uint32_t __fastcall GameAssistantManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	void __fastcall GameMap_Checksum_Calculate(void* map, void* unusedEdx, SGameChecksum* output);
	std::uint32_t MapWalkSectorManager_Checksum_Calculate();
	std::uint32_t __fastcall GameMapSectorAttachmentManager_Checksum_Calculate(const void* manager, void* unusedEdx);
	std::uint32_t __fastcall MapFishManager_Checksum_Calculate(const void* manager, void* unusedEdx);
}
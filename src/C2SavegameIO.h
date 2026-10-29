#pragma once

#include <cstdint>

namespace NC2Logic
{
	class CIoHelper;
	void GameIO_Savegame_Save(const char* filename, const char* info, std::int32_t slotId);
	void GameIO_Savegame_Load(const char* filename, bool network);
	void System_RestartCurrentMap();
	bool SubMap_StartSubMap(std::uint32_t campaignId, std::uint32_t mapId, std::uint32_t pathfinderLimitOverride);
	bool SubMap_BackToParent();
	struct SSavegameInfoHeaderData;
	std::int32_t GameIO_Savegame_GetInfoHeader(const char* filename, SSavegameInfoHeaderData* header);
	std::int32_t l_GameIO_Savegame_DoLoad_CheckHeaderAndGetInfos(CIoHelper& io, SSavegameInfoHeaderData* header);
}
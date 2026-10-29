#pragma once
#include <cstdint>

namespace NC2Logic
{
	std::uint32_t WallBuildManager_Checksum_Calculate(const void* manager);
	std::uint32_t RoadBuildManager_Checksum_Calculate(const void* manager);
	void* RoadBuildManager_GetRoadBuildDataPtr(void* manager, std::int32_t player);
	void* RoadBuildManager_GetRoadBuildDataPtr(void* manager, std::int32_t player, const void* point);
	bool RoadBuildManager_l_IsRoadBuildFlagSetForAnyPlayer(const void* manager, const void* point);
	void __fastcall WallBuildManager_SetWallBuildFlag(void* manager, void* unusedEdx, std::uint32_t player, const void* point);
	bool WallBuildManager_ClearWallBuildFlag(void* manager, std::int32_t player, const void* point);
	void __fastcall WallBuildManager_ClearWallBuildFlagArea(void* manager, void* unusedEdx, std::uint32_t player, const void* center, std::uint32_t radius);
	bool WallBuildManager_FinishWall(void* manager, std::int32_t player, const void* point);
	void WallBuildManager_ClearWallBuildFlagForAnyPlayer(void* manager, const void* point);
	void __fastcall WallBuildManager_DoOneGameSecond(void* manager, void* unusedEdx);
	void __fastcall RoadBuildManager_SetRoadBuildFlag(void* manager, void* unusedEdx, std::uint32_t player, const void* point);
	bool RoadBuildManager_ClearRoadBuildFlag(void* manager, std::int32_t player, const void* point);
	void __fastcall RoadBuildManager_ClearRoadBuildFlagArea(void* manager, void* unusedEdx, std::uint32_t player, const void* center, std::uint32_t radius);
	bool RoadBuildManager_FinishRoad(void* manager, std::int32_t player, const void* point);
	void RoadBuildManager_ClearRoadBuildFlagForAnyPlayer(void* manager, const void* point);
	void __fastcall RoadBuildManager_DoOneGameSecond(void* manager, void* unusedEdx);
	bool MapTool_WallGate_IsPossibleMapPosition(std::uint32_t player, const void* point);
	bool ls_MapTool_WallGate_IsPossibleMapPosition_GatePossible(std::uint32_t player, const void* point, std::uint32_t direction);
	void MapTool_WallGate_Remove(std::uint32_t player, std::uint32_t point, std::uint32_t radius);
	void MapTool_WallGate_SetIntoWall(std::uint32_t player, const void* point);
	void MapTool_WallGate_SetState(std::uint32_t player, const void* point, bool closed);
}
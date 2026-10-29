#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	class CMapPathfinderSpecialWallRoadElement
	{
	  public:
		CMapPathfinderSpecialWallRoadElement();
		void StartPathfinder(std::uint32_t playerId, bool road, const void* source, const void* target, std::uint32_t radius);
		std::int32_t GetNextDirection();
		bool l_FindWayAndBuildVectorList(const void* source, const void* target, std::uint32_t radius);
		bool l_FindWayWithLine(const void* source, const void* target, std::uint32_t radius);
		bool l_FindWayDirected(const void* source, const void* target, std::uint32_t radius);
		void l_GetPreferedDirections(const void* source, const void* target);
		void l_GetPreferedDirectionsAndDistance(const void* source, const void* target);
		bool l_IsPointOkForBuilding(std::uint32_t point, std::uint32_t direction) const;
		static bool IsPointOkForBuilding(std::uint32_t playerId, bool road, const void* point);
		static void SearchOverMap(std::uint32_t playerId, bool road, const void* source, std::uint32_t radius);

		std::uint32_t player;
		bool road;
		std::byte padding05[3];
		std::uint32_t radius;
		std::uint32_t state;
		std::uint32_t preferred[2];
		std::uint32_t distance[2];
		std::uint32_t source;
		std::uint32_t target;
		std::uint32_t vectorIndex;

		struct SVector
		{
			std::uint8_t direction, amount;
		} vectors[20];
	};

	static_assert(sizeof(CMapPathfinderSpecialWallRoadElement) == 84);
	static_assert(offsetof(CMapPathfinderSpecialWallRoadElement, source) == 0x20);
	static_assert(offsetof(CMapPathfinderSpecialWallRoadElement, vectors) == 0x2C);

	void Tool_RoadBuildManager_MarkPossibleEndPoints(std::uint32_t player, const void* point);
	void Tool_RoadBuildManager_GetPossibleVectorList(std::uint32_t player, void* path, const void* source, const void* target);
	void MapTool_Wall_Build_MarkPossibleEndPoints(std::uint32_t player, const void* point);
	void MapTool_Wall_Build_GetPossibleVectorList(std::uint32_t player, void* path, const void* source, const void* target);
	std::int32_t __fastcall MapPathfinderSpecialWallRoadElement_GetNextDirection(void* path, void* unusedEdx);
}
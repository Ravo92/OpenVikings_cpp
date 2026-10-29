#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Complete physical Windows pathfinder record; runtime byte access remains unchanged.
	struct SMapPathfinderDirectionRunStorage
	{
		std::uint8_t direction, count;
	};

	struct SMapPathfinderElementStorage
	{
		std::uint8_t allocated;
		std::byte padding01[3];
		std::uint32_t searchState, targetMode, searchRadius, requiredClearance;
		std::uint8_t useResistance, useSectors, allowVerticalDirections, useGuides, deterministicDirectionChoice;
		std::byte padding19[3];
		std::int32_t sourceGuideId;
		std::uint32_t sourceGuideRevision;
		std::int32_t currentGuideId;
		std::uint32_t currentGuideRevision;
		std::int32_t playerId;
		std::uint32_t preferredDirections[2], preferredDirectionWeights[2], currentPosition, destinationPosition;
		std::int32_t targetMoveableId;
		std::uint32_t targetObjectRevision;
		std::int32_t targetDistance;
		std::uint32_t directionRunIndex, blockedAreaSteps;
		SMapPathfinderDirectionRunStorage directionRuns[20];
	};

	struct SMapPathfinderPointQueue
	{
		std::uint32_t count;
		std::uint32_t points[4000];
	};

	static_assert(sizeof(SMapPathfinderPointQueue) == 16004);
	static_assert(offsetof(SMapPathfinderPointQueue, points) == 4);

	// Complete shared Windows region 00512620..00550E5F; Mac: sMapPathfinderStaticVariables.
	inline SMapPathfinderPointQueue sMapPathfinderStaticVariables[16]{ };
	static_assert(sizeof(sMapPathfinderStaticVariables) == 256064);

	struct SMapPathfinderManagerSearchOverMapCallbackData
	{
		std::uint32_t distance;
		void* userData;
	};

	static_assert(sizeof(SMapPathfinderManagerSearchOverMapCallbackData) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SMapPathfinderManagerSearchOverMapCallbackData, userData) == sizeof(void*));
	using MapSearchCallback = bool(__cdecl*)(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data);

	void MapPathfinderElement_StartPathfinder(void* pathfinder, const void* source, const void* destination, std::uint32_t radius, bool useResistance, bool useSectors, bool useGuides, bool deterministic);
	void MapPathfinderElement_StartPathfinder(void* pathfinder, const void* source, std::int32_t moveableId, std::int32_t distance, std::uint32_t radius, bool useResistance, bool useSectors, bool useGuides, bool deterministic);
	void MapPathfinderElement_StopPathfinder(void* pathfinder);
	std::int32_t MapPathfinderElement_GetNextDirection(void* pathfinder);
	bool MapPathfinderElement_l_FindWayAndBuildVectorList(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
	bool MapPathfinderElement_l_FindWayWithLine(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
	bool MapPathfinderElement_l_CheckOtherDirection(void* pathfinder, std::uint32_t direction, void* point, const void* microPoint, std::uint32_t remaining);
	bool MapPathfinderElement_l_VehicleFindWayWithLine(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
	bool MapPathfinderElement_l_VehicleCheckOtherDirection(void* pathfinder, std::uint32_t direction, void* point, std::uint32_t remaining);
	void MapPathfinderElement_l_MakeMoveableArea(void* pathfinder, const void* point);
	void MapPathfinderElement_l_GetPreferedDirections(void* pathfinder, const void* source, const void* destination);
	void MapPathfinderElement_l_GetPreferedDirectionsAndDistance(void* pathfinder, const void* source, const void* destination);
	bool MapPathfinderElement_l_FindWayDirected(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
	bool MapPathfinderElement_l_VehicleFindWayDirected(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
	bool MapPathfinderElement_l_FindWayDirectedWithResitance(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
	bool MapPathfinderElement_l_VehicleFindWayDirectedWithResitance(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);

	class CMapPathfinderElement
	{
	  public:
		static bool SearchOverMap(const void* point, std::uint32_t radius, bool skipBlocked, MapSearchCallback callback, void* userData, bool useWalkCosts, std::int32_t playerId);
	};
}
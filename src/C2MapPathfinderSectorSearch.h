#pragma once
#include <cstdint>

namespace NC2Logic
{
	class CMapPathfinderSpecialSectorSearchElement
	{
	  public:
		static void ConstructMembers(void* pathfinder);
		static void Init(void* pathfinder);
		static void StartPathfinder(void* pathfinder, const void* source, const void* destination, std::uint32_t radius, bool useResistance, const std::int16_t* rectangle);
		static std::int32_t GetNextDirection(void* pathfinder);
		static bool l_FindWayAndBuildVectorList(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
		static void l_UnblockArea(void* pathfinder, const void* point);
		static void l_GetPreferedDirections(void* pathfinder, const void* source, const void* destination);
		static bool l_FindWayDirected(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
		static bool l_VehicleFindWayDirected(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
		static bool l_FindWayDirectedWithResitance(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
		static bool l_VehicleFindWayDirectedWithResitance(void* pathfinder, const void* source, const void* destination, std::uint32_t radius);
	};
}
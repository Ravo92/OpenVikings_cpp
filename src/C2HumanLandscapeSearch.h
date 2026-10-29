#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SHumanSearchLandscapeEntry
	{
		std::uint8_t enabled;
		std::byte padding01[3];
		std::uint32_t landscape;
		std::uint32_t minimumValency;
		std::uint32_t count;
		std::uint32_t points[10];
		void CopyCandidatesFrom(const SHumanSearchLandscapeEntry& source);
	};

	struct SHumanSearchLandscapesData
	{
		SHumanSearchLandscapesData();
		void Add(std::uint32_t landscape);
		void AddBio(std::uint32_t landscape);
		void SetCurrent(std::uint32_t landscape);
		std::int32_t firstDistance;
		std::uint32_t typeCount;
		std::uint8_t scanned;
		std::byte padding09[3];
		std::uint32_t candidateCount;
		std::uint32_t selectedCount;
		SHumanSearchLandscapeEntry entries[50];
	};

	static_assert(sizeof(SHumanSearchLandscapeEntry) == 56);
	static_assert(offsetof(SHumanSearchLandscapeEntry, points) == 0x10);
	static_assert(sizeof(SHumanSearchLandscapesData) == 2820);
	static_assert(offsetof(SHumanSearchLandscapesData, entries) == 0x14);
	bool Human_FindLandscapesNearPos_Sorted(const void* human, SHumanSearchLandscapesData& data, void* real, void* work, const void* source);
	bool Human_FindLandscapesNearPos_Random(const void* human, SHumanSearchLandscapesData& data, void* real, void* work, const void* source);
	bool Human_FindBioLandscapeNearWorkCenter(const void* human, std::uint32_t landscape, void* real, void* work);
	bool Human_FindLandscapeNearWorkCenter(const void* human, std::uint32_t landscape, void* real, void* work);
	bool Human_FindGrowPatternNearWorkCenter(const void* human, void* real, void* work);
	bool Human_FindGrowLandscapeNearWorkCenter(const void* human, std::uint32_t landscape, void* real, void* work);
	bool Human_FindShorePointForFishing(const void* human, void* real, void* work);
	bool Human_FindPilePointNearPosition(const void* human, std::int32_t engineId, const void* source, void* point);
	bool Human_FindLandscapesNearPos_NoBlock(const void* human, SHumanSearchLandscapesData& data, void* real, void* work, const void* source);
}
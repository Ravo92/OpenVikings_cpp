#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Physical Windows records; legacy Resource() views start four bytes before these points.
	struct SAIResourcePointStorage
	{
		std::uint32_t point;
		std::uint8_t updated;
		std::uint8_t onMap;
		std::byte unresolved06[2];
		std::uint32_t goodType;
		std::uint32_t minimumValency;
		std::uint32_t storedLandscapeType;
	};

	struct SAIResourceGoodStorage
	{
		std::uint32_t type;
		std::uint32_t workerJob;
		std::uint32_t houseType;
		std::uint8_t checkTerrain;
		std::byte unresolved0D[3];
	};

	struct SAIHouseExportStorage
	{
		std::uint32_t count;
		std::uint32_t houseTypes[5];
	};

	struct SAIResourceManagerStorage
	{
		std::uint32_t playerId;
		SAIResourcePointStorage resourcePoints[20];
		std::int32_t landscapeResourceIndices[88];
		std::uint32_t resourceCount;
		std::int32_t scanIndex;
		SAIResourceGoodStorage goodRequirements[66];
		std::uint32_t houseGoods[55];
		SAIHouseExportStorage houseExports[55];
	};

	struct SAIResourceSearchStorage
	{
		const void* villages;
		const void* resourcePoints;
		std::int32_t resourceIndex;
		std::uint32_t continentId;
		std::int32_t bestVillageId;
		std::int32_t bestScore;
		std::int32_t playerId;
		std::uint32_t bestPoint;
	};

	static_assert(sizeof(SAIResourceSearchStorage) == (sizeof(void*) == 8 ? 40 : 32));
	static_assert(offsetof(SAIResourceSearchStorage, resourcePoints) == sizeof(void*));
	static_assert(offsetof(SAIResourceSearchStorage, resourceIndex) == 2 * sizeof(void*));

	void AIResourceManager_Init(void* manager, std::uint32_t player, const void* villages);
	void AIResourceManager_UpdateResourcePointArray(void* manager, std::uint32_t player, const void* villages);
	bool AIResourceManager_l_CheckOldResourceArea(void* manager, std::int32_t index);
	void AIResourceManager_l_UpdateResourcePoint(void* manager, std::uint32_t player, std::int32_t index, const void* villages);
	void* AIResourceSearchTool_Construct(void* search, const void* villages, const void* entries, std::int32_t index);
	bool AIResourceSearchTool_IsCandidate(const void* search, const void* point);
	void AIResourceSearchTool_CheckAndSetData(void* search, const void* point);
	bool AIResourceManager_GetResourcePosition(const void* manager, std::uint32_t good, void* point);
	void AIResourceManager_UpdateGoodResource(void* manager, std::uint32_t good, const void* invalidAreas, const void* villages);
	std::uint8_t AIResourceManager_WasResourceDataUpdated(const void* manager, std::uint32_t good);
	std::uint8_t AIResourceManager_IsResourceOnMap(const void* manager, std::uint32_t good);
	void AIResourceManager_ClearResourceUpdateFlags(void* manager);
	bool AIResourceManager_IsExportingHouseType(const void* manager, std::uint32_t house, std::uint32_t exportedHouse);
	bool AIInvalidAreaManager_AddArea(void* manager, const void* area);
	bool AIInvalidAreaManager_IsInsideAnyArea(const void* manager, const void* point);
	std::int32_t AIVillagesData_GetVillageId(const void* villages, const void* point);
	void* AIVillagesData_GetVillageDataById(void* villages, std::int32_t id);
	const void* AIVillagesData_GetConstVillageDataById(const void* villages, std::int32_t id);
}
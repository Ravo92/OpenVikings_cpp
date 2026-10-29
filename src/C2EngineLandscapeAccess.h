#pragma once

#include <cstdint>
#include <cstddef>

namespace NC2Logic
{
	class CMapArea;
}

namespace NC2E2
{
	struct SMiscHouseLandscapeLinkStorage
	{
		std::uint32_t tribeType, logicHouseType, choiceCount, totalWeight;
		std::byte* choices;
	};

	static_assert(sizeof(SMiscHouseLandscapeLinkStorage) == (sizeof(void*) == 8 ? 24 : 20));
	const void* Engine2d_EntryPoint_GetHouseDataPtr(std::uint32_t type);
	bool EngineMap_MapMicroPoint_Landscape_SetType(const void* point, std::int32_t engineId, std::uint32_t valency, bool preserveWalkingBlock, std::int32_t player);
	bool Engine2d_EntryPoint_Edit_SetMapMicroPointLandscape(const void* point, std::int32_t engineId, std::uint32_t valency, bool preserveWalkingBlock, std::int32_t player);
	std::int32_t Engine2d_EntryPoint_GetLandscapeIdByNamePtr(const char* name);
	void* __fastcall E2StaticDataMiscLandscapeManager_Construct(void* manager, void* unusedEdx);
}

namespace NC2LogicExternals
{
	const NC2Logic::CMapArea* External_Landscape_GetAreaPtr(const void* point, std::uint32_t areaKind);
	bool External_Landscape_SetMapMicroPointLandscape(const void* point, std::int32_t engineId, std::uint32_t valency, bool preserveWalkingBlock, std::int32_t player);
	std::int32_t External_Landscape_GetLandscapeIdByNamePtr(const char* name);
	std::int32_t External_Misc_GetEngineDepotLandscapeId(std::uint32_t tribe, std::uint32_t type);
	std::int32_t External_Misc_GetEngineCadaverLandscapeId(std::uint32_t tribe, std::uint32_t type);
	std::int32_t External_HouseType_GetEngineRuineLandscapeId(std::uint32_t tribe, std::uint32_t type);
	std::uint32_t External_Landscape_GetMaximumValency(std::int32_t engineId);
	std::int32_t External_Landscape_GetTransitionTarget(std::int32_t engineId, std::uint32_t transition);
	void External_Landscape_Clear(const void* point);
	void External_Landscape_InternallySetData(const void* point, std::int32_t engineId);
	const void* External_Landscape_GetLogicDataPtr(std::int32_t engineId);
	std::int32_t External_Misc_GetEngineWallElementLandscapeId();
	std::int32_t External_Misc_GetEngineWallGateLandscapeId(std::uint32_t gate, bool open);
	const NC2Logic::CMapArea* External_Landscape_GetAreaPtr(std::int32_t engineId, std::uint32_t valency, std::uint32_t areaKind);
	std::int32_t External_Landscape_GetEngineIdOfMapLandscape(const void* packedMigPoint);
	std::int32_t External_Landscape_GetEngineIdOfMapLandscape(std::int32_t landscapeType, bool requireMapLandscape);
}
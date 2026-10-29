#pragma once

#include "XBTokenList.h"

#include <cstddef>
#include <cstdint>

namespace NC2E2
{
	std::int32_t StaticDataHouses_GetHouseId(const char* name);
	std::int32_t StaticDataHouses_GetNextHouseId(std::int32_t previous, std::uint32_t tribe, std::uint32_t logicType, std::uint32_t* stage);

	// Complete Windows singleton region 00568A58..00568A6F.
	struct SStaticHouseStorage
	{
		const NXBasics::SToken* tokens;
		std::uint32_t unresolved04;
		std::int32_t count;
		void* entries;
		std::uint8_t tokenFlags;
		std::byte padding11[3];
		void* owner;
	};

	inline SStaticHouseStorage gStaticHouseStorage{ };
	static_assert(sizeof(SStaticHouseStorage) == (sizeof(void*) == 8 ? 40 : 24));
	static_assert(offsetof(SStaticHouseStorage, count) == (sizeof(void*) == 8 ? 12 : 8));
	static_assert(offsetof(SStaticHouseStorage, entries) == (sizeof(void*) == 8 ? 16 : 0x0C));
	static_assert(offsetof(SStaticHouseStorage, tokenFlags) == (sizeof(void*) == 8 ? 24 : 0x10));
	static_assert(offsetof(SStaticHouseStorage, owner) == (sizeof(void*) == 8 ? 32 : 0x14));

	// Physical Windows house graphics records; managers and palettes remain borrowed.
	struct SHousePointArrayStorage
	{
		std::uint32_t count;
		void* points;
	};

	struct SHouseStagePointsStorage
	{
		SHousePointArrayStorage fire, forwardSmoke, backSmoke, holyFire;
		std::int32_t flagX, flagY;
		std::uint8_t soldierFlagEnabled;
		std::byte padding29[3];
		std::int32_t soldierFlagX, soldierFlagY;
	};

	struct SHouseDoorOverlayStorage
	{
		std::uint8_t enabled;
		std::byte padding01[3];
		std::uint32_t openBob, closedBob;
	};

	struct SHouseConstructionLayerStorage
	{
		std::uint8_t upgrade;
		std::byte padding01[3];
		std::uint32_t bob, shadowBob;
		std::int32_t firstPercentage, lastPercentage;
	};

	struct SHouseConstructionLayersStorage
	{
		std::uint32_t count;
		SHouseConstructionLayerStorage layers[5];
	};

	struct SHouseActionOverlayStorage
	{
		std::uint32_t developmentStage, action;
		std::uint8_t requiredActive;
		std::byte padding09[3];
		std::int32_t offsetX, offsetY;
		std::uint32_t frameTicks, frameCount, bobIds[50];
	};

	struct SHouseLandscapeOverlayStorage
	{
		std::uint8_t enabled;
		std::byte padding01[3];
		std::int32_t offsetX, offsetY;
		std::uint32_t landscapeId;
	};

	struct SHouseConstructionGoodsStorage
	{
		std::uint32_t count, goods[15];
	};

	struct SHouseArrowSlotStorage
	{
		std::int32_t x, y;
		std::uint32_t parameter;
	};

	struct SEngineHouseTypeStorage
	{
		char editName[30], editGroups[30];
		std::uint32_t logicTypes[5], tribeType, developmentStageCount;
		void* buildArea;
		void* walkAreas[5];
		void* workAreas[5];
		SHouseConstructionGoodsStorage constructionGoods[5];
		std::uint32_t doorPoints[5], maximumHitpoints[5], arrowSlotCount;
		SHouseArrowSlotStorage arrowSlots[3];
		void* bobManager;
		void* shadowBobManager;
		std::uint32_t paletteCount, paletteIds[5];
		std::int32_t shadingFactor256;
		std::uint32_t stageBobIds[5];
		SHouseDoorOverlayStorage doorOverlays[5];
		SHouseConstructionLayersStorage constructionLayers[5];
		std::uint32_t actionOverlayCount;
		void* actionOverlays;
		SHouseLandscapeOverlayStorage landscapeOverlays[5][10];
		SHouseStagePointsStorage stagePoints[5];
	};

	static_assert(sizeof(SEngineHouseTypeStorage) == (sizeof(void*) == 8 ? 2472 : 2236));
	static_assert(sizeof(SHousePointArrayStorage) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(sizeof(SHouseStagePointsStorage) == (sizeof(void*) == 8 ? 88 : 52));
	void* __fastcall E2StaticDataHousesManager_Construct(void* manager, void* unusedEdx);
}
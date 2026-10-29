#pragma once

#include "C2E2StaticDataPalettes.h"

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	class CMapArea;
}

namespace NC2E2
{
	struct SE2StaticDataPatternData
	{
		char name[40];
		std::uint32_t groupCount;
		std::int32_t groupIds[3];
		std::int32_t logicType;
		std::int32_t textureId;
		std::int32_t coordinatesA[6];
		std::int32_t coordinatesB[6];
	};

	struct SStaticLandscapeArea
	{
		std::uint32_t valency;
		NC2Logic::CMapArea* area;
	};

	struct SStaticLandscapeAreaList
	{
		std::uint32_t count;
		SStaticLandscapeArea* entries;
	};

	struct SStaticLandscapeTransition
	{
		std::int32_t type;
		std::int32_t landscapeId;
	};

	struct SStaticLandscapeFrames
	{
		std::uint32_t valency;
		std::uint32_t count;
		std::int32_t* frames;
	};

	struct SE2StaticDataLandscape
	{
		char name[40];
		std::uint32_t groupCount;
		std::int32_t groupIds[3];
		std::int32_t logicType;
		std::int32_t maximumValency;
		std::uint8_t isWorkable;
		std::uint8_t isPileableOnMap;
		std::byte padding42[2];
		// Walk blocking, build blocking, work, additional attachment points.
		SStaticLandscapeAreaList areas[4];
		std::uint32_t transitionCount;
		SStaticLandscapeTransition* transitions;
		void* bobManagers[2];
		std::int32_t paletteId;
		// The default landscape draw pass, disabled by static/background rendering.
		std::uint8_t dynamicForeground;
		std::uint8_t isStatic;
		std::uint8_t dynamicBackground;
		std::uint8_t loopAnimation;
		std::uint8_t hasFireTransition;
		std::uint8_t userFxMatrix;
		std::uint8_t drawVoidEver;
		std::byte padding7F;
		std::uint32_t shadingFactor;
		std::uint32_t frameGroupCount;
		SStaticLandscapeFrames* frameGroups;
	};

	// Complete Windows static regions, including every cached special ID.
	struct SStaticPatternStorage
	{
		std::uint32_t count;
		void* entries;
		std::int32_t borderIds[2];
		std::int32_t roadOverlayId;
		std::int32_t houseOverlayId;
	};

	inline SStaticPatternStorage gStaticPatternStorage{ }; // 00568A30..00568A47

	struct STransitionPointType
	{
		char name[40];
		std::uint32_t groupCount;
		std::int32_t groupIds[5];
	};

	struct SE2StaticDataTransition
	{
		char name[40];
		std::int32_t pointTypeId;
		std::int32_t textureId;
		std::int32_t alphaTextureId;
		std::int32_t coordinatesA[6][6];
		std::int32_t coordinatesB[6][6];
	};

	struct SStaticTransitionStorage
	{
		std::int32_t pointTypeCount;
		STransitionPointType* pointTypes;
		std::int32_t count;
		SE2StaticDataTransition* entries;
		std::int32_t roadOverlayIds[2];
		std::int32_t houseOverlayIds[2];
	};

	inline SStaticTransitionStorage gStaticTransitionStorage{ }; // 005689F0..00568A0F
	static_assert(sizeof(STransitionPointType) == 64);
	static_assert(sizeof(SE2StaticDataTransition) == 340);
	static_assert(offsetof(SE2StaticDataTransition, coordinatesA) == 0x34);
	static_assert(offsetof(SE2StaticDataTransition, coordinatesB) == 0xC4);
	static_assert(sizeof(SStaticTransitionStorage) == (sizeof(void*) == 8 ? 48 : 32));
	void* __fastcall E2StaticDataTransitionsManager_Construct(void* manager, void* unusedEdx);
	std::int32_t E2StaticDataTransitionsManager_GetTransitionId(const char* name);
	std::int32_t E2StaticDataTransitionsManager_l_GetPointTypeId(const char* name);

	struct SStaticLandscapeStorage
	{
		std::int32_t count;
		void* entries;
		std::int32_t houseFireIds[5];
		std::int32_t smokeIds[5];
		std::int32_t incenseFireIds[5];
		std::int32_t wallIds[5];
		// Original order: gate 02 open/closed, gate 01 open/closed, gate 03 open/closed.
		std::int32_t gateIds[6];
	};

	inline SStaticLandscapeStorage gStaticLandscapeStorage{ }; // 00568A78..00568AE7

	void* __fastcall E2StaticDataPatternManager_Construct(void* manager, void* unusedEdx);
	std::int32_t E2StaticDataPatternManager_GetPatternId(const char* name);
	void* __fastcall E2StaticDataLandscapesManager_Construct(void* manager, void* unusedEdx);
	std::int32_t E2StaticDataLandscapesManager_GetLandscapeId(const char* name);
	std::int32_t __fastcall E2StaticDataTexturesManager_GetTextureId(void* manager, void* unusedEdx, const char* fileName);

	static_assert(sizeof(SE2StaticDataPatternData) == 112);
	static_assert(offsetof(SE2StaticDataPatternData, groupCount) == 0x28);
	static_assert(offsetof(SE2StaticDataPatternData, textureId) == 0x3C);
	static_assert(offsetof(SE2StaticDataPatternData, coordinatesA) == 0x40);
	static_assert(offsetof(SE2StaticDataPatternData, coordinatesB) == 0x58);
	// Native runtime storage; configuration IDs and coordinate payloads remain fixed-width.
	static_assert(sizeof(SStaticLandscapeTransition) == 8);
	static_assert(sizeof(SStaticLandscapeArea) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SStaticLandscapeArea, valency) == 0);
	static_assert(offsetof(SStaticLandscapeArea, area) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(sizeof(SStaticLandscapeAreaList) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SStaticLandscapeAreaList, count) == 0);
	static_assert(offsetof(SStaticLandscapeAreaList, entries) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(sizeof(SStaticLandscapeFrames) == (sizeof(void*) == 8 ? 16 : 12));
	static_assert(offsetof(SStaticLandscapeFrames, valency) == 0);
	static_assert(offsetof(SStaticLandscapeFrames, count) == 4);
	static_assert(offsetof(SStaticLandscapeFrames, frames) == 8);
	static_assert(sizeof(SE2StaticDataLandscape) == (sizeof(void*) == 8 ? 200 : 140));
	static_assert(offsetof(SE2StaticDataLandscape, name) == 0);
	static_assert(offsetof(SE2StaticDataLandscape, groupCount) == 40);
	static_assert(offsetof(SE2StaticDataLandscape, groupIds) == 44);
	static_assert(offsetof(SE2StaticDataLandscape, logicType) == 56);
	static_assert(offsetof(SE2StaticDataLandscape, maximumValency) == 60);
	static_assert(offsetof(SE2StaticDataLandscape, isWorkable) == 64);
	static_assert(offsetof(SE2StaticDataLandscape, isPileableOnMap) == 65);
	static_assert(offsetof(SE2StaticDataLandscape, padding42) == 66);
	static_assert(offsetof(SE2StaticDataLandscape, areas) == (sizeof(void*) == 8 ? 72 : 68));
	static_assert(offsetof(SE2StaticDataLandscape, transitionCount) == (sizeof(void*) == 8 ? 136 : 100));
	static_assert(offsetof(SE2StaticDataLandscape, transitions) == (sizeof(void*) == 8 ? 144 : 104));
	static_assert(offsetof(SE2StaticDataLandscape, bobManagers) == (sizeof(void*) == 8 ? 152 : 108));
	static_assert(offsetof(SE2StaticDataLandscape, paletteId) == (sizeof(void*) == 8 ? 168 : 116));
	static_assert(offsetof(SE2StaticDataLandscape, dynamicForeground) == (sizeof(void*) == 8 ? 172 : 120));
	static_assert(offsetof(SE2StaticDataLandscape, isStatic) == (sizeof(void*) == 8 ? 173 : 121));
	static_assert(offsetof(SE2StaticDataLandscape, dynamicBackground) == (sizeof(void*) == 8 ? 174 : 122));
	static_assert(offsetof(SE2StaticDataLandscape, loopAnimation) == (sizeof(void*) == 8 ? 175 : 123));
	static_assert(offsetof(SE2StaticDataLandscape, hasFireTransition) == (sizeof(void*) == 8 ? 176 : 124));
	static_assert(offsetof(SE2StaticDataLandscape, userFxMatrix) == (sizeof(void*) == 8 ? 177 : 125));
	static_assert(offsetof(SE2StaticDataLandscape, drawVoidEver) == (sizeof(void*) == 8 ? 178 : 126));
	static_assert(offsetof(SE2StaticDataLandscape, padding7F) == (sizeof(void*) == 8 ? 179 : 127));
	static_assert(offsetof(SE2StaticDataLandscape, shadingFactor) == (sizeof(void*) == 8 ? 180 : 128));
	static_assert(offsetof(SE2StaticDataLandscape, frameGroupCount) == (sizeof(void*) == 8 ? 184 : 132));
	static_assert(offsetof(SE2StaticDataLandscape, frameGroups) == (sizeof(void*) == 8 ? 192 : 136));
	static_assert(sizeof(SStaticPatternStorage) == (sizeof(void*) == 8 ? 32 : 24));
	static_assert(offsetof(SStaticPatternStorage, count) == 0);
	static_assert(offsetof(SStaticPatternStorage, entries) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(offsetof(SStaticPatternStorage, borderIds) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SStaticPatternStorage, roadOverlayId) == (sizeof(void*) == 8 ? 24 : 16));
	static_assert(offsetof(SStaticPatternStorage, houseOverlayId) == (sizeof(void*) == 8 ? 28 : 20));
	static_assert(offsetof(SStaticTransitionStorage, pointTypeCount) == 0);
	static_assert(offsetof(SStaticTransitionStorage, pointTypes) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(offsetof(SStaticTransitionStorage, count) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SStaticTransitionStorage, entries) == (sizeof(void*) == 8 ? 24 : 12));
	static_assert(offsetof(SStaticTransitionStorage, roadOverlayIds) == (sizeof(void*) == 8 ? 32 : 16));
	static_assert(offsetof(SStaticTransitionStorage, houseOverlayIds) == (sizeof(void*) == 8 ? 40 : 24));
	static_assert(sizeof(SStaticLandscapeStorage) == (sizeof(void*) == 8 ? 120 : 112));
	static_assert(offsetof(SStaticLandscapeStorage, count) == 0);
	static_assert(offsetof(SStaticLandscapeStorage, entries) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(offsetof(SStaticLandscapeStorage, houseFireIds) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SStaticLandscapeStorage, smokeIds) == (sizeof(void*) == 8 ? 36 : 28));
	static_assert(offsetof(SStaticLandscapeStorage, incenseFireIds) == (sizeof(void*) == 8 ? 56 : 48));
	static_assert(offsetof(SStaticLandscapeStorage, wallIds) == (sizeof(void*) == 8 ? 76 : 68));
	static_assert(offsetof(SStaticLandscapeStorage, gateIds) == (sizeof(void*) == 8 ? 96 : 88));
}
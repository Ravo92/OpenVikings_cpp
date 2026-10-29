#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"
#pragma once
#include "C2MapPathfinderSectorSearch.h"
#include "C2ObjectMemberConstructors.h"

#include "ApplicationGlobals.h"
#include "C2MapGeometry.h"
#include "C2MapWalkSectorManager.h"
#include "NetworkShutdown.h"
#include "XCrtFileAccess.h"

#include <cstring>

namespace NApplicationStartup
{
	// Original data: 004f4d00 through 004f4d37.
	// Preserve the original binary64 operands, including the truncated pi value.
	// Volatile reads keep the arithmetic in the initialization callbacks.
	inline const volatile double gOriginalMathOperands[7] = {
		3.1415926535,
		0.0006337135614702154,
		0.0030959752321981426,
		0.0008481764206955047,
		0.006134969325153374,
		10874.0,
		7189.0
	};

	// Original range: 004fe000 through 004fe057.
	// One leading null entry followed by 20 application initializers.
	// User-authorized cleanup omits the write-only global initializer at 00471615.
	inline NXBasics::RuntimeVoidCallback gApplicationInitializers[] = {
		nullptr,

		// Original function: FUN_004058dd
		&NNetwork::InitializeGlobalDynamicNetworkData,

		// Original function: FUN_00406b1e
		+[]()
		{
		    std::memset(&NC2Logic::gGameControlStartupData, 0, sizeof(NC2Logic::gGameControlStartupData));
		},

		// Original function: FUN_00406b5b
		+[]()
		{
		    // Original chain: FUN_00406b60 -> FUN_00410c2d -> FUN_00410c3f.
		    // This fixed seed has 13 set bits; neither seed-adjustment branch runs.
		    NC2Logic::gGameRandomState.seed = 0x5432ACE0u;
		    NC2Logic::gGameRandomState.currentState = 0x5432ACE0u;
		},

		// Original function: FUN_0041ae34
		+[]()
		{
		    NC2Logic::ConstructPointMembers(&NC2Logic::gGuideManagerStaticVars, nullptr);
		},

		// Original function: FUN_00423112
		+[]()
		{
		    // Native startup chain: 0042311c calls the map-record member constructor 004231bd.
		    NC2Logic::GameMapStaticVars_ConstructMembers(gGameMapStaticStorage + NC2Layout::GameMapOffset::ContinentCount, nullptr);

		    std::memset(gGameMapStaticStorage + offsetof(NC2Logic::SLogicMapStorage, walkFlagChangedArea), 0, 24);
		    std::memset(gGameMapStaticStorage + offsetof(NC2Logic::SLogicMapStorage, resistanceChangedArea), 0, 16);
		},

		// Original function: FUN_004477ce
		+[]()
		{
		    // Original constructor: FUN_004477d8.

		    for (std::size_t index = 0; index < 16; ++index)
		    {
			    for (auto& point : NC2Logic::sMapPathfinderStaticVariables[index].points)
			    {
				    NC2Logic::MapMigPoint_ConstructZero(&point);
			    }
		    }
		},

		// Original function: FUN_0044f107
		+[]()
		{
		    // Original constructors: FUN_0044f111, FUN_0044f14c and FUN_0042ae54.
		    auto& staticVars = NC2Logic::CMapWalkSectorManager::mStaticVars;
		    auto* workspace = staticVars.specialSectorSearchWorkspace;
		    NC2Logic::CMapPathfinderSpecialSectorSearchElement::ConstructMembers(workspace);

		    for (std::size_t index = 0; index < 250; ++index)
		    {
			    staticVars.basePointCandidates[index].packedBasePoint = 0;
		    }

		    std::memset(staticVars.floodFillPoints, 0, sizeof(staticVars.floodFillPoints));
		},

		// Original function: FUN_0047dca4
		+[]()
		{
		    // Original constructor: FUN_0047dca9.
		    std::memset(gStaticMapModifiedBounds, 0, sizeof(gStaticMapModifiedBounds));
		},

		// Original function: FUN_0047f3a7
		+[]()
		{
		    // Original constructor: FUN_0047f3ac.
		    std::memset(&NC2E2::gAmbientSoundChangeState, 0, 8);
		},

		// Original function: FUN_00482212
		+[]()
		{
		    // Original constructor: FUN_0048221c.
		    std::memset(gEngineMapStaticStorage + NC2Layout::EngineMapStaticOffset::HeightDirtyBounds, 0, NC2Layout::RecordSize::EngineDirtyBoundsPair);
		},

		// Original function: FUN_0048acd4
		+[]()
		{
		    // Original constructors: FUN_0048acde and FUN_0040f7b4.
		    // Each SColorRGB constructor clears only the first four bytes.

		    for (std::size_t index = 0; index < 10; ++index)
		    {
			    std::memset(NC2E2::gStaticHumanColors + index * NC2Layout::RecordSize::StaticHumanColor, 0, 4);
		    }
		},

		// Original function: FUN_00499a63
		+[]()
		{
		    gFishHorizontalMotionFrequency = gOriginalMathOperands[0] * gOriginalMathOperands[1];
		},

		// Original function: FUN_00499a76
		+[]()
		{
		    gFishSecondaryHorizontalMotionFrequency = gOriginalMathOperands[0] * gOriginalMathOperands[2];
		},

		// Original function: FUN_00499a89
		+[]()
		{
		    gFishVerticalMotionFrequency = gOriginalMathOperands[0] * gOriginalMathOperands[3];
		},

		// Original function: FUN_00499a9c
		+[]()
		{
		    gFishSecondaryVerticalMotionFrequency = gOriginalMathOperands[0] * gOriginalMathOperands[4];
		},

		// Original function: FUN_00499aaf
		+[]()
		{
		    gFishScatterXFrequency = gOriginalMathOperands[5] / gOriginalMathOperands[0];
		},

		// Original function: FUN_00499ac2
		+[]()
		{
		    gFishScatterYFrequency = gOriginalMathOperands[6] / gOriginalMathOperands[0];
		},

		// Original function: FUN_004b1b44
		+[]()
		{
		    // Original chain: FUN_004b1b49 -> FUN_00410340.
		    auto& rectangle = NC2InGameGuiMiscWindows::cTabClientButtonRectangle;

		    rectangle.x = 4;
		    rectangle.y = 50;
		    rectangle.width = 124;
		    rectangle.height = 24;
		},

		// Original function: FUN_004b1b5c
		+[]()
		{
		    // Original chain: FUN_004b1b61 -> FUN_00410340.
		    auto& rectangle = NC2InGameGuiMiscWindows::cTimeButtonElementRectangle;

		    rectangle.x = 4;
		    rectangle.y = 400;
		    rectangle.width = 100;
		    rectangle.height = 16;
		},

		// Original function: FUN_004b7f97
		+[]()
		{
		    // Original constructor: FUN_004b7f9c.
		    NC2InGameGuiOverviewDisplay::CWorldOverviewElementHandler::InitializeStaticChangedArea();
		}
	};

	inline constexpr std::size_t ApplicationInitializerCount = sizeof(gApplicationInitializers) / sizeof(gApplicationInitializers[0]);

	static_assert(ApplicationInitializerCount == 21);
}
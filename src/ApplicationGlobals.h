#pragma once

#include "C2ControlStartup.h"
#include "C2GameMapAccess.h"
#include "C2GameMapStorage.h"
#include "C2LifetimeGlobals.h"
#include "C2MapPathfinderSearch.h"
#include "C2MapWalkSectorManager.h"
#include "InGameGuiOverviewHandler.h"
#include "XBasicTypes.h"
#include "DMManager.h"

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Original data: DAT_005106f8.
	// Mac reference: NC2Logic::gGameControlStartupData.
	// Startup input is declared in C2LogicLifetimeGlobals.h.

	// Complete Windows region 00511540..0051154B; CGuideManager::mStaticVars and map-change bounds.
	// Field meanings follow the scan and two-pass map-change callback accesses.
	struct SGuideManagerStaticVars
	{
		std::int32_t scanGuideId;
		std::int16_t modifiedAreaMinimum[2];
		std::int16_t modifiedAreaMaximum[2];
	};

	inline SGuideManagerStaticVars gGuideManagerStaticVars{ };
	static_assert(sizeof(SGuideManagerStaticVars) == 12);
	static_assert(offsetof(SGuideManagerStaticVars, scanGuideId) == 0);
	static_assert(offsetof(SGuideManagerStaticVars, modifiedAreaMinimum) == 4);
	static_assert(offsetof(SGuideManagerStaticVars, modifiedAreaMaximum) == 8);
}

// These byte ranges cover the storage addressed by the startup routines.
// They are not declarations of complete reconstructed game classes.

// Original data: DAT_00511570; the next global begins at 0051257c.
// Game-map dimensions, rectangles, owned map buffers, continent records and static tail.
// The region owner is established; its remaining field layout stays explicit raw storage.
// Native map storage is declared in C2GameMapStorage.h.

// Complete static-display dirty bounds 00554D98..00554D9F: min X/Y, max X/Y.
alignas(4) inline std::byte gStaticMapModifiedBounds[8]{ };

namespace NC2E2
{
	// Complete ambient change region: 00554DA8..00554DB1.
	struct SAmbientSoundChangeState
	{
		std::int16_t first[2], last[2];
		std::uint8_t patternsChanged, landscapesChanged;
	};

	inline SAmbientSoundChangeState gAmbientSoundChangeState{ };
	static_assert(sizeof(SAmbientSoundChangeState) == 10);
}

// Complete engine-map region 00568358..00568373: macro/micro owners, two
// dirty bounds at +0x08/+0x10, missing-resource flag at +0x18 and padding.
alignas(4) inline std::byte gEngineMapStaticStorage[28]{ };

// Original data: DAT_00569078.
inline double gFishScatterYFrequency = 0.0;

// Original data: DAT_00569080.
inline double gFishHorizontalMotionFrequency = 0.0;

// Original data: DAT_00569088.
inline double gFishScatterXFrequency = 0.0;

// Original data: DAT_00569090.
inline double gFishSecondaryVerticalMotionFrequency = 0.0;

// Original data: DAT_00569098.
inline double gFishSecondaryHorizontalMotionFrequency = 0.0;

// Original data: DAT_005690a0.
inline double gFishVerticalMotionFrequency = 0.0;

namespace NC2InGameGuiMiscWindows
{
	// Original data: DAT_00569200.
	// Mac reference: NC2InGameGuiMiscWindows::cTabClientButtonRectangle.
	inline NXBasics::SRectangle cTabClientButtonRectangle(0, 0, 0, 0);

	// Original data: DAT_00569210.
	// Mac reference: NC2InGameGuiMiscWindows::cTimeButtonElementRectangle.
	inline NXBasics::SRectangle cTimeButtonElementRectangle(0, 0, 0, 0);
}

namespace NC2InGameGuiInputManager
{
	// Original singleton: 00569410; CInputManager lifecycle and selection access.
	inline void* gInputManager = nullptr;
}
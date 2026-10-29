#pragma once

#include "C2EngineMoveableDrawing.h"
#include "XBasicTypes.h"
#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CFont;
}

namespace NC2E2
{
	struct SDisplayDrawCounters
	{
		std::uint32_t visitedMapPoints;
		std::uint32_t landscapeDrawCount;
		std::uint32_t seenHumans;
		std::uint32_t seenLocalAdultMen;
		std::uint32_t seenLocalAdultWomen;
		std::uint32_t seenLocalChildren;
		std::uint32_t seenHouses;
		std::uint32_t seenDamagedHouses;
	};

	static_assert(sizeof(SDisplayDrawCounters) == 32);

	struct SDisplayObjectReference
	{
		std::uint32_t type;
		std::uint32_t mapPosition;
		std::uint32_t humanId;
		std::uint32_t humanUniqueId;
		std::uint32_t animalId;
		std::uint32_t animalUniqueId;
		std::uint32_t vehicleId;
		std::uint32_t vehicleUniqueId;
		std::uint32_t mapMoveableId;
		std::uint32_t mapMoveableUniqueId;
		std::uint32_t houseId;
		std::uint32_t houseUniqueId;
		std::uint32_t guideId;
		std::uint32_t guideUniqueId;
		std::uint32_t landscapeId;
		std::uint32_t landscapeVariant;
		std::int32_t playerId;
	};

	struct SDisplayMarkedObject
	{
		SDisplayObjectReference reference;
		std::uint32_t markType;
	};

	struct SDisplaySearchObject
	{
		SDisplayMarkedObject markedObject;
		S2DEngineDisplaySearchObjectBobData bobData;
	};

	struct SDisplayMarkingState
	{
		std::uint8_t primaryReferenceEnabled;
		std::byte padding[3];
		SDisplaySearchObject* primaryMarkedObject;
		std::uint32_t secondaryMarkedObjectCount;
		SDisplayMarkedObject secondaryMarkedObjects[NC2Runtime::DisplaySelectionLimits::SecondaryMarks];
	};

	static_assert(sizeof(SDisplayObjectReference) == 68);
	static_assert(sizeof(SDisplayMarkedObject) == 72);
	static_assert(sizeof(SDisplaySearchObject) == (sizeof(void*) == 8 ? 560 : 396));
	static_assert(sizeof(SDisplayMarkingState) == (sizeof(void*) == 8 ? 2184 : 2172));

	// Windows fixed-point values retain eight fractional bits in both raster sizes.
	struct SDisplayProjectionSpacing
	{
		std::int32_t verticalFixed;
		std::int32_t horizontalFixed;
		std::int32_t heightFixed;
		std::int32_t halfHorizontalFixed;
		std::int32_t halfVerticalFixed;
		std::int32_t verticalPixels;
		std::int32_t horizontalPixels;
		std::int32_t heightPixels;
		std::int32_t halfHorizontalPixels;
		std::int32_t halfVerticalPixels;
	};

	struct SDisplayDescriptionData
	{
		float verticalSpacing;
		float horizontalSpacing;
		float heightScale;
		SDisplayProjectionSpacing micro;
		SDisplayProjectionSpacing macro;
		std::int32_t worldWidthPixels;
		std::int32_t worldHeightPixels;
	};

	static_assert(sizeof(SDisplayProjectionSpacing) == 40);
	static_assert(sizeof(SDisplayDescriptionData) == 100);

	// The contiguous Windows scroll state begins at display +0x94C.
	struct SDisplayScrollState
	{
		std::int32_t wantedWorldPixelX;
		std::int32_t wantedWorldPixelY;
		std::int32_t worldPixelX;
		std::int32_t worldPixelY;
		NXBasics::SRectangle bufferRectangle;
		NXBasics::CBitmap* staticBitmap;
		std::int32_t staticBitmapX;
		std::int32_t staticBitmapY;
		std::uint8_t staticRebuildRequired;
		std::byte padding2D;
		std::int16_t mapRectangle[4];
		std::uint16_t mapMinimumX;
		std::uint16_t mapMaximumX;
		std::uint16_t mapMinimumY;
		std::uint16_t mapMaximumY;
		std::byte padding3E[2];
		std::int32_t mapRectanglePixelX;
		std::int32_t mapRectanglePixelY;
	};

	static_assert(sizeof(SDisplayScrollState) == (sizeof(void*) == 8 ? 80 : 72));

	struct S2DEngineDisplayRectangle
	{
		std::uint32_t worldX;
		std::uint32_t worldY;
		std::uint32_t bitmapX;
		std::uint32_t bitmapY;
		std::uint32_t width;
		std::uint32_t height;
	};

	static_assert(sizeof(S2DEngineDisplayRectangle) == 24);

	// Complete display owner. Unknown weather bytes retain their original eight-byte slots.
	struct SRuntimeDisplayStorage
	{
		SDisplayDescriptionData description;
		NXBasics::CBitmap* destinationBitmap;
		NXBasics::CBitmap* drawingBitmap;
		std::uint8_t active;
		std::byte padding6D[3];
		std::uint32_t drawFlags, highlightingEnabled;
		std::int32_t zoomCounter, playerId;
		void* gouraudShader;
		SDisplayDrawCounters counters;
		SDisplayMarkingState marking;
		std::uint8_t tooltipOverlaysEnabled;
		std::byte padding921[3];
		NXBasics::CFont* font;
		std::byte rain[8], snow[8], sand[8];
		std::uint32_t earthquakeEndTime;
		std::int32_t earthquakeOffsetX, earthquakeOffsetY;
		SDisplayScrollState scroll;
	};

	static_assert(sizeof(SRuntimeDisplayStorage) == (sizeof(void*) == 8 ? 2504 : 2452));
	static_assert(offsetof(SRuntimeDisplayStorage, destinationBitmap) == (sizeof(void*) == 8 ? 104 : 100));
	static_assert(offsetof(SRuntimeDisplayStorage, marking) == (sizeof(void*) == 8 ? 184 : 164));
	static_assert(offsetof(SRuntimeDisplayStorage, scroll) == (sizeof(void*) == 8 ? 2424 : 2380));
}
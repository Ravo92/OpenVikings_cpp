#pragma once

#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	struct SRectangle;
	struct SColorRGB;
}

namespace NC2InGameGuiOverviewDisplay
{
	void* WorldOverviewElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t viewIndex);
	void WorldOverviewElement_Destruct(void* element);
	void WorldOverviewElement_Draw(void* element, NXBasics::CBitmap* bitmap, NXBasics::SRectangle* rectangle);
	void WorldOverviewElement_RedrawAll(void* element);
	void WorldOverviewElement_DrawGuides(void* element);
	void WorldOverviewElement_DrawHouses(void* element);
	void WorldOverviewElement_DrawSightRectangle(void* element);
	void WorldOverviewElement_DrawMapMoveables(void* element);
	void WorldOverviewElement_DrawChestMarker(void* element);
	void WorldOverviewElement_UpdateArea(void* element, const std::int32_t* minimum, const std::int32_t* maximum);
	void WorldOverviewElement_ScanTextures(void* element);
	void WorldOverviewElement_GetSampleColor(void* element, const void* pattern, NXBasics::SColorRGB* color);
	void WorldOverviewElement_BrightColor(NXBasics::SColorRGB* color, std::uint32_t brightness);
	void WorldOverviewElement_CheckForScrolling(void* element);
	void WorldOverviewElement_TransformToMapCoord(const void* element, const std::int32_t* point, std::int16_t* mapPoint);
	std::int32_t* WorldOverviewElement_TransformToOverViewDisplayCoord(const void* element, std::int32_t* point, const std::int16_t* mapPoint);
	void WorldOverviewElement_DrawMarker(void* element);
	bool WorldOverviewElement_MessageHandle(void* element, std::uint32_t messageBits, std::uint32_t param1Bits, std::uintptr_t param2Payload, std::uintptr_t param3Payload, std::uintptr_t param4Payload);
	void WorldOverviewElement_ModifyZoom(void* element, std::int32_t delta);
}
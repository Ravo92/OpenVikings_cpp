#pragma once

#include <cstdint>

namespace NXBasics
{
	class CBitmap;
}

namespace NC2E2
{
	std::int32_t Display_DE_Hightlighting_GetMapBrightness(const void* display, const void* point);
	std::int32_t Display_DE_Hightlighting_GetMapLandscapeBrightness(const void* display, const void* point);
	void DrawAtom_Landscape_Wall_DoDraw(NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::uint32_t phase, const void* display, const void* point, std::int32_t player);
	void Display_l_DE_DrawLandscapeAndCreaturesField(void* display, const std::int16_t* rectangle, NXBasics::CBitmap* bitmap, const std::int32_t* pixelPoint, bool staticPart, bool backgroundPart);
}
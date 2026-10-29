#pragma once

#include "C2EngineMoveableDrawing.h"

namespace NC2E2
{
	void DrawAtom_Guide_DoDraw(std::uint32_t id, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness, const void* display);
	void DrawAtom_Guide_GetBobData(std::uint32_t id, const void* display, S2DEngineDisplaySearchObjectBobData* output);
}
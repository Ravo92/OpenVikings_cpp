#pragma once
#include "C2EngineMoveableDrawing.h"

namespace NC2E2
{
	struct S2DEngineDrawAtomHouseSignData
	{
		struct SSign
		{
			std::int32_t landscape, x, y, human;
		};

		std::uint32_t count;
		SSign signs[30];
	};

	static_assert(sizeof(S2DEngineDrawAtomHouseSignData) == 484);
	void DrawAtom_House_GetBobData(std::uint32_t id, S2DEngineDisplaySearchObjectBobData* output);
	void DrawAtom_House_GetSignData(std::uint32_t id, S2DEngineDrawAtomHouseSignData* output);
	void DrawAtom_House_DoDraw(std::uint32_t id, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness, const void* display);
	void DrawAtom_House_Tool_UpdateFirePoints(std::uint32_t id);
	void DrawAtom_House_Tool_CreateDestroySmokeParticelForward(std::uint32_t id, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness);
	void DrawAtom_House_Tool_CreateDestroySmokeParticelBack(std::uint32_t id, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness);
	void ls_DrawAtom_House_DoDraw_PrintImageBob(const void* bobManager, std::uint32_t bobId, std::uint32_t colorTableIndex, std::int32_t brightness, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, bool drawFlag);
}
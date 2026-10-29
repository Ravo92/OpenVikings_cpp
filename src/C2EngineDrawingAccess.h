#pragma once

#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	class CPalette;
}

namespace NC2E2
{
	void DrawAtom_MapMoveable_DoDraw(std::uint32_t moveableId, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, const void* display, bool centered, std::int32_t playerId);
	void DrawAtom_House_Tool_DoDrawBob(std::int32_t houseId, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness, std::uint32_t bobId);
	void DrawAtom_House_Tool_DoDrawEngineHouseType(std::int32_t houseType, std::uint32_t variant, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t colorTableIndex, bool drawShadow, bool drawFlag);
	void EngineGraphic_Draw(std::uint32_t graphicId, std::uint32_t variant, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::uint32_t animationValue, std::int32_t brightness, std::int32_t parameter24, bool centered);
	void EngineGraphic_DrawData(const void* graphic, std::uint32_t variant, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::uint32_t animationValue, std::int32_t brightness, std::int32_t parameter24, bool centered);
	NXBasics::CPalette* EnginePalette_GetBrightnessPalette(const void* colorData, std::int32_t brightness);
	NXBasics::CPalette* EnginePalette_GetPlayerPalette(std::int32_t playerId);
	NXBasics::CPalette* EnginePalette_GetPlayerShipPalette(std::int32_t playerId);
	std::int32_t StaticHumanMisc_InhabitantSign_GetLandscapeId(std::int32_t playerId, std::uint32_t sign);
	std::int32_t StaticHumanMisc_SoldierSign_GetLandscapeId(std::int32_t playerId, std::uint32_t count);
	std::int32_t StaticHumanMisc_RoadBuildSign_GetLandscapeId(std::int32_t playerId);
	std::int32_t StaticHumanMisc_WallBuildSign_GetLandscapeId(std::int32_t playerId);
}
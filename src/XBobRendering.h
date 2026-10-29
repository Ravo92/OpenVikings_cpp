#pragma once

#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	struct SColorRGB;
	struct SRectangle;
	class CPalette;

	std::uint32_t BobManager_GetBobType(const void* bobManager, std::uint32_t bobId);
	bool BobManager_DoesBobExists(const void* bobManager, std::uint32_t bobId);
	const void* BobManager_GetBobAreaRectanglePtr(const void* bobManager, std::uint32_t bobId);
	bool BobManager_IsBobHit(const void* bobManager, std::uint32_t bobId, const std::int32_t* point);
	void BobManager_PrintBob(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette);
	void BobManager_PrintBob_Remapped(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette, const void* remapping);
	void BobManager_PrintBob_UsingEffectMatrix(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y);
	void BobManager_PrintBob_Shade_SSE2(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette, std::int32_t brightness);
	void BobManager_PrintBob_FX_DrawArea(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, const SColorRGB* colour, std::int32_t strength);
	void BobManager_PrintBob_FX_DrawOutline(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, const SColorRGB* colour, std::int32_t radius);
	void BobManager_PrintBob_Glow_AddColor_OnlyBobByteRange(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, const SColorRGB* colour, std::uint32_t strength, std::uint8_t firstIndex, std::uint8_t lastIndex);
	void BobManager_PrintBob_UsingTransparency(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::uint32_t strength, std::int32_t x, std::int32_t y, CPalette* palette);
	void BobManager_PrintBob_UsingShadedAlpha(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette, std::int32_t brightness);
	void BobManager_PrintBob_DoubleByteBobNormal(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette);
	void BobManager_PrintBob_FX_DrawOutline_Lines(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, const SColorRGB* colour, std::int32_t length);
	void BobManager_PrintBob_UsingTimeMask(const void* bobManager, std::uint32_t bobId, std::uint32_t time, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette);
	std::int32_t BobManager_PrintBob_UsingCollapseTimeMask(const void* bobManager, std::uint32_t bobId, std::uint32_t time, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette);
	void BobManager_PrintBob_UsingRangedCollapseTimeMask(const void* bobManager, std::uint32_t bobId, std::uint32_t start, std::uint32_t end, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette);
	void BobManager_PrintBob_Shadow_TimeMask(const void* bobManager, std::uint32_t bobId, std::uint32_t time, CBitmap* bitmap, std::int32_t x, std::int32_t y);
	void BobManager_PrintBob_Shadow(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y);
	void BobManager_PrintBob_Shadow16(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, std::uint32_t mode);
	void BobManager_PrintBob_Shadow32(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, std::uint32_t mode);
	void BobManager_PrintBobUsingTransparency(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette);
	void BobManager_PrintBobUsingTransparency(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, const SColorRGB* colour);
}
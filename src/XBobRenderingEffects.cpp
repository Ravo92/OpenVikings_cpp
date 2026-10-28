#include "XBasicTypes.h"
#include "XBitmap.h"
#include "X2DDisplay.h"
#include "XBobRendering.h"
#include "XBobManager.h"
#include "XCrtFileAccess.h"
#include "XDiagnostics.h"
#include "XNumericConstants.h"
#include "XPaletteAccess.h"
#include "XPixelFormatConverter.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>

namespace NXBasics
{
	namespace
	{
		std::byte* BobAddress(const void* base, std::size_t offset)
		{
			return reinterpret_cast<std::byte*>(reinterpret_cast<std::uintptr_t>(base) + offset);
		}

		template<typename T>
		T BobRead(const void* base, std::uint32_t offset = 0)
		{
			T value;
			std::memcpy(&value, BobAddress(base, offset), sizeof(value));
			return value;
		}

		std::byte* BobPointer(const void* base, std::uint32_t offset)
		{
			return reinterpret_cast<std::byte*>(static_cast<std::uintptr_t>(BobRead<std::uintptr_t>(base, offset)));
		}

		struct BobDrawRegion
		{
			SRectangle placed{ 0, 0, 0, 0 };
			SRectangle clipped{ 0, 0, 0, 0 };
			std::byte* rows = nullptr;
			std::uint32_t type = 0;
			bool wasClipped = false;
			bool clipRight = false;
		};

		bool PrepareBobDraw(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, BobDrawRegion& region)
		{
			static_assert(sizeof(std::uintptr_t) == sizeof(void*));
			const std::uint32_t index = bobId - BobRead<std::uint32_t>(bobManager, BobManagerOffset::FirstBobId);
			if (index >= BobRead<std::uint32_t>(bobManager, BobManagerOffset::BobCount))
				return false;

			const std::byte* entry = BobAddress(BobPointer(bobManager, BobManagerOffset::BobData), index * 24u);
			region.type = BobRead<std::uint32_t>(entry);
			if (region.type == 0)
				return false;

			std::memcpy(&region.placed, entry + 4, sizeof(SRectangle));
			region.placed.x = Runtime_TimeSigned32(static_cast<std::uint32_t>(region.placed.x) + static_cast<std::uint32_t>(x));
			region.placed.y = Runtime_TimeSigned32(static_cast<std::uint32_t>(region.placed.y) + static_cast<std::uint32_t>(y));
			SRectangle bounds(0, 0, 0, 0);
			std::memcpy(&bounds, BobAddress(bitmap, BitmapOffset::Rectangle), sizeof(bounds));
			if (!region.placed.IsTouching(bounds))
				return false;

			region.clipped = region.placed;
			// FUN_004104d6 has the same cuts as CutInside and returns whether a cut occurred.
			region.clipped.CutInside(bounds);
			region.wasClipped = !(region.clipped == region.placed);
			region.rows = BobAddress(BobPointer(bobManager, BobManagerOffset::LineControlData), BobRead<std::uint32_t>(entry, 0x14) * 4u);
			if (region.placed.y < 0)
				region.rows = BobAddress(region.rows, (0u - static_cast<std::uint32_t>(region.placed.y)) * 4u);

			const std::int32_t right = Runtime_TimeSigned32(static_cast<std::uint32_t>(region.placed.x) + static_cast<std::uint32_t>(region.placed.width) - 1u);
			region.clipRight = right >= BobRead<std::int32_t>(bitmap, BitmapOffset::Width);
			return true;
		}

		template<typename DrawPixel>
		void DrawBobRuns(const void* bobManager, CBitmap* bitmap, const BobDrawRegion& region, std::uint32_t sourceBytes, bool whollyUnchecked, bool useBytePitch, DrawPixel drawPixel)
		{
			const std::uint32_t depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
			const std::uint32_t pixelBytes = depth == 1 ? 2u : 4u;
			const std::byte* pixels = BobPointer(bitmap, depth == 1 ? BitmapOffset::HighColorMemory : BitmapOffset::TrueColorMemory);
			const std::uint32_t stride = BobRead<std::uint32_t>(bitmap, BitmapOffset::RowPixelCount);
			const std::uint32_t rowStep = useBytePitch ? BobRead<std::uint32_t>(bitmap, BitmapOffset::RowByteCount) : stride * pixelBytes;
			const std::int32_t right = Runtime_TimeSigned32(static_cast<std::uint32_t>(region.clipped.x) + static_cast<std::uint32_t>(region.clipped.width) - 1u);
			std::byte* destinationRow = BobAddress(pixels, stride * static_cast<std::uint32_t>(region.clipped.y) * pixelBytes);
			const std::byte* rows = region.rows;

			for (std::int32_t row = 0; row < region.clipped.height; ++row)
			{
				const std::uint32_t descriptor = BobRead<std::uint32_t>(rows);
				rows = BobAddress(rows, 4);
				if (descriptor != NXBasics::IntegerRange::UInt32Maximum)
				{
					std::uint32_t currentX = static_cast<std::uint32_t>(region.placed.x) + (descriptor >> 22);
					const std::byte* source = BobAddress(BobPointer(bobManager, BobManagerOffset::PackedLineData), descriptor & NXBasics::BitMask::Low22Bits);
					bool unchecked = whollyUnchecked || (!region.clipRight && Runtime_TimeSigned32(currentX) >= 0);
					for (;;)
					{
						const std::uint8_t token = BobRead<std::uint8_t>(source);
						source = BobAddress(source, 1);
						if (token == 0)
							break;
						const std::uint32_t count = token & NXBasics::IntegerRange::Int8Maximum;
						if ((token & NXBasics::BitMask::Bit7) != 0)
							currentX += count;
						else
						{
							for (std::uint32_t pixel = 0; pixel < count; ++pixel)
							{
								// Windows checks against zero, even when bounds.x is nonzero.
								const std::int32_t signedX = Runtime_TimeSigned32(currentX);
								if (unchecked || (signedX >= 0 && signedX <= right))
									drawPixel(BobAddress(destinationRow, currentX * pixelBytes), source, unchecked, static_cast<std::uint32_t>(region.clipped.y) + static_cast<std::uint32_t>(row), currentX, pixel, count);
								source = BobAddress(source, sourceBytes);
								++currentX;
							}
						}
						// The switch to the unchecked path occurs between runs.
						if (!unchecked && !region.clipRight && Runtime_TimeSigned32(currentX) >= 0)
							unchecked = true;
					}
				}
				destinationRow = BobAddress(destinationRow, rowStep);
			}
		}

		std::uint32_t ScaleColourSse2(std::uint32_t colour, std::uint32_t brightness)
		{
			const std::uint32_t half = brightness >> 1;
			const std::uint32_t repeated = half | ((half >> 16) | (half << 16));
			const auto zero = _mm_setzero_si128();
			const auto channels = _mm_unpacklo_epi8(_mm_cvtsi32_si128(static_cast<int>(colour)), zero);
			const auto product = _mm_mullo_epi16(channels, _mm_set1_epi32(static_cast<int>(repeated)));
			const auto scaled = _mm_srli_epi16(product, 6);
			return static_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_packus_epi16(scaled, zero)));
		}

		std::uint32_t PackHighColour(std::uint32_t colour, bool rgb565)
		{
			const std::uint32_t mask = rgb565 ? 63u : 31u;
			return ((colour >> 3) & mask) | ((colour >> (rgb565 ? 5u : 6u)) & (mask << 5)) | ((colour >> (rgb565 ? 8u : 9u)) & (mask << (rgb565 ? 11u : 10u)));
		}

		std::uint32_t ScalePaletteColour(const void* palette, std::uint8_t index, std::uint32_t brightness, const void* converter, bool highColour)
		{
			const std::byte* colour = BobAddress(palette, PaletteOffset::Colors + static_cast<std::uint32_t>(index) * 4u);
			std::uint32_t result = 0;
			for (std::uint32_t channel = 0; channel < 3; ++channel)
			{
				const std::uint32_t rgbChannel = 2u - channel;
				const std::uint32_t value = static_cast<std::uint32_t>(Runtime_TimeSigned32(BobRead<std::uint8_t>(colour, rgbChannel) * brightness) >> 7);
				const std::uint32_t mask = highColour ? static_cast<const C16BitPixelConverter*>(converter)->GetChannelMask(channel) : static_cast<const C32BitPixelConverter*>(converter)->GetChannelMask(channel);
				const std::uint32_t right = (highColour ? static_cast<const C16BitPixelConverter*>(converter)->GetChannelRightShift(channel) : static_cast<const C32BitPixelConverter*>(converter)->GetChannelRightShift(channel)) & 31u;
				const std::uint32_t left = (highColour ? static_cast<const C16BitPixelConverter*>(converter)->GetChannelLeftShift(channel) : static_cast<const C32BitPixelConverter*>(converter)->GetChannelLeftShift(channel)) & 31u;
				result |= value > 255u ? mask : (value >> right) << left;
			}
			return result;
		}

		std::uint32_t BlendPackedColour(std::uint32_t background, std::uint32_t foreground, std::uint32_t alpha, std::uint32_t inverse, std::uint32_t redMask, std::uint32_t greenMask, std::uint32_t redResultMask)
		{
			const auto red = (((background & redMask) * inverse + (foreground & redMask) * alpha) >> 8) & redResultMask;
			const auto green = (((background & greenMask) * inverse + (foreground & greenMask) * alpha) >> 8) & greenMask;
			const auto blueMask = redMask == NXBasics::Rgb888::RedMaskUnsigned ? 255u : 31u;
			const auto blue = ((background & blueMask) * inverse + (foreground & blueMask) * alpha) >> 8;
			return red + green + blue;
		}

		std::uint64_t TransparencyFactors(std::uint32_t value)
		{
			return Runtime_MultiplyUInt64(value, 0x100000001ull);
		}

		std::uint32_t BlendTransparentSse2(std::uint32_t background, std::uint32_t foreground, std::uint64_t inversePacked, std::uint64_t alphaPacked, std::uint32_t greenMask, bool highColour, bool paired)
		{
			const auto inverse = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&inversePacked));
			const auto alpha = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&alphaPacked));
			const auto zero = _mm_setzero_si128();
			auto bg = _mm_cvtsi32_si128(static_cast<int>(background));
			auto fg = _mm_cvtsi32_si128(static_cast<int>(foreground));
			if (!highColour)
			{
				bg = _mm_mullo_epi16(_mm_unpacklo_epi8(bg, zero), inverse);
				fg = _mm_mullo_epi16(_mm_unpacklo_epi8(fg, zero), alpha);

				const std::uint32_t result = static_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_packus_epi16(_mm_srli_epi16(_mm_adds_epu16(bg, fg), 8), zero)));
				return result;
			}
			const auto mask = _mm_cvtsi32_si128(static_cast<int>(greenMask));
			auto bgGreen = _mm_and_si128(bg, mask);
			auto fgGreen = _mm_and_si128(fg, mask);
			bg = _mm_unpacklo_epi8(_mm_xor_si128(bg, bgGreen), zero);
			fg = _mm_unpacklo_epi8(_mm_xor_si128(fg, fgGreen), zero);
			bgGreen = _mm_srli_epi16(bgGreen, 4);
			fgGreen = _mm_srli_epi16(fgGreen, 4);
			if (paired)
			{
				bgGreen = _mm_unpacklo_epi8(bgGreen, zero);
				fgGreen = _mm_unpacklo_epi8(fgGreen, zero);
			}
			auto other = _mm_adds_epu16(_mm_mullo_epi16(bg, inverse), _mm_mullo_epi16(fg, alpha));
			auto green = _mm_adds_epu16(_mm_mullo_epi16(bgGreen, inverse), _mm_mullo_epi16(fgGreen, alpha));
			other = _mm_packus_epi16(_mm_srli_epi16(other, 8), zero);
			other = _mm_and_si128(other, _mm_cvtsi32_si128(static_cast<int>(~greenMask)));
			green = _mm_srli_epi16(green, 4);
			if (paired) green = _mm_packs_epi32(green, zero);
			green = _mm_and_si128(green, mask);
			const std::uint32_t result = static_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_adds_epu16(other, green)));
			return result;
		}
	}

	// Original function: FUN_00463f7c
	void BobManager_PrintBob_Remapped(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette, const void* remapping)
	{
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || region.type != 1u)
			return;
		const std::uint32_t depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth != 1u && depth != 2u)
			return;
		const void* table = depth == 1u ? static_cast<const void*>(Palette_GetHighColorTablePtr(palette)) : Palette_GetTrueColorTablePtr(palette);
		if (table == nullptr)
			return;
		DrawBobRuns(bobManager, bitmap, region, 1, depth == 1u && !region.wasClipped, false, [&] (std::byte* destination, const std::byte* source, bool, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
		{
			const std::uint32_t index = BobRead<std::uint8_t>(remapping, BobRead<std::uint8_t>(source));
			const std::uint32_t bytes = depth == 1u ? 2u : 4u;
			std::memcpy(destination, BobAddress(table, index * bytes), bytes);
		});
	}

	// Original function: FUN_00466d39
	void BobManager_PrintBob_UsingShadedAlpha(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette, std::int32_t brightness)
	{
		const bool hud = NXSys2DApp::IsRemasterHudBitmap(bitmap);
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || region.type != 4u)
			return;
		const std::uint32_t depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth != 1u && depth != 2u)
		{
			return;
		}
		const void* table = depth == 1u ? static_cast<const void*>(Palette_GetHighColorTablePtr(palette)) : Palette_GetTrueColorTablePtr(palette);
		if (table == nullptr)
			return;
		const bool rgb565 = depth == 1u && g16BitPixelConverter->UsesRgb565();
		const std::uint32_t greenMask = rgb565 ? NXBasics::Rgb565::GreenMaskUnsigned : NXBasics::Rgb555::GreenMask;
		const std::uint32_t* trueTable = nullptr;
		const std::uint32_t* conversionTable = nullptr;
		if (depth == 1u)
		{
			trueTable = Palette_GetTrueColorTablePtr(palette);
			conversionTable = g16BitPixelConverter->GetTrueColorConvertTablePtr();
		}
		const std::uint32_t factor = static_cast<std::uint32_t>(brightness);
		DrawBobRuns(bobManager, bitmap, region, 2, depth == 1u && !region.wasClipped, false, [&] (std::byte* destination, const std::byte* source, bool unchecked, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
		{
			const std::uint32_t index = BobRead<std::uint8_t>(source);
			const std::uint32_t alpha = BobRead<std::uint8_t>(source, 1);
			std::uint32_t result = 0;
			if (unchecked)
			{
				const std::uint32_t foreground = ScaleColourSse2(depth == 1u ? trueTable[index] : BobRead<std::uint32_t>(table, index * 4u), factor);
				const std::uint32_t background = depth == 1u ? conversionTable[BobRead<std::uint16_t>(destination)] : BobRead<std::uint32_t>(destination);
				for (std::uint32_t channel = 0; channel < 4; ++channel)
				{
					const std::uint32_t shift = channel * 8u;
					const std::uint32_t sum = ((background >> shift) & 255u) * (256u - alpha) + ((foreground >> shift) & 255u) * alpha;
					result |= ((std::min) (sum, 65535u) >> 8) << shift;
				}
				if (depth == 1u)
					result = PackHighColour(result, rgb565);
			}
			else
			{
				const std::uint32_t foreground = depth == 1u ? BobRead<std::uint16_t>(table, index * 2u) : BobRead<std::uint32_t>(table, index * 4u);
				const std::uint32_t background = depth == 1u ? BobRead<std::uint16_t>(destination) : BobRead<std::uint32_t>(destination);
				const std::uint32_t masks[3] = { depth == 1u ? NXBasics::Rgb565::RedMaskUnsigned : NXBasics::Rgb888::RedMaskUnsigned, depth == 1u ? greenMask : NXBasics::Rgb888::GreenMaskUnsigned, depth == 1u ? 0x1Fu : NXBasics::IntegerRange::UInt8Maximum };
				for (std::uint32_t channel = 0; channel < 3; ++channel)
				{
					const std::uint32_t mask = masks[channel];
					const std::uint32_t sum = (((foreground & mask) * alpha) >> 7) * factor + (background & mask) * (256u - alpha);
					const std::uint32_t limited = (std::min) (sum, mask << 8);
					result += channel == 2 ? limited >> 8 : (limited >> 8) & mask;
				}
			}
			if (hud && depth == 2u) result = (result & 0x00FFFFFF) | ((((BobRead<std::uint32_t>(destination) >> 24) * (256u - alpha)) >> 8) << 24);
			std::memcpy(destination, &result, depth == 1u ? 2u : 4u);
		});
	}

	// Original function: FUN_00498128
	void BobManager_PrintBob_UsingEffectMatrix(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y)
	{
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || region.type != 1u)
			return;
		const std::uint32_t depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth != 1u && depth != 2u)
			return;
		const std::uint32_t bytes = depth == 1u ? 2u : 4u;
		DrawBobRuns(bobManager, bitmap, region, 1, depth == 1u && !region.wasClipped, false, [&] (std::byte* destination, const std::byte* source, bool, std::uint32_t destinationY, std::uint32_t destinationX, std::uint32_t, std::uint32_t)
		{
			const std::uint32_t sourceY = destinationY + BobRead<std::uint8_t>(source);
			if (Runtime_TimeSigned32(sourceY) < BobRead<std::int32_t>(bitmap, BitmapOffset::Height))
			{
				const std::uint32_t index = sourceY * BobRead<std::uint32_t>(bitmap, BitmapOffset::RowPixelCount) + destinationX;
				const std::byte* displaced = BobAddress(BobPointer(bitmap, depth == 1u ? BitmapOffset::HighColorMemory : BitmapOffset::TrueColorMemory), index * bytes);
				const std::uint32_t pixel = depth == 1u ? BobRead<std::uint16_t>(displaced) : BobRead<std::uint32_t>(displaced);
				std::memcpy(destination, &pixel, bytes);
			}
		});
	}

	// Original function: FUN_0049bf5a
	void BobManager_PrintBob_Shade_SSE2(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette, std::int32_t brightness)
	{
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || region.type != 1u)
			return;
		const std::uint32_t depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth != 1u && depth != 2u)
			return;
		const bool rgb565 = depth == 1u && g16BitPixelConverter->UsesRgb565();
		DrawBobRuns(bobManager, bitmap, region, 1, !region.wasClipped, !region.wasClipped, [&] (std::byte* destination, const std::byte* source, bool, std::uint32_t, std::uint32_t, std::uint32_t pixel, std::uint32_t count)
		{
			std::uint32_t result;
			if (region.wasClipped)
				result = ScalePaletteColour(palette, BobRead<std::uint8_t>(source), static_cast<std::uint32_t>(brightness), depth == 1u ? static_cast<const void*>(g16BitPixelConverter) : g32BitPixelConverter, depth == 1u);
			else
			{
				result = ScaleColourSse2(BobRead<std::uint32_t>(palette, PaletteOffset::Colors + BobRead<std::uint8_t>(source) * 4u), static_cast<std::uint32_t>(brightness));
				if (depth == 1u)
				{
					result = PackHighColour(result, rgb565);
					// Paired pixels pass through PSRLW/PACKSSDW/PSLLW and lose bit zero.
					if (pixel < (count & ~1u))
						result &= NXBasics::BitMask::WordExceptBit0;
				}
			}
			std::memcpy(destination, &result, depth == 1u ? 2u : 4u);
		});
	}

	// Original function: FUN_0049c80f
	void BobManager_PrintBob_FX_DrawArea(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, const SColorRGB* colour, std::int32_t strength)
	{
		const bool hud = NXSys2DApp::IsRemasterHudBitmap(bitmap);
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || (region.type != 1u && region.type != 4u))
			return;
		const std::uint32_t depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth != 1u && depth != 2u)
			return;
		const std::uint32_t foreground = depth == 1u ? g16BitPixelConverter->PackColor(*colour) : g32BitPixelConverter->PackColor(*colour);
		const std::uint32_t greenMask = depth == 1u && g16BitPixelConverter->UsesRgb565() ? NXBasics::Rgb565::PairedGreenMask : NXBasics::Rgb555::PairedGreenMask;
		const std::uint32_t alpha = static_cast<std::uint32_t>(strength);
		const std::uint32_t inverseAlpha = (depth == 1u && region.wasClipped ? 255u : 256u) - alpha;
		DrawBobRuns(bobManager, bitmap, region, region.type == 1u ? 1u : 2u, depth == 1u && !region.wasClipped, false, [&] (std::byte* destination, const std::byte*, bool, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
		{
			const std::uint32_t background = depth == 1u ? BobRead<std::uint16_t>(destination) : BobRead<std::uint32_t>(destination);
			const std::uint32_t redMask = depth == 1u ? NXBasics::Rgb565::RedMaskUnsigned : NXBasics::Rgb888::RedMaskUnsigned;
			const std::uint32_t middleMask = depth == 1u ? greenMask : NXBasics::Rgb888::GreenMaskUnsigned;
			const std::uint32_t blueMask = depth == 1u ? 0x1Fu : NXBasics::IntegerRange::UInt8Maximum;
			const std::uint32_t red = (((background & redMask) * inverseAlpha + (foreground & redMask) * alpha) >> 8) & redMask;
			const std::uint32_t green = (((background & middleMask) * inverseAlpha + (foreground & middleMask) * alpha) >> 8) & middleMask;
			const std::uint32_t blue = ((background & blueMask) * inverseAlpha + (foreground & blueMask) * alpha) >> 8;
			const std::uint32_t coverage = hud && depth == 2u ? ((((background >> 24) * inverseAlpha) >> 8) << 24) : 0u;
			const std::uint32_t result = (red + green + blue) | coverage;
			std::memcpy(destination, &result, depth == 1u ? 2u : 4u);
		});
	}

	// Original function: FUN_0049bdc8; bitmap line drawing clips the expanded runs.
	void BobManager_PrintBob_FX_DrawOutline(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, const SColorRGB* colour, std::int32_t radius)
	{
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || (region.type != 1 && region.type != 4) || region.clipped.height <= 0) return;
		auto* rows = region.rows;
		std::uint32_t row = 0;
		do
		{
			const auto descriptor = BobRead<std::uint32_t>(rows);
			rows = BobAddress(rows, 4);
			if (descriptor != NXBasics::IntegerRange::UInt32Maximum)
			{
				auto currentX = static_cast<std::uint32_t>(region.placed.x) + (descriptor >> 22);
				auto* source = BobAddress(BobPointer(bobManager, BobManagerOffset::PackedLineData), descriptor & NXBasics::BitMask::Low22Bits);
				for (auto token = BobRead<std::uint8_t>(source); token != 0; token = BobRead<std::uint8_t>(source))
				{
					source = BobAddress(source, 1);
					const auto count = static_cast<std::uint32_t>(token & 127u);
					if ((token & 128u) == 0)
					{
						bitmap->Draw_HorizontalLine(Runtime_TimeSigned32(currentX - static_cast<std::uint32_t>(radius)), Runtime_TimeSigned32(static_cast<std::uint32_t>(region.clipped.y) + row), Runtime_TimeSigned32(count + static_cast<std::uint32_t>(radius) * 2u), *colour);
						source = BobAddress(source, count * (region.type == 1 ? 1u : 2u));
					}
					currentX += count;
				}
			}
			++row;
		} while (static_cast<std::uint16_t>(row) < region.clipped.height);
	}

	// Original function: FUN_0049d3e8; Windows blends only the inclusive source-byte range.
	void BobManager_PrintBob_Glow_AddColor_OnlyBobByteRange(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, const SColorRGB* colour, std::uint32_t strength, std::uint8_t firstIndex, std::uint8_t lastIndex)
	{
		const bool hud = NXSys2DApp::IsRemasterHudBitmap(bitmap);
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || region.type != 1) return;
		const auto depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth != 1 && depth != 2) return;
		const auto foreground = depth == 1 ? g16BitPixelConverter->PackColor(*colour) : g32BitPixelConverter->PackColor(*colour);
		const auto greenMask = depth == 1 ? (g16BitPixelConverter->UsesRgb565() ? NXBasics::Rgb565::PairedGreenMask : NXBasics::Rgb555::PairedGreenMask) : NXBasics::Rgb888::GreenMaskUnsigned;
		const auto redMask = depth == 1 ? NXBasics::Rgb565::RedMaskUnsigned : NXBasics::Rgb888::RedMaskUnsigned;
		const auto inverse = (depth == 1 && region.wasClipped ? 255u : 256u) - strength;
		DrawBobRuns(bobManager, bitmap, region, 1, depth == 1 && !region.wasClipped, false, [&] (std::byte* destination, const std::byte* source, bool, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
		{
			const auto index = BobRead<std::uint8_t>(source);
			if (index < firstIndex || index > lastIndex) return;
			const auto background = depth == 1 ? BobRead<std::uint16_t>(destination) : BobRead<std::uint32_t>(destination);
			auto result = BlendPackedColour(background, foreground, strength, inverse, redMask, greenMask, redMask);
			if (hud && depth == 2u) result |= ((((background >> 24) * inverse) >> 8) << 24);
			std::memcpy(destination, &result, depth == 1 ? 2u : 4u);
		});
	}

	// Original function: FUN_0046467d; arbitrary opacity; packed SSE2 and clipped scalar branches.
	void BobManager_PrintBob_UsingTransparency(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::uint32_t strength, std::int32_t x, std::int32_t y, CPalette* palette)
	{
		const bool hud = NXSys2DApp::IsRemasterHudBitmap(bitmap);
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || region.type != 1) return;
		const auto depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth == 0) return;
		if (depth != 1 && depth != 2)
		{
			return;
		}
		const void* table = depth == 1 ? static_cast<const void*>(Palette_GetHighColorTablePtr(palette)) : Palette_GetTrueColorTablePtr(palette);
		if (table == nullptr) return;
		const auto greenMask = depth == 1 ? (g16BitPixelConverter->UsesRgb565() ? NXBasics::Rgb565::PairedGreenMask : NXBasics::Rgb555::PairedGreenMask) : NXBasics::Rgb888::GreenMaskUnsigned;
		// SSE2 blending does not share registers with the floating-point return ABI.
		const auto inverse = TransparencyFactors(0x01000100u - strength * 0x10001u);
		const auto alpha = TransparencyFactors(strength * 0x10001u);
		DrawBobRuns(bobManager, bitmap, region, 1, depth == 1 && !region.wasClipped, false, [&] (std::byte* destination, const std::byte* source, bool unchecked, std::uint32_t, std::uint32_t, std::uint32_t pixel, std::uint32_t count)
		{
			if (unchecked)
			{
				if (depth == 1)
				{
					const auto pairedPixels = count & ~1u;
					if (pixel < pairedPixels && (pixel & 1u) != 0) return;
					const bool paired = pixel < pairedPixels;
					auto foreground = static_cast<std::uint32_t>(BobRead<std::uint16_t>(table, BobRead<std::uint8_t>(source) * 2u));
					if (paired) foreground |= static_cast<std::uint32_t>(BobRead<std::uint16_t>(table, BobRead<std::uint8_t>(source, 1) * 2u)) << 16;
					const auto background = paired ? BobRead<std::uint32_t>(destination) : BobRead<std::uint16_t>(destination);
					const auto result = BlendTransparentSse2(background, foreground, inverse, alpha, greenMask, true, paired);
					std::memcpy(destination, &result, paired ? 4u : 2u);
				}
				else
				{
					const auto foreground = BobRead<std::uint32_t>(table, BobRead<std::uint8_t>(source) * 4u);
					const auto result = BlendTransparentSse2(BobRead<std::uint32_t>(destination), foreground, inverse, alpha, 0, false, false);
					std::memcpy(destination, &result, 4);
				}
			}
			else
			{
				const auto foreground = depth == 1 ? BobRead<std::uint16_t>(table, BobRead<std::uint8_t>(source) * 2u) : BobRead<std::uint32_t>(table, BobRead<std::uint8_t>(source) * 4u);
				const auto background = depth == 1 ? BobRead<std::uint16_t>(destination) : BobRead<std::uint32_t>(destination);
				const auto inverseStrength = 256u - strength;
				auto result = BlendPackedColour(background, foreground, strength, inverseStrength, depth == 1 ? NXBasics::Rgb565::RedMaskUnsigned : NXBasics::Rgb888::RedMaskUnsigned, greenMask, depth == 1 ? NXBasics::Rgb565::RedProductMask : NXBasics::Rgb888::RedMaskUnsigned);
				if (hud && depth == 2u) result |= ((((background >> 24) * inverseStrength) >> 8) << 24);
				std::memcpy(destination, &result, depth == 1 ? 2u : 4u);
			}
		});
	}

	// Original function: FUN_0049bba4; vertical end marks, distinct from the filled-run outline.
	void BobManager_PrintBob_FX_DrawOutline_Lines(const void* bobManager, std::uint32_t bobId, CBitmap* bitmap, std::int32_t x, std::int32_t y, const SColorRGB* colour, std::int32_t length)
	{
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || (region.type != 1 && region.type != 4) || region.clipped.height <= 0) return;
		auto* rows = region.rows;
		std::uint32_t row = 0;
		do
		{
			const auto descriptor = BobRead<std::uint32_t>(rows);
			rows = BobAddress(rows, 4);
			if (descriptor != NXBasics::IntegerRange::UInt32Maximum)
			{
				auto currentX = static_cast<std::uint32_t>(region.placed.x) + (descriptor >> 22);
				auto* source = BobAddress(BobPointer(bobManager, BobManagerOffset::PackedLineData), descriptor & NXBasics::BitMask::Low22Bits);
				bool previousRun = false;
				for (auto token = BobRead<std::uint8_t>(source); token != 0; token = BobRead<std::uint8_t>(source))
				{
					source = BobAddress(source, 1);
					const auto count = static_cast<std::uint32_t>(token & 127u);
					if ((token & 128u) == 0)
					{
						source = BobAddress(source, count * (region.type == 1 ? 1u : 2u));
						if (!previousRun) bitmap->Draw_VerticalLine(Runtime_TimeSigned32(currentX - static_cast<std::uint32_t>(length)), static_cast<std::uint32_t>(region.clipped.y) + row, length, *colour);
						const auto next = BobRead<std::uint8_t>(source);
						previousRun = next != 0 && (next & 128u) == 0;
						if (!previousRun) bitmap->Draw_VerticalLine(Runtime_TimeSigned32(currentX + count), static_cast<std::uint32_t>(region.clipped.y) + row, length, *colour);
					}
					currentX += count;
				}
			}
			++row;
		} while (static_cast<std::uint16_t>(row) < region.clipped.height);
	}

	namespace
	{
		const std::byte* TimeMaskEntry(const void* manager, std::uint32_t id)
		{
			const auto index = id - BobRead<std::uint32_t>(manager, BobManagerOffset::FirstBobId);
			if (index >= BobRead<std::uint32_t>(manager, BobManagerOffset::BobCount)) return nullptr;
			const auto* entry = BobAddress(BobPointer(manager, BobManagerOffset::BobData), index * 24u);
			return BobRead<std::uint32_t>(entry) == 4 ? entry : nullptr;
		}

		// Original function: FUN_0049e5a3; scan packed double-byte rows from the bottom.
		std::int32_t l_PrintTimeMask_BottomUpScanTimeMask(const void* manager, std::uint32_t time, const void* entry)
		{
			auto remaining = BobRead<std::uint32_t>(entry, 0x10);
			auto* rows = BobAddress(BobPointer(manager, BobManagerOffset::LineControlData), (BobRead<std::uint32_t>(entry, 0x14) + remaining) * 4u - 4u);
			std::uint32_t removed = 0;
			while (remaining != 0)
			{
				const auto descriptor = BobRead<std::uint32_t>(rows);
				rows = BobAddress(rows, std::size_t{ 0 } - 4);
				if (descriptor != NXBasics::IntegerRange::UInt32Maximum)
				{
					auto* source = BobAddress(BobPointer(manager, BobManagerOffset::PackedLineData), descriptor & NXBasics::BitMask::Low22Bits);
					for (auto token = BobRead<std::uint8_t>(source); token != 0; token = BobRead<std::uint8_t>(source))
					{
						source = BobAddress(source, 1);
						if ((token & 128u) != 0) continue;
						for (std::uint32_t pixel = 0; pixel < (token & 127u); ++pixel, source = BobAddress(source, 2))
							if (BobRead<std::uint8_t>(source, 1) > time) return Runtime_TimeSigned32(removed);
					}
				}
				--remaining;
				++removed;
			}
			return Runtime_TimeSigned32(removed);
		}

		bool PrepareCollapsedTimeMask(const void* manager, const void* entry, CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t removed, BobDrawRegion& region)
		{
			region.type = 4;
			std::memcpy(&region.placed, static_cast<const std::byte*>(entry) + 4, sizeof(SRectangle));
			region.placed.y = Runtime_TimeSigned32(static_cast<std::uint32_t>(region.placed.y) + static_cast<std::uint32_t>(removed));
			region.placed.height = Runtime_TimeSigned32(static_cast<std::uint32_t>(region.placed.height) - static_cast<std::uint32_t>(removed));
			region.placed.x = Runtime_TimeSigned32(static_cast<std::uint32_t>(region.placed.x) + static_cast<std::uint32_t>(x));
			region.placed.y = Runtime_TimeSigned32(static_cast<std::uint32_t>(region.placed.y) + static_cast<std::uint32_t>(y));
			const auto& bounds = bitmap->GetRectangle();
			if (!region.placed.IsTouching(bounds)) return false;
			region.clipped = region.placed;
			region.clipped.CutInside(bounds);
			region.wasClipped = !(region.clipped == region.placed);
			region.rows = BobAddress(BobPointer(manager, BobManagerOffset::LineControlData), BobRead<std::uint32_t>(entry, 0x14) * 4u);
			if (region.placed.y < 0) region.rows = BobAddress(region.rows, (0u - static_cast<std::uint32_t>(region.placed.y)) * 4u);
			const auto right = Runtime_TimeSigned32(static_cast<std::uint32_t>(region.placed.x) + static_cast<std::uint32_t>(region.placed.width) - 1u);
			region.clipRight = right >= BobRead<std::int32_t>(bitmap, BitmapOffset::Width);
			return true;
		}
	}

	// Original function: FUN_0049da62; the time byte is an inclusive construction threshold.
	void BobManager_PrintBob_UsingTimeMask(const void* bobManager, std::uint32_t bobId, std::uint32_t time, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette)
	{
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || region.type != 4) return;
		const auto depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth != 1 && depth != 2) return;
		const void* table = depth == 1 ? static_cast<const void*>(Palette_GetHighColorTablePtr(palette)) : Palette_GetTrueColorTablePtr(palette);
		if (table == nullptr) return;
		DrawBobRuns(bobManager, bitmap, region, 2, depth == 1 && !region.wasClipped, false, [&] (std::byte* destination, const std::byte* source, bool, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
		{
			if (BobRead<std::uint8_t>(source, 1) > time) return;
			const auto index = BobRead<std::uint8_t>(source);
			const auto value = depth == 1 ? BobRead<std::uint16_t>(table, index * 2u) : BobRead<std::uint32_t>(table, index * 4u);
			std::memcpy(destination, &value, depth == 1 ? 2u : 4u);
		});
	}

	// Original function: FUN_0049ddf8; removed bottom rows shift the remaining image down.
	std::int32_t BobManager_PrintBob_UsingCollapseTimeMask(const void* bobManager, std::uint32_t bobId, std::uint32_t time, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette)
	{
		const auto* entry = TimeMaskEntry(bobManager, bobId);
		if (entry == nullptr) return 0;
		const auto removed = l_PrintTimeMask_BottomUpScanTimeMask(bobManager, time, entry);
		BobDrawRegion region;
		if (!PrepareCollapsedTimeMask(bobManager, entry, bitmap, x, y, removed, region)) return removed;
		const auto depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth == 0) return 0;
		if (depth != 1 && depth != 2) return removed;
		const void* table = depth == 1 ? static_cast<const void*>(Palette_GetHighColorTablePtr(palette)) : Palette_GetTrueColorTablePtr(palette);
		if (table == nullptr) return 0;
		DrawBobRuns(bobManager, bitmap, region, 2, depth == 1 && !region.wasClipped, false, [&] (std::byte* destination, const std::byte* source, bool, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
		{
			if (BobRead<std::uint8_t>(source, 1) <= time) return;
			const auto index = BobRead<std::uint8_t>(source);
			const auto value = depth == 1 ? BobRead<std::uint16_t>(table, index * 2u) : BobRead<std::uint32_t>(table, index * 4u);
			std::memcpy(destination, &value, depth == 1 ? 2u : 4u);
		});
		return removed;
	}

	// Original function: FUN_0049e1b7; the start/end comparisons are both strict.
	void BobManager_PrintBob_UsingRangedCollapseTimeMask(const void* bobManager, std::uint32_t bobId, std::uint32_t start, std::uint32_t end, CBitmap* bitmap, std::int32_t x, std::int32_t y, CPalette* palette)
	{
		if (start >= end) return;
		const auto* entry = TimeMaskEntry(bobManager, bobId);
		if (entry == nullptr) return;
		const auto removed = l_PrintTimeMask_BottomUpScanTimeMask(bobManager, start, entry);
		BobDrawRegion region;
		if (!PrepareCollapsedTimeMask(bobManager, entry, bitmap, x, y, removed, region)) return;
		const auto depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth != 1 && depth != 2) return;
		const void* table = depth == 1 ? static_cast<const void*>(Palette_GetHighColorTablePtr(palette)) : Palette_GetTrueColorTablePtr(palette);
		if (table == nullptr) return;
		DrawBobRuns(bobManager, bitmap, region, 2, depth == 1 && !region.wasClipped, false, [&] (std::byte* destination, const std::byte* source, bool, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
		{
			const auto time = BobRead<std::uint8_t>(source, 1);
			if (time <= start || time >= end) return;
			const auto index = BobRead<std::uint8_t>(source);
			const auto value = depth == 1 ? BobRead<std::uint16_t>(table, index * 2u) : BobRead<std::uint32_t>(table, index * 4u);
			std::memcpy(destination, &value, depth == 1 ? 2u : 4u);
		});
	}

	// Original function: FUN_0049e618; a single-byte mask applies the original darkening operation.
	void BobManager_PrintBob_Shadow_TimeMask(const void* bobManager, std::uint32_t bobId, std::uint32_t time, CBitmap* bitmap, std::int32_t x, std::int32_t y)
	{
		BobDrawRegion region;
		if (!PrepareBobDraw(bobManager, bobId, bitmap, x, y, region) || region.type != 1) return;
		const auto depth = BobRead<std::uint32_t>(bitmap, BitmapOffset::Depth);
		if (depth == 0)
		{
			ReportWarning("NXBasics::CBobManager::PrintBob_1Bit_TimeMask\n\nUnsupported bitmap depth!"); return;
		}
		if (depth != 1 && depth != 2) return;
		DrawBobRuns(bobManager, bitmap, region, 1, !region.wasClipped, false, [&] (std::byte* destination, const std::byte* source, bool, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
		{
			if (BobRead<std::uint8_t>(source) > time) return;
			if (depth == 1)
			{
				const auto* table = reinterpret_cast<const std::byte*>(g16BitPixelConverter->GetDarkenedColorTablePtr());
				const auto value = BobRead<std::uint16_t>(table, BobRead<std::uint16_t>(destination) * 2u);
				std::memcpy(destination, &value, 2);
			}
			else
			{
				const auto background = BobRead<std::uint32_t>(destination);
				const auto red = ((background >> 15) & NXBasics::BitMask::Bits1Through16) / 3u;
				const auto blue = ((background & 255u) * 2u) / 3u;
				const auto green = ((background >> 7) & NXBasics::BitMask::Bits1Through8) / 3u;
				const auto value = (((red << 8) | green) << 8) | (blue + ((red + blue) >> 4));
				std::memcpy(destination, &value, 4);
			}
		});
	}
}

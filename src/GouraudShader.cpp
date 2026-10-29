#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "GouraudShader.h"
#include "XBitmap.h"
#include "XPalette.h"
#include "XPixelFormatConverter.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <emmintrin.h>

namespace NToolGouraudShader
{
	namespace
	{
		using NC2Lifetime::Read;
		using I = std::int32_t;
		using U = std::uint32_t;

		// The Win32 target preserves the DWORD bit pattern on signed conversion.
		// Keep pixel arithmetic local instead of calling the runtime time helper.
		I Bits(U value)
		{

			return static_cast<I>(value);
		}

		I Add(I a, I b)
		{

			return Bits(U(a) + U(b));
		}

		I Sub(I a, I b)
		{

			return Bits(U(a) - U(b));
		}

		I Mul(I a, I b)
		{

			return Bits(U(a) * U(b));
		}

		I Fixed(I value)
		{

			return Bits(U(value) << 16);
		}

		U Shr(U value, U shift)
		{

			return value >> (shift & 31);
		}

		U Shl(U value, U shift)
		{

			return value << (shift & 31);
		}

		// Complete shared Windows scanline region, 00555118..00568357.
		// Attribute pairs are interleaved in the original 0x38-byte row.
		struct SOriginalScanline
		{
			I x, width;

			struct SAttribute
			{
				I value, delta;
			} attributes[6];
		};

		SOriginalScanline gScanlines[1400]{ };
		static_assert(sizeof(SOriginalScanline) == 56);

		struct SEdge
		{
			I value[7], delta[7];

			void Advance(I rows)
			{
				for (int i = 0; i != 7; ++i)
				{
					value[i] = Add(value[i], Mul(delta[i], rows));
				}
			}
		};

		SEdge MakeEdge(const SVertex& source, const SVertex& target)
		{
			const I start[7] = { source.x, source.u, source.v, source.brightness, source.red, source.green, source.blue };
			const I end[7] = { target.x, target.u, target.v, target.brightness, target.red, target.green, target.blue };
			const I dy = Sub(target.y, source.y);
			SEdge edge{ };

			for (int i = 0; i != 7; ++i)
			{
				edge.value[i] = Fixed(start[i]);
				edge.delta[i] = dy > 0 ? Fixed(Sub(end[i], start[i])) / dy : 0;
			}

			return edge;
		}

		void StoreRow(CGouraudShader& shader, I rowIndex, const SEdge& left, const SEdge& right)
		{
			auto& row = gScanlines[rowIndex];
			row.x = left.value[0] >> 16;
			const I rightX = right.value[0] >> 16;
			row.width = Sub(rightX, row.x);

			for (int i = 0; i != 6; ++i)
			{
				row.attributes[i].value = left.value[i + 1];

				if (row.width != 0)
					row.attributes[i].delta = Sub(right.value[i + 1], left.value[i + 1]) / row.width;
			}

			if (row.width == 0)
			{

				return;
			}

			if (row.x < shader.clipLeft)
			{
				const I skip = Sub(shader.clipLeft, row.x);

				for (auto& attribute : row.attributes)
				{
					attribute.value = Add(attribute.value, Mul(attribute.delta, skip));
				}

				row.width = Sub(row.width, skip);
				row.x = shader.clipLeft;
			}

			if (rightX > shader.clipRight)
				row.width = Add(row.width, Sub(shader.clipRight, rightX));
		}

		// Remaster: retain scalar vertex-color arithmetic and use SSE2 for packed shading.
		template <bool vertexColor, bool alpha, bool simd>
		void Shade(CGouraudShader& shader)
		{
			const void* converter = NXBasics::g32BitPixelConverter;
			U masks[3], leftShifts[3];

			if constexpr (!simd)
				for (int i = 0; i != 3; ++i)
				{
					// BGR order; scalar 32-bit Windows routines leave blue unshifted.
					const U channel = 2 - i;
					masks[i] = Read<U>(converter, NC2Layout::TrueColorConverterOffset::RedMask + channel * NC2Layout::RecordSize::PixelChannelValue32);
					leftShifts[i] = i == 0 ? 0 : Read<U>(converter, NC2Layout::TrueColorConverterOffset::RedLeftShift + channel * NC2Layout::RecordSize::PixelChannelValue32);
				}

			auto* storage = Read<std::byte*>(shader.bitmap, NC2Layout::BitmapStorageOffset::TrueColorMemory);
			const I pitch = Read<I>(shader.bitmap, NC2Layout::BitmapStorageOffset::RowPixelCount);
			const auto* colors = shader.palette->GetColorData();

			for (I index = shader.skipRows; index < shader.height; ++index)
			{
				const auto& row = gScanlines[index];

				if (row.width <= 0)
					continue;

				auto* destination = storage + Mul(Add(Mul(Add(shader.firstY, index), pitch), row.x), 4);
				I u = row.attributes[0].value, v = row.attributes[1].value;
				I light = row.attributes[2].value >> 7, dLight = row.attributes[2].delta >> 7;
				I vertex[3] = { row.attributes[5].value >> 8, row.attributes[4].value >> 8, row.attributes[3].value >> 8 };
				I dVertex[3] = { row.attributes[5].delta >> 8, row.attributes[4].delta >> 8, row.attributes[3].delta >> 8 };
				auto packedLight = _mm_set1_epi16(static_cast<std::int16_t>(row.attributes[2].value >> 12));
				const auto packedDelta = _mm_set1_epi16(static_cast<std::int16_t>(row.attributes[2].delta >> 12));

				for (I pixel = 0; pixel < row.width; ++pixel)
				{
					const U offset = simd ? (U(u) >> 16) | ((U(v) >> 16) << 8) : U((v >> 8) & ~255) + U(u >> 16);
					const auto* color = colors + shader.texture[offset] * 4;
					U old = alpha ? Read<U>(destination, 0) : 0;
					const U coverage = alpha ? shader.alphaTexture[offset] : 0;
					U result = 0;

					if constexpr (simd)
					{
						const auto zero = _mm_setzero_si128();
						const auto channels = _mm_unpacklo_epi8(_mm_cvtsi32_si128(Bits(Read<U>(color, 0))), zero);
						const auto product = _mm_mullo_epi16(_mm_srli_epi16(channels, 4), packedLight);
						auto shaded = _mm_packus_epi16(_mm_srli_epi16(product, 7), zero);

						if constexpr (alpha)
						{
							const auto background = _mm_unpacklo_epi8(_mm_cvtsi32_si128(Bits(old)), zero);
							const auto foreground = _mm_mullo_epi16(_mm_unpacklo_epi8(shaded, zero), _mm_set1_epi16(static_cast<short>(coverage)));
							const auto retained = _mm_mullo_epi16(background, _mm_set1_epi16(static_cast<short>(256 - coverage)));
							shaded = _mm_packus_epi16(_mm_srli_epi16(_mm_adds_epu16(foreground, retained), 8), zero);
						}

						result = static_cast<U>(_mm_cvtsi128_si32(shaded));
						packedLight = _mm_adds_epi16(packedLight, packedDelta);
					}
					else
					{
						for (int i = 0; i != 3; ++i)
						{
							const U product = U(color[i]) * U(light);
							U channel = product >> 16;

							if constexpr (vertexColor)
							{
								channel = std::min<U>((channel * U(vertex[i])) >> 15, 255);

								if constexpr (alpha)
								{
									U previous = Shr(old & masks[i], leftShifts[i]);
									channel = (previous * (255 - coverage) + channel * coverage) >> 8;
								}

								result |= Shl(channel, leftShifts[i]);
							}
							else if constexpr (alpha)
							{
								U packed;

								if (U(light) > 0x10000)
								{
									packed = channel > 255 ? masks[i] : Shl(channel, leftShifts[i]);
								}
								else
								{
									packed = Shr(product, 16 - leftShifts[i]);
								}

								U blended = ((old & masks[i]) * (255 - coverage) + packed * coverage) >> 8;

								if (i != 0)
									blended &= masks[i];

								result += blended;
							}
							else if (U(light) > 0x10000)
							{
								result |= channel > 255 ? masks[i] : Shl(channel, leftShifts[i]);
							}
							else
							{
								result |= Shl(Shr(product, 16), leftShifts[i]);
							}
						}
					}

					NC2Lifetime::Write<U>(destination, 0, result);
					destination += 4;
					u = Add(u, row.attributes[0].delta);
					v = Add(v, row.attributes[1].delta);

					if constexpr (!simd)
						light = Add(light, dLight);

					if constexpr (vertexColor)
						for (int i = 0; i != 3; ++i)
						{
							vertex[i] = Add(vertex[i], dVertex[i]);
						}
				}
			}
		}
	}

	// Original function: FUN_00481e51.
	CGouraudShader::CGouraudShader()
	{
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_0049ec09.
	void CGouraudShader::l_SetupAndClip()
	{
		const auto& rectangle = bitmap->GetRectangle();
		clipLeft = rectangle.x;
		clipRight = Add(rectangle.x, rectangle.width);
		clipTop = rectangle.y;
		clipBottom = Add(rectangle.y, rectangle.height);
		int top = 0, middle = 1, bottom = 2;

		if (vertices[middle].y < vertices[top].y)
			std::swap(top, middle);

		if (vertices[bottom].y < vertices[top].y)
		{
			const int previous = top;
			top = bottom;
			bottom = middle;
			middle = previous;
		}
		else if (vertices[bottom].y < vertices[middle].y)
		{
			std::swap(middle, bottom);
		}

		const auto& a = vertices[top];
		const auto& b = vertices[middle];
		const auto& c = vertices[bottom];
		firstY = a.y;
		height = Sub((std::min)(c.y, clipBottom), a.y);
		skipRows = std::max<I>(Sub(clipTop, a.y), 0);

		if (height <= skipRows)
		{

			return;
		}

		const I middleDy = Sub(b.y, a.y), bottomDy = Sub(c.y, a.y);
		const bool middleOnLeft = middleDy != 0 ? Fixed(Sub(b.x, a.x)) / middleDy < Fixed(Sub(c.x, a.x)) / bottomDy : a.x > b.x;
		SEdge left = MakeEdge(a, middleOnLeft ? b : c);
		SEdge right = MakeEdge(a, middleOnLeft ? c : b);
		left.Advance(skipRows);
		right.Advance(skipRows);
		I row = skipRows;
		const I firstEnd = (std::min)(Sub(b.y, a.y), height);

		for (; row < firstEnd; ++row)
		{
			StoreRow(*this, row, left, right);
			left.Advance(1);
			right.Advance(1);
		}

		SEdge shorter = MakeEdge(b, c);
		shorter.Advance(std::max<I>(Sub(clipTop, b.y), 0));

		if (middleOnLeft)
		{
			left = shorter;
		}
		else
		{
			right = shorter;
		}

		for (; row < height; ++row)
		{
			StoreRow(*this, row, left, right);
			left.Advance(1);
			right.Advance(1);
		}
	}

	// Original function: FUN_0049a12f.
	void CGouraudShader::XShade_VertexColor_Alpha()
	{
		l_SetupAndClip();

		if (Read<I>(bitmap, NC2Layout::BitmapStorageOffset::BytesPerPixel) == 4)
			XShade_VertexColor_Alpha_32Bit();
	}

	// Original function: FUN_0049a3c4.
	void CGouraudShader::XShade_VertexColor_Alpha_32Bit()
	{
		Shade<true, true, false>(*this);
	}

	// Original function: FUN_0049a5f0.
	void CGouraudShader::XShade_VertexColor()
	{
		l_SetupAndClip();

		if (Read<I>(bitmap, NC2Layout::BitmapStorageOffset::BytesPerPixel) == 4)
			XShade_VertexColor_32Bit();
	}

	// Original function: FUN_0049a7f7.
	void CGouraudShader::XShade_VertexColor_32Bit()
	{
		Shade<true, false, false>(*this);
	}

	// Original function: FUN_0049a9ab.
	void CGouraudShader::XShade()
	{
		XShade_SSE2();
	}

	// Original function: FUN_0049abf0.
	void CGouraudShader::XShade_32Bit()
	{
		Shade<false, false, false>(*this);
	}

	// Original function: FUN_0049ada7.
	void CGouraudShader::XShade_Alpha()
	{
		XShade_Alpha_SSE2();
	}

	// Original function: FUN_0049b089.
	void CGouraudShader::XShade_Alpha_32Bit()
	{
		Shade<false, true, false>(*this);
	}

	// Original function: FUN_0049f5d5.
	void CGouraudShader::XShade_SSE2()
	{
		if (vertices[0].brightness != 0 || vertices[1].brightness != 0 || vertices[2].brightness != 0)
			for (auto& vertex : vertices)
			{
				vertex.brightness = std::max<I>(vertex.brightness, 5);
			}

		l_SetupAndClip();

		if (Read<I>(bitmap, NC2Layout::BitmapStorageOffset::BytesPerPixel) == 4)
			XShade_SSE2_32Bit();
	}

	// Original function: FUN_0049f7fb.
	void CGouraudShader::XShade_SSE2_32Bit()
	{
		Shade<false, false, true>(*this);
	}

	// Original function: FUN_0049f951.
	void CGouraudShader::XShade_Alpha_SSE2()
	{
		if (vertices[0].brightness != 0 || vertices[1].brightness != 0 || vertices[2].brightness != 0)
			for (auto& vertex : vertices)
			{
				vertex.brightness = std::max<I>(vertex.brightness, 5);
			}

		l_SetupAndClip();

		if (Read<I>(bitmap, NC2Layout::BitmapStorageOffset::BytesPerPixel) == 4)
			XShade_Alpha_SSE2_32Bit();
	}

	// Original function: FUN_0049fbdb.
	void CGouraudShader::XShade_Alpha_SSE2_32Bit()
	{
		Shade<false, true, true>(*this);
	}
}
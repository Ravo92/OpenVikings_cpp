#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include "XPixelFormatConverter.h"

#include <cstdint>

namespace
{
	std::uint32_t FindFirstSetBit(std::uint32_t mask, std::uint32_t maximumBitCount)
	{
		for (std::uint32_t bit = 0; bit < maximumBitCount; ++bit)
		{
			if ((mask & (std::uint32_t{ 1 } << bit)) != 0)
			{
				return bit;
			}
		}

		// This matches the original fallback.
		return 0;
	}

	std::uint32_t CountConsecutiveSetBits(std::uint32_t mask, std::uint32_t firstBit, std::uint32_t maximumBitCount)
	{
		std::uint32_t count = 0;

		for (std::uint32_t bit = firstBit;
			 bit < maximumBitCount;
			 ++bit)
		{
			if ((mask & (std::uint32_t{ 1 } << bit)) == 0)
			{
				break;
			}

			++count;
		}

		return count;
	}
}

namespace NXBasics
{
	// Original function: FUN_00410117
	std::uint16_t C16BitPixelConverter::PackColor(const SColorRGB& color) const
	{
		const std::uint32_t blue = color.blue;
		const std::uint32_t green = color.green;
		const std::uint32_t red = color.red;

		return PackColor(red, green, blue);
	}

	// Original function: FUN_00410131
	const std::uint32_t* C16BitPixelConverter::GetTrueColorConvertTablePtr()
	{
		if (mTrueColorConvertTable == nullptr)
		{
			mTrueColorConvertTable = static_cast<std::uint32_t*>(Runtime_AllocateWithNewHandler(262144, 1));

			if (mUsesRgb565 || mUsesRgb555)
			{
				const std::uint32_t greenCount = mUsesRgb565 ? 64u : 32u;
				std::uint32_t index = 0;

				// RGB555 initializes only the lower half of the allocation.
				for (std::uint32_t red = 0; red < 32; ++red)
					for (std::uint32_t green = 0; green < greenCount; ++green)
						for (std::uint32_t blue = 0; blue < 32; ++blue)
							mTrueColorConvertTable[index++] = red * 0x80000u + green * (mUsesRgb565 ? 0x400u : 0x800u) + blue * 8u;
			}
			else
			{
				for (std::uint32_t pixel = 0; pixel < 0x10000; ++pixel)
				{
					const std::uint32_t red = (pixel & mRedMask) >> (static_cast<std::uint32_t>(mRedAdjustment) & 31u);
					const std::uint32_t green = (pixel & mGreenMask) >> (static_cast<std::uint32_t>(mGreenAdjustment) & 31u);
					const std::uint32_t blue = (pixel & mBlueMask) >> (static_cast<std::uint32_t>(mBlueAdjustment) & 31u);
					mTrueColorConvertTable[pixel] = (((red << 8) | green) << 8 | blue) << 3;
				}
			}
		}

		return mTrueColorConvertTable;
	}

	// Original function: FUN_004102e8
	std::uint32_t C32BitPixelConverter::PackColor(std::uint32_t red, std::uint32_t green, std::uint32_t blue) const
	{
		if (!mInitialized)
		{
			return 0;
		}

		// x86 masks each 32-bit shift count to five bits.
		const std::uint32_t packedRed = (red >> (mRedRightShift & 31u)) << (mRedLeftShift & 31u);
		const std::uint32_t packedGreen = (green >> (mGreenRightShift & 31u)) << (mGreenLeftShift & 31u);
		const std::uint32_t packedBlue = (blue >> (mBlueRightShift & 31u)) << (mBlueLeftShift & 31u);

		return packedRed | packedGreen | packedBlue;
	}

	// Original function: FUN_00410326
	std::uint32_t C32BitPixelConverter::PackColor(const SColorRGB& color) const
	{
		const std::uint32_t blue = color.blue;
		const std::uint32_t green = color.green;
		const std::uint32_t red = color.red;

		return PackColor(red, green, blue);
	}

	static std::uint32_t ExtractShadowChannel(std::uint16_t pixel, std::uint16_t mask, std::int32_t adjustment)
	{
		const std::uint32_t shift = static_cast<std::uint32_t>(adjustment) & 31u;
		return static_cast<std::uint32_t>(pixel & mask) >> shift;
	}

	// Original function: FUN_0049b7a5
	std::uint16_t C16BitPixelConverter::ApplyShadowMode1(std::uint16_t pixel) const
	{
		const std::uint32_t red = static_cast<std::uint16_t>(ExtractShadowChannel(pixel, mRedMask, mRedAdjustment) << 3);
		const std::uint32_t green = static_cast<std::uint16_t>(ExtractShadowChannel(pixel, mGreenMask, mGreenAdjustment) << 3);
		const std::uint32_t blue = static_cast<std::uint16_t>(ExtractShadowChannel(pixel, mBlueMask, mBlueAdjustment) << 3);

		// The original compares and forwards only the low WORD.
		std::uint32_t intensity = static_cast<std::uint16_t>(((red + green + blue) * 2u + 240u) / 4u);

		if (intensity > 255u)
			intensity = 255u;

		return PackColor(intensity, intensity, intensity);
	}

	// Original function: FUN_0049b818
	std::uint16_t C16BitPixelConverter::ApplyShadowMode2(std::uint16_t pixel) const
	{
		const std::uint32_t red = (ExtractShadowChannel(pixel, mRedMask, mRedAdjustment) & NXBasics::BitMask::Low13Bits) * 4u + 128u;
		const std::uint32_t green = (ExtractShadowChannel(pixel, mGreenMask, mGreenAdjustment) & NXBasics::BitMask::Low13Bits) * 4u + 128u;
		const std::uint32_t blue = (ExtractShadowChannel(pixel, mBlueMask, mBlueAdjustment) & NXBasics::BitMask::Low13Bits) * 4u + 128u;

		// No additional saturation is performed before packing.
		return PackColor(static_cast<std::uint16_t>(red), static_cast<std::uint16_t>(green), static_cast<std::uint16_t>(blue));
	}

	// Original function: FUN_0049b88d
	std::uint16_t C16BitPixelConverter::ApplyShadowMode3(std::uint16_t pixel) const
	{
		const auto brighten = [] (std::uint32_t channel) -> std::uint32_t
		{
			const std::uint32_t expanded = static_cast<std::uint16_t>(channel << 3);

			if (expanded >= 170u)
				return 255u;

			return expanded + (expanded >> 1);
		};

		const std::uint32_t red = brighten(ExtractShadowChannel(pixel, mRedMask, mRedAdjustment));
		const std::uint32_t green = brighten(ExtractShadowChannel(pixel, mGreenMask, mGreenAdjustment));
		const std::uint32_t blue = brighten(ExtractShadowChannel(pixel, mBlueMask, mBlueAdjustment));

		return PackColor(red, green, blue);
	}

	C16BitPixelConverter* g16BitPixelConverter = nullptr;
	C32BitPixelConverter* g32BitPixelConverter = nullptr;

	// Original function: FUN_0040fe27
	C16BitPixelConverter::C16BitPixelConverter() : mInitialized(false), mTrueColorConvertTable(nullptr), mDarkenedColorTable(nullptr)
	{
		static_assert(offsetof(C16BitPixelConverter, mTrueColorConvertTable) == (sizeof(void*) == 8 ? 8 : 4));
		static_assert(offsetof(C16BitPixelConverter, mDarkenedColorTable) == (sizeof(void*) == 8 ? 16 : 8));
		static_assert(offsetof(C16BitPixelConverter, mRedMask) == PixelConverter16Offset::RedMask);
		static_assert(offsetof(C16BitPixelConverter, mGreenMask) == PixelConverter16Offset::GreenMask);
		static_assert(offsetof(C16BitPixelConverter, mBlueMask) == PixelConverter16Offset::BlueMask);
		static_assert(offsetof(C16BitPixelConverter, mRedRightShift) == (sizeof(void*) == 8 ? 0x20 : 0x14));
		static_assert(offsetof(C16BitPixelConverter, mRedLeftShift) == (sizeof(void*) == 8 ? 0x2C : 0x20));
		static_assert(offsetof(C16BitPixelConverter, mRedAdjustment) == PixelConverter16Offset::RedAdjustment);
		static_assert(offsetof(C16BitPixelConverter, mGreenAdjustment) == PixelConverter16Offset::GreenAdjustment);
		static_assert(offsetof(C16BitPixelConverter, mBlueAdjustment) == PixelConverter16Offset::BlueAdjustment);
		static_assert(offsetof(C16BitPixelConverter, mUsesRgb565) == (sizeof(void*) == 8 ? 0x46 : 0x3A));
	}

	// Original function: FUN_0040fe34
	C16BitPixelConverter::~C16BitPixelConverter()
	{
		if (mTrueColorConvertTable != nullptr)
		{
			Runtime_Free(mTrueColorConvertTable);
			mTrueColorConvertTable = nullptr;
		}

		if (mDarkenedColorTable != nullptr)
		{
			Runtime_Free(mDarkenedColorTable);
			mDarkenedColorTable = nullptr;
		}
	}

	// Original function: FUN_0040fe5d
	void C16BitPixelConverter::SetColorMasks(std::uint16_t redMask, std::uint16_t greenMask, std::uint16_t blueMask)
	{
		mRedMask = redMask;
		mGreenMask = greenMask;
		mBlueMask = blueMask;

		mRedLeftShift = FindFirstSetBit(redMask, 16);

		const std::uint32_t redBitCount = CountConsecutiveSetBits(redMask, mRedLeftShift, 16);

		mRedRightShift = 8 - redBitCount;

		mRedAdjustment = static_cast<std::int32_t>(mRedLeftShift + redBitCount) - 5;

		mGreenLeftShift = FindFirstSetBit(greenMask, 16);

		const std::uint32_t greenBitCount = CountConsecutiveSetBits(greenMask, mGreenLeftShift, 16);

		mGreenRightShift = 8 - greenBitCount;
		mGreenAdjustment = static_cast<std::int32_t>(mGreenLeftShift + greenBitCount) - 5;
		mBlueLeftShift = FindFirstSetBit(blueMask, 16);

		const std::uint32_t blueBitCount = CountConsecutiveSetBits(blueMask, mBlueLeftShift, 16);

		mBlueRightShift = 8 - blueBitCount;
		mBlueAdjustment = static_cast<std::int32_t>(mBlueLeftShift + blueBitCount) - 5;

		mInitialized = true;

		mMiddleGrayColor = PackColor(NXBasics::IntegerRange::Int8MaximumSigned, NXBasics::IntegerRange::Int8MaximumSigned, NXBasics::IntegerRange::Int8MaximumSigned);
		mUsesRgb565 = redMask == NXBasics::Rgb565::RedMask && greenMask == NXBasics::Rgb565::GreenMask && blueMask == 0x001F;
		mUsesRgb555 = redMask == NXBasics::Rgb555::RedMask && greenMask == NXBasics::Rgb555::GreenMaskSigned && blueMask == 0x001F;

		if (mTrueColorConvertTable != nullptr)
		{
			Runtime_Free(mTrueColorConvertTable);
			mTrueColorConvertTable = nullptr;
		}

		if (mDarkenedColorTable != nullptr)
		{
			Runtime_Free(mDarkenedColorTable);
			mDarkenedColorTable = nullptr;
		}

		mDarkenedColorTable = static_cast<std::uint16_t*>(Runtime_AllocateWithNewHandler(131072, 1));
		// Windows fills the table without checking the allocation result.
		for (std::uint32_t color = 0; color < 0x10000; ++color)
		{
			const std::uint32_t red = ((color & mRedMask) >> mRedLeftShift) * 2 / 3;
			const std::uint32_t green = ((color & mGreenMask) >> mGreenLeftShift) * 2 / 3;
			const std::uint32_t blue = ((color & mBlueMask) >> mBlueLeftShift) * 2 / 3;
			const std::uint32_t packedRed = red << mRedLeftShift;
			const std::uint32_t packedGreen = green << mGreenLeftShift;

			/*
			 * The original adds 1/16 of red + blue to the blue
			 * component before placing it in the destination mask.
			 */
			const std::uint32_t adjustedBlue = blue + ((red + blue) >> 4);
			const std::uint32_t packedBlue = adjustedBlue << mBlueLeftShift;

			mDarkenedColorTable[color] = static_cast<std::uint16_t>(packedRed | packedGreen | packedBlue);
		}
	}

	// Original function: FUN_004100d8
	std::uint16_t C16BitPixelConverter::PackColor(std::uint32_t red, std::uint32_t green, std::uint32_t blue) const
	{
		if (!mInitialized)
		{
			return 0;
		}

		// x86 masks each 32-bit shift count to five bits.
		const std::uint32_t packedRed = (red >> (mRedRightShift & 31u)) << (mRedLeftShift & 31u);
		const std::uint32_t packedGreen = (green >> (mGreenRightShift & 31u)) << (mGreenLeftShift & 31u);
		const std::uint32_t packedBlue = (blue >> (mBlueRightShift & 31u)) << (mBlueLeftShift & 31u);

		return static_cast<std::uint16_t>(packedRed | packedGreen | packedBlue);
	}

	// Original function: FUN_0041022c
	C32BitPixelConverter::C32BitPixelConverter() : mInitialized(false)
	{
		static_assert(offsetof(C32BitPixelConverter, mRedMask) == 4);
		static_assert(offsetof(C32BitPixelConverter, mRedRightShift) == 0x10);
		static_assert(offsetof(C32BitPixelConverter, mRedLeftShift) == 0x1C);
	}

	// Original function: FUN_00410232
	void C32BitPixelConverter::SetColorMasks(std::uint32_t redMask, std::uint32_t greenMask, std::uint32_t blueMask)
	{
		mRedMask = redMask;
		mRedLeftShift = FindFirstSetBit(redMask, 32);

		const std::uint32_t redBitCount = CountConsecutiveSetBits(redMask, mRedLeftShift, 32);

		mRedRightShift = 8 - redBitCount;
		mGreenMask = greenMask;
		mGreenLeftShift = FindFirstSetBit(greenMask, 32);

		const std::uint32_t greenBitCount = CountConsecutiveSetBits(greenMask, mGreenLeftShift, 32);

		mGreenRightShift = 8 - greenBitCount;
		mBlueMask = blueMask;
		mBlueLeftShift = FindFirstSetBit(blueMask, 32);

		const std::uint32_t blueBitCount = CountConsecutiveSetBits(blueMask, mBlueLeftShift, 32);

		mBlueRightShift = 8 - blueBitCount;

		mInitialized = true;
	}
}

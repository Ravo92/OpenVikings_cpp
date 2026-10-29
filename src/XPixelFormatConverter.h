#pragma once

#include "XBasicTypes.h"

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	namespace PixelConverter16Offset
	{
		inline constexpr auto RedMask = sizeof(void*) == 8 ? 0x18 : 0xC;
		inline constexpr auto GreenMask = sizeof(void*) == 8 ? 0x1A : 0xE;
		inline constexpr auto BlueMask = sizeof(void*) == 8 ? 0x1C : 0x10;
		inline constexpr auto RedAdjustment = sizeof(void*) == 8 ? 0x38 : 0x2C;
		inline constexpr auto GreenAdjustment = sizeof(void*) == 8 ? 0x3C : 0x30;
		inline constexpr auto BlueAdjustment = sizeof(void*) == 8 ? 0x40 : 0x34;
	}

	class C16BitPixelConverter
	{
	  public:
		// Original function: FUN_0040fe27
		C16BitPixelConverter();

		// Original function: FUN_0040fe34
		~C16BitPixelConverter();

		// Original function: FUN_0040fe5d
		void SetColorMasks(std::uint16_t redMask, std::uint16_t greenMask, std::uint16_t blueMask);

		// Original function: FUN_004100d8
		std::uint16_t PackColor(std::uint32_t red, std::uint32_t green, std::uint32_t blue) const;

		// Original function: FUN_00410117
		std::uint16_t PackColor(const SColorRGB& color) const;

		const std::uint32_t* GetTrueColorConvertTablePtr();

		bool UsesRgb565() const noexcept
		{

			return mUsesRgb565;
		}

		const std::uint16_t* GetDarkenedColorTablePtr() const noexcept
		{

			return mDarkenedColorTable;
		}

		std::uint16_t ApplyShadowMode1(std::uint16_t pixel) const;
		std::uint16_t ApplyShadowMode2(std::uint16_t pixel) const;
		std::uint16_t ApplyShadowMode3(std::uint16_t pixel) const;

		// Native source-level accessors for renderer consumers of the original fields.
		std::uint32_t GetChannelMask(std::uint32_t channel) const noexcept
		{

			return channel == 0 ? mRedMask : channel == 1 ? mGreenMask
			                                              : mBlueMask;
		}

		std::uint32_t GetChannelRightShift(std::uint32_t channel) const noexcept
		{

			return channel == 0 ? mRedRightShift : channel == 1 ? mGreenRightShift
			                                                    : mBlueRightShift;
		}

		std::uint32_t GetChannelLeftShift(std::uint32_t channel) const noexcept
		{

			return channel == 0 ? mRedLeftShift : channel == 1 ? mGreenLeftShift
			                                                   : mBlueLeftShift;
		}

		bool IsInitialized() const noexcept
		{

			return mInitialized;
		}

	  private:
		friend class CBitmap;

		bool mInitialized;                     // +0x00
		std::byte mPadding01[3];               // +0x01
		std::uint32_t* mTrueColorConvertTable; // +0x04
		std::uint16_t* mDarkenedColorTable;    // +0x08

		std::uint16_t mRedMask;   // +0x0C
		std::uint16_t mGreenMask; // +0x0E
		std::uint16_t mBlueMask;  // +0x10
		std::byte mPadding12[2];  // +0x12

		std::uint32_t mRedRightShift;   // +0x14
		std::uint32_t mGreenRightShift; // +0x18
		std::uint32_t mBlueRightShift;  // +0x1C

		std::uint32_t mRedLeftShift;   // +0x20
		std::uint32_t mGreenLeftShift; // +0x24
		std::uint32_t mBlueLeftShift;  // +0x28

		std::int32_t mRedAdjustment;   // +0x2C
		std::int32_t mGreenAdjustment; // +0x30
		std::int32_t mBlueAdjustment;  // +0x34

		std::uint16_t mMiddleGrayColor; // +0x38
		bool mUsesRgb565;               // +0x3A
		bool mUsesRgb555;               // +0x3B
	};

	static_assert(sizeof(C16BitPixelConverter) == (sizeof(void*) == 8 ? 72 : 60), "Unexpected native high-color converter layout");

	class C32BitPixelConverter
	{
	  public:
		// Original function: FUN_0041022c
		C32BitPixelConverter();

		// Original function: FUN_00410232
		void SetColorMasks(std::uint32_t redMask, std::uint32_t greenMask, std::uint32_t blueMask);

		// Original function: FUN_004102e8
		std::uint32_t PackColor(std::uint32_t red, std::uint32_t green, std::uint32_t blue) const;

		// Original function: FUN_00410326
		std::uint32_t PackColor(const SColorRGB& color) const;

		// Native source-level accessors for renderer consumers of the original fields.
		std::uint32_t GetChannelMask(std::uint32_t channel) const noexcept
		{

			return channel == 0 ? mRedMask : channel == 1 ? mGreenMask
			                                              : mBlueMask;
		}

		std::uint32_t GetChannelRightShift(std::uint32_t channel) const noexcept
		{

			return channel == 0 ? mRedRightShift : channel == 1 ? mGreenRightShift
			                                                    : mBlueRightShift;
		}

		std::uint32_t GetChannelLeftShift(std::uint32_t channel) const noexcept
		{

			return channel == 0 ? mRedLeftShift : channel == 1 ? mGreenLeftShift
			                                                   : mBlueLeftShift;
		}

		bool IsInitialized() const noexcept
		{

			return mInitialized;
		}

	  private:
		bool mInitialized;       // +0x00
		std::byte mPadding01[3]; // +0x01

		std::uint32_t mRedMask;   // +0x04
		std::uint32_t mGreenMask; // +0x08
		std::uint32_t mBlueMask;  // +0x0C

		std::uint32_t mRedRightShift;   // +0x10
		std::uint32_t mGreenRightShift; // +0x14
		std::uint32_t mBlueRightShift;  // +0x18

		std::uint32_t mRedLeftShift;   // +0x1C
		std::uint32_t mGreenLeftShift; // +0x20
		std::uint32_t mBlueLeftShift;  // +0x24
	};

	static_assert(sizeof(C32BitPixelConverter) == 40);

	// Original globals: DAT_005106e0
	extern C16BitPixelConverter* g16BitPixelConverter;

	// DAT_005106e4
	extern C32BitPixelConverter* g32BitPixelConverter;
}
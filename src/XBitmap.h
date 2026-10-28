#pragma once

#include "XBasicTypes.h"
#include "XStorable.h"

#include <cstddef>
#include <cstdint>

namespace NXSys2DApp
{
	class CDisplay;
}

namespace NXBasics
{
	namespace BitmapOffset
	{
		inline constexpr auto Depth = sizeof(void*) == 8 ? 8 : 4;
		inline constexpr auto Rectangle = sizeof(void*) == 8 ? 12 : 8;
		inline constexpr auto Width = sizeof(void*) == 8 ? 20 : 16;
		inline constexpr auto Height = sizeof(void*) == 8 ? 24 : 20;
		inline constexpr auto RightBits = sizeof(void*) == 8 ? 28 : 24;
		inline constexpr auto BottomBits = sizeof(void*) == 8 ? 32 : 28;
		inline constexpr auto Memory = sizeof(void*) == 8 ? 40 : 32;
		inline constexpr auto RawMemory = sizeof(void*) == 8 ? 48 : 36;
		inline constexpr auto HighColorMemory = sizeof(void*) == 8 ? 56 : 40;
		inline constexpr auto TrueColorMemory = sizeof(void*) == 8 ? 64 : 44;
		inline constexpr auto RowPixelCount = sizeof(void*) == 8 ? 72 : 48;
		inline constexpr auto BytesPerPixel = sizeof(void*) == 8 ? 76 : 52;
		inline constexpr auto RowByteCount = sizeof(void*) == 8 ? 80 : 56;
		inline constexpr auto IsVirtual = sizeof(void*) == 8 ? 84 : 60;
		inline constexpr auto SourceX = sizeof(void*) == 8 ? 88 : 64;
		inline constexpr auto SourceY = sizeof(void*) == 8 ? 92 : 68;
		inline constexpr auto SourceBitmap = sizeof(void*) == 8 ? 96 : 72;
		inline constexpr auto Palette = sizeof(void*) == 8 ? 104 : 76;
		inline constexpr auto FixedXDistance = sizeof(void*) == 8 ? 112 : 80;
	}

	class CFont;
	class CPalette;

	class CBitmap : public CStorable
	{
	public:
		// Original function: FUN_0040c601
		CBitmap(const SRectangle& rectangle, std::int32_t depth);
		CBitmap(const CBitmap&) = delete;
		CBitmap& operator=(const CBitmap&) = delete;

		CBitmap(std::int32_t width, std::int32_t height, std::int32_t depth);
		CBitmap(CBitmap* sourceBitmap, const SRectangle& rectangle);
		CBitmap(CBitmap* sourceBitmap, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);

		// Original function: FUN_0040c6dc
		CBitmap(void* file, std::uint32_t version);

		virtual ~CBitmap();
		void* Delete(std::uint32_t flags);

		// Original function: FUN_0040ee8c
		void SetPalettePtr(CPalette* palette);

		void Storable_SaveData(void* file) override;

		std::uint32_t Storable_GetVersion() override;

		void Fill(std::uint8_t colorIndex);
		void Fill(std::uint16_t color);
		void Fill(std::uint32_t color);
		void Fill(const SColorRGB& color);

		void CopyIntoBitmap(CBitmap* destination, std::int32_t destinationX, std::int32_t destinationY) const;
		void CopyIntoBitmap_ColorKeyed(CBitmap* destination, std::int32_t destinationX, std::int32_t destinationY, std::uint8_t colorKey) const;
		std::uint32_t Storable_GetId() override;
		void CopyIntoBitmap_FitIn(CBitmap* destination) const;
		void CopyIntoBitmap_HalfSize(CBitmap* destination, std::int32_t destinationX, std::int32_t destinationY) const;
		void CopyIntoBitmap_HalfSize_Optimized(CBitmap* destination) const;
		void ZoomIn() const;
		void* GetUnclippedPixelPtr(std::int32_t x, std::int32_t y) const;
		void FillWithBitmap(CBitmap* sourceBitmap, std::int32_t x, std::int32_t y);
		void FillWithBitmap(const SRectangle& rectangle, CBitmap* sourceBitmap, std::int32_t x, std::int32_t y);

		void Draw_SetPixel(std::int32_t x, std::int32_t y, std::uint8_t color);
		void Draw_Line(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint8_t color);
		void Draw_Circle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint8_t color);
		void Draw_SetPixel(std::int32_t x, std::int32_t y, std::uint16_t color);
		void Draw_Line(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint16_t color);
		void Draw_Circle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint16_t color);
		void Draw_SetPixel(std::int32_t x, std::int32_t y, std::uint32_t color);
		void Draw_Line(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t color);
		void Draw_Circle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint32_t color);
		void Draw_SetPixel(std::int32_t x, std::int32_t y, const SColorRGB& color);
		void Draw_Line(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, const SColorRGB& color);
		void Draw_Circle(std::int32_t x, std::int32_t y, std::uint32_t radius, const SColorRGB& color);
		void Draw_FilledCircle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint8_t color);
		void Draw_FilledCircle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint16_t color);
		void Draw_FilledCircle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint32_t color);
		void Draw_FilledCircle(std::int32_t x, std::int32_t y, std::uint32_t radius, const SColorRGB& color);
		void Draw_SetPixelUnclipped(std::int32_t x, std::int32_t y, const SColorRGB& color);
		void Draw_HorizontalLine(std::int32_t x, std::int32_t y, std::int32_t length, const SColorRGB& color);

		std::uint8_t Draw_GetPixel(std::int32_t x, std::int32_t y) const;
		std::uint16_t Draw_GetPixel_Word(std::int32_t x, std::int32_t y) const;
		std::uint32_t Draw_GetPixel_Long(std::int32_t x, std::int32_t y) const;

		void Draw_Box(SRectangle rectangle, std::uint8_t colorIndex);
		void Draw_Box(SRectangle rectangle, std::uint16_t color);
		void Draw_Box(SRectangle rectangle, std::uint32_t color);

		void Draw_Rectangle(SRectangle rectangle, std::uint8_t colorIndex);
		void Draw_Rectangle(SRectangle rectangle, std::uint16_t color);
		void Draw_Rectangle(SRectangle rectangle, std::uint32_t color);
		void Draw_Rectangle(const SRectangle& rectangle, const SColorRGB& color);

		void Draw_HorizontalLine(std::int32_t x, std::int32_t y, std::int32_t length, std::uint8_t colorIndex);
		void Draw_HorizontalLine(std::int32_t x, std::int32_t y, std::int32_t length, std::uint16_t color);
		void Draw_HorizontalLine(std::int32_t x, std::int32_t y, std::int32_t length, std::uint32_t color);

		void Draw_VerticalLine(std::int32_t x, std::uint32_t y, std::int32_t length, std::uint8_t colorIndex);
		void Draw_VerticalLine(std::int32_t x, std::uint32_t y, std::int32_t length, std::uint16_t color);
		void Draw_VerticalLine(std::int32_t x, std::uint32_t y, std::int32_t length, std::uint32_t color);
		void Draw_VerticalLine(std::int32_t x, std::uint32_t y, std::int32_t length, const SColorRGB& color);

		void Text_Print(CFont* font, std::int32_t x, std::int32_t y, const char* text);
		void __cdecl Text_PrintV(CFont* font, std::int32_t x, std::int32_t y, const char* format, ...);
		void Text_Print(CFont* font, const SColorRGB* colour, std::int32_t x, std::int32_t y, const char* text);
		void __cdecl Text_PrintV(CFont* font, const SColorRGB* colour, std::int32_t x, std::int32_t y, const char* format, ...);

		std::uint16_t GetHighColorWord(std::uint8_t colorIndex) const;
		const std::uint16_t* GetHighColorTablePtr() const;
		std::uint32_t GetTrueColorWord(std::uint8_t colorIndex) const;
		const std::uint32_t* GetTrueColorTablePtr() const;

		void Tool_Darken();
		void Draw_Box(const SRectangle& rectangle, const SColorRGB& color);
		void Tool_Darken(SRectangle rectangle);
		void Tool_Darken(SRectangle rectangle, std::uint8_t redMask, std::uint8_t greenMask, std::uint8_t blueMask);

		const SRectangle& GetRectangle() const noexcept
		{
			return mRectangle;
		}

		CBitmap* GetSourceBitmap() const noexcept
		{
			return mSourceBitmap;
		}

		std::int32_t GetSourceX() const noexcept
		{
			return mSourceX;
		}

		std::int32_t GetSourceY() const noexcept
		{
			return mSourceY;
		}

		static void operator delete(void* allocation) noexcept;
		static void operator delete(void* allocation, const std::nothrow_t&) noexcept;

	private:
		friend class CPicture;
		friend class NXSys2DApp::CDisplay;

		void l_ConstructNonVirtual(const SRectangle& rectangle, std::int32_t depth);
		void l_ConstructVirtual(CBitmap* sourceBitmap, const SRectangle& rectangle);
		void l_InitObject();
		void l_SetRawMemoryPtr();

		std::int32_t mDepth;                  // +0x04
		SRectangle mRectangle;                // +0x08
		std::uint32_t mRightBits;             // +0x18
		std::uint32_t mBottomBits;            // +0x1C
		void* mMemory;                        // +0x20
		std::uint8_t* mRawMemory;             // +0x24
		std::uint16_t* mHighColorMemory;      // +0x28
		std::uint32_t* mTrueColorMemory;      // +0x2C
		std::uint32_t mRowPixelCount;         // +0x30
		std::uint32_t mBytesPerPixel;         // +0x34
		std::uint32_t mRowByteCount;          // +0x38
		std::uint8_t mIsVirtual;              // +0x3C
		std::byte mPadding3D[3];              // +0x3D
		std::int32_t mSourceX;                // +0x40
		std::int32_t mSourceY;                // +0x44
		CBitmap* mSourceBitmap;               // +0x48
		void* mPalette;                       // +0x4C
		std::int32_t mFixedXDistance;         // +0x50
	};

	static_assert(sizeof(CBitmap) == (sizeof(void*) == 8 ? 120 : 84), "Unexpected native bitmap layout");
}

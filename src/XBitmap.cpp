#include "XFont.h"
#include "OriginalStrings.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "X2DDisplay.h"
#include "XBitmapTools.h"
#include "XCrtFileAccess.h"
#include "XDiagnostics.h"
#include "XFileAccess.h"
#include "XFontMetrics.h"
#include "XUnicodeFont.h"
#include "XMemory.h"
#include "XNumericConstants.h"
#include "XPaletteAccess.h"
#include "XPixelFormatConverter.h"
#include "XStorableIO.h"
#include "XSystemManager.h"

#include <cstdarg>
#include <cstddef>
#include <cstring>
#include <new>

namespace NXBasics
{
	template <typename T>

	T* BitmapPixelAddress(T* pixels, std::uint32_t index)
	{
		// Preserve the original 32-bit byte-offset arithmetic.
		const std::uint32_t offset = index * static_cast<std::uint32_t>(sizeof(T));

		return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(pixels) + offset);
	}

	std::int32_t ScaleBitmapCoordinate(std::int32_t sourceExtent, std::int32_t coordinate, std::int32_t destinationExtent)
	{
		// IMUL keeps the low 32 bits; IDIV then interprets them as signed.
		const std::uint32_t productBits = static_cast<std::uint32_t>(sourceExtent) * static_cast<std::uint32_t>(coordinate);
		std::int32_t product;
		std::memcpy(&product, &productBits, sizeof(product));

		return product / destinationExtent;
	}

	// Original function: LAB_0040c775
	std::uint32_t CBitmap::Storable_GetVersion()
	{

		return 0;
	}

	// Original function: LAB_0040d00a
	std::uint32_t CBitmap::Storable_GetId()
	{

		return 0x3F3;
	}

	// Original function: FUN_0040c5e5.
	void* CBitmap::Delete(std::uint32_t flags)
	{
		auto* result = this;
		this->~CBitmap();

		if ((flags & 1u) != 0)
			Runtime_Free(result);

		return result;
	}

	// Original function: FUN_0040c601
	CBitmap::CBitmap(const SRectangle& rectangle, std::int32_t depth) : mRectangle(0, 0, 0, 0)
	{
		l_InitObject();
		l_ConstructNonVirtual(rectangle, depth);
	}

	// Original function: FUN_0040c624
	CBitmap::CBitmap(std::int32_t width, std::int32_t height, std::int32_t depth) : mRectangle(0, 0, 0, 0)
	{
		l_InitObject();
		SRectangle rectangle(0, 0, width, height);
		l_ConstructNonVirtual(rectangle, depth);
	}

	// Original function: FUN_0040c65f
	CBitmap::CBitmap(CBitmap* sourceBitmap, const SRectangle& rectangle) : mRectangle(0, 0, 0, 0)
	{
		l_InitObject();
		l_ConstructVirtual(sourceBitmap, rectangle);
	}

	// Original function: FUN_0040c682.
	CBitmap::CBitmap(CBitmap* sourceBitmap, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) : mRectangle(0, 0, 0, 0)
	{
		l_InitObject();
		const SRectangle rectangle(x, y, width, height);
		l_ConstructVirtual(sourceBitmap, rectangle);
	}

	// Original function: FUN_0040c6dc
	CBitmap::CBitmap(void* file, std::uint32_t /*version*/) : mRectangle(0, 0, 0, 0)
	{
		l_InitObject();

		File_Read(file, &mDepth, 0x04);
		File_Read(file, &mRectangle, 0x10);

		mMemory = XB_Storable_LoadObject(file);
		l_SetRawMemoryPtr();

		mIsVirtual = 0;
		mRowPixelCount = static_cast<std::uint32_t>(mRectangle.width);
		mRowByteCount = mRowPixelCount * mBytesPerPixel;
	}

	// Original function: FUN_0040c6bf
	CBitmap::~CBitmap()
	{
		auto* memory = static_cast<CMemory*>(mMemory);

		if (memory != nullptr)
		{
			delete memory;
			mMemory = nullptr;
		}
	}

	// Original function: FUN_0040ee8c
	void CBitmap::SetPalettePtr(CPalette* palette)
	{
		mPalette = palette;
	}

	// Original function: FUN_0040c734
	void CBitmap::Storable_SaveData(void* file)
	{
		if (mIsVirtual != 0)
		{
			NXBasics::ReportFatalError(OriginalStrings::CannotSaveVirtualBitmap);
		}

		File_Write(file, &mDepth, 0x04);
		File_Write(file, &mRectangle, 0x10);
		static_cast<CMemory*>(mMemory)->Storable_Save(file);
	}

	// Original function: FUN_0040c778
	void CBitmap::l_ConstructNonVirtual(const SRectangle& rectangle, std::int32_t depth)
	{
		const std::uint32_t pixelCount = static_cast<std::uint32_t>(rectangle.width) * static_cast<std::uint32_t>(rectangle.height);

		std::uint32_t allocationSize;

		if (depth == 1)
		{
			mDepth = 1;
			allocationSize = pixelCount * 2u;
		}
		else if (depth == 2)
		{
			mDepth = 2;
			allocationSize = pixelCount * 4u;
		}
		else
		{
			mDepth = 0;
			allocationSize = pixelCount;
		}

		void* memoryStorage = Runtime_Allocate(sizeof(CMemory));
		void* memory = nullptr;

		if (memoryStorage != nullptr)
		{
			memory = ::new (memoryStorage) CMemory(allocationSize);
		}

		mMemory = memory;

		l_SetRawMemoryPtr();

		const std::uint32_t rowPixelCount = static_cast<std::uint32_t>(rectangle.width);

		mRowByteCount = mBytesPerPixel * rowPixelCount;
		mRowPixelCount = rowPixelCount;

		mRectangle.SetVariables(0, 0, rectangle.width, rectangle.height);

		mRightBits = static_cast<std::uint32_t>(mRectangle.x) + static_cast<std::uint32_t>(mRectangle.width) - 1u;
		mBottomBits = static_cast<std::uint32_t>(mRectangle.y) + static_cast<std::uint32_t>(mRectangle.height) - 1u;

		mIsVirtual = 0;
		mSourceX = 0;
		mSourceY = 0;
	}

	// Original function: FUN_0040c836
	void CBitmap::l_ConstructVirtual(CBitmap* sourceBitmap, const SRectangle& rectangle)
	{
		mIsVirtual = 1;
		mMemory = nullptr;
		mRawMemory = nullptr;
		mHighColorMemory = nullptr;
		mTrueColorMemory = nullptr;

		mDepth = sourceBitmap->mDepth;

		SRectangle clippedRectangle(rectangle);

		if (sourceBitmap->mRawMemory == nullptr)
		{
			return;
		}

		if (!clippedRectangle.IsTouching(sourceBitmap->mRectangle))
		{

			return;
		}

		clippedRectangle.CutInside(sourceBitmap->mRectangle);

		const std::int32_t sourceY = clippedRectangle.y;
		const std::uint32_t pixelOffset = sourceBitmap->mRowPixelCount * static_cast<std::uint32_t>(sourceY) + static_cast<std::uint32_t>(clippedRectangle.x);

		// Preserve the original 32-bit address arithmetic.

		if (mDepth == 1)
		{
			const std::uint32_t byteOffset = pixelOffset * 2u;

			mRawMemory = reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(sourceBitmap->mRawMemory) + byteOffset);
			mHighColorMemory = reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::uintptr_t>(sourceBitmap->mHighColorMemory) + byteOffset);
			mTrueColorMemory = nullptr;
		}
		else if (mDepth == 2)
		{
			const std::uint32_t byteOffset = pixelOffset * 4u;

			mRawMemory = reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(sourceBitmap->mRawMemory) + byteOffset);
			mTrueColorMemory = reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uintptr_t>(sourceBitmap->mTrueColorMemory) + byteOffset);
			mHighColorMemory = nullptr;
		}
		else
		{
			mRawMemory = reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(sourceBitmap->mRawMemory) + pixelOffset);
			mHighColorMemory = nullptr;
			mTrueColorMemory = nullptr;
		}

		mSourceX = clippedRectangle.x;
		mSourceY = sourceY;
		mSourceBitmap = sourceBitmap;

		mRectangle.SetVariables(0, 0, clippedRectangle.width, clippedRectangle.height);

		mRightBits = static_cast<std::uint32_t>(mRectangle.x) + static_cast<std::uint32_t>(mRectangle.width) - 1u;
		mBottomBits = static_cast<std::uint32_t>(mRectangle.y) + static_cast<std::uint32_t>(mRectangle.height) - 1u;

		mRowPixelCount = sourceBitmap->mRowPixelCount;
		mBytesPerPixel = sourceBitmap->mBytesPerPixel;
		mRowByteCount = sourceBitmap->mRowByteCount;
	}

	// Original function: FUN_0040c931
	void CBitmap::l_InitObject()
	{
		static_assert(offsetof(CBitmap, mDepth) == BitmapOffset::Depth);
		static_assert(offsetof(CBitmap, mRectangle) == BitmapOffset::Rectangle);
		static_assert(offsetof(CBitmap, mRightBits) == BitmapOffset::RightBits);
		static_assert(offsetof(CBitmap, mBottomBits) == BitmapOffset::BottomBits);
		static_assert(offsetof(CBitmap, mMemory) == BitmapOffset::Memory);
		static_assert(offsetof(CBitmap, mRawMemory) == BitmapOffset::RawMemory);
		static_assert(offsetof(CBitmap, mHighColorMemory) == BitmapOffset::HighColorMemory);
		static_assert(offsetof(CBitmap, mTrueColorMemory) == BitmapOffset::TrueColorMemory);
		static_assert(offsetof(CBitmap, mRowPixelCount) == BitmapOffset::RowPixelCount);
		static_assert(offsetof(CBitmap, mBytesPerPixel) == BitmapOffset::BytesPerPixel);
		static_assert(offsetof(CBitmap, mRowByteCount) == BitmapOffset::RowByteCount);
		static_assert(offsetof(CBitmap, mIsVirtual) == BitmapOffset::IsVirtual);
		static_assert(offsetof(CBitmap, mSourceX) == BitmapOffset::SourceX);
		static_assert(offsetof(CBitmap, mSourceY) == BitmapOffset::SourceY);
		static_assert(offsetof(CBitmap, mSourceBitmap) == BitmapOffset::SourceBitmap);
		static_assert(offsetof(CBitmap, mPalette) == BitmapOffset::Palette);
		static_assert(offsetof(CBitmap, mFixedXDistance) == BitmapOffset::FixedXDistance);
		std::memset(&mDepth, 0, sizeof(CBitmap) - offsetof(CBitmap, mDepth));
		mDepth = 0;
		mRectangle.Init();
	}

	// Original function: FUN_0040c953
	void CBitmap::l_SetRawMemoryPtr()
	{
		auto* memory = static_cast<CMemory*>(mMemory);

		if (mDepth == 1)
		{
			mRawMemory = memory->GetData();

			std::uint16_t* highColorMemory;
			highColorMemory = reinterpret_cast<std::uint16_t*>(memory->GetData());

			mTrueColorMemory = nullptr;
			mHighColorMemory = highColorMemory;
			mBytesPerPixel = 2;
		}
		else if (mDepth == 2)
		{
			mRawMemory = memory->GetData();

			std::uint32_t* trueColorMemory;
			trueColorMemory = reinterpret_cast<std::uint32_t*>(memory->GetData());

			mHighColorMemory = nullptr;
			mTrueColorMemory = trueColorMemory;
			mBytesPerPixel = 4;
		}
		else
		{
			std::uint8_t* rawMemory;
			rawMemory = memory->GetData();

			mHighColorMemory = nullptr;
			mTrueColorMemory = nullptr;
			mRawMemory = rawMemory;
			mBytesPerPixel = 1;
		}
	}

	// Original function: FUN_0040c9a8
	void CBitmap::Fill(std::uint8_t colorIndex)
	{
		std::uint8_t* pixels = mRawMemory;

		if (pixels == nullptr)
		{

			return;
		}

		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
				XB_Tool_Byte_FillBlock(pixels, static_cast<std::uint32_t>(mRectangle.width), mRectangle.height, mRowPixelCount, colorIndex);
				break;

			case SwitchIds::BitmapDepth::HighColor16:
			{
				const std::uint16_t packedColor = GetHighColorWord(colorIndex);
				Fill(packedColor);
				break;
			}

			case SwitchIds::BitmapDepth::TrueColor32:
			{
				const std::uint32_t packedColor = GetTrueColorWord(colorIndex);
				Fill(packedColor);
				break;
			}
		}
	}

	// Original function: FUN_0040ca04
	void CBitmap::Fill(std::uint16_t color)
	{
		std::uint16_t* pixels = mHighColorMemory;

		if (pixels == nullptr || mDepth != 1)
		{

			return;
		}

		XB_Tool_Word_FillBlock(pixels, mRectangle.width, mRectangle.height, mRowPixelCount, color);
	}

	// Original function: FUN_0040ca2a
	void CBitmap::Fill(std::uint32_t color)
	{
		std::uint32_t* pixels = mTrueColorMemory;

		if (pixels == nullptr || mDepth != 2)
		{

			return;
		}

		XB_Tool_Long_FillBlock(pixels, mRectangle.width, mRectangle.height, mRowPixelCount, color);
	}

	// Original function: FUN_0040ca50
	void CBitmap::Fill(const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
			{
				void* palette = mPalette;

				if (palette == nullptr)
				{
					palette = NXBasics::CXBSystemManager::sPalettePtr;
				}

				const std::uint8_t colorIndex = Palette_FindMatchingColor(palette, color);
				Fill(colorIndex);
				break;
			}

			case SwitchIds::BitmapDepth::HighColor16:
			{
				const std::uint16_t packedColor = g16BitPixelConverter->PackColor(color);
				Fill(packedColor);
				break;
			}

			case SwitchIds::BitmapDepth::TrueColor32:
			{
				const std::uint32_t packedColor = g32BitPixelConverter->PackColor(color);
				Fill(packedColor);
				break;
			}
		}
	}

	// Original function: FUN_0040cabb; preserve the original tile loop and its
	// unchecked source dimensions, including zero-size nontermination.
	void CBitmap::FillWithBitmap(CBitmap* sourceBitmap, std::int32_t x, std::int32_t y)
	{
		if (mRawMemory == nullptr || sourceBitmap->mRawMemory == nullptr)
		{

			return;
		}

		while (y < mRectangle.height)
		{
			std::int32_t tileX = x;

			while (tileX < mRectangle.width)
			{
				sourceBitmap->CopyIntoBitmap(this, tileX, y);
				tileX = Runtime_TimeSigned32(static_cast<std::uint32_t>(tileX) + static_cast<std::uint32_t>(sourceBitmap->mRectangle.width));
			}

			y = Runtime_TimeSigned32(static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(sourceBitmap->mRectangle.height));
		}
	}

	// Original function: FUN_0040cb0a; temporary virtual bitmap owns no pixels.
	void CBitmap::FillWithBitmap(const SRectangle& rectangle, CBitmap* sourceBitmap, std::int32_t x, std::int32_t y)
	{
		CBitmap area(this, rectangle);
		area.FillWithBitmap(sourceBitmap, x, y);
	}

	// Original function: FUN_0040cb4c
	void CBitmap::CopyIntoBitmap(CBitmap* destination, std::int32_t destinationX, std::int32_t destinationY) const
	{
		enum class CopyMode
		{
			None,
			Byte,
			Word,
			Long,
			ByteToWord,
			ByteToLong
		};

		CopyMode mode = CopyMode::None;
		const std::uint16_t* highColorTable = nullptr;
		const std::uint32_t* trueColorTable = nullptr;

		if (mDepth == 0 && destination->mDepth == 0)
		{
			if (mRawMemory == nullptr || destination->mRawMemory == nullptr)
				return;

			mode = CopyMode::Byte;
		}
		else if (mDepth == 1 && destination->mDepth == 1)
		{
			if (mHighColorMemory == nullptr || destination->mHighColorMemory == nullptr)
				return;

			mode = CopyMode::Word;
		}
		else if (mDepth == 2 && destination->mDepth == 2)
		{
			if (mTrueColorMemory == nullptr || destination->mTrueColorMemory == nullptr)
				return;

			mode = CopyMode::Long;
		}
		else if (mDepth == 0 && destination->mDepth == 1)
		{
			highColorTable = GetHighColorTablePtr();

			if (highColorTable == nullptr)
				return;

			if (mRawMemory == nullptr || destination->mHighColorMemory == nullptr)
				return;

			mode = CopyMode::ByteToWord;
		}
		else if (mDepth == 0 && destination->mDepth == 2)
		{
			trueColorTable = GetTrueColorTablePtr();

			if (trueColorTable == nullptr)
				return;

			if (mRawMemory == nullptr || destination->mTrueColorMemory == nullptr)
				return;

			mode = CopyMode::ByteToLong;
		}

		if (mode != CopyMode::None)
		{
			const SRectangle destinationBounds(0, 0, destination->mRectangle.width, destination->mRectangle.height);
			SRectangle area(destinationX, destinationY, mRectangle.width, mRectangle.height);

			if (!area.IsTouching(destinationBounds))
				return;

			area.CutInside(destinationBounds);

			const std::uint32_t sourceX = static_cast<std::uint32_t>(area.x) - static_cast<std::uint32_t>(destinationX);
			const std::uint32_t sourceY = static_cast<std::uint32_t>(area.y) - static_cast<std::uint32_t>(destinationY);
			const std::uint32_t sourceIndex = sourceY * mRowPixelCount + sourceX;
			const std::uint32_t destinationIndex = static_cast<std::uint32_t>(area.y) * destination->mRowPixelCount + static_cast<std::uint32_t>(area.x);

			switch (mode)
			{
				case CopyMode::Byte:
					XB_Tool_Byte_CopyBlock(BitmapPixelAddress(mRawMemory, sourceIndex), BitmapPixelAddress(destination->mRawMemory, destinationIndex), static_cast<std::uint32_t>(area.width), area.height, mRowPixelCount, destination->mRowPixelCount);
					break;

				case CopyMode::Word:
					XB_Tool_Word_CopyBlock(BitmapPixelAddress(mHighColorMemory, sourceIndex), BitmapPixelAddress(destination->mHighColorMemory, destinationIndex), static_cast<std::uint32_t>(area.width), area.height, mRowPixelCount, destination->mRowPixelCount);
					break;

				case CopyMode::Long:
					XB_Tool_Long_CopyBlock(BitmapPixelAddress(mTrueColorMemory, sourceIndex), BitmapPixelAddress(destination->mTrueColorMemory, destinationIndex), static_cast<std::uint32_t>(area.width), area.height, mRowPixelCount, destination->mRowPixelCount);
					break;

				case CopyMode::ByteToWord:
					XB_Tool_ByteToWord_CopyBlock(BitmapPixelAddress(mRawMemory, sourceIndex), BitmapPixelAddress(destination->mHighColorMemory, destinationIndex), area.width, area.height, mRowPixelCount, destination->mRowPixelCount, highColorTable);
					break;

				case CopyMode::ByteToLong:
					XB_Tool_ByteToLong_CopyBlock(BitmapPixelAddress(mRawMemory, sourceIndex), BitmapPixelAddress(destination->mTrueColorMemory, destinationIndex), area.width, area.height, mRowPixelCount, destination->mRowPixelCount, trueColorTable);
					break;

				default:
					break;
			}

			return;
		}

		// These conversion paths ignore the requested destination position.

		if (mDepth == 1 && destination->mDepth == 2)
		{
			if (mRectangle == destination->mRectangle)
			{
				const auto* highColorConverter = g16BitPixelConverter;
				const auto* trueColorConverter = g32BitPixelConverter;

				for (std::int32_t y = 0; y < mRectangle.height; ++y)
				{
					for (std::int32_t x = 0; x < mRectangle.width; ++x)
					{
						const std::uint16_t pixel = Draw_GetPixel_Word(x, y);

						// The original uses adjustment fields at +0x2C/+0x30/+0x34.
						const std::uint32_t red = ((static_cast<std::uint32_t>(pixel) & highColorConverter->mRedMask) >> (static_cast<std::uint32_t>(highColorConverter->mRedAdjustment) & 31u)) << 3;
						const std::uint32_t green = ((static_cast<std::uint32_t>(pixel) & highColorConverter->mGreenMask) >> (static_cast<std::uint32_t>(highColorConverter->mGreenAdjustment) & 31u)) << 3;
						const std::uint32_t blue = ((static_cast<std::uint32_t>(pixel) & highColorConverter->mBlueMask) >> (static_cast<std::uint32_t>(highColorConverter->mBlueAdjustment) & 31u)) << 3;
						const std::uint32_t color = trueColorConverter->PackColor(red, green, blue);

						const std::uint32_t index = static_cast<std::uint32_t>(y) * destination->mRowPixelCount + static_cast<std::uint32_t>(x);
						std::memcpy(BitmapPixelAddress(destination->mTrueColorMemory, index), &color, sizeof(color));
					}
				}
			}
		}

		if (mDepth == 2 && destination->mDepth == 1)
		{
			if (!(mRectangle == destination->mRectangle))
			{

				return;
			}

			for (std::int32_t y = 0; y < mRectangle.height; ++y)
			{
				for (std::int32_t x = 0; x < mRectangle.width; ++x)
				{
					const std::uint32_t pixel = Draw_GetPixel_Long(x, y);
					const std::uint32_t red = (pixel >> 16) & NXBasics::IntegerRange::UInt8Maximum;
					const std::uint32_t green = (pixel >> 8) & NXBasics::IntegerRange::UInt8Maximum;
					const std::uint32_t blue = pixel & NXBasics::IntegerRange::UInt8Maximum;
					const std::uint16_t color = g16BitPixelConverter->PackColor(red, green, blue);

					const std::uint32_t index = static_cast<std::uint32_t>(y) * destination->mRowPixelCount + static_cast<std::uint32_t>(x);
					std::memcpy(BitmapPixelAddress(destination->mHighColorMemory, index), &color, sizeof(color));
				}
			}
		}
	}

	// Original function: FUN_0040d010; only indexed source bitmaps are supported.
	void CBitmap::CopyIntoBitmap_ColorKeyed(CBitmap* destination, std::int32_t destinationX, std::int32_t destinationY, std::uint8_t colorKey) const
	{
		if (mDepth != 0 || mRawMemory == nullptr || destination->mRawMemory == nullptr)
			return;

		const SRectangle bounds(0, 0, destination->mRectangle.width, destination->mRectangle.height);
		SRectangle area(destinationX, destinationY, mRectangle.width, mRectangle.height);

		if (!area.IsTouching(bounds))
			return;

		area.CutInside(bounds);
		const std::uint16_t* highColorTable = nullptr;
		const std::uint32_t* trueColorTable = nullptr;

		if (destination->mDepth == 1)
		{
			highColorTable = GetHighColorTablePtr();

			if (highColorTable == nullptr)
				return;
		}
		else if (destination->mDepth == 2)
		{
			trueColorTable = GetTrueColorTablePtr();

			if (trueColorTable == nullptr)
				return;
		}
		else if (destination->mDepth != 0)
		{

			return;
		}

		std::uint32_t sourceIndex = (static_cast<std::uint32_t>(area.y) - static_cast<std::uint32_t>(destinationY)) * mRowPixelCount + static_cast<std::uint32_t>(area.x) - static_cast<std::uint32_t>(destinationX);
		std::uint32_t destinationIndex = static_cast<std::uint32_t>(area.y) * destination->mRowPixelCount + static_cast<std::uint32_t>(area.x);

		for (std::int32_t row = 0; row < area.height; ++row)
		{
			for (std::int32_t column = 0; column < area.width; ++column)
			{
				const std::uint8_t pixel = *BitmapPixelAddress(mRawMemory, sourceIndex + static_cast<std::uint32_t>(column));

				if (pixel == colorKey)
					continue;

				const std::uint32_t target = destinationIndex + static_cast<std::uint32_t>(column);

				if (destination->mDepth == 0)
				{
					*BitmapPixelAddress(destination->mRawMemory, target) = pixel;
				}
				else if (destination->mDepth == 1)
				{
					*BitmapPixelAddress(destination->mHighColorMemory, target) = highColorTable[pixel];
				}
				else
				{
					*BitmapPixelAddress(destination->mTrueColorMemory, target) = trueColorTable[pixel];
				}
			}

			sourceIndex += mRowPixelCount;
			destinationIndex += destination->mRowPixelCount;
		}
	}

	// Original function: FUN_0040d1f8.
	void CBitmap::CopyIntoBitmap_HalfSize(CBitmap* destination, std::int32_t destinationX, std::int32_t destinationY) const
	{
		if (mDepth != destination->mDepth || mRawMemory == nullptr || destination->mRawMemory == nullptr)
			return;

		const SRectangle bounds(0, 0, destination->mRectangle.width, destination->mRectangle.height);
		SRectangle area(destinationX, destinationY, mRectangle.width / 2, mRectangle.height / 2);
		std::int32_t sourceY = 0, sourceX = 0;

		if (area.y < 0)
		{
			sourceY = Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(area.y));
			area.y = 0;
		}

		if (area.x < 0)
		{
			sourceX = Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(area.x));
			area.x = 0;
		}
		// Windows checks full containment here; it does not clip the far edges.

		if (!area.IsInside(bounds))
			return;

		std::uint32_t sourceIndex = (mRowPixelCount * static_cast<std::uint32_t>(sourceY) + static_cast<std::uint32_t>(sourceX)) * 2u;
		std::uint32_t destinationIndex = destination->mRowPixelCount * static_cast<std::uint32_t>(area.y) + static_cast<std::uint32_t>(area.x);

		for (std::int32_t row = sourceY; row < area.height; ++row)
		{
			// Retain the original additional sourceX offset and destination gap.

			for (std::int32_t column = sourceX; column < area.width; ++column)
			{
				const auto sourcePixel = sourceIndex + static_cast<std::uint32_t>(column) * 2u;
				const auto destinationPixel = destinationIndex + static_cast<std::uint32_t>(column);

				if (mDepth == 0)
				{
					*BitmapPixelAddress(destination->mRawMemory, destinationPixel) = *BitmapPixelAddress(mRawMemory, sourcePixel);
				}
				else if (mDepth == 1)
				{
					*BitmapPixelAddress(destination->mHighColorMemory, destinationPixel) = *BitmapPixelAddress(mHighColorMemory, sourcePixel);
				}
				else if (mDepth == 2)
				{
					*BitmapPixelAddress(destination->mTrueColorMemory, destinationPixel) = *BitmapPixelAddress(mTrueColorMemory, sourcePixel);
				}
				else
				{

					return;
				}
			}

			sourceIndex += mRowPixelCount * 2u;
			destinationIndex += destination->mRowPixelCount;
		}
	}

	// Original function: FUN_0040d3f8; the packed-copy path keeps its original look-ahead reads.
	void CBitmap::CopyIntoBitmap_HalfSize_Optimized(CBitmap* destination) const
	{
		const auto sourceWidth = static_cast<std::uint32_t>(mRectangle.width);
		const auto destinationWidth = static_cast<std::uint32_t>(destination->mRectangle.width);

		if (sourceWidth != destinationWidth * 2u || static_cast<std::uint32_t>(mRectangle.height) != static_cast<std::uint32_t>(destination->mRectangle.height) * 2u || mDepth != destination->mDepth || (mDepth != 1 && mDepth != 2) || (sourceWidth & 63u) != 0 || (static_cast<std::uint32_t>(mRectangle.height) & 1u) != 0)
		{
			CopyIntoBitmap_HalfSize(destination, 0, 0);

			return;
		}

		if (mDepth == 1)
		{
			auto* source = mHighColorMemory;
			auto* target = destination->mHighColorMemory;
			std::uint32_t sourceIndex = 0, destinationIndex = 0;
			auto rows = static_cast<std::uint32_t>(destination->mRectangle.height);
			const auto groupsPerRow = static_cast<std::uint32_t>(destination->mRectangle.width / 8);
			do
			{
				auto groups = groupsPerRow;
				do
				{
					// These are real original MOV reads, rather than CPU prefetch hints.
					(void)*reinterpret_cast<const volatile std::uint32_t*>(BitmapPixelAddress(source, sourceIndex + 64u));
					(void)*reinterpret_cast<const volatile std::uint32_t*>(BitmapPixelAddress(target, destinationIndex + 32u));

					for (std::uint32_t pair = 0; pair < 4; ++pair)
					{
						std::uint32_t packed;
						std::memcpy(&packed, BitmapPixelAddress(source, sourceIndex + 1u), sizeof(packed));
						packed = (packed & NXBasics::BitMask::DwordExceptBits0Through15) | *BitmapPixelAddress(source, sourceIndex);
						std::memcpy(BitmapPixelAddress(target, destinationIndex), &packed, sizeof(packed));
						sourceIndex += 4u;
						destinationIndex += 2u;
					}
				} while (--groups != 0);
				sourceIndex += mRowPixelCount * 2u - sourceWidth;
				destinationIndex += destination->mRowPixelCount - destinationWidth;
			} while (--rows != 0);
		}
		else
		{
			std::uint32_t sourceIndex = 0, destinationIndex = 0;
			auto rows = static_cast<std::uint32_t>(destination->mRectangle.height);
			const auto groupsPerRow = static_cast<std::uint32_t>(destination->mRectangle.width / 2);
			do
			{
				auto groups = groupsPerRow;
				do
				{
					*BitmapPixelAddress(destination->mTrueColorMemory, destinationIndex++) = *BitmapPixelAddress(mTrueColorMemory, sourceIndex);
					sourceIndex += 2u;
					*BitmapPixelAddress(destination->mTrueColorMemory, destinationIndex++) = *BitmapPixelAddress(mTrueColorMemory, sourceIndex);
					sourceIndex += 2u;
				} while (--groups != 0);
				sourceIndex += mRowPixelCount * 2u - sourceWidth;
				destinationIndex += destination->mRowPixelCount - destinationWidth;
			} while (--rows != 0);
		}
	}

	// Original function: FUN_0040d56b
	void CBitmap::CopyIntoBitmap_FitIn(CBitmap* destination) const
	{
		if (mRawMemory == nullptr)
			return;

		if (destination->mRawMemory == nullptr)
			return;

		const std::int32_t destinationWidth = destination->mRectangle.width;
		const std::int32_t destinationHeight = destination->mRectangle.height;

		enum class ScaleMode
		{
			Byte,
			ByteToWord,
			ByteToLong,
			Word,
			Long
		};

		ScaleMode mode;
		const std::uint16_t* highColorTable = nullptr;
		const std::uint32_t* trueColorTable = nullptr;

		if (mDepth == 0 && destination->mDepth == 0)
		{
			mode = ScaleMode::Byte;
		}
		else if (mDepth == 0 && destination->mDepth == 1)
		{
			highColorTable = GetHighColorTablePtr();

			if (highColorTable == nullptr)
				return;

			mode = ScaleMode::ByteToWord;
		}
		else if (mDepth == 0 && destination->mDepth == 2)
		{
			trueColorTable = GetTrueColorTablePtr();

			if (trueColorTable == nullptr)
				return;

			mode = ScaleMode::ByteToLong;
		}
		else if (mDepth == 1 && destination->mDepth == 1)
		{
			mode = ScaleMode::Word;
		}
		else if (mDepth == 2 && destination->mDepth == 2)
		{
			mode = ScaleMode::Long;
		}
		else
		{
			return;
		}

		if (mode == ScaleMode::Byte || mode == ScaleMode::ByteToWord)
		{
			for (std::int32_t y = 0; y < destinationHeight; ++y)
			{
				const std::int32_t sourceY = ScaleBitmapCoordinate(mRectangle.height, y, destinationHeight);
				const std::uint32_t sourceRowIndex = static_cast<std::uint32_t>(sourceY) * mRowPixelCount;
				const auto* sourceRow = BitmapPixelAddress(mRawMemory, sourceRowIndex);

				const std::uint32_t destinationRowIndex = static_cast<std::uint32_t>(y) * destination->mRowPixelCount;
				std::uintptr_t destinationAddress;

				if (mode == ScaleMode::Byte)
				{
					destinationAddress = reinterpret_cast<std::uintptr_t>(BitmapPixelAddress(destination->mRawMemory, destinationRowIndex));
				}
				else
				{
					destinationAddress = reinterpret_cast<std::uintptr_t>(BitmapPixelAddress(destination->mHighColorMemory, destinationRowIndex));
				}

				for (std::int32_t x = 0; x < destinationWidth; ++x)
				{
					const std::int32_t sourceX = ScaleBitmapCoordinate(mRectangle.width, x, destinationWidth);
					const std::uint8_t index = *BitmapPixelAddress(sourceRow, static_cast<std::uint32_t>(sourceX));

					if (mode == ScaleMode::Byte)
					{
						*reinterpret_cast<std::uint8_t*>(destinationAddress) = index;
						++destinationAddress;
					}
					else
					{
						const std::uint16_t color = highColorTable[index];
						std::memcpy(reinterpret_cast<void*>(destinationAddress), &color, sizeof(color));
						destinationAddress += 2u;
					}
				}
			}

			return;
		}

		for (std::int32_t y = 0; y < destinationHeight; ++y)
		{
			for (std::int32_t x = 0; x < destinationWidth; ++x)
			{
				const std::int32_t sourceX = ScaleBitmapCoordinate(mRectangle.width, x, destinationWidth);
				const std::int32_t sourceY = ScaleBitmapCoordinate(mRectangle.height, y, destinationHeight);

				if (mode == ScaleMode::ByteToLong)
				{
					const std::uint8_t index = Draw_GetPixel(sourceX, sourceY);
					const std::uint32_t color = trueColorTable[index];
					const std::uint32_t destinationIndex = static_cast<std::uint32_t>(y) * destination->mRowPixelCount + static_cast<std::uint32_t>(x);
					std::memcpy(BitmapPixelAddress(destination->mTrueColorMemory, destinationIndex), &color, sizeof(color));
				}
				else if (mode == ScaleMode::Word)
				{
					const std::uint16_t color = Draw_GetPixel_Word(sourceX, sourceY);
					const std::uint32_t destinationIndex = static_cast<std::uint32_t>(y) * destination->mRowPixelCount + static_cast<std::uint32_t>(x);
					std::memcpy(BitmapPixelAddress(destination->mHighColorMemory, destinationIndex), &color, sizeof(color));
				}
				else
				{
					const std::uint32_t color = Draw_GetPixel_Long(sourceX, sourceY);
					const std::uint32_t destinationIndex = static_cast<std::uint32_t>(y) * destination->mRowPixelCount + static_cast<std::uint32_t>(x);
					std::memcpy(BitmapPixelAddress(destination->mTrueColorMemory, destinationIndex), &color, sizeof(color));
				}
			}
		}
	}

	// Original function: FUN_0040d7f5.
	void CBitmap::Draw_SetPixel(std::int32_t x, std::int32_t y, std::uint8_t color)
	{
		if (mRawMemory == nullptr || x < mRectangle.x || x > Runtime_TimeSigned32(mRightBits) || y < mRectangle.y || y > Runtime_TimeSigned32(mBottomBits))
		{

			return;
		}

		const auto index = static_cast<std::uint32_t>(y) * mRowPixelCount + static_cast<std::uint32_t>(x);

		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
				*BitmapPixelAddress(mRawMemory, index) = color;
				break;
			case SwitchIds::BitmapDepth::HighColor16:
				*BitmapPixelAddress(mHighColorMemory, index) = GetHighColorWord(color);
				break;
			case SwitchIds::BitmapDepth::TrueColor32:
				*BitmapPixelAddress(mTrueColorMemory, index) = GetTrueColorWord(color);
				break;
		}
	}

	// Original function: FUN_0040d872.
	void CBitmap::Draw_SetPixel(std::int32_t x, std::int32_t y, std::uint16_t color)
	{
		if (mRawMemory == nullptr || x < mRectangle.x || x > Runtime_TimeSigned32(mRightBits) || y < mRectangle.y || y > Runtime_TimeSigned32(mBottomBits))
		{

			return;
		}

		const auto index = static_cast<std::uint32_t>(y) * mRowPixelCount + static_cast<std::uint32_t>(x);

		if (mDepth == 1)
			*BitmapPixelAddress(mHighColorMemory, index) = color;
	}

	// Original function: FUN_0040d8b3.
	void CBitmap::Draw_SetPixel(std::int32_t x, std::int32_t y, std::uint32_t color)
	{
		if (mRawMemory == nullptr || x < mRectangle.x || x > Runtime_TimeSigned32(mRightBits) || y < mRectangle.y || y > Runtime_TimeSigned32(mBottomBits))
		{

			return;
		}

		const auto index = static_cast<std::uint32_t>(y) * mRowPixelCount + static_cast<std::uint32_t>(x);

		if (mDepth == 2)
			*BitmapPixelAddress(mTrueColorMemory, index) = color;
	}

	// Original function: FUN_0040d8f2.
	void CBitmap::Draw_SetPixel(std::int32_t x, std::int32_t y, const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
				Draw_SetPixel(x, y, Palette_FindMatchingColor(mPalette != nullptr ? mPalette : NXBasics::CXBSystemManager::sPalettePtr, color));
				break;
			case SwitchIds::BitmapDepth::HighColor16:
				Draw_SetPixel(x, y, g16BitPixelConverter->PackColor(color));
				break;
			case SwitchIds::BitmapDepth::TrueColor32:
				Draw_SetPixel(x, y, g32BitPixelConverter->PackColor(color));
				break;
		}
	}

	// Original function: FUN_0040df78.
	void CBitmap::Draw_HorizontalLine(std::int32_t x, std::int32_t y, std::int32_t length, const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
				Draw_HorizontalLine(x, y, length, Palette_FindMatchingColor(mPalette != nullptr ? mPalette : NXBasics::CXBSystemManager::sPalettePtr, color));
				break;
			case SwitchIds::BitmapDepth::HighColor16:
				Draw_HorizontalLine(x, y, length, g16BitPixelConverter->PackColor(color));
				break;
			case SwitchIds::BitmapDepth::TrueColor32:
				Draw_HorizontalLine(x, y, length, g32BitPixelConverter->PackColor(color));
				break;
		}
	}

	// Original function: FUN_0040e4d1.
	void CBitmap::Draw_Line(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
				Draw_Line(x1, y1, x2, y2, Palette_FindMatchingColor(mPalette != nullptr ? mPalette : NXBasics::CXBSystemManager::sPalettePtr, color));
				break;
			case SwitchIds::BitmapDepth::HighColor16:
				Draw_Line(x1, y1, x2, y2, g16BitPixelConverter->PackColor(color));
				break;
			case SwitchIds::BitmapDepth::TrueColor32:
				Draw_Line(x1, y1, x2, y2, g32BitPixelConverter->PackColor(color));
				break;
		}
	}

	// Original function: FUN_0040e8bd.
	void CBitmap::Draw_Circle(std::int32_t x, std::int32_t y, std::uint32_t radius, const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
				Draw_Circle(x, y, radius, Palette_FindMatchingColor(mPalette != nullptr ? mPalette : NXBasics::CXBSystemManager::sPalettePtr, color));
				break;
			case SwitchIds::BitmapDepth::HighColor16:
				Draw_Circle(x, y, radius, g16BitPixelConverter->PackColor(color));
				break;
			case SwitchIds::BitmapDepth::TrueColor32:
				Draw_Circle(x, y, radius, g32BitPixelConverter->PackColor(color));
				break;
		}
	}

	// Original function: FUN_0040d96f; deliberately skips memory and bounds checks.
	void CBitmap::Draw_SetPixelUnclipped(std::int32_t x, std::int32_t y, const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
				*BitmapPixelAddress(mRawMemory, static_cast<std::uint32_t>(y) * mRowPixelCount + static_cast<std::uint32_t>(x)) = Palette_FindMatchingColor(mPalette != nullptr ? mPalette : NXBasics::CXBSystemManager::sPalettePtr, color);
				break;
			case SwitchIds::BitmapDepth::HighColor16:
			{
				const auto packed = g16BitPixelConverter->PackColor(color);
				*BitmapPixelAddress(mHighColorMemory, static_cast<std::uint32_t>(y) * mRowPixelCount + static_cast<std::uint32_t>(x)) = packed;
				break;
			}
			case SwitchIds::BitmapDepth::TrueColor32:
			{
				const auto packed = g32BitPixelConverter->PackColor(color);
				*BitmapPixelAddress(mTrueColorMemory, static_cast<std::uint32_t>(y) * mRowPixelCount + static_cast<std::uint32_t>(x)) = packed;
				break;
			}
		}
	}

	// Original function: FUN_0040e12f.
	void CBitmap::Draw_Line(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint8_t color)
	{
		if (mRawMemory == nullptr)
			return;

		if ((x1 < 0 && x2 < 0) || (y1 < 0 && y2 < 0) || (x1 > Runtime_TimeSigned32(mRightBits) && x2 > Runtime_TimeSigned32(mRightBits)) || (y1 > Runtime_TimeSigned32(mBottomBits) && y2 > Runtime_TimeSigned32(mBottomBits)))
		{

			return;
		}

		auto dx = static_cast<std::uint32_t>(x2) - static_cast<std::uint32_t>(x1);
		auto dy = static_cast<std::uint32_t>(y2) - static_cast<std::uint32_t>(y1);
		const std::uint32_t stepX = Runtime_TimeSigned32(dx) > 0 ? 1u : (dx == 0 ? 0u : NXBasics::IntegerRange::UInt32Maximum);
		const std::uint32_t stepY = Runtime_TimeSigned32(dy) > 0 ? 1u : (dy == 0 ? 0u : NXBasics::IntegerRange::UInt32Maximum);

		if (Runtime_TimeSigned32(dx) < 0)
			dx = 0u - dx;

		if (Runtime_TimeSigned32(dy) < 0)
			dy = 0u - dy;

		auto x = static_cast<std::uint32_t>(x1), y = static_cast<std::uint32_t>(y1);

		if (Runtime_TimeSigned32(dy) <= Runtime_TimeSigned32(dx))
		{
			auto error = 2u * dy - dx;
			Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);

			while (x != static_cast<std::uint32_t>(x2))
			{
				if (Runtime_TimeSigned32(error) >= 0)
				{
					error -= 2u * dx;
					y += stepY;
				}

				x += stepX;
				error += 2u * dy;
				Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);
			}
		}
		else
		{
			auto error = 2u * dx - dy;
			Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);

			while (y != static_cast<std::uint32_t>(y2))
			{
				if (Runtime_TimeSigned32(error) >= 0)
				{
					error -= 2u * dy;
					x += stepX;
				}

				y += stepY;
				error += 2u * dx;
				Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);
			}
		}
	}

	// Original function: FUN_0040e265.
	void CBitmap::Draw_Line(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint16_t color)
	{
		if (mRawMemory == nullptr)
			return;

		if ((x1 < 0 && x2 < 0) || (y1 < 0 && y2 < 0) || (x1 > Runtime_TimeSigned32(mRightBits) && x2 > Runtime_TimeSigned32(mRightBits)) || (y1 > Runtime_TimeSigned32(mBottomBits) && y2 > Runtime_TimeSigned32(mBottomBits)))
		{

			return;
		}

		auto dx = static_cast<std::uint32_t>(x2) - static_cast<std::uint32_t>(x1);
		auto dy = static_cast<std::uint32_t>(y2) - static_cast<std::uint32_t>(y1);
		const std::uint32_t stepX = Runtime_TimeSigned32(dx) > 0 ? 1u : (dx == 0 ? 0u : NXBasics::IntegerRange::UInt32Maximum);
		const std::uint32_t stepY = Runtime_TimeSigned32(dy) > 0 ? 1u : (dy == 0 ? 0u : NXBasics::IntegerRange::UInt32Maximum);

		if (Runtime_TimeSigned32(dx) < 0)
			dx = 0u - dx;

		if (Runtime_TimeSigned32(dy) < 0)
			dy = 0u - dy;

		auto x = static_cast<std::uint32_t>(x1), y = static_cast<std::uint32_t>(y1);

		if (Runtime_TimeSigned32(dy) <= Runtime_TimeSigned32(dx))
		{
			auto error = 2u * dy - dx;
			Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);

			while (x != static_cast<std::uint32_t>(x2))
			{
				if (Runtime_TimeSigned32(error) >= 0)
				{
					error -= 2u * dx;
					y += stepY;
				}

				x += stepX;
				error += 2u * dy;
				Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);
			}
		}
		else
		{
			auto error = 2u * dx - dy;
			Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);

			while (y != static_cast<std::uint32_t>(y2))
			{
				if (Runtime_TimeSigned32(error) >= 0)
				{
					error -= 2u * dy;
					x += stepX;
				}

				y += stepY;
				error += 2u * dx;
				Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);
			}
		}
	}

	// Original function: FUN_0040e39b.
	void CBitmap::Draw_Line(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t color)
	{
		if (mRawMemory == nullptr)
			return;

		if ((x1 < 0 && x2 < 0) || (y1 < 0 && y2 < 0) || (x1 > Runtime_TimeSigned32(mRightBits) && x2 > Runtime_TimeSigned32(mRightBits)) || (y1 > Runtime_TimeSigned32(mBottomBits) && y2 > Runtime_TimeSigned32(mBottomBits)))
		{

			return;
		}

		auto dx = static_cast<std::uint32_t>(x2) - static_cast<std::uint32_t>(x1);
		auto dy = static_cast<std::uint32_t>(y2) - static_cast<std::uint32_t>(y1);
		const std::uint32_t stepX = Runtime_TimeSigned32(dx) > 0 ? 1u : (dx == 0 ? 0u : NXBasics::IntegerRange::UInt32Maximum);
		const std::uint32_t stepY = Runtime_TimeSigned32(dy) > 0 ? 1u : (dy == 0 ? 0u : NXBasics::IntegerRange::UInt32Maximum);

		if (Runtime_TimeSigned32(dx) < 0)
			dx = 0u - dx;

		if (Runtime_TimeSigned32(dy) < 0)
			dy = 0u - dy;

		auto x = static_cast<std::uint32_t>(x1), y = static_cast<std::uint32_t>(y1);

		if (Runtime_TimeSigned32(dy) <= Runtime_TimeSigned32(dx))
		{
			auto error = 2u * dy - dx;
			Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);

			while (x != static_cast<std::uint32_t>(x2))
			{
				if (Runtime_TimeSigned32(error) >= 0)
				{
					error -= 2u * dx;
					y += stepY;
				}

				x += stepX;
				error += 2u * dy;
				Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);
			}
		}
		else
		{
			auto error = 2u * dx - dy;
			Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);

			while (y != static_cast<std::uint32_t>(y2))
			{
				if (Runtime_TimeSigned32(error) >= 0)
				{
					error -= 2u * dy;
					x += stepX;
				}

				y += stepY;
				error += 2u * dx;
				Draw_SetPixel(Runtime_TimeSigned32(x), Runtime_TimeSigned32(y), color);
			}
		}
	}

	// Original function: FUN_0040e560.
	void CBitmap::Draw_Circle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint8_t color)
	{
		std::uint32_t offset = 0, error = 3u - 2u * radius, fourRadius = 4u * radius;

		if (Runtime_TimeSigned32(radius) < 0)
		{

			return;
		}

		const auto centerX = static_cast<std::uint32_t>(x), centerY = static_cast<std::uint32_t>(y);
		do
		{
			Draw_SetPixel(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY + radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + offset), Runtime_TimeSigned32(centerY + radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY - radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + offset), Runtime_TimeSigned32(centerY - radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY + offset), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + radius), Runtime_TimeSigned32(centerY + offset), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY - offset), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + radius), Runtime_TimeSigned32(centerY - offset), color);

			if (Runtime_TimeSigned32(error) >= 0)
			{
				error += 4u - fourRadius;
				--radius;
				fourRadius -= 4u;
			}

			error += 4u * offset + 2u;
			++offset;
		} while (Runtime_TimeSigned32(offset) <= Runtime_TimeSigned32(radius));
	}

	// Original function: FUN_0040e67f.
	void CBitmap::Draw_Circle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint16_t color)
	{
		std::uint32_t offset = 0, error = 3u - 2u * radius, fourRadius = 4u * radius;

		if (Runtime_TimeSigned32(radius) < 0)
		{

			return;
		}

		const auto centerX = static_cast<std::uint32_t>(x), centerY = static_cast<std::uint32_t>(y);
		do
		{
			Draw_SetPixel(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY + radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + offset), Runtime_TimeSigned32(centerY + radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY - radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + offset), Runtime_TimeSigned32(centerY - radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY + offset), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + radius), Runtime_TimeSigned32(centerY + offset), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY - offset), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + radius), Runtime_TimeSigned32(centerY - offset), color);

			if (Runtime_TimeSigned32(error) >= 0)
			{
				error += 4u - fourRadius;
				--radius;
				fourRadius -= 4u;
			}

			error += 4u * offset + 2u;
			++offset;
		} while (Runtime_TimeSigned32(offset) <= Runtime_TimeSigned32(radius));
	}

	// Original function: FUN_0040e79e.
	void CBitmap::Draw_Circle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint32_t color)
	{
		std::uint32_t offset = 0, error = 3u - 2u * radius, fourRadius = 4u * radius;

		if (Runtime_TimeSigned32(radius) < 0)
		{

			return;
		}

		const auto centerX = static_cast<std::uint32_t>(x), centerY = static_cast<std::uint32_t>(y);
		do
		{
			Draw_SetPixel(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY + radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + offset), Runtime_TimeSigned32(centerY + radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY - radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + offset), Runtime_TimeSigned32(centerY - radius), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY + offset), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + radius), Runtime_TimeSigned32(centerY + offset), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY - offset), color);
			Draw_SetPixel(Runtime_TimeSigned32(centerX + radius), Runtime_TimeSigned32(centerY - offset), color);

			if (Runtime_TimeSigned32(error) >= 0)
			{
				error += 4u - fourRadius;
				--radius;
				fourRadius -= 4u;
			}

			error += 4u * offset + 2u;
			++offset;
		} while (Runtime_TimeSigned32(offset) <= Runtime_TimeSigned32(radius));
	}

	// Original function: FUN_0040d9ee
	std::uint8_t CBitmap::Draw_GetPixel(std::int32_t x, std::int32_t y) const
	{
		const auto* pixels = mRawMemory;

		if (pixels == nullptr)
			return 0;

		// The cached bounds are compared as signed 32-bit values.
		std::int32_t right;
		std::memcpy(&right, &mRightBits, sizeof(right));

		if (x < mRectangle.x || x > right)
			return 0;

		std::int32_t bottom;
		std::memcpy(&bottom, &mBottomBits, sizeof(bottom));

		if (y < mRectangle.y || y > bottom)
			return 0;

		if (mDepth != 0)
			return 0;

		// Coordinates are used directly, without subtracting the rectangle origin.
		const std::uint32_t index = static_cast<std::uint32_t>(y) * mRowPixelCount + static_cast<std::uint32_t>(x);
		const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pixels) + index;

		std::uint8_t result;
		std::memcpy(&result, reinterpret_cast<const void*>(address), sizeof(result));

		return result;
	}

	// Original function: FUN_0040da2b
	std::uint16_t CBitmap::Draw_GetPixel_Word(std::int32_t x, std::int32_t y) const
	{
		const auto* pixels = mHighColorMemory;

		if (pixels == nullptr)
			return 0;

		std::int32_t right;
		std::memcpy(&right, &mRightBits, sizeof(right));

		if (x < mRectangle.x || x > right)
			return 0;

		std::int32_t bottom;
		std::memcpy(&bottom, &mBottomBits, sizeof(bottom));

		if (y < mRectangle.y || y > bottom)
			return 0;

		if (mDepth != 1)
			return 0;

		// Preserve 32-bit wraparound before adding the base address.
		const std::uint32_t index = static_cast<std::uint32_t>(y) * mRowPixelCount + static_cast<std::uint32_t>(x);
		const std::uint32_t byteOffset = index * 2u;
		const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pixels) + byteOffset;

		std::uint16_t result;
		std::memcpy(&result, reinterpret_cast<const void*>(address), sizeof(result));

		return result;
	}

	// Original function: FUN_0040da6a
	std::uint32_t CBitmap::Draw_GetPixel_Long(std::int32_t x, std::int32_t y) const
	{
		const auto* pixels = mTrueColorMemory;

		if (pixels == nullptr)
			return 0;

		std::int32_t right;
		std::memcpy(&right, &mRightBits, sizeof(right));

		if (x < mRectangle.x || x > right)
			return 0;

		std::int32_t bottom;
		std::memcpy(&bottom, &mBottomBits, sizeof(bottom));

		if (y < mRectangle.y || y > bottom)
			return 0;

		if (mDepth != 2)
			return 0;

		const std::uint32_t index = static_cast<std::uint32_t>(y) * mRowPixelCount + static_cast<std::uint32_t>(x);
		const std::uint32_t byteOffset = index * 4u;
		const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pixels) + byteOffset;

		std::uint32_t result;
		std::memcpy(&result, reinterpret_cast<const void*>(address), sizeof(result));

		return result;
	}

	// Original function: FUN_0040daa7
	void CBitmap::Draw_Box(SRectangle rectangle, std::uint8_t colorIndex)
	{
		if (mRawMemory == nullptr)
		{
			return;
		}

		rectangle.Validate();

		if (!rectangle.IsTouching(mRectangle))
		{

			return;
		}

		rectangle.CutInside(mRectangle);

		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
			{
				const std::uint32_t rowPixelCount = mRowPixelCount;
				const std::uint32_t pixelOffset = rowPixelCount * static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.x);

				auto* pixels = reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(mRawMemory) + pixelOffset);

				XB_Tool_Byte_FillBlock(pixels, static_cast<std::uint32_t>(rectangle.width), rectangle.height, rowPixelCount, colorIndex);
				break;
			}

			case SwitchIds::BitmapDepth::HighColor16:
			{
				const std::uint32_t pixelOffset = mRowPixelCount * static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.x);
				const std::uint32_t byteOffset = pixelOffset * 2u;

				auto* pixels = reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::uintptr_t>(mHighColorMemory) + byteOffset);

				const std::uint16_t packedColor = GetHighColorWord(colorIndex);

				XB_Tool_Word_FillBlock(pixels, rectangle.width, rectangle.height, mRowPixelCount, packedColor);
				break;
			}

			case SwitchIds::BitmapDepth::TrueColor32:
			{
				const std::uint32_t pixelOffset = mRowPixelCount * static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.x);
				const std::uint32_t byteOffset = pixelOffset * 4u;

				auto* pixels = reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uintptr_t>(mTrueColorMemory) + byteOffset);

				const std::uint32_t packedColor = GetTrueColorWord(colorIndex);

				XB_Tool_Long_FillBlock(pixels, rectangle.width, rectangle.height, mRowPixelCount, packedColor);
				break;
			}
		}
	}

	// Original function: FUN_0040db69
	void CBitmap::Draw_Box(SRectangle rectangle, std::uint16_t color)
	{
		rectangle.Validate();

		if (mRawMemory == nullptr || mDepth != 1)
		{
			return;
		}

		if (!rectangle.IsTouching(mRectangle))
		{

			return;
		}

		rectangle.CutInside(mRectangle);

		const std::uint32_t rowPixelCount = mRowPixelCount;
		const std::uint32_t pixelOffset = rowPixelCount * static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.x);
		const std::uint32_t byteOffset = pixelOffset * 2u;

		auto* pixels = reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::uintptr_t>(mHighColorMemory) + byteOffset);

		XB_Tool_Word_FillBlock(pixels, rectangle.width, rectangle.height, rowPixelCount, color);
	}

	// Original function: FUN_0040dbc6
	void CBitmap::Draw_Box(SRectangle rectangle, std::uint32_t color)
	{
		rectangle.Validate();

		if (mRawMemory == nullptr || mDepth != 2)
		{
			return;
		}

		if (!rectangle.IsTouching(mRectangle))
		{

			return;
		}

		rectangle.CutInside(mRectangle);

		const std::uint32_t rowPixelCount = mRowPixelCount;
		const std::uint32_t pixelOffset = rowPixelCount * static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.x);
		const std::uint32_t byteOffset = pixelOffset * 4u;

		auto* pixels = reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uintptr_t>(mTrueColorMemory) + byteOffset);

		XB_Tool_Long_FillBlock(pixels, rectangle.width, rectangle.height, rowPixelCount, color);
	}

	// Original function: FUN_0040dc23; preserve palette lookup and depth-specific fills.
	void CBitmap::Draw_Box(const SRectangle& rectangle, const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
				Draw_Box(rectangle, static_cast<std::uint8_t>(Palette_FindMatchingColor(mPalette == nullptr ? NXBasics::CXBSystemManager::sPalettePtr : mPalette, color)));
				break;
			case SwitchIds::BitmapDepth::HighColor16:
				Draw_Box(rectangle, g16BitPixelConverter->PackColor(color));
				break;
			case SwitchIds::BitmapDepth::TrueColor32:
				Draw_Box(rectangle, g32BitPixelConverter->PackColor(color));
				break;
		}
	}

	// Original function: FUN_0040dcb5
	void CBitmap::Draw_Rectangle(SRectangle rectangle, std::uint8_t colorIndex)
	{
		rectangle.Validate();

		if (mRawMemory == nullptr)
			return;

		if (!rectangle.IsTouching(mRectangle))
		{

			return;
		}

		Draw_HorizontalLine(rectangle.x, rectangle.y, rectangle.width, colorIndex);
		Draw_VerticalLine(rectangle.x, static_cast<std::uint32_t>(rectangle.y), rectangle.height, colorIndex);

		const std::uint32_t bottomBits = static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.height) - 1u;
		Draw_HorizontalLine(rectangle.x, Runtime_TimeSigned32(bottomBits), rectangle.width, colorIndex);

		const std::uint32_t rightBits = static_cast<std::uint32_t>(rectangle.x) + static_cast<std::uint32_t>(rectangle.width) - 1u;
		Draw_VerticalLine(Runtime_TimeSigned32(rightBits), static_cast<std::uint32_t>(rectangle.y), rectangle.height, colorIndex);
	}

	// Original function: FUN_0040dd37
	void CBitmap::Draw_Rectangle(SRectangle rectangle, std::uint16_t color)
	{
		rectangle.Validate();

		if (mRawMemory == nullptr)
			return;

		if (!rectangle.IsTouching(mRectangle))
		{

			return;
		}

		Draw_HorizontalLine(rectangle.x, rectangle.y, rectangle.width, color);
		Draw_VerticalLine(rectangle.x, static_cast<std::uint32_t>(rectangle.y), rectangle.height, color);

		const std::uint32_t bottomBits = static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.height) - 1u;
		Draw_HorizontalLine(rectangle.x, Runtime_TimeSigned32(bottomBits), rectangle.width, color);

		const std::uint32_t rightBits = static_cast<std::uint32_t>(rectangle.x) + static_cast<std::uint32_t>(rectangle.width) - 1u;
		Draw_VerticalLine(Runtime_TimeSigned32(rightBits), static_cast<std::uint32_t>(rectangle.y), rectangle.height, color);
	}

	// Original function: FUN_0040ddb9
	void CBitmap::Draw_Rectangle(SRectangle rectangle, std::uint32_t color)
	{
		rectangle.Validate();

		if (mRawMemory == nullptr)
			return;

		if (!rectangle.IsTouching(mRectangle))
		{

			return;
		}

		Draw_HorizontalLine(rectangle.x, rectangle.y, rectangle.width, color);
		Draw_VerticalLine(rectangle.x, static_cast<std::uint32_t>(rectangle.y), rectangle.height, color);

		const std::uint32_t bottomBits = static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.height) - 1u;
		Draw_HorizontalLine(rectangle.x, Runtime_TimeSigned32(bottomBits), rectangle.width, color);

		const std::uint32_t rightBits = static_cast<std::uint32_t>(rectangle.x) + static_cast<std::uint32_t>(rectangle.width) - 1u;
		Draw_VerticalLine(Runtime_TimeSigned32(rightBits), static_cast<std::uint32_t>(rectangle.y), rectangle.height, color);
	}

	// Original function: FUN_0040de3b
	void CBitmap::Draw_Rectangle(const SRectangle& rectangle, const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
			{
				const void* palette = mPalette != nullptr ? mPalette : CXBSystemManager::sPalettePtr;
				const std::uint8_t colorIndex = Palette_FindMatchingColor(palette, color);
				Draw_Rectangle(rectangle, colorIndex);
				break;
			}

			case SwitchIds::BitmapDepth::HighColor16:
			{
				const std::uint16_t packedColor = g16BitPixelConverter->PackColor(color);
				Draw_Rectangle(rectangle, packedColor);
				break;
			}

			case SwitchIds::BitmapDepth::TrueColor32:
			{
				const std::uint32_t packedColor = g32BitPixelConverter->PackColor(color);
				Draw_Rectangle(rectangle, packedColor);
				break;
			}
		}
	}

	// Original function: FUN_0040decd
	void CBitmap::Draw_HorizontalLine(std::int32_t x, std::int32_t y, std::int32_t length, std::uint8_t colorIndex)
	{
		SRectangle rectangle(x, y, length, 1);
		Draw_Box(rectangle, colorIndex);
	}

	// Original function: FUN_0040df06
	void CBitmap::Draw_HorizontalLine(std::int32_t x, std::int32_t y, std::int32_t length, std::uint16_t color)
	{
		SRectangle rectangle(x, y, length, 1);
		Draw_Box(rectangle, color);
	}

	// Original function: FUN_0040df3f
	void CBitmap::Draw_HorizontalLine(std::int32_t x, std::int32_t y, std::int32_t length, std::uint32_t color)
	{
		SRectangle rectangle(x, y, length, 1);
		Draw_Box(rectangle, color);
	}

	// Original function: FUN_0040dffe
	void CBitmap::Draw_VerticalLine(std::int32_t x, std::uint32_t y, std::int32_t length, std::uint8_t colorIndex)
	{
		// Preserve the coordinate's original 32-bit representation.
		std::int32_t rectangleY;
		std::memcpy(&rectangleY, &y, sizeof(rectangleY));

		SRectangle rectangle(x, rectangleY, 1, length);

		Draw_Box(rectangle, colorIndex);
	}

	// Original function: FUN_0040e037
	void CBitmap::Draw_VerticalLine(std::int32_t x, std::uint32_t y, std::int32_t length, std::uint16_t color)
	{
		// Preserve the coordinate's original 32-bit representation.
		std::int32_t rectangleY;
		std::memcpy(&rectangleY, &y, sizeof(rectangleY));

		SRectangle rectangle(x, rectangleY, 1, length);

		Draw_Box(rectangle, color);
	}

	// Original function: FUN_0040e070
	void CBitmap::Draw_VerticalLine(std::int32_t x, std::uint32_t y, std::int32_t length, std::uint32_t color)
	{
		// Preserve the coordinate's original 32-bit representation.
		std::int32_t rectangleY;
		std::memcpy(&rectangleY, &y, sizeof(rectangleY));

		SRectangle rectangle(x, rectangleY, 1, length);

		Draw_Box(rectangle, color);
	}

	// Original function: FUN_0040e0a9
	void CBitmap::Draw_VerticalLine(std::int32_t x, std::uint32_t y, std::int32_t length, const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
			{
				void* palette = mPalette;

				if (palette == nullptr)
				{
					palette = NXBasics::CXBSystemManager::sPalettePtr;
				}

				const std::uint8_t colorIndex = Palette_FindMatchingColor(palette, color);
				Draw_VerticalLine(x, y, length, colorIndex);
				break;
			}

			case SwitchIds::BitmapDepth::HighColor16:
			{
				const std::uint16_t packedColor = g16BitPixelConverter->PackColor(color);
				Draw_VerticalLine(x, y, length, packedColor);
				break;
			}

			case SwitchIds::BitmapDepth::TrueColor32:
			{
				const std::uint32_t packedColor = g32BitPixelConverter->PackColor(color);
				Draw_VerticalLine(x, y, length, packedColor);
				break;
			}
		}
	}

	// Original function: FUN_0040ec57
	void CBitmap::Text_Print(CFont* font, std::int32_t x, std::int32_t y, const char* text)
	{
		if (text == nullptr || font == nullptr)
			return;

		std::string legacy;

		if (!Font_TryLegacyText(font, text, legacy))
		{
			UnicodeFont_Print(font, this, nullptr, x, y, text);
			return;
		}

		text = legacy.c_str();

		const std::uint32_t initialX = static_cast<std::uint32_t>(x);
		std::uint32_t currentX = initialX;
		std::uint32_t currentY = static_cast<std::uint32_t>(y);

		for (;;)
		{
			const std::uint8_t character = static_cast<std::uint8_t>(*text++);

			if (character == 0)
			{

				return;
			}

			if (character == 10 || character == 13)
			{
				// The original also folds two consecutive LF characters.

				if (static_cast<std::uint8_t>(*text) == 10)
					++text;

				std::uint32_t lineHeight;
				std::memcpy(&lineHeight, reinterpret_cast<const std::byte*>(font) + NXBasics::FontStorageOffset::Height, sizeof(lineHeight));

				currentX = initialX;
				currentY += lineHeight + 2u;
				continue;
			}

			if (character == 9 || character == 32 || character == 95)
			{
				// Underscore follows the same spacing branch as space and tab.
				std::uint32_t distance = static_cast<std::uint32_t>(mFixedXDistance);

				if (distance == 0)
					distance = static_cast<std::uint32_t>(Font_GetCharacterWidth(font, 0x69));

				currentX += distance;
				continue;
			}

			if (character < 32)
				continue;

			const std::uint32_t characterWidth = static_cast<std::uint32_t>(Font_GetCharacterWidth(font, character));
			const std::uint32_t fixedDistance = static_cast<std::uint32_t>(mFixedXDistance);

			if (fixedDistance != 0)
			{
				const std::int32_t difference = Runtime_TimeSigned32(fixedDistance - characterWidth);
				const std::uint32_t characterX = currentX + static_cast<std::uint32_t>(difference / 2);

				Font_PrintCharacter(font, this, character, Runtime_TimeSigned32(characterX), Runtime_TimeSigned32(currentY));

				// Reload the field after drawing, as the original does.
				currentX += static_cast<std::uint32_t>(mFixedXDistance);
			}
			else
			{
				Font_PrintCharacter(font, this, character, Runtime_TimeSigned32(currentX), Runtime_TimeSigned32(currentY));
				currentX += characterWidth;
			}
		}
	}

	// Original function: FUN_0040ed30
	void __cdecl CBitmap::Text_PrintV(CFont* font, std::int32_t x, std::int32_t y, const char* format, ...)
	{
		char text[1000];

		va_list arguments;
		va_start(arguments, format);
		Runtime_FormatString(text, 999, format, arguments);
		va_end(arguments);

		text[999] = '\0';
		Text_Print(font, x, y, text);
	}

	// Original function: FUN_0040ed72
	void CBitmap::Text_Print(CFont* font, const SColorRGB* colour, std::int32_t x, std::int32_t y, const char* text)
	{
		if (text == nullptr || font == nullptr)
			return;

		std::string legacy;

		if (!Font_TryLegacyText(font, text, legacy))
		{
			UnicodeFont_Print(font, this, colour, x, y, text);
			return;
		}

		text = legacy.c_str();

		const std::uint32_t initialX = static_cast<std::uint32_t>(x);
		std::uint32_t currentX = initialX;
		std::uint32_t currentY = static_cast<std::uint32_t>(y);

		for (;;)
		{
			const std::uint8_t character = static_cast<std::uint8_t>(*text++);

			if (character == 0)
			{

				return;
			}

			if (character == 10 || character == 13)
			{
				// The original also folds two consecutive LF characters.

				if (static_cast<std::uint8_t>(*text) == 10)
					++text;

				std::uint32_t lineHeight;
				std::memcpy(&lineHeight, reinterpret_cast<const std::byte*>(font) + NXBasics::FontStorageOffset::Height, sizeof(lineHeight));

				currentX = initialX;
				currentY += lineHeight + 2u;
				continue;
			}

			if (character == 9 || character == 32 || character == 95)
			{
				// Underscore follows the same spacing branch as space and tab.
				std::uint32_t distance = static_cast<std::uint32_t>(mFixedXDistance);

				if (distance == 0)
					distance = static_cast<std::uint32_t>(Font_GetCharacterWidth(font, 0x69));

				currentX += distance;
				continue;
			}

			if (character < 32)
				continue;

			const std::uint32_t characterWidth = static_cast<std::uint32_t>(Font_GetCharacterWidth(font, character));
			const std::uint32_t fixedDistance = static_cast<std::uint32_t>(mFixedXDistance);

			if (fixedDistance != 0)
			{
				const std::int32_t difference = Runtime_TimeSigned32(fixedDistance - characterWidth);
				const std::uint32_t characterX = currentX + static_cast<std::uint32_t>(difference / 2);

				Font_PrintCharacter(font, colour, this, character, Runtime_TimeSigned32(characterX), Runtime_TimeSigned32(currentY));

				// Reload the field after drawing, as the original does.
				currentX += static_cast<std::uint32_t>(mFixedXDistance);
			}
			else
			{
				Font_PrintCharacter(font, colour, this, character, Runtime_TimeSigned32(currentX), Runtime_TimeSigned32(currentY));
				currentX += characterWidth;
			}
		}
	}

	// Original function: FUN_0040ee47
	void __cdecl CBitmap::Text_PrintV(CFont* font, const SColorRGB* colour, std::int32_t x, std::int32_t y, const char* format, ...)
	{
		char text[1000];

		va_list arguments;
		va_start(arguments, format);
		Runtime_FormatString(text, 999, format, arguments);
		va_end(arguments);

		text[999] = '\0';
		Text_Print(font, colour, x, y, text);
	}

	// Original function: FUN_0040ee96
	std::uint16_t CBitmap::GetHighColorWord(std::uint8_t colorIndex) const
	{
		const std::uint32_t paletteIndex = colorIndex;
		void* palette = mPalette;

		if (palette == nullptr)
		{
			palette = NXBasics::CXBSystemManager::sPalettePtr;
		}

		return Palette_GetHighColorWord(palette, paletteIndex);
	}

	// Original function: FUN_0040eeb1
	const std::uint16_t* CBitmap::GetHighColorTablePtr() const
	{
		void* palette = mPalette;

		if (palette == nullptr)
		{
			palette = NXBasics::CXBSystemManager::sPalettePtr;
		}

		return Palette_GetHighColorTablePtr(palette);
	}

	// Original function: FUN_0040eec8
	std::uint32_t CBitmap::GetTrueColorWord(std::uint8_t colorIndex) const
	{
		const std::uint32_t paletteIndex = colorIndex;
		void* palette = mPalette;

		if (palette == nullptr)
		{
			palette = NXBasics::CXBSystemManager::sPalettePtr;
		}

		return Palette_GetTrueColorWord(palette, paletteIndex);
	}

	// Original function: FUN_0040eee3
	const std::uint32_t* CBitmap::GetTrueColorTablePtr() const
	{
		void* palette = mPalette;

		if (palette == nullptr)
		{
			palette = NXBasics::CXBSystemManager::sPalettePtr;
		}

		return Palette_GetTrueColorTablePtr(palette);
	}

	template <typename T>
	static void ZoomBitmapPixels(T* pixels, std::int32_t width, std::int32_t height, std::uint32_t stride)
	{
		const auto quarterWidth = width / 4;
		const auto quarterHeight = height / 4;
		const auto halfWidth = static_cast<std::uint32_t>(width / 2);
		const auto halfHeight = static_cast<std::uint32_t>(height / 2);
		const auto pixel = [pixels](std::uint32_t index)
		{

			return BitmapPixelAddress(pixels, index);
		};

		for (std::int32_t row = 0; row < quarterHeight; ++row)
		{
			for (std::int32_t column = 0; column < quarterWidth; ++column)
			{
				const auto x = static_cast<std::uint32_t>(column);
				const auto y = static_cast<std::uint32_t>(row);
				const auto sourceTopLeft = (static_cast<std::uint32_t>(quarterHeight) + y) * stride + static_cast<std::uint32_t>(quarterWidth) + x;
				const auto sourceBottomLeft = (static_cast<std::uint32_t>(quarterHeight) + halfHeight - 1u - y) * stride + static_cast<std::uint32_t>(quarterWidth) + x;
				const auto sourceTopRight = sourceTopLeft + halfWidth - 1u - x * 2u;
				const auto sourceBottomRight = sourceBottomLeft + halfWidth - 1u - x * 2u;
				const auto topLeft = y * 2u * stride + x * 2u;
				const auto topRight = y * 2u * stride + static_cast<std::uint32_t>(width) - 1u - x * 2u;
				const auto bottomLeft = (static_cast<std::uint32_t>(height) - 2u - y * 2u) * stride + x * 2u;
				const auto bottomRight = (static_cast<std::uint32_t>(height) - 2u - y * 2u) * stride + static_cast<std::uint32_t>(width) - 1u - x * 2u;
				// Preserve the four quadrants' original read/write order for aliasing.
				*pixel(topLeft + stride + 1u) = *pixel(sourceTopLeft);
				*pixel(topLeft + stride) = *pixel(topLeft + stride + 1u);
				*pixel(topLeft + 1u) = *pixel(topLeft + stride);
				*pixel(topLeft) = *pixel(topLeft + stride);
				*pixel(topRight + stride - 1u) = *pixel(sourceTopRight);
				*pixel(topRight + stride) = *pixel(topRight + stride - 1u);
				*pixel(topRight - 1u) = *pixel(topRight + stride);
				*pixel(topRight) = *pixel(topRight + stride);
				*pixel(bottomLeft + stride + 1u) = *pixel(sourceBottomLeft);
				*pixel(bottomLeft + stride) = *pixel(bottomLeft + stride + 1u);
				*pixel(bottomLeft + 1u) = *pixel(bottomLeft + stride);
				*pixel(bottomLeft) = *pixel(bottomLeft + stride);
				*pixel(bottomRight + stride - 1u) = *pixel(sourceBottomRight);
				*pixel(bottomRight + stride) = *pixel(bottomRight + stride - 1u);
				*pixel(bottomRight - 1u) = *pixel(bottomRight + stride);
				*pixel(bottomRight) = *pixel(bottomRight + stride);
			}
		}
	}

	// Original function: FUN_0040eefa; pixels change in place, bitmap fields do not.
	void CBitmap::ZoomIn() const
	{
		if (mRawMemory == nullptr)
		{

			return;
		}

		if (mDepth == 0)
		{
			ZoomBitmapPixels(mRawMemory, mRectangle.width, mRectangle.height, mRowPixelCount);
		}
		else if (mDepth == 1)
		{
			ZoomBitmapPixels(mHighColorMemory, mRectangle.width, mRectangle.height, mRowPixelCount);
		}
		else if (mDepth == 2)
		{
			ZoomBitmapPixels(mTrueColorMemory, mRectangle.width, mRectangle.height, mRowPixelCount);
		}
	}

	// Original function: FUN_0040f522; no memory or coordinate validation.
	void* CBitmap::GetUnclippedPixelPtr(std::int32_t x, std::int32_t y) const
	{
		const auto index = mRowPixelCount * static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(x);

		if (mDepth == 0)
			return BitmapPixelAddress(mRawMemory, index);

		if (mDepth == 1)
			return BitmapPixelAddress(mHighColorMemory, index);

		if (mDepth == 2)
			return BitmapPixelAddress(mTrueColorMemory, index);

		return nullptr;
	}

	// Original function: FUN_0040f56f
	void CBitmap::Tool_Darken()
	{
		Tool_Darken(mRectangle);
	}

	// Original function: FUN_0040f589
	void CBitmap::Tool_Darken(SRectangle rectangle)
	{
		const bool hud = NXSys2DApp::IsRemasterHudBitmap(this);

		if (mRawMemory == nullptr)
			return;

		rectangle.Validate();

		if (!rectangle.IsTouching(mRectangle))
			return;

		rectangle.CutInside(mRectangle);

		if (mDepth == 2)
		{
			const std::uint32_t pixelOffset = mRowPixelCount * static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.x);
			std::uintptr_t rowAddress = reinterpret_cast<std::uintptr_t>(mTrueColorMemory) + pixelOffset * 4u;
			const std::uint32_t mask = g32BitPixelConverter->PackColor(127, 127, 127);

			for (std::int32_t row = 0; row < rectangle.height; ++row)
			{
				std::uintptr_t pixelAddress = rowAddress;

				for (std::int32_t column = 0; column < rectangle.width; ++column)
				{
					std::uint32_t pixel;
					std::memcpy(&pixel, reinterpret_cast<const void*>(pixelAddress), sizeof(pixel));

					const auto inverseCoverage = hud ? (((pixel >> 24) / 2u) << 24) : 0u;
					pixel = ((pixel >> 1) & mask) | inverseCoverage;

					std::memcpy(reinterpret_cast<void*>(pixelAddress), &pixel, sizeof(pixel));
					pixelAddress += 4u;
				}

				rowAddress += mRowPixelCount * 4u;
			}
		}
		else if (mDepth == 1)
		{
			if (g16BitPixelConverter == nullptr)
			{

				return;
			}

			const std::uint16_t mask = g16BitPixelConverter->PackColor(127, 127, 127);
			const std::uint32_t pixelOffset = mRowPixelCount * static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.x);
			std::uintptr_t rowAddress = reinterpret_cast<std::uintptr_t>(mHighColorMemory) + pixelOffset * 2u;

			for (std::int32_t row = 0; row < rectangle.height; ++row)
			{
				std::uintptr_t pixelAddress = rowAddress;

				for (std::int32_t column = 0; column < rectangle.width; ++column)
				{
					std::uint16_t pixel;
					std::memcpy(&pixel, reinterpret_cast<const void*>(pixelAddress), sizeof(pixel));

					pixel = static_cast<std::uint16_t>((pixel >> 1) & mask);

					std::memcpy(reinterpret_cast<void*>(pixelAddress), &pixel, sizeof(pixel));
					pixelAddress += 2u;
				}

				rowAddress += mRowPixelCount * 2u;
			}
		}
	}

	// Original function: FUN_0040f688
	void CBitmap::Tool_Darken(SRectangle rectangle, std::uint8_t redMask, std::uint8_t greenMask, std::uint8_t blueMask)
	{
		const bool hud = NXSys2DApp::IsRemasterHudBitmap(this);

		if (mRawMemory == nullptr)
			return;

		rectangle.Validate();

		if (!rectangle.IsTouching(mRectangle))
			return;

		rectangle.CutInside(mRectangle);

		if (mDepth == 2)
		{
			const std::uint32_t pixelOffset = mRowPixelCount * static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.x);
			std::uintptr_t rowAddress = reinterpret_cast<std::uintptr_t>(mTrueColorMemory) + pixelOffset * 4u;
			const std::uint32_t mask = g32BitPixelConverter->PackColor(redMask, greenMask, blueMask);

			for (std::int32_t row = 0; row < rectangle.height; ++row)
			{
				std::uintptr_t pixelAddress = rowAddress;

				for (std::int32_t column = 0; column < rectangle.width; ++column)
				{
					std::uint32_t pixel;
					std::memcpy(&pixel, reinterpret_cast<const void*>(pixelAddress), sizeof(pixel));

					const auto inverseCoverage = hud ? (((pixel >> 24) / 2u) << 24) : 0u;
					pixel = ((pixel >> 1) & mask) | inverseCoverage;

					std::memcpy(reinterpret_cast<void*>(pixelAddress), &pixel, sizeof(pixel));
					pixelAddress += 4u;
				}

				rowAddress += mRowPixelCount * 4u;
			}
		}
		else if (mDepth == 1)
		{
			if (g16BitPixelConverter == nullptr)
			{

				return;
			}

			const std::uint16_t mask = g16BitPixelConverter->PackColor(redMask, greenMask, blueMask);
			const std::uint32_t pixelOffset = mRowPixelCount * static_cast<std::uint32_t>(rectangle.y) + static_cast<std::uint32_t>(rectangle.x);
			std::uintptr_t rowAddress = reinterpret_cast<std::uintptr_t>(mHighColorMemory) + pixelOffset * 2u;

			for (std::int32_t row = 0; row < rectangle.height; ++row)
			{
				std::uintptr_t pixelAddress = rowAddress;

				for (std::int32_t column = 0; column < rectangle.width; ++column)
				{
					std::uint16_t pixel;
					std::memcpy(&pixel, reinterpret_cast<const void*>(pixelAddress), sizeof(pixel));

					pixel = static_cast<std::uint16_t>((pixel >> 1) & mask);

					std::memcpy(reinterpret_cast<void*>(pixelAddress), &pixel, sizeof(pixel));
					pixelAddress += 2u;
				}

				rowAddress += mRowPixelCount * 2u;
			}
		}
	}

	// Original function: FUN_004e5e48
	void CBitmap::operator delete(void* allocation) noexcept
	{
		Runtime_Free(allocation);
	}

	void CBitmap::operator delete(void* allocation, const std::nothrow_t&) noexcept
	{
		Runtime_Free(allocation);
	}
}

namespace NXBasics
{
	// Original function: FUN_0040e943.
	void CBitmap::Draw_FilledCircle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint8_t color)
	{
		std::uint32_t offset = 0, error = 3u - 2u * radius, fourRadius = 4u * radius;

		if (Runtime_TimeSigned32(radius) < 0)
		{

			return;
		}

		const auto centerX = static_cast<std::uint32_t>(x), centerY = static_cast<std::uint32_t>(y);
		do
		{
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY + radius), Runtime_TimeSigned32(2u * offset), color);
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY - radius), Runtime_TimeSigned32(2u * offset), color);
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY + offset), Runtime_TimeSigned32(2u * radius), color);
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY - offset), Runtime_TimeSigned32(2u * radius), color);

			if (Runtime_TimeSigned32(error) >= 0)
			{
				error += 4u - fourRadius;
				--radius;
				fourRadius -= 4u;
			}

			error += 4u * offset + 2u;
			++offset;
		} while (Runtime_TimeSigned32(offset) <= Runtime_TimeSigned32(radius));
	}

	// Original function: FUN_0040ea1d.
	void CBitmap::Draw_FilledCircle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint16_t color)
	{
		std::uint32_t offset = 0, error = 3u - 2u * radius, fourRadius = 4u * radius;

		if (Runtime_TimeSigned32(radius) < 0)
		{

			return;
		}

		const auto centerX = static_cast<std::uint32_t>(x), centerY = static_cast<std::uint32_t>(y);
		do
		{
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY + radius), Runtime_TimeSigned32(2u * offset), color);
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY - radius), Runtime_TimeSigned32(2u * offset), color);
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY + offset), Runtime_TimeSigned32(2u * radius), color);
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY - offset), Runtime_TimeSigned32(2u * radius), color);

			if (Runtime_TimeSigned32(error) >= 0)
			{
				error += 4u - fourRadius;
				--radius;
				fourRadius -= 4u;
			}

			error += 4u * offset + 2u;
			++offset;
		} while (Runtime_TimeSigned32(offset) <= Runtime_TimeSigned32(radius));
	}

	// Original function: FUN_0040eaf7.
	void CBitmap::Draw_FilledCircle(std::int32_t x, std::int32_t y, std::uint32_t radius, std::uint32_t color)
	{
		std::uint32_t offset = 0, error = 3u - 2u * radius, fourRadius = 4u * radius;

		if (Runtime_TimeSigned32(radius) < 0)
		{

			return;
		}

		const auto centerX = static_cast<std::uint32_t>(x), centerY = static_cast<std::uint32_t>(y);
		do
		{
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY + radius), Runtime_TimeSigned32(2u * offset), color);
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - offset), Runtime_TimeSigned32(centerY - radius), Runtime_TimeSigned32(2u * offset), color);
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY + offset), Runtime_TimeSigned32(2u * radius), color);
			Draw_HorizontalLine(Runtime_TimeSigned32(centerX - radius), Runtime_TimeSigned32(centerY - offset), Runtime_TimeSigned32(2u * radius), color);

			if (Runtime_TimeSigned32(error) >= 0)
			{
				error += 4u - fourRadius;
				--radius;
				fourRadius -= 4u;
			}

			error += 4u * offset + 2u;
			++offset;
		} while (Runtime_TimeSigned32(offset) <= Runtime_TimeSigned32(radius));
	}

	// Original function: FUN_0040ebd1.
	void CBitmap::Draw_FilledCircle(std::int32_t x, std::int32_t y, std::uint32_t radius, const SColorRGB& color)
	{
		switch (mDepth)
		{
			case SwitchIds::BitmapDepth::Indexed8:
				Draw_FilledCircle(x, y, radius, Palette_FindMatchingColor(mPalette != nullptr ? mPalette : NXBasics::CXBSystemManager::sPalettePtr, color));
				break;
			case SwitchIds::BitmapDepth::HighColor16:
				Draw_FilledCircle(x, y, radius, g16BitPixelConverter->PackColor(color));
				break;
			case SwitchIds::BitmapDepth::TrueColor32:
				Draw_FilledCircle(x, y, radius, g32BitPixelConverter->PackColor(color));
				break;
		}
	}
}
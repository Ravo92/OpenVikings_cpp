#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XDiagnostics.h"
#include "XMemory.h"
#include "XNumericConstants.h"
#include "XPalette.h"
#include "XPicture.h"
#include "XPixelFormatConverter.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

namespace NXBasics
{
	struct SPictureFormatEntry
	{
		char extension[8];                       // +0x00; null-terminated extension.
		std::uint32_t format;                    // +0x08; 0 = PCX, 1 = BMP, 2 = terminator.
	};

	static_assert(sizeof(SPictureFormatEntry) == 12);
	static_assert(offsetof(SPictureFormatEntry, format) == 0x08);

	// Original data: 004f3668–004f368b.
	static constexpr SPictureFormatEntry kPictureFormats[] =
	{
		{ ".pcx", 0 },
		{ ".bmp", 1 },
		{ "", 2 }
	};

	static_assert(sizeof(kPictureFormats) == 36);

	// Original function: FUN_0040bd76
	CPicture::CPicture(const char* fileName) : mBitmap(nullptr), mPalette(nullptr), mOwnsResources(0), mReserved09{ }
	{
		static_assert(offsetof(CPicture, mBitmap) == 0);
		static_assert(offsetof(CPicture, mPalette) == sizeof(void*));
		static_assert(offsetof(CPicture, mOwnsResources) == 2 * sizeof(void*));
		char extension[256];
		Runtime_SplitPath(fileName, nullptr, nullptr, nullptr, extension);

		for (char* character = extension; *character != '\0'; ++character)
		{
			const auto signedCharacter = static_cast<std::int8_t>(*character);
			*character = static_cast<char>(Runtime_ToLower(static_cast<std::int32_t>(signedCharacter)));
		}

		CMemory* memory = new (std::nothrow) CMemory(fileName);

		std::uint32_t format = 2;

		for (const SPictureFormatEntry* entry = kPictureFormats; entry->format != 2; ++entry)
		{
			// Preserve the original three-character comparison and continued search.
			if (std::strncmp(extension, entry->extension, 3) == 0)
			{
				format = entry->format;
			}
		}

		switch (format)
		{
			case SwitchIds::PictureFileType::Pcx:
				l_UnPackPCX(memory);
				break;

			case SwitchIds::PictureFileType::Bmp:
				l_UnPackBMP(memory);
				break;

			case SwitchIds::PictureFileType::Unknown:
				ReportFatalError("NXBasics::CPicture::CPicture\n\nunknown GFX format: %s", extension);
				break;
		}

		delete memory;

		mBitmap->SetPalettePtr(mPalette);
		mOwnsResources = 1;
	}

	// Original function: FUN_0040be7d
	CPicture::~CPicture()
	{
		if (mOwnsResources != 0)
		{
			if (mBitmap != nullptr)
			{
				delete mBitmap;
				mBitmap = nullptr;
			}

			if (mPalette != nullptr)
			{
				delete mPalette;
				mPalette = nullptr;
			}
		}
	}

	// Read an unaligned field from the reconstructed x86 BMP header.
	static std::uint32_t Picture_ReadBmpDword(const std::byte* header, std::uint32_t offset)
	{
		std::uint32_t value;
		std::memcpy(&value, header + offset, sizeof(value));
		return value;
	}

	// Source-level representation of the original row-loop variables.
	struct SPictureBmpRows
	{
		std::uint32_t heightBits;
		std::uint32_t firstRow;
		std::uint32_t endRow;
		std::uint32_t rowStep;
	};

	static SPictureBmpRows Picture_GetBmpRows(std::uint32_t heightBits)
	{
		if ((heightBits & NXBasics::IntegerRange::Bit31) != 0)
		{
			const std::uint32_t positiveHeightBits = 0u - heightBits;
			return { positiveHeightBits, 0u, positiveHeightBits, 1u };
		}

		return { heightBits, heightBits - 1u, NXBasics::IntegerRange::UInt32Maximum, NXBasics::IntegerRange::UInt32Maximum };
	}

	// Original function: FUN_0040beaa.
	CBitmap* CPicture::ExtractBitmap()
	{
		mBitmap->SetPalettePtr(nullptr);
		CBitmap* bitmap = mBitmap;
		mBitmap = nullptr;
		return bitmap;
	}

	// Original function: FUN_0040bebd
	CPalette* CPicture::ExtractPalette()
	{
		if (mBitmap != nullptr)
		{
			mBitmap->SetPalettePtr(nullptr);
		}

		CPalette* palette = mPalette;
		mPalette = nullptr;
		return palette;
	}

	// Interpret the low 16 bits exactly as an x86 signed word.
	static std::int32_t Picture_SignedWord(std::uint32_t value)
	{
		value &= NXBasics::IntegerRange::UInt16Maximum;
		return value < 0x8000u ? static_cast<std::int32_t>(value) : static_cast<std::int32_t>(value) - 0x10000;
	}

	// Original function: FUN_0040bed6
	void CPicture::l_UnPackPCX(CMemory* memory)
	{
		const std::uint8_t* source = memory->mData;

		const auto readWord = [source] (std::uint32_t offset) -> std::uint32_t
		{
			return static_cast<std::uint32_t>(source[offset]) | (static_cast<std::uint32_t>(source[offset + 1u]) << 8u);
		};

		const std::uint32_t widthBits = ((readWord(0x08) - readWord(0x04)) & NXBasics::IntegerRange::UInt16Maximum) + 1u;
		const std::uint32_t heightBits = ((readWord(0x0A) - readWord(0x06)) & NXBasics::IntegerRange::UInt16Maximum) + 1u;
		const std::uint32_t paddedWidthBits = (widthBits + 1u) & ~1u;

		const std::int32_t width = Picture_SignedWord(widthBits);
		const std::int32_t height = Picture_SignedWord(heightBits);
		const std::int32_t paddedWidth = Picture_SignedWord(paddedWidthBits);
		const bool hasPadding = (widthBits & 1u) != 0;

		mBitmap = new (std::nothrow) CBitmap(width, height, 0);

		CBitmap* paddedBitmap = nullptr;

		if (hasPadding)
		{
			paddedBitmap = new (std::nothrow) CBitmap(paddedWidth, height, 0);
		}

		CBitmap* decodeBitmap = hasPadding ? paddedBitmap : mBitmap;
		std::uint8_t* row = decodeBitmap->mRawMemory;
		std::uint32_t sourceOffset = 0x80;

		for (std::int32_t y = 0; y < height; ++y)
		{
			std::uint8_t* output = row;
			std::uint32_t decodedCount = 0;

			do
			{
				const std::uint8_t token = source[sourceOffset++];

				if (token > 0xBF)
				{
					const std::uint32_t runLength = token & NXBasics::BitMask::Low6Bits;
					const std::uint8_t value = source[sourceOffset++];

					std::memset(output, value, runLength);
					output += runLength;
					decodedCount += runLength;
				}
				else
				{
					*output++ = token;
					++decodedCount;
				}
			} while (Picture_SignedWord(decodedCount) < paddedWidth);

			row += paddedWidth;
		}

		if (hasPadding)
		{
			paddedBitmap->CopyIntoBitmap(mBitmap, 0, 0);
			delete paddedBitmap;
		}

		mPalette = new (std::nothrow) CPalette();

		const std::uint8_t* paletteSource = source + (memory->mSize - 0x300u);

		for (std::uint32_t index = 0; index < 256; ++index)
		{
			mPalette->mColors[index][2] = *paletteSource++;
			mPalette->mColors[index][1] = *paletteSource++;
			mPalette->mColors[index][0] = *paletteSource++;
		}
	}

	// Original function: FUN_0040c054
	void CPicture::l_UnPackBMP(CMemory* memory)
	{
		const std::uint8_t* source = memory->mData;

		// Preserve the original 54-byte header layout for the decoder calls.
		alignas(4) std::byte header[54]{ };

		const auto readWord = [source] (std::uint32_t offset) -> std::uint32_t
		{
			return static_cast<std::uint32_t>(source[offset]) | (static_cast<std::uint32_t>(source[offset + 1u]) << 8u);
		};

		const auto readDword = [&readWord] (std::uint32_t offset) -> std::uint32_t
		{
			return readWord(offset) | (readWord(offset + 2u) << 16u);
		};

		const auto storeDword = [&header] (std::uint32_t offset, std::uint32_t value)
		{
			std::memcpy(header + offset, &value, sizeof(value));
		};

		const std::uint32_t fileSize = readDword(0x02);
		const std::uint32_t pixelOffset = readDword(0x0A);
		const std::uint32_t widthBits = readDword(0x12);
		const std::uint32_t heightBits = readDword(0x16);
		const std::uint16_t bitsPerPixel = static_cast<std::uint16_t>(readWord(0x1C));
		const std::uint32_t compression = readDword(0x1E);
		const std::uint32_t colorsUsed = readDword(0x2E);

		storeDword(0x02, fileSize);
		storeDword(0x0A, pixelOffset);
		storeDword(0x12, widthBits);
		storeDword(0x16, heightBits);
		std::memcpy(header + 0x1C, &bitsPerPixel, sizeof(bitsPerPixel));
		storeDword(0x1E, compression);
		storeDword(0x2E, colorsUsed);

		std::uint8_t* pixelData = memory->mData + pixelOffset;
		const std::uint32_t paletteEntryCount = colorsUsed != 0 ? colorsUsed : 256u;

		if (bitsPerPixel == 8)
		{
			mPalette = new (std::nothrow) CPalette();

			const std::uint8_t* paletteSource = source + 0x36;
			const std::int32_t signedEntryCount = Runtime_TimeSigned32(paletteEntryCount);

			for (std::int32_t index = 0; index < signedEntryCount; ++index)
			{
				mPalette->mColors[index][0] = *paletteSource++;
				mPalette->mColors[index][1] = *paletteSource++;
				mPalette->mColors[index][2] = *paletteSource++;
				++paletteSource;
			}
		}

		if (compression == 1)
		{
			mBitmap = l_DecodeRLE8(header, pixelData);
			return;
		}

		if (compression == 0)
		{
			switch (bitsPerPixel)
			{
				case SwitchIds::PixelBitCount::Indexed8:
					mBitmap = l_DecodeUnpacked8(header, pixelData);
					return;

				case SwitchIds::PixelBitCount::HighColor16:
					mBitmap = l_DecodeUnpacked16(header, pixelData);
					return;

				case SwitchIds::PixelBitCount::Rgb24:
					mBitmap = l_DecodeUnpacked24(header, pixelData);
					return;

				case SwitchIds::PixelBitCount::TrueColor32:
					mBitmap = l_DecodeUnpacked32(header, pixelData);
					return;
			}
		}

		ReportFatalError("NXBasics::CPicture::l_UnPackBMP\n\nThe bitmap to load was compressed");
	}

	// Original function: FUN_0040c1b8
	CBitmap* CPicture::l_DecodeUnpacked8(const std::byte* header, const std::uint8_t* pixelData)
	{
		const std::uint32_t widthBits = Picture_ReadBmpDword(header, 0x12);
		const SPictureBmpRows rows = Picture_GetBmpRows(Picture_ReadBmpDword(header, 0x16));

		CBitmap* bitmap = new (std::nothrow) CBitmap(Runtime_TimeSigned32(widthBits), Runtime_TimeSigned32(rows.heightBits), 0);
		std::uint8_t* output = bitmap->mRawMemory;
		const std::uint8_t* source = pixelData;

		for (std::uint32_t row = rows.firstRow; row != rows.endRow; row += rows.rowStep)
		{
			for (std::uint32_t x = 0; x < widthBits; ++x)
			{
				output[row * widthBits + x] = *source++;
			}

			// The original does not skip source-row padding here.
		}

		return bitmap;
	}

	// Original function: FUN_0040c247
	CBitmap* CPicture::l_DecodeUnpacked16(const std::byte* header, const std::uint8_t* pixelData)
	{
		const std::uint32_t widthBits = Picture_ReadBmpDword(header, 0x12);
		const SPictureBmpRows rows = Picture_GetBmpRows(Picture_ReadBmpDword(header, 0x16));

		CBitmap* bitmap = new (std::nothrow) CBitmap(Runtime_TimeSigned32(widthBits), Runtime_TimeSigned32(rows.heightBits), 2);
		std::uint32_t* output = bitmap->mTrueColorMemory;
		const std::uint8_t* source = pixelData;

		for (std::uint32_t row = rows.firstRow; row != rows.endRow; row += rows.rowStep)
		{
			for (std::uint32_t x = 0; x < widthBits; ++x)
			{
				const std::uint32_t pixel = static_cast<std::uint32_t>(source[0]) | (static_cast<std::uint32_t>(source[1]) << 8u);
				source += 2;

				const std::uint32_t blue = (pixel & NXBasics::BitMask::Low5Bits) << 3u;
				const std::uint32_t green = (pixel >> 2u) & NXBasics::BitMask::ByteExceptBits0Through2;
				const std::uint32_t red = (pixel >> 7u) & NXBasics::BitMask::ByteExceptBits0Through2;

				output[row * widthBits + x] = g32BitPixelConverter->PackColor(red, green, blue);
			}

			// The original does not skip source-row padding here.
		}

		return bitmap;
	}

	// Original function: FUN_0040c310
	CBitmap* CPicture::l_DecodeUnpacked24(const std::byte* header, const std::uint8_t* pixelData)
	{
		const std::uint32_t widthBits = Picture_ReadBmpDword(header, 0x12);
		const SPictureBmpRows rows = Picture_GetBmpRows(Picture_ReadBmpDword(header, 0x16));

		CBitmap* bitmap = new (std::nothrow) CBitmap(Runtime_TimeSigned32(widthBits), Runtime_TimeSigned32(rows.heightBits), 2);
		std::uint32_t* output = bitmap->mTrueColorMemory;
		const std::uint32_t rowPixelCount = bitmap->mRowPixelCount;
		const std::uint8_t* source = pixelData;

		for (std::uint32_t row = rows.firstRow; row != rows.endRow; row += rows.rowStep)
		{
			for (std::uint32_t x = 0; x < widthBits; ++x)
			{
				const std::uint32_t blue = source[0];
				const std::uint32_t green = source[1];
				const std::uint32_t red = source[2];
				source += 3;

				output[row * rowPixelCount + x] = g32BitPixelConverter->PackColor(red, green, blue);
			}

			const std::uint32_t sourceRowBytes = widthBits * 3u;
			source += (0u - sourceRowBytes) & 3u;
		}

		return bitmap;
	}

	// Original function: FUN_0040c3fc
	CBitmap* CPicture::l_DecodeUnpacked32(const std::byte* header, const std::uint8_t* pixelData)
	{
		const std::uint32_t widthBits = Picture_ReadBmpDword(header, 0x12);
		const SPictureBmpRows rows = Picture_GetBmpRows(Picture_ReadBmpDword(header, 0x16));

		CBitmap* bitmap = new (std::nothrow) CBitmap(Runtime_TimeSigned32(widthBits), Runtime_TimeSigned32(rows.heightBits), 2);
		std::uint32_t* output = bitmap->mTrueColorMemory;
		const std::uint8_t* source = pixelData;

		for (std::uint32_t row = rows.firstRow; row != rows.endRow; row += rows.rowStep)
		{
			for (std::uint32_t x = 0; x < widthBits; ++x)
			{
				const std::uint32_t blue = source[0];
				const std::uint32_t green = source[1];
				const std::uint32_t red = source[2];
				source += 4;

				output[row * widthBits + x] = g32BitPixelConverter->PackColor(red, green, blue);
			}
		}

		return bitmap;
	}

	// Original function: FUN_0040c4c1
	CBitmap* CPicture::l_DecodeRLE8(const std::byte* header, const std::uint8_t* pixelData)
	{
		const std::uint32_t widthBits = Picture_ReadBmpDword(header, 0x12);
		const std::uint32_t heightBits = Picture_ReadBmpDword(header, 0x16);

		CBitmap* bitmap = new (std::nothrow) CBitmap(Runtime_TimeSigned32(widthBits), Runtime_TimeSigned32(heightBits), 0);
		std::uint8_t* output = bitmap->mRawMemory;

		std::uint32_t x = 0;
		std::uint32_t row = heightBits - 1u;
		std::uint32_t sourceOffset = 0;

		for (;;)
		{
			const std::uint8_t count = pixelData[sourceOffset++];
			const std::uint8_t value = pixelData[sourceOffset++];

			if (count != 0)
			{
				for (std::uint32_t index = 0; index < count; ++index)
				{
					output[row * widthBits + x] = value;
					++x;
				}

				continue;
			}

			switch (value)
			{
				case SwitchIds::BmpRleEscape::EndOfLine:
					--row;
					x = 0;
					break;

				case SwitchIds::BmpRleEscape::EndOfBitmap:
					return bitmap;

				case SwitchIds::BmpRleEscape::Delta:
					{
						const std::uint32_t deltaX = pixelData[sourceOffset++];
						const std::uint32_t deltaY = pixelData[sourceOffset++];

						x += deltaX;
						row -= deltaY;
						break;
					}

				default:
					for (std::uint32_t index = 0; index < value; ++index)
					{
						output[row * widthBits + x] = pixelData[sourceOffset++];
						++x;
					}

					// Align the consumed byte count relative to the RLE stream.
					if ((sourceOffset & 1u) != 0)
					{
						++sourceOffset;
					}

					break;
			}
		}
	}
}

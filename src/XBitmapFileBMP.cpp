#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "XBitmap.h"
#include "XBitmapFileBMP.h"
#include "XCrtFileAccess.h"
#include "XFile.h"

#include <cstring>
#include <new>

namespace NXBasics
{
	// Original function: FUN_004984eb.
	CBitmapFileBMP::CBitmapFileBMP() : mFile(nullptr), mRowsWritten(0), mWidth(0), mHeight(0)
	{
		static_assert(offsetof(CBitmapFileBMP, mFile) == 0);
		static_assert(offsetof(CBitmapFileBMP, mRowsWritten) == sizeof(void*));
		static_assert(offsetof(CBitmapFileBMP, mWidth) == sizeof(void*) + 4);
		static_assert(offsetof(CBitmapFileBMP, mHeight) == sizeof(void*) + 8);
	}

	// Original function: FUN_004984fc; bottom-up bands stop at the declared image height.
	void CBitmapFileBMP::WriteBitmap(const CBitmap* bitmap)
	{
		using namespace NC2Lifetime;

		if (mFile == nullptr || Read<std::uint32_t>(bitmap, NC2Layout::BitmapStorageOffset::Depth) != 2 || Read<std::int32_t>(bitmap, NC2Layout::BitmapStorageOffset::Width) != mWidth)
		{

			return;
		}

		const auto rowBytes = Runtime_TimeSigned32((static_cast<std::uint32_t>(mWidth) * 24u + 31u) & ~31u) / 8;
		auto* row = static_cast<std::uint8_t*>(Runtime_Allocate(static_cast<std::uint32_t>(rowBytes)));
		std::memset(row, 0, rowBytes);

		for (std::int32_t y = Read<std::int32_t>(bitmap, NC2Layout::BitmapStorageOffset::Height) - 1; y >= 0 && mRowsWritten < mHeight; --y)
		{
			// 0040F522 resolves this pointer from +2C and the pixel stride at +30.
			const auto* pixels = Read<const std::uint8_t*>(bitmap, NC2Layout::BitmapStorageOffset::TrueColorMemory) + static_cast<std::uint32_t>(y) * Read<std::uint32_t>(bitmap, NC2Layout::BitmapStorageOffset::RowPixelCount) * 4u;

			for (std::int32_t x = 0; x < mWidth; ++x)
			{
				std::memcpy(row + x * 3, pixels + x * 4, 3);
			}

			mFile->Write(row, rowBytes);
			++mRowsWritten;
		}

		if (row != nullptr)
			Runtime_Free(row);
	}

	// Original function: FUN_004985c1; explicit little-endian packed 14-byte file header.
	void CBitmapFileBMP::Open(std::int32_t width, std::int32_t height, const char* filename)
	{
		using namespace NC2Lifetime;
		mRowsWritten = 0;
		mWidth = width;
		mHeight = height;
		std::byte info[40]{ };
		Write<std::uint32_t>(info, NC2Layout::BitmapInfoHeaderOffset::Size, 40);
		Write(info, NC2Layout::BitmapInfoHeaderOffset::Width, width);
		Write(info, NC2Layout::BitmapInfoHeaderOffset::Height, height);
		Write<std::uint16_t>(info, NC2Layout::BitmapInfoHeaderOffset::Planes, 1);
		Write<std::uint16_t>(info, NC2Layout::BitmapInfoHeaderOffset::BitCount, 24);
		const auto rowBytes = Runtime_TimeSigned32((static_cast<std::uint32_t>(width) * 24u + 31u) & ~31u) / 8;
		const auto imageBytes = static_cast<std::uint32_t>(rowBytes) * static_cast<std::uint32_t>(height);
		Write(info, NC2Layout::BitmapInfoHeaderOffset::ImageSize, imageBytes);
		std::byte header[14]{ };
		Write<std::uint16_t>(header, NC2Layout::BitmapFileHeaderOffset::Signature, 0x4D42);
		Write(header, NC2Layout::BitmapFileHeaderOffset::FileSize, imageBytes + 54u);
		Write<std::uint32_t>(header, NC2Layout::BitmapFileHeaderOffset::PixelDataOffset, 54);

		if (mFile != nullptr)
		{
			mFile->Delete(1);
			mFile = nullptr;
		}

		void* memory = Runtime_AllocateWithNewHandler(sizeof(CFile));
		mFile = memory == nullptr ? nullptr : new (memory) CFile(filename, false);
		mFile->OpenForWriting(true);
		mFile->Write(header, sizeof(header));
		mFile->Write(info, sizeof(info));
	}

	// Original function: FUN_00498697; repeated closes are explicitly supported.
	void CBitmapFileBMP::Close()
	{
		if (mFile != nullptr)
		{
			mFile->Delete(1);
			mFile = nullptr;
		}
	}
}
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace NXBasics
{
	class CBitmap;
	class CMemory;
	class CPalette;

	class CPicture
	{
	public:
		// Original function: FUN_0040bd76
		explicit CPicture(const char* fileName);
		CPicture(const CPicture&) = delete;
		CPicture& operator=(const CPicture&) = delete;

		// Original function: FUN_0040be7d
		~CPicture();

		CBitmap* ExtractBitmap();

		// Original function: FUN_0040bebd
		CPalette* ExtractPalette();

		// Source-level accessor for the original field at +0x00.
		CBitmap* GetBitmap() const noexcept
		{
			return mBitmap;
		}

		// Remaster accessor; borrowed together with GetBitmap(), owned by the picture.
		CPalette* GetPalette() const noexcept
		{
			return mPalette;
		}

	private:
		// Original function: FUN_0040bed6
		void l_UnPackPCX(CMemory* memory);

		// Original function: FUN_0040c054
		void l_UnPackBMP(CMemory* memory);

		// Original function: FUN_0040c1b8
		CBitmap* l_DecodeUnpacked8(const std::byte* header, const std::uint8_t* pixelData);

		// Original function: FUN_0040c247
		CBitmap* l_DecodeUnpacked16(const std::byte* header, const std::uint8_t* pixelData);

		// Original function: FUN_0040c310
		CBitmap* l_DecodeUnpacked24(const std::byte* header, const std::uint8_t* pixelData);

		// Original function: FUN_0040c3fc
		CBitmap* l_DecodeUnpacked32(const std::byte* header, const std::uint8_t* pixelData);

		// Original function: FUN_0040c4c1
		CBitmap* l_DecodeRLE8(const std::byte* header, const std::uint8_t* pixelData);

		CBitmap* mBitmap;                         // +0x00
		CPalette* mPalette;                       // +0x04
		std::uint8_t mOwnsResources;              // +0x08; controls deletion of both objects.
		std::byte mReserved09[3];                 // +0x09; zeroed by the original constructor.
	};

	static_assert(sizeof(CPicture) == (sizeof(void*) == 8 ? 24 : 12), "Unexpected native picture layout");
	static_assert(!std::is_polymorphic_v<CPicture>, "CPicture must not contain a virtual-table pointer");
}

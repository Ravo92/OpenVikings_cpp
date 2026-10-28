#pragma once

#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	class CFile;

	// Windows-only BMP stream writer; its original class name is not in the Mac header.
	class CBitmapFileBMP
	{
	public:
		CBitmapFileBMP();
		void WriteBitmap(const CBitmap* bitmap);
		void Open(std::int32_t width, std::int32_t height, const char* filename);
		void Close();
	private:
		CFile* mFile;
		std::int32_t mRowsWritten;
		std::int32_t mWidth;
		std::int32_t mHeight;
	};
	static_assert(sizeof(CBitmapFileBMP) == (sizeof(void*) == 8 ? 24 : 16));
}

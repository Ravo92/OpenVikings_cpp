#include "XCrtFileAccess.h"
#include "XPicture.h"
#include "XPictureAccess.h"

#include <cstdint>
#include <new>

namespace NXBasics
{
	// Compatibility entry point for the existing callers.
	CPalette* Picture_ExtractPalette(void* picture)
	{

		return static_cast<CPicture*>(picture)->ExtractPalette();
	}

	// Original function: FUN_0040c5a6
	CPalette* XB_PictureTool_LoadPaletteOutOfPicture(const char* fileName)
	{
		void* allocation = Runtime_AllocateWithNewHandler(sizeof(CPicture), 1);
		CPicture* picture = nullptr;

		if (allocation != nullptr)
		{
			picture = ::new (allocation) CPicture(fileName);
		}

		CPalette* palette = picture->ExtractPalette();

		if (picture != nullptr)
		{
			picture->~CPicture();
			Runtime_Free(allocation);
		}

		return palette;
	}
}
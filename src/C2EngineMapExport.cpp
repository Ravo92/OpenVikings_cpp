#include "C2EngineDisplayAccess.h"
#include "C2GameMapAccess.h"
#include "XBitmap.h"
#include "XBitmapFileBMP.h"
#include "XCrtFileAccess.h"

#include <cstddef>

namespace NC2E2
{
	// Original function: FUN_00490d97; Game.exe constants 004F4B64=34 and 004F4B60=19.
	void Tool_SaveWorldAsBMP(const char* filename, std::uint32_t percent)
	{
		const auto width = static_cast<std::int32_t>(NXBasics::Runtime_TruncateDoubleToInt64(static_cast<double>(NC2Logic::GameMap_GetWidth()) * 34.0));
		const auto height = static_cast<std::int32_t>(NXBasics::Runtime_TruncateDoubleToInt64(static_cast<double>(NC2Logic::GameMap_GetHeight()) * 19.0));
		const auto scaledWidth = (static_cast<std::uint32_t>(width) * percent) / 100u;
		NXBasics::CBitmapFileBMP file;
		file.Open(scaledWidth, (static_cast<std::uint32_t>(height) * percent) / 100u, filename);
		{
			NXBasics::CBitmap scaled(scaledWidth, (percent * 100u) / 100u, 2);
			NXBasics::CBitmap band(width, 100, 2);
			alignas(SRuntimeDisplayStorage) std::byte display[sizeof(SRuntimeDisplayStorage)];
			Display_Construct(display, &band, 0, nullptr);
			DE_ToDraw_SetState(display, 4, false);
			for (std::int32_t y = height - 100; y >= -100; y -= 100)
			{
				band.Fill(NXBasics::SColorRGB(0, 0, 0));
				DE_SetWantedPositionToPixelPosition(display, 0, y);
				Display_UpdateDisplay(display);
				band.CopyIntoBitmap_FitIn(&scaled);
				file.WriteBitmap(&scaled);
			}
			file.Close();
			Display_Destruct(display);
		}
		file.Close();
	}
}

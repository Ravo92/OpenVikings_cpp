#include "C2EngineDisplayAccess.h"
#include "C2EngineLifetimeAccess.h"
#include "C2EngineWeatherDrawing.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include "XRandom.h"

#include <algorithm>
#include <windows.h>

namespace NC2E2
{
	using NC2Lifetime::Read;

	namespace
	{
		struct SWeatherSpeed
		{
			std::int32_t vertical;
			std::int32_t horizontal;
		};

		// Complete original arrays 004F4458..004F4597; Mac supplies only their names.
		constexpr SWeatherSpeed sRainSpeedArray[] = { { 20, 50 }, { 19, 51 }, { 15, 55 }, { 16, 60 }, { 22, 45 }, { 25, 57 }, { 18, 64 }, { 16, 47 }, { 18, 52 }, { 16, 59 } };
		constexpr SWeatherSpeed sSnowSpeedArray[] = { { 20, 50 }, { 19, 51 }, { 15, 55 }, { 16, 60 }, { 22, 45 }, { 25, 57 }, { 18, 64 }, { 16, 47 }, { 18, 52 }, { 16, 59 } };
		constexpr SWeatherSpeed sSandSpeedArray[] = { { 90, 2 }, { 99, 3 }, { 95, 5 }, { 96, 7 }, { 82, 5 }, { 85, 6 }, { 88, 4 }, { 86, 7 }, { 88, 2 }, { 86, 9 }, { -90, 2 }, { -99, 3 }, { -95, 5 }, { -96, 7 }, { -82, 5 }, { -85, 6 }, { -88, 4 }, { -86, 7 }, { -88, 2 }, { -86, 9 } };

		std::int32_t Signed(std::uint32_t value)
		{

			return NXBasics::Runtime_TimeSigned32(value);
		}

		std::int32_t Add(std::int32_t first, std::int32_t second)
		{

			return Signed(static_cast<std::uint32_t>(first) + static_cast<std::uint32_t>(second));
		}

		std::uint32_t Weather_GetMapPoint(void* display)
		{
			const std::int32_t center[] = { Add(Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelX), Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth) / 2), Add(Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelY), Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight) / 2) };
			std::uint32_t point = 0;
			DisplayDescription_WorldPixelToMapMigCoordinates(display, center, &point);

			return point;
		}

		std::int32_t Weather_GetCount(std::int32_t density, std::int32_t width, std::int32_t height, double coverage)
		{
			// Original constants 004F45A0 and 004F4598/A8/B0, in x87 multiplication order.
			const double count = ((static_cast<double>(density) * 0.0001 * static_cast<double>(height)) * static_cast<double>(width)) * coverage;

			return Signed(static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(count)));
		}

		std::int32_t Weather_GetOrigin(void* display, std::uint32_t positionOffset, std::uint32_t extentOffset, std::uint32_t scale)
		{
			const auto origin = 0u - Read<std::uint32_t>(display, positionOffset) - static_cast<std::uint32_t>(Read<std::int32_t>(display, extentOffset) / 2);

			return Signed(origin * scale) / 10;
		}

		// Remaster: apply the original channel additions/divisor directly to BGRX,
		// retaining 8-bit precision without format-specific lookup allocations.
		std::uint32_t Weather_AdjustColor(std::uint32_t pixel, std::uint32_t redAdd, std::uint32_t greenAdd, std::uint32_t blueAdd, std::uint32_t divisor)
		{
			const auto red = (std::min)((((pixel >> 16) & 255u) + redAdd) / divisor, 255u);
			const auto green = (std::min)((((pixel >> 8) & 255u) + greenAdd) / divisor, 255u);
			const auto blue = (std::min)(((pixel & 255u) + blueAdd) / divisor, 255u);

			return (pixel & 0xFF000000u) | (red << 16) | (green << 8) | blue;
		}

		bool Weather_IsInside(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height)
		{

			return x >= 2 && y >= 2 && x < width - 2 && y < height - 2;
		}
	}

	// Original function: FUN_0047d37b; remaster writes 32-bit BGRX pixels.
	void Display_l_DE_SpecialOverlay_Rain_Draw(void* display)
	{
		if (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::RainEnabled) == 0)
			return;
		// The two original SColorRGB temporaries have no observable side effects.
		void* bitmap = Read<void*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap);
		const auto width = Read<std::int32_t>(bitmap, NXBasics::BitmapOffset::Width);
		const auto height = Read<std::int32_t>(bitmap, NXBasics::BitmapOffset::Height);
		const auto stride = Read<std::uint32_t>(bitmap, NXBasics::BitmapOffset::RowPixelCount);
		const auto point = Weather_GetMapPoint(display);
		const auto ignoreDark = ((Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) >> 2) & 1u) != 0;
		const auto playerId = Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId);
		auto* manager = E2MapWeatherManagerSectorManager_GetExistingObjectPtr();
		const auto density = E2MapWeatherManagerSectorManager_Rain_GetDensity(manager, point, ignoreDark, playerId);
		const auto count = Weather_GetCount(density, width, height, 0.025);

		if (count == 0)
			return;

		NXBasics::CRandom random(2);
		const auto time = Signed(timeGetTime() * 2u);
		const auto originY = Weather_GetOrigin(display, NC2Layout::EngineDisplayOffset::WorldPixelY, NC2Layout::EngineDisplayOffset::BufferHeight, 10);
		const auto originX = Weather_GetOrigin(display, NC2Layout::EngineDisplayOffset::WorldPixelX, NC2Layout::EngineDisplayOffset::BufferWidth, 10);

		if (Read<std::uint32_t>(bitmap, NXBasics::BitmapOffset::Depth) != 2)
		{

			return;
		}

		auto* pixels = Read<std::uint32_t*>(bitmap, NC2Layout::BitmapStorageOffset::TrueColorMemory);

		for (std::int32_t remaining = count; remaining > 0; --remaining)
		{
			const auto horizontal = random.Get() % 10u;
			const auto vertical = random.Get() % 10u;
			const auto driftY = Add(time / sRainSpeedArray[vertical].vertical, originY);
			const auto driftX = Add(time / sRainSpeedArray[horizontal].horizontal, originX);
			const auto x = Signed((random.Get() + static_cast<std::uint32_t>(driftX)) % static_cast<std::uint32_t>(width));
			const auto y = Signed((random.Get() + static_cast<std::uint32_t>(driftY)) % static_cast<std::uint32_t>(height));

			if (!Weather_IsInside(x, y, width, height))
				continue;

			auto* pixel = pixels + static_cast<std::uint32_t>(y) * stride + static_cast<std::uint32_t>(x);
			pixel[-static_cast<std::ptrdiff_t>(stride)] = Weather_AdjustColor(pixel[-static_cast<std::ptrdiff_t>(stride)], 50, 50, 70, 1);
			*pixel = Weather_AdjustColor(*pixel, 50, 50, 70, 1);
			pixel[stride] = Weather_AdjustColor(pixel[stride], 50, 50, 70, 1);
		}
	}

	// Original function: FUN_0047d64a; remaster writes 32-bit BGRX pixels.
	void Display_l_DE_SpecialOverlay_Snow_Draw(void* display)
	{
		if (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::SnowEnabled) == 0)
			return;

		void* bitmap = Read<void*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap);
		const auto width = Read<std::int32_t>(bitmap, NXBasics::BitmapOffset::Width);
		const auto height = Read<std::int32_t>(bitmap, NXBasics::BitmapOffset::Height);
		const auto stride = Read<std::uint32_t>(bitmap, NXBasics::BitmapOffset::RowPixelCount);
		const auto point = Weather_GetMapPoint(display);
		const auto ignoreDark = ((Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) >> 2) & 1u) != 0;
		const auto playerId = Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId);
		auto* manager = E2MapWeatherManagerSectorManager_GetExistingObjectPtr();
		const auto density = E2MapWeatherManagerSectorManager_Snow_GetDensity(manager, point, ignoreDark, playerId);
		const auto count = Weather_GetCount(density, width, height, 0.02);

		if (count == 0)
			return;

		NXBasics::CRandom random(1);
		const auto time = Signed(timeGetTime());
		const auto originY = Weather_GetOrigin(display, NC2Layout::EngineDisplayOffset::WorldPixelY, NC2Layout::EngineDisplayOffset::BufferHeight, 9);
		const auto originX = Weather_GetOrigin(display, NC2Layout::EngineDisplayOffset::WorldPixelX, NC2Layout::EngineDisplayOffset::BufferWidth, 9);

		if (Read<std::uint32_t>(bitmap, NXBasics::BitmapOffset::Depth) != 2)
		{

			return;
		}

		auto* pixels = Read<std::uint32_t*>(bitmap, NC2Layout::BitmapStorageOffset::TrueColorMemory);

		for (std::int32_t index = 0; index < count; ++index)
		{
			const auto horizontal = random.Get() % 10u;
			const auto vertical = random.Get() % 10u;
			const auto driftY = Add(time / sSnowSpeedArray[vertical].vertical, originY);
			const auto driftX = Add(time / sSnowSpeedArray[horizontal].horizontal, originX);
			const auto x = Signed((random.Get() + static_cast<std::uint32_t>(driftX)) % static_cast<std::uint32_t>(width));
			const auto y = Signed((random.Get() + static_cast<std::uint32_t>(driftY)) % static_cast<std::uint32_t>(height));

			if (!Weather_IsInside(x, y, width, height))
				continue;

			auto* pixel = pixels + static_cast<std::uint32_t>(y) * stride + static_cast<std::uint32_t>(x);
			// The old 0x7FFF center was format-dependent; BGRX uses opaque white.
			*pixel = (*pixel & 0xFF000000u) | 0x00FFFFFFu;

			if ((index & 1) != 0)
				continue;

			pixel[-1] = Weather_AdjustColor(pixel[-1], 170, 170, 170, 1);
			pixel[1] = Weather_AdjustColor(pixel[1], 170, 170, 170, 1);
			pixel[-static_cast<std::ptrdiff_t>(stride)] = Weather_AdjustColor(pixel[-static_cast<std::ptrdiff_t>(stride)], 170, 170, 170, 1);
			pixel[stride] = Weather_AdjustColor(pixel[stride], 170, 170, 170, 1);
		}
	}

	// Original function: FUN_0047d918; remaster writes 32-bit BGRX pixels.
	void Display_l_DE_SpecialOverlay_Sand_Draw(void* display)
	{
		if (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::SandEnabled) == 0)
			return;

		void* bitmap = Read<void*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap);
		const auto width = Read<std::int32_t>(bitmap, NXBasics::BitmapOffset::Width);
		const auto height = Read<std::int32_t>(bitmap, NXBasics::BitmapOffset::Height);
		const auto stride = Read<std::uint32_t>(bitmap, NXBasics::BitmapOffset::RowPixelCount);
		const auto point = Weather_GetMapPoint(display);
		const auto ignoreDark = ((Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) >> 2) & 1u) != 0;
		const auto playerId = Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId);
		auto* manager = E2MapWeatherManagerSectorManager_GetExistingObjectPtr();
		const auto density = E2MapWeatherManagerSectorManager_Sand_GetDensity(manager, point, ignoreDark, playerId);
		const auto count = Weather_GetCount(density, width, height, 0.05);

		if (count == 0)
			return;

		NXBasics::CRandom random(1);
		const auto time = Signed(timeGetTime());
		const auto originY = Weather_GetOrigin(display, NC2Layout::EngineDisplayOffset::WorldPixelY, NC2Layout::EngineDisplayOffset::BufferHeight, 9);
		const auto originX = Weather_GetOrigin(display, NC2Layout::EngineDisplayOffset::WorldPixelX, NC2Layout::EngineDisplayOffset::BufferWidth, 9);

		if (Read<std::uint32_t>(bitmap, NXBasics::BitmapOffset::Depth) != 2)
		{

			return;
		}

		auto* pixels = Read<std::uint32_t*>(bitmap, NC2Layout::BitmapStorageOffset::TrueColorMemory);

		for (std::int32_t remaining = count; remaining > 0; --remaining)
		{
			const auto horizontal = random.Get() % 20u;
			const auto vertical = random.Get() % 20u;
			const auto driftY = Add(time / sSandSpeedArray[vertical].vertical, originY);
			const auto driftX = Add(time / sSandSpeedArray[horizontal].horizontal, originX);
			const auto x = Signed((random.Get() + static_cast<std::uint32_t>(driftX)) % static_cast<std::uint32_t>(width));
			const auto y = Signed((random.Get() + static_cast<std::uint32_t>(driftY)) % static_cast<std::uint32_t>(height));

			if (!Weather_IsInside(x, y, width, height))
				continue;

			auto* pixel = pixels + static_cast<std::uint32_t>(y) * stride + static_cast<std::uint32_t>(x);
			const auto color = Weather_AdjustColor(*pixel, 300, 300, 200, 3);
			pixel[1] = color;
			*pixel = color;
			pixel[-2] = color;
			pixel[-1] = color;
		}
	}
}
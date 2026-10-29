#include "C2E2StaticDataPalettes.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineGuideDrawing.h"
#include "C2EngineLifetimeAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"

#include <cmath>
#include <cstring>

namespace NC2E2
{
	using NC2Lifetime::Read;

	namespace
	{
		const std::byte* Guide(std::uint32_t id)
		{

			return NC2Logic::CGuideArray::mStaticVars + id * 0x184u;
		}

		std::int32_t Sub(std::int32_t a, std::int32_t b)
		{

			return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b));
		}

		std::int32_t Add(std::int32_t a, std::int32_t b)
		{

			return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
		}

		// Original function: FUN_00495843. Constants are from Game.exe.
		float GuideDirectionAngle(float dx, float dy)
		{
			constexpr double pi = 3.1415927410125732421875;
			double angle = dy > 0.000001 ? std::atan(static_cast<double>(dx) / dy) : dy < -0.000001 ? std::atan(static_cast<double>(dx) / dy) + pi
			                                                                     : dx > 0.0f        ? pi * 0.5
			                                                                                        : pi * -0.5;

			while (angle < 0.0)
				angle += pi * 2.0;

			while (angle >= pi * 2.0)
				angle -= pi * 2.0;

			return static_cast<float>(angle);
		}

		// Original function: FUN_0049563f.
		std::uint32_t ls_AngleToBobId(float sourceAngle)
		{
			constexpr double pi = 3.1415927410125732421875;
			double angle = -static_cast<double>(sourceAngle) + pi + pi * 0.0555555559694767f;

			while (angle < 0.0)
				angle += pi * 2.0;

			while (angle >= pi * 2.0)
				angle -= pi * 2.0;

			return static_cast<std::uint32_t>(1 - static_cast<std::int32_t>(angle * -18.0 / (pi * 2.0)));
		}

		// Original function: FUN_00495a40; Windows returns only zero or one.
		int __cdecl ls_QSortCompareFunction(const void* first, const void* second)
		{
			const double firstCosine = std::cos(static_cast<double>(Read<float>(first, NC2Layout::MapCoordinatePairOffset::Coordinates)));
			const double secondCosine = std::cos(static_cast<double>(Read<float>(second, NC2Layout::MapCoordinatePairOffset::Coordinates)));

			return secondCosine >= firstCosine ? 0 : 1;
		}

		// Original function: FUN_004958fb; collect and sort at most twenty arms in the requested half-plane.
		void ls_DrawDirectionMarker(const void* guide, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness, const void* display, const void* manager, NXBasics::CPalette* palette, bool back)
		{
			float angles[20];
			std::uint32_t count = 0;

			for (std::int32_t i = 0; i < Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount); ++i)
			{
				const auto* target = Guide(Read<std::uint32_t>(guide, NC2Layout::GuideOffset::ConnectionGuideIdsUnsigned + static_cast<std::uint32_t>(i) * NC2Layout::RecordSize::GuideConnectionUnsigned));
				std::int32_t dx, dy;

				if (display != nullptr)
				{
					std::int32_t point[2];
					Display_MapMigToLocalDisplayPixelCoordinates(display, target + 0x14, point, false);
					dx = Sub(point[0], x);
					dy = Sub(point[1], y);
				}
				else
				{
					dx = Read<std::int16_t>(target, NC2Layout::GuideOffset::MapPositionX) - Read<std::int16_t>(guide, NC2Layout::GuideOffset::MapPositionX);
					dy = Read<std::int16_t>(target, NC2Layout::GuideOffset::MapPositionY) - Read<std::int16_t>(guide, NC2Layout::GuideOffset::MapPositionY);
				}

				const float angle = GuideDirectionAngle(static_cast<float>(dx), static_cast<float>(dy));
				const double cosine = std::cos(static_cast<double>(angle));

				if (back ? cosine >= 0.0 : !(cosine >= 0.0))
					continue;

				angles[count++] = angle;

				if (count == 20)
					break;
			}

			NXBasics::Runtime_QuickSort(angles, count, sizeof(float), ls_QSortCompareFunction);

			for (std::uint32_t i = 0; i < count; ++i)
			{
				NXBasics::BobManager_PrintBob_UsingShadedAlpha(manager, ls_AngleToBobId(angles[i]), bitmap, x, y, palette, brightness / 2);
			}
		}
	}

	// Original function: FUN_004956b7; arms behind the post, the post itself, then the front arms.
	void DrawAtom_Guide_DoDraw(std::uint32_t id, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness, const void* display)
	{
		const auto* guide = Guide(id);
		auto* manager = E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, "data\\engine2d\\bin\\bobs\\ls_guidepost.bmd");
		const auto marking = display == nullptr ? 0u : Display_DE_ObjectsToMark_Guide_GetMarkType(display, id);

		if (marking != 0)
		{
			const auto* color = DrawAtom_Tool_GetMarkingColorRef(marking, Read<std::int32_t>(guide, NC2Layout::GuideOffset::PlayerId));

			for (std::int32_t dy = -1; dy <= 1; ++dy)
			{
				NXBasics::BobManager_PrintBob_FX_DrawOutline(manager, 0, bitmap, x, Add(y, dy), color, 1);

				for (std::int32_t i = 0; i < Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount); ++i)
				{
					const auto* target = Guide(Read<std::uint32_t>(guide, NC2Layout::GuideOffset::ConnectionGuideIdsUnsigned + static_cast<std::uint32_t>(i) * NC2Layout::RecordSize::GuideConnectionUnsigned));
					std::int32_t point[2];
					Display_MapMigToLocalDisplayPixelCoordinates(display, target + 0x14, point, false);
					const auto angle = GuideDirectionAngle(static_cast<float>(Sub(point[0], x)), static_cast<float>(Sub(point[1], y)));
					NXBasics::BobManager_PrintBob_FX_DrawOutline(manager, ls_AngleToBobId(angle), bitmap, x, Add(y, dy), color, 1);
				}
			}
		}

		NXBasics::CPalette* palette;
		const auto player = Read<std::int32_t>(guide, NC2Layout::GuideOffset::PlayerId);

		if (player == -1)
		{
			palette = gStaticPalettes.palettes256[E2StaticDataPalettesManager_GetPalette256Id("tree01")].palette;
		}
		else
		{
			palette = EnginePalette_GetPlayerPalette(player);
		}

		ls_DrawDirectionMarker(guide, bitmap, x, y, brightness, display, manager, palette, true);
		NXBasics::BobManager_PrintBob_UsingShadedAlpha(manager, 0, bitmap, x, y, palette, brightness / 2);
		ls_DrawDirectionMarker(guide, bitmap, x, y, brightness, display, manager, palette, false);
	}

	// Original function: FUN_00495a77.
	void DrawAtom_Guide_GetBobData(std::uint32_t id, const void* display, S2DEngineDisplaySearchObjectBobData* output)
	{
		const auto* guide = Guide(id);
		std::memset(output, 0, sizeof(*output));
		auto* manager = E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, "data\\engine2d\\bin\\bobs\\ls_guidepost.bmd");
		output->AddBob(manager, 0, 0, 0);

		if (display == nullptr)
		{

			return;
		}

		std::int32_t source[2], target[2];
		Display_MapMigToLocalDisplayPixelCoordinates(display, static_cast<const std::byte*>(guide) + 0x14, source, false);

		for (std::int32_t i = 0; i < Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount); ++i)
		{
			Display_MapMigToLocalDisplayPixelCoordinates(display, static_cast<const std::byte*>(Guide(Read<std::uint32_t>(guide, NC2Layout::GuideOffset::ConnectionGuideIdsUnsigned + i * NC2Layout::RecordSize::GuideConnectionUnsigned))) + 0x14, target, false);
			output->AddBob(manager, ls_AngleToBobId(GuideDirectionAngle(static_cast<float>(Sub(target[0], source[0])), static_cast<float>(Sub(target[1], source[1])))), 0, 0);
		}
	}
}
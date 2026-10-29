#include "C2EngineDisplayAccess.h"
#include "C2EngineLandscapeDrawing.h"
#include "C2EngineTerrainData.h"
#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "GouraudShader.h"
#include "XBitmap.h"
#include "XNumericConstants.h"
#include "XPalette.h"

#include <algorithm>
#include <cstring>

namespace NC2E2
{
	namespace
	{
		using NC2Lifetime::Read;
		using I = std::int32_t;
		using U = std::uint32_t;
		using Shader = NToolGouraudShader::CGouraudShader;

		I Bits(U value)
		{

			return NXBasics::Runtime_TimeSigned32(value);
		}

		I Add(I a, I b)
		{

			return Bits(U(a) + U(b));
		}

		I Sub(I a, I b)
		{

			return Bits(U(a) - U(b));
		}

		I Mul(I a, I b)
		{

			return Bits(U(a) * U(b));
		}

		I Fixed(I value)
		{

			return Bits(U(value) << 8);
		}

		I Round(I value)
		{

			return Add(value, 127) >> 8;
		}

		I Wrap(I coordinate, I dimension)
		{
			if (coordinate == 0)
				return 0;

			if (coordinate < 0)
				coordinate = Sub(dimension, Bits(0u - U(coordinate)) % dimension);

			return coordinate % dimension;
		}

		struct SCorner
		{
			I x, y, brightness;
			U color;
		};

		SCorner Midpoint(const SCorner& a, const SCorner& b)
		{

			return { Add(a.x, b.x) / 2, Add(a.y, b.y) / 2, Add(a.brightness, b.brightness) / 2, 0 };
		}

		void AssignTexture(Shader& shader, I id)
		{
			shader.texture = Read<const std::uint8_t*>(gStaticDataTextures.bitmaps[id], NC2Layout::BitmapStorageOffset::RawMemory);
			shader.palette = gStaticDataTextures.palettes[id];
		}

		void AssignVertex(NToolGouraudShader::SVertex& vertex, const SCorner& corner, const I* coordinates, bool colors)
		{
			vertex.x = corner.x;
			vertex.y = corner.y;
			vertex.u = coordinates[0];
			vertex.v = coordinates[1];
			vertex.brightness = corner.brightness;

			if (colors)
			{
				const auto* color = gVertexColorsPalette->GetColorData() + corner.color * 4;
				vertex.red = color[2];
				vertex.green = color[1];
				vertex.blue = color[0];
			}
		}
	}

	// Original function: FUN_0047ccdd.
	I Display_l_DE_Hightlighting_GetMapBrightness(void* display, const void* packedPoint)
	{
		std::int16_t micro[2];

		if ((Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) & 4) != 0)
		{
			NC2Logic::MapMigPoint_ConstructFromMag(micro, packedPoint, false);

			if (!NC2Logic::GameMap_Exploration_HasBeenSeenOnce(Read<U>(display, NC2Layout::EngineDisplayOffset::PlayerId), micro))
				return 0;
		}

		const auto* macro = EngineMap_GetConstMapMacroPointPtr(packedPoint);

		if (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::HighlightingEnabled) != 0)
		{
			if (macro == nullptr)
				return 0;

			NC2Logic::MapMigPoint_ConstructFromMag(micro, packedPoint, false);
			const auto* engineMicro = EngineMap_GetConstMapMicroPointPtr(micro);

			if (engineMicro == nullptr)
				return 0;

			const I brightness = Read<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::Brightness);

			if (brightness == 0)
				return 0;

			const auto flags = Read<std::uint8_t>(engineMicro, NC2Layout::EngineMapMicroPointOffset::Flags);

			if ((flags & 1) == 0)
				return brightness;

			return (flags & 2) != 0 ? std::clamp(brightness * 3 / 2, 160, 255) : std::clamp(brightness * 2 / 5, 0, 100);
		}

		return macro != nullptr ? Read<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::Brightness) : 0;
	}

	// Original function: FUN_0047cda2.
	I Display_l_DE_Hightlighting_GetMapBrightness(void* display, U x, U y)
	{
		const std::int16_t point[2] = { static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) };

		return Display_l_DE_Hightlighting_GetMapBrightness(display, point);
	}

	// Original function: FUN_0049341c.
	I Ground_GetMacroHeight(U x, U y)
	{
		const auto* macro = NC2Logic::GameMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

		return macro != nullptr ? Read<std::uint8_t>(macro, NC2Layout::MapMacroPointOffset::Height) : 0;
	}

	// Original function: FUN_00493447.
	I Ground_GetMacroVertexColor(U x, U y)
	{
		const std::int16_t point[2] = { static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) };
		const auto* macro = EngineMap_GetConstMapMacroPointPtr(point);

		return macro != nullptr ? Read<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::VertexColorIndex) : 0;
	}

	// Original function: FUN_00492f26.
	void Display_l_DE_DrawSingleQuadrangle_DrawOverlayPattern(void* display, NXBasics::CBitmap* bitmap, bool triangleA, I x0, I y0, I brightness0, U mask0, I x1, I y1, I brightness1, U mask1, I x2, I y2, I brightness2, U mask2)
	{
		const auto* patterns = static_cast<const SE2StaticDataPatternData*>(gStaticPatternStorage.entries);
		const SE2StaticDataPatternData* overlays[2] = { patterns + gStaticPatternStorage.houseOverlayId, patterns + gStaticPatternStorage.roadOverlayId };
		const auto& transitions = gStaticTransitionStorage;
		const SE2StaticDataTransition* road1 = transitions.entries + transitions.roadOverlayIds[0];
		const SE2StaticDataTransition* road2 = transitions.entries + transitions.roadOverlayIds[1];
		const SE2StaticDataTransition* house1 = transitions.entries + transitions.houseOverlayIds[0];
		const SE2StaticDataTransition* house2 = transitions.entries + transitions.houseOverlayIds[1];
		auto& shader = *Read<Shader*>(display, NC2Layout::EngineDisplayOffset::GouraudShader);
		const SCorner corners[3] = { { x0, y0, brightness0, 0 }, { x1, y1, brightness1, 0 }, { x2, y2, brightness2, 0 } };
		const U masks[3] = { mask0, mask1, mask2 };
		static constexpr U passMasks[2] = { 0x30, 0x0C };
		// Complete shape table 004F4B70..004F4BA7, distinct from 004F4628.
		static constexpr I shapes[14] = { -1, 5, 3, 1, 4, 0, 2, -1, 5, 4, 0, 3, 1, 2 };

		for (unsigned pass = 0; pass != 2; ++pass)
		{
			const U flags = Read<U>(display, NC2Layout::EngineDisplayOffset::DrawFlags);

			if ((flags & NXBasics::BitMask::Bit16Signed) != 0 && (flags & NXBasics::BitMask::Bit17Signed) != 0 && (passMasks[pass] & NXBasics::BitMask::Bits2Through5) != 0)
				continue;

			U present = 0, firstMask = 0;

			for (unsigned i = 0; i != 3; ++i)
			{
				const U masked = masks[i] & passMasks[pass];

				if (masked != 0)
				{
					present |= 1u << i;

					if (firstMask == 0)
						firstMask = masked;
				}
			}

			if (present == 0)
				continue;

			const I* coordinates;

			if (present == 7)
			{
				const auto& pattern = *overlays[pass];
				AssignTexture(shader, pattern.textureId);
				coordinates = triangleA ? pattern.coordinatesA : pattern.coordinatesB;
			}
			else
			{
				const auto* transition = (firstMask & 8) != 0 ? road2 : (firstMask & 4) != 0                           ? road1
				                                                    : (firstMask & NXBasics::BitMask::Bit5Signed) != 0 ? house2
				                                                    : (firstMask & NXBasics::BitMask::Bit4Signed) != 0 ? house1
				                                                                                                       : nullptr;
				AssignTexture(shader, transition->textureId);
				shader.alphaTexture = Read<const std::uint8_t*>(gStaticDataTextures.bitmaps[transition->alphaTextureId], NC2Layout::BitmapStorageOffset::RawMemory);
				const I shape = shapes[present + (triangleA ? 0 : 7)];
				coordinates = triangleA ? transition->coordinatesA[shape] : transition->coordinatesB[shape];
			}

			if (triangleA)
			{
				AssignVertex(shader.vertices[0], corners[2], coordinates + 4, false);
				AssignVertex(shader.vertices[1], corners[0], coordinates, false);
				AssignVertex(shader.vertices[2], corners[1], coordinates + 2, false);
			}
			else
			{
				AssignVertex(shader.vertices[0], corners[1], coordinates + 2, false);
				AssignVertex(shader.vertices[1], corners[2], coordinates + 4, false);
				AssignVertex(shader.vertices[2], corners[0], coordinates, false);
			}

			if (present == 7)
			{
				shader.XShade();
			}
			else
			{
				shader.XShade_Alpha();
			}
		}
	}

	// Original function: FUN_00491ea8.
	bool Display_l_DE_DrawSingleQuadrangle_MigMag(void* display, const void* packedPoint, NXBasics::CBitmap* bitmap, NXBasics::SPoint position)
	{
		const auto* macro = EngineMap_GetConstMapMacroPointPtr(packedPoint);
		const U x = Read<std::uint16_t>(packedPoint, NC2Layout::MapCoordinatePairOffset::X), y = Read<std::uint16_t>(packedPoint, NC2Layout::MapCoordinatePairOffset::Y);
		const U left = static_cast<std::uint16_t>(x - ((y & 1) == 0 ? 1 : 0)), right = static_cast<std::uint16_t>(left + 1), bottom = static_cast<std::uint16_t>(y + 1), east = static_cast<std::uint16_t>(x + 1);
		const auto* mapMacro = NC2Logic::GameMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
		const I h0 = mapMacro != nullptr ? Read<std::uint8_t>(mapMacro, NC2Layout::MapMacroPointOffset::Height) : 0;
		const I h1 = Ground_GetMacroHeight(east, y);
		const I b0 = Display_l_DE_Hightlighting_GetMapBrightness(display, x, y), b1 = Display_l_DE_Hightlighting_GetMapBrightness(display, east, y);
		const auto* colorMacro = EngineMap_GetConstMapMacroPointPtr(packedPoint);
		const U c0 = colorMacro != nullptr ? Read<std::uint8_t>(colorMacro, NC2Layout::EngineMapMacroPointOffset::VertexColorIndex) : 0, c1 = Ground_GetMacroVertexColor(east, y);
		const I h2 = Ground_GetMacroHeight(left, bottom), h3 = Ground_GetMacroHeight(right, bottom);
		const I b2 = Display_l_DE_Hightlighting_GetMapBrightness(display, left, bottom), b3 = Display_l_DE_Hightlighting_GetMapBrightness(display, right, bottom);
		const U c2 = Ground_GetMacroVertexColor(left, bottom), c3 = Ground_GetMacroVertexColor(right, bottom);
		const I baseX = Sub(position.x, Fixed(Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroHalfHorizontalPixels))), scale = Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroHeightFixed);
		const I baseY = Sub(position.y, Mul(scale, h0)), spacing = Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroVerticalFixed), horizontal = Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroHorizontalFixed), parity = Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroHalfHorizontalFixed);
		SCorner origin{ Round(Add(baseX, parity)), Round(baseY), b0, c0 };
		SCorner eastCorner{ Round(Add(Add(baseX, horizontal), parity)), Round(Add(baseY, Mul(Sub(h0, h1), scale))), b1, c1 };
		SCorner lowerLeft{ Round(baseX), Round(Add(Add(baseY, Mul(Sub(h0, h2), scale)), spacing)), b2, c2 };
		SCorner lowerRight{ Round(Add(baseX, horizontal)), Round(Add(Add(baseY, Mul(Sub(h0, h3), scale)), spacing)), b3, c3 };

		if (eastCorner.x < 0 || (std::max)({ origin.y, eastCorner.y, lowerLeft.y, lowerRight.y }) < 0 || lowerLeft.x >= bitmap->GetRectangle().width || (std::min)({ origin.y, eastCorner.y, lowerLeft.y, lowerRight.y }) >= bitmap->GetRectangle().height)
			return false;

		const bool visibleA = Add(Add(b2, b3), b0) / 3 != 0, visibleB = Add(Add(b3, b1), b0) / 3 != 0;
		auto& shader = *Read<Shader*>(display, NC2Layout::EngineDisplayOffset::GouraudShader);
		shader.bitmap = bitmap;
		const auto* patterns = static_cast<const SE2StaticDataPatternData*>(gStaticPatternStorage.entries);
		const SCorner triangles[2][3] = { { lowerLeft, origin, lowerRight }, { eastCorner, lowerRight, origin } };
		const bool visible[2] = { visibleA, visibleB };

		for (unsigned triangle = 0; triangle != 2; ++triangle)
		{
			const auto& corners = triangles[triangle];
			const auto& pattern = patterns[visible[triangle] ? Read<std::uint16_t>(macro, triangle * NC2Layout::RecordSize::EnginePatternId) : gStaticPatternStorage.borderIds[0]];
			AssignTexture(shader, pattern.textureId);
			const I* coordinates = triangle == 0 ? pattern.coordinatesA : pattern.coordinatesB;
			const unsigned indicesA[3] = { 4, 0, 2 }, indicesB[3] = { 2, 4, 0 };
			const auto* indices = triangle == 0 ? indicesA : indicesB;
			const bool colored = gVertexColorsPalette != nullptr && (corners[0].color != 0 || corners[1].color != 0 || corners[2].color != 0);

			for (unsigned i = 0; i != 3; ++i)
			{
				AssignVertex(shader.vertices[i], corners[i], coordinates + indices[i], colored);
			}

			if (colored)
			{
				shader.XShade_VertexColor();
			}
			else
			{
				shader.XShade();
			}

			if (!visible[triangle])
				continue;

			for (int layer = 1; layer >= 0; --layer)
			{
				const auto encoded = Read<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::Transitions + triangle + layer * NC2Layout::RecordSize::MacroTransitionLayer);

				if (encoded == NXBasics::IntegerRange::UInt8MaximumSigned || (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlagsByte1) & (layer == 0 ? 8 : 0x10)) != 0)
					continue;

				const auto& transition = gStaticTransitionStorage.entries[encoded / 6];
				AssignTexture(shader, transition.textureId);
				shader.alphaTexture = Read<const std::uint8_t*>(gStaticDataTextures.bitmaps[transition.alphaTextureId], NC2Layout::BitmapStorageOffset::RawMemory);
				coordinates = triangle == 0 ? transition.coordinatesA[encoded % 6] : transition.coordinatesB[encoded % 6];
				const bool vertexColors = gVertexColorsPalette != nullptr;

				for (unsigned i = 0; i != 3; ++i)
				{
					AssignVertex(shader.vertices[i], corners[i], coordinates + indices[i], vertexColors);
				}

				if (vertexColors)
				{
					shader.XShade_VertexColor_Alpha();
				}
				else
				{
					shader.XShade_Alpha();
				}
			}
		}

		const auto drawOverlay = [&](bool a, const SCorner& p, U mp, const SCorner& q, U mq, const SCorner& r, U mr)
		{
			Display_l_DE_DrawSingleQuadrangle_DrawOverlayPattern(display, bitmap, a, p.x, p.y, p.brightness, mp, q.x, q.y, q.brightness, mq, r.x, r.y, r.brightness, mr);
		};

		if (visibleA && (Read<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::Flags) & 1) != 0)
		{
			std::int16_t cursor[2];
			NC2Logic::MapMigPoint_ConstructFromMag(cursor, packedPoint, false);
			U masks[6];
			masks[0] = Read<std::uint8_t>(EngineMap_GetConstMapMicroPointPtr(cursor), NC2Layout::EngineMapMicroPointOffset::Flags);
			const U directions[5] = { 1, 3, 2, 0, 0 };

			for (unsigned i = 0; i != 5; ++i)
			{
				NC2Logic::MapMigPoint_MoveCursorWrapped(cursor, directions[i]);
				masks[i + 1] = Read<std::uint8_t>(EngineMap_GetConstMapMicroPointPtr(cursor), NC2Layout::EngineMapMicroPointOffset::Flags);
			}

			const auto midOL = Midpoint(origin, lowerLeft), midOB = Midpoint(origin, lowerRight), midLB = Midpoint(lowerLeft, lowerRight);
			drawOverlay(true, origin, masks[0], midOB, masks[1], midOL, masks[2]);
			drawOverlay(true, midOL, masks[2], midLB, masks[4], lowerLeft, masks[3]);
			drawOverlay(false, midOL, masks[2], midOB, masks[1], midLB, masks[4]);
			drawOverlay(true, midOB, masks[1], lowerRight, masks[5], midLB, masks[4]);
		}

		if (visibleB && (Read<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::Flags) & 2) != 0)
		{
			std::int16_t cursor[2];
			NC2Logic::MapMigPoint_ConstructFromMag(cursor, packedPoint, false);
			U masks[6];
			masks[0] = Read<std::uint8_t>(EngineMap_GetConstMapMicroPointPtr(cursor), NC2Layout::EngineMapMicroPointOffset::Flags);
			const U directions[5] = { 0, 0, 2, 3, 1 };

			for (unsigned i = 0; i != 5; ++i)
			{
				NC2Logic::MapMigPoint_MoveCursorWrapped(cursor, directions[i]);
				masks[i + 1] = Read<std::uint8_t>(EngineMap_GetConstMapMicroPointPtr(cursor), NC2Layout::EngineMapMicroPointOffset::Flags);
			}

			const auto midOR = Midpoint(origin, eastCorner), midOB = Midpoint(origin, lowerRight), midRB = Midpoint(eastCorner, lowerRight);
			drawOverlay(false, origin, masks[0], midOR, masks[1], midOB, masks[4]);
			drawOverlay(true, midOR, masks[1], midRB, masks[3], midOB, masks[4]);
			drawOverlay(false, midOR, masks[1], eastCorner, masks[2], midRB, masks[3]);
			drawOverlay(false, midOB, masks[4], midRB, masks[3], lowerRight, masks[5]);
		}

		return true;
	}

	// Original function: FUN_00491de2.
	void Display_l_DE_DrawQuadrangleField_MigMag(void* display, const std::int16_t* rectangle, NXBasics::CBitmap* bitmap, const I* pixelPoint)
	{
		std::int16_t point[2]{ };

		for (I row = 0; row < rectangle[3]; ++row)
		{
			point[1] = static_cast<std::int16_t>(rectangle[1] + row);
			NC2Logic::MapMagPoint_ValidateYPosition(point);
			const I y = Add(Mul(Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroVerticalFixed), row), pixelPoint[1]);
			const I parity = (point[1] & 1) != 0 ? Fixed(Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroHalfHorizontalPixels)) : 0;

			for (I column = 0; column < rectangle[2]; ++column)
			{
				point[0] = static_cast<std::int16_t>(rectangle[0] + column);
				NC2Logic::MapMagPoint_ValidateXPosition(point);
				Display_l_DE_DrawSingleQuadrangle_MigMag(display, point, bitmap, { Add(Add(Mul(Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroHorizontalFixed), column), pixelPoint[0]), parity), y });
			}
		}
	}

	// Original function: FUN_00491c12.
	void Display_l_DE_DrawStatic(void* display, bool outside, I worldX, I worldY, NXBasics::CBitmap* bitmap)
	{
		if (outside || (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlagsByte1) & 4) != 0)
		{
			bitmap->Fill(std::uint8_t(0));
			return;
		}

		*reinterpret_cast<std::uint8_t*>(&Read<Shader*>(display, NC2Layout::EngineDisplayOffset::GouraudShader)->flags) = 0;
		worldX = Wrap(worldX, Read<I>(display, NC2Layout::DisplayDescriptionOffset::WorldWidthPixels));
		worldY = Wrap(worldY, Read<I>(display, NC2Layout::DisplayDescriptionOffset::WorldHeightPixels));
		std::int16_t rectangle[4]{ };
		rectangle[1] = static_cast<std::int16_t>(Fixed(worldY) / Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroVerticalFixed));
		rectangle[0] = static_cast<std::int16_t>(Fixed(Add(Mul(rectangle[1] & 1, Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroHalfHorizontalPixels)), worldX)) / Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroHorizontalFixed) - 2);
		I pixelPoint[2] = { Sub(Mul(rectangle[0], Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroHorizontalFixed)), Fixed(worldX)), Sub(Mul(rectangle[1], Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroVerticalFixed)), Fixed(worldY)) };
		NC2Logic::MapMagPoint_ValidateXPosition(rectangle);
		rectangle[2] = static_cast<std::int16_t>(bitmap->GetRectangle().width / Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroHorizontalPixels) + 5);
		rectangle[3] = static_cast<std::int16_t>(bitmap->GetRectangle().height / Read<I>(display, NC2Layout::DisplayDescriptionOffset::MacroVerticalPixels) + 20);
		Display_l_DE_DrawQuadrangleField_MigMag(display, rectangle, bitmap, pixelPoint);

		if ((Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) & 1) == 0)
		{

			return;
		}

		rectangle[1] = static_cast<std::int16_t>(Fixed(worldY) / Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroVerticalFixed));
		rectangle[0] = static_cast<std::int16_t>(Fixed(Add(Mul(rectangle[1] & 1, Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroHalfHorizontalPixels)), worldX)) / Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroHorizontalFixed) - 2);
		pixelPoint[0] = Sub(Mul(rectangle[0], Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroHorizontalFixed)), Fixed(worldX));
		pixelPoint[1] = Sub(Mul(rectangle[1], Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroVerticalFixed)), Fixed(worldY));

		if (rectangle[0] < 0)
			pixelPoint[0] = Sub(pixelPoint[0], Mul(rectangle[0], Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroHorizontalFixed)));

		if (rectangle[1] < 0)
			pixelPoint[1] = Sub(pixelPoint[1], Mul(rectangle[1], Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroVerticalFixed)));

		rectangle[2] = static_cast<std::int16_t>(bitmap->GetRectangle().width / Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroHorizontalPixels) + 7);
		rectangle[3] = static_cast<std::int16_t>(bitmap->GetRectangle().height / Read<I>(display, NC2Layout::DisplayDescriptionOffset::MicroVerticalPixels) + 37);
		NC2Logic::MapMigRectangle_CutInsideMap(rectangle);
		pixelPoint[0] = Round(pixelPoint[0]);
		pixelPoint[1] = Round(pixelPoint[1]);
		Display_l_DE_DrawLandscapeAndCreaturesField(display, rectangle, bitmap, pixelPoint, true, false);
	}

	// Original function: FUN_00481e20.
	bool Display_l_DE_IsRectangleOutOfWorldArea(void* display, const S2DEngineDisplayRectangle* rectangle)
	{

		return !(Bits(rectangle->worldX) < Read<I>(display, NC2Layout::DisplayDescriptionOffset::WorldWidthPixels) && Bits(rectangle->worldX + rectangle->width) > 0 && Bits(rectangle->worldY) < Read<I>(display, NC2Layout::DisplayDescriptionOffset::WorldHeightPixels) && Bits(rectangle->worldY + rectangle->height) > 0);
	}

	// Original function: FUN_00481cdd.
	void Display_l_DE_DoUpdateStaticPart4(void* display, S2DEngineDisplayRectangle rectangle)
	{
		auto* source = Read<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::StaticBitmap);
		const U height = Read<U>(display, NC2Layout::EngineDisplayOffset::BufferHeight);

		if (Bits(rectangle.bitmapY + rectangle.height) > Bits(height))
		{
			const U firstHeight = height - rectangle.bitmapY;
			NXBasics::CBitmap first(source, { Bits(rectangle.bitmapX), Bits(rectangle.bitmapY), Bits(rectangle.width), Bits(firstHeight) });
			Display_l_DE_DrawStatic(display, Display_l_DE_IsRectangleOutOfWorldArea(display, &rectangle), Bits(rectangle.worldX), Bits(rectangle.worldY), &first);
			NXBasics::CBitmap second(source, { Bits(rectangle.bitmapX), 0, Bits(rectangle.width), Bits(rectangle.height - firstHeight) });
			Display_l_DE_DrawStatic(display, Display_l_DE_IsRectangleOutOfWorldArea(display, &rectangle), Bits(rectangle.worldX), Bits(rectangle.worldY + firstHeight), &second);
		}
		else
		{
			NXBasics::CBitmap bitmap(source, { Bits(rectangle.bitmapX), Bits(rectangle.bitmapY), Bits(rectangle.width), Bits(rectangle.height) });
			Display_l_DE_DrawStatic(display, Display_l_DE_IsRectangleOutOfWorldArea(display, &rectangle), Bits(rectangle.worldX), Bits(rectangle.worldY), &bitmap);
		}
	}

	// Original function: FUN_00481c10.
	void Display_l_DE_DoUpdateStaticPart3(void* display, S2DEngineDisplayRectangle rectangle)
	{
		const I end = Bits(rectangle.worldY + rectangle.height), worldHeight = Read<I>(display, NC2Layout::DisplayDescriptionOffset::WorldHeightPixels);
		U split = 0;
		const bool crossesZero = end > 0 && Bits(rectangle.worldY) < 0;

		if (crossesZero)
		{
			split = 0u - rectangle.worldY;
		}
		else if (end > 0 && Bits(rectangle.worldY) < worldHeight && end > worldHeight)
		{
			split = U(worldHeight) - rectangle.worldY;
		}
		else
		{
			Display_l_DE_DoUpdateStaticPart4(display, rectangle);

			return;
		}

		auto first = rectangle;
		first.height = split;
		Display_l_DE_DoUpdateStaticPart4(display, first);
		rectangle.bitmapY += split;
		const U bufferHeight = Read<U>(display, NC2Layout::EngineDisplayOffset::BufferHeight);

		if (Bits(rectangle.bitmapY) >= Bits(bufferHeight))
			rectangle.bitmapY -= bufferHeight;

		rectangle.worldY = crossesZero ? 0 : U(worldHeight);
		rectangle.height -= split;

		if (crossesZero)
		{
			Display_l_DE_DoUpdateStaticPart3(display, rectangle);
		}
		else
		{
			Display_l_DE_DoUpdateStaticPart4(display, rectangle);
		}
	}

	// Original function: FUN_00481baf.
	void Display_l_DE_DoUpdateStaticPart2(void* display, S2DEngineDisplayRectangle rectangle)
	{
		const U width = Read<U>(display, NC2Layout::EngineDisplayOffset::BufferWidth);

		if (Bits(rectangle.bitmapX + rectangle.width) > Bits(width))
		{
			const U split = width - rectangle.bitmapX;
			auto first = rectangle;
			first.width = split;
			Display_l_DE_DoUpdateStaticPart3(display, first);
			rectangle.bitmapX = 0;
			rectangle.worldX += split;
			rectangle.width -= split;
		}

		Display_l_DE_DoUpdateStaticPart3(display, rectangle);
	}

	// Original function: FUN_00481ae2.
	void __fastcall Display_l_DE_DoUpdateStaticPart1(void* display, void* /*unusedEdx*/, S2DEngineDisplayRectangle rectangle)
	{
		const I end = Bits(rectangle.worldX + rectangle.width), worldWidth = Read<I>(display, NC2Layout::DisplayDescriptionOffset::WorldWidthPixels);
		U split = 0;
		const bool crossesZero = end > 0 && Bits(rectangle.worldX) < 0;

		if (crossesZero)
		{
			split = 0u - rectangle.worldX;
		}
		else if (end > 0 && Bits(rectangle.worldX) < worldWidth && end > worldWidth)
		{
			split = U(worldWidth) - rectangle.worldX;
		}
		else
		{
			Display_l_DE_DoUpdateStaticPart2(display, rectangle);

			return;
		}

		auto first = rectangle;
		first.width = split;
		Display_l_DE_DoUpdateStaticPart2(display, first);
		rectangle.bitmapX += split;
		const U bufferWidth = Read<U>(display, NC2Layout::EngineDisplayOffset::BufferWidth);

		if (Bits(rectangle.bitmapX) >= Bits(bufferWidth))
			rectangle.bitmapX -= bufferWidth;

		rectangle.worldX = crossesZero ? 0 : U(worldWidth);
		rectangle.width -= split;

		if (crossesZero)
		{
			Display_l_DE_DoUpdateStaticPart1(display, nullptr, rectangle);
		}
		else
		{
			Display_l_DE_DoUpdateStaticPart2(display, rectangle);
		}
	}
}
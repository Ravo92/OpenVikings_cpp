#pragma once

#include "XNumericConstants.h"
#include "XPalette.h"

#include "XCrtFileAccess.h"
#include "XPixelFormatConverter.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

namespace NXBasics
{

	// Original function: FUN_0040faf5
	inline std::uint8_t Palette_FindMatchingColor(const void* palette, const SColorRGB& color)
	{
		const auto* paletteBytes = static_cast<const std::uint8_t*>(palette);

		std::uint32_t bestIndex = 0;
		std::uint32_t bestDistance = NXBasics::IntegerRange::Int32Maximum;

		for (std::uint32_t index = 0; index < 256; ++index)
		{
			const auto* entry = paletteBytes + PaletteOffset::Colors + index * 4u;

			const std::int32_t redDifference = static_cast<std::int32_t>(color.red) - static_cast<std::int32_t>(entry[2]);
			const std::int32_t redMagnitude = redDifference < 0 ? -redDifference : redDifference;
			const std::uint32_t weightedRed = static_cast<std::uint32_t>(redMagnitude) * 29u;

			const std::int32_t greenDifference = static_cast<std::int32_t>(color.green) - static_cast<std::int32_t>(entry[1]);
			const std::int32_t greenMagnitude = greenDifference < 0 ? -greenDifference : greenDifference;
			const std::uint32_t weightedGreen = static_cast<std::uint32_t>(greenMagnitude) * 65u;

			const std::int32_t blueDifference = static_cast<std::int32_t>(color.blue) - static_cast<std::int32_t>(entry[0]);
			const std::int32_t blueMagnitude = blueDifference < 0 ? -blueDifference : blueDifference;
			const std::uint32_t weightedBlue = static_cast<std::uint32_t>(blueMagnitude) * 15u;

			const std::uint32_t distance = weightedRed * weightedRed + weightedGreen * weightedGreen + weightedBlue * weightedBlue;

			if (distance < bestDistance)
			{
				if (distance == 0)
				{
					return static_cast<std::uint8_t>(index);
				}

				bestDistance = distance;
				bestIndex = index;
			}
		}

		return static_cast<std::uint8_t>(bestIndex);
	}

	// Original function: FUN_0040fbd9
	inline void Palette_RebuildHighColorTable(void* palette)
	{
		auto* paletteBytes = static_cast<std::byte*>(palette);

		std::uint16_t* table;
		std::memcpy(&table, paletteBytes + PaletteOffset::HighColorTable, sizeof(table));

		if (table != nullptr)
		{
			Runtime_Free(table);

			table = nullptr;
			std::memcpy(paletteBytes + PaletteOffset::HighColorTable, &table, sizeof(table));
		}

		if (!g16BitPixelConverter->IsInitialized())
		{

			return;
		}

		table = static_cast<std::uint16_t*>(Runtime_Allocate(512));
		std::memcpy(paletteBytes + PaletteOffset::HighColorTable, &table, sizeof(table));

		for (std::uint32_t index = 0; index < 256; ++index)
		{
			const auto* entry = reinterpret_cast<const std::uint8_t*>(paletteBytes + PaletteOffset::Colors + index * 4u);

			const std::uint32_t blue = entry[0];
			auto* converter = g16BitPixelConverter;
			const std::uint32_t green = entry[1];
			const std::uint32_t red = entry[2];

			const std::uint16_t packedColor = converter->PackColor(red, green, blue);

			// Reload the destination table after the conversion call.
			std::memcpy(&table, paletteBytes + PaletteOffset::HighColorTable, sizeof(table));
			table[index] = packedColor;
		}
	}

	// Original function: FUN_0040fc47
	inline const std::uint16_t* Palette_GetHighColorTablePtr(void* palette)
	{
		const auto* paletteBytes = static_cast<const std::byte*>(palette);

		const std::uint16_t* table;
		std::memcpy(&table, paletteBytes + PaletteOffset::HighColorTable, sizeof(table));

		if (table == nullptr)
		{
			Palette_RebuildHighColorTable(palette);
			std::memcpy(&table, paletteBytes + PaletteOffset::HighColorTable, sizeof(table));
		}

		return table;
	}

	// Original function: FUN_0040fc61
	inline std::uint16_t Palette_GetHighColorWord(void* palette, std::uint32_t colorIndex)
	{
		auto* paletteBytes = static_cast<std::byte*>(palette);

		std::uint16_t* table;
		std::memcpy(&table, paletteBytes + PaletteOffset::HighColorTable, sizeof(table));

		if (table == nullptr)
		{
			Palette_RebuildHighColorTable(palette);
			std::memcpy(&table, paletteBytes + PaletteOffset::HighColorTable, sizeof(table));
		}

		if (table == nullptr)
		{
			return 0;
		}

		const std::uint32_t byteOffset = colorIndex * 2u;
		const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(table) + byteOffset;

		std::uint16_t result;
		std::memcpy(&result, reinterpret_cast<const void*>(address), sizeof(result));

		return result;
	}

	// Original function: FUN_0040fc8e
	inline void Palette_RebuildTrueColorTable(void* palette)
	{
		auto* paletteBytes = static_cast<std::byte*>(palette);

		std::uint32_t* table;
		std::memcpy(&table, paletteBytes + PaletteOffset::TrueColorTable, sizeof(table));

		if (table != nullptr)
		{
			Runtime_Free(table);

			table = nullptr;
			std::memcpy(paletteBytes + PaletteOffset::TrueColorTable, &table, sizeof(table));
		}

		if (!g32BitPixelConverter->IsInitialized())
		{

			return;
		}

		table = static_cast<std::uint32_t*>(Runtime_Allocate(1024));
		std::memcpy(paletteBytes + PaletteOffset::TrueColorTable, &table, sizeof(table));

		for (std::uint32_t index = 0; index < 256; ++index)
		{
			const auto* entry = reinterpret_cast<const std::uint8_t*>(paletteBytes + PaletteOffset::Colors + index * 4u);

			const std::uint32_t blue = entry[0];
			auto* converter = g32BitPixelConverter;
			const std::uint32_t green = entry[1];
			const std::uint32_t red = entry[2];

			const std::uint32_t packedColor = converter->PackColor(red, green, blue);

			// Reload the destination table after the conversion call.
			std::memcpy(&table, paletteBytes + PaletteOffset::TrueColorTable, sizeof(table));
			table[index] = packedColor;
		}
	}

	// Original function: FUN_0040fd04
	inline const std::uint32_t* Palette_GetTrueColorTablePtr(void* palette)
	{
		const auto* paletteBytes = static_cast<const std::byte*>(palette);

		const std::uint32_t* table;
		std::memcpy(&table, paletteBytes + PaletteOffset::TrueColorTable, sizeof(table));

		if (table == nullptr)
		{
			Palette_RebuildTrueColorTable(palette);
			std::memcpy(&table, paletteBytes + PaletteOffset::TrueColorTable, sizeof(table));
		}

		return table;
	}

	// Original function: FUN_0040fd1e
	inline std::uint32_t Palette_GetTrueColorWord(void* palette, std::uint32_t colorIndex)
	{
		auto* paletteBytes = static_cast<std::byte*>(palette);

		std::uint32_t* table;
		std::memcpy(&table, paletteBytes + PaletteOffset::TrueColorTable, sizeof(table));

		if (table == nullptr)
		{
			Palette_RebuildTrueColorTable(palette);
			std::memcpy(&table, paletteBytes + PaletteOffset::TrueColorTable, sizeof(table));
		}

		if (table == nullptr)
		{
			return 0;
		}

		const std::uint32_t byteOffset = colorIndex * 4u;
		const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(table) + byteOffset;

		std::uint32_t result;
		std::memcpy(&result, reinterpret_cast<const void*>(address), sizeof(result));

		return result;
	}

	// Original function: FUN_0040fd45.
	inline void Palette_PaletteChanged(void* palette)
	{
		auto* paletteBytes = static_cast<std::byte*>(palette);

		void* table;
		std::memcpy(&table, paletteBytes + PaletteOffset::HighColorTable, sizeof(table));

		if (table != nullptr)
		{
			Runtime_Free(table);

			table = nullptr;
			std::memcpy(paletteBytes + PaletteOffset::HighColorTable, &table, sizeof(table));
		}

		std::memcpy(&table, paletteBytes + PaletteOffset::TrueColorTable, sizeof(table));

		if (table != nullptr)
		{
			Runtime_Free(table);

			table = nullptr;
			std::memcpy(paletteBytes + PaletteOffset::TrueColorTable, &table, sizeof(table));
		}
	}

}
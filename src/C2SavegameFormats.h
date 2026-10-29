#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

// Windows little-endian file contracts, independent of runtime pointers and padding.
namespace NC2SaveFormat
{
	inline constexpr std::uint32_t ChunkHeaderBytes = 32;
	inline constexpr std::uint32_t ArrayHeaderBytes = 24;
	inline constexpr std::uint32_t GameStateBytes = 152;
	inline constexpr std::uint32_t GuiMessageBytes = 780;
	inline constexpr std::uint32_t DynamicHouseBytes = 204;
	inline constexpr std::uint32_t DynamicAnimalVehicleBytes = 4;

	// Windows clears the embedded palette vptr with the dynamic pool and invalidates
	// its conversion caches before saving. These slots are reserved zero DWORDs.
	struct SPaletteRecord
	{
		std::uint32_t reservedVirtualTable;
		std::uint8_t colors[256][4];
		std::uint32_t reservedHighColorTable;
		std::uint32_t reservedTrueColorTable;
	};

	struct SHumanGraphicsRecord
	{
		std::uint32_t bodyVariant, headVariant;
		SPaletteRecord bodyPalette, headPalette;
		char name[100];
		std::uint32_t hasUserMessage;
	};

	static_assert(sizeof(SPaletteRecord) == 1036);
	static_assert(offsetof(SPaletteRecord, colors) == 4);
	static_assert(offsetof(SPaletteRecord, reservedHighColorTable) == 1028);
	static_assert(offsetof(SPaletteRecord, reservedTrueColorTable) == 1032);
	static_assert(sizeof(SHumanGraphicsRecord) == 2184);
	static_assert(offsetof(SHumanGraphicsRecord, bodyPalette) == 8);
	static_assert(offsetof(SHumanGraphicsRecord, headPalette) == 1044);
	static_assert(offsetof(SHumanGraphicsRecord, name) == 2080);
	static_assert(offsetof(SHumanGraphicsRecord, hasUserMessage) == 2180);
	static_assert(std::is_trivially_copyable_v<SHumanGraphicsRecord>);
	static_assert(std::is_standard_layout_v<SHumanGraphicsRecord>);
}
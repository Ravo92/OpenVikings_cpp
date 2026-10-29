#pragma once

#include <cstdint>

namespace NCRC32
{
	// Original function: FUN_004f2981
	std::uint32_t Calculate(const void* data, std::int32_t byteCount);

	// Original function: FUN_004f29c1
	void UpdateByte(std::uint32_t* checksum, std::uint8_t value);
}
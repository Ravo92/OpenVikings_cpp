#pragma once

#include <cstdint>

namespace NCRC16
{
	// Original function: FUN_004f28cf
	void InitializeCRCTable();

	// Original function: FUN_004f2902
	std::uint16_t Calculate(const void* data, std::int32_t length);
}
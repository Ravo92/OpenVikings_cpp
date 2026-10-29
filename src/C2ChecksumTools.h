#pragma once
#include <cstdint>

namespace NC2Logic
{
	std::uint32_t Tool_Checksum_Calculate(const void* data, std::uint32_t byteCount);
	void Tool_Checksum_Combine(std::uint32_t* checksum, std::uint32_t value);
	void Tool_Checksum_CalculateAndCombine(std::uint32_t* checksum, const void* data, std::uint32_t byteCount);
}
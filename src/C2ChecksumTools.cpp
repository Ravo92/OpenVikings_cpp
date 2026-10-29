#include "C2ChecksumTools.h"
#include "XMemoryChecksum.h"

namespace NC2Logic
{
	// Original function: FUN_0042a4dd.
	std::uint32_t Tool_Checksum_Calculate(const void* data, std::uint32_t byteCount)
	{

		return NXBasics::XB_GetMemoryChecksum(data, byteCount);
	}

	// Original function: FUN_0042a4ed; unsigned addition preserves the native DWORD wrap.
	void Tool_Checksum_Combine(std::uint32_t* checksum, std::uint32_t value)
	{
		*checksum = (*checksum ^ value) + 0x1234EDCBu;
	}

	// Original function: FUN_0042a500; the original directly calls the memory checksum.
	void Tool_Checksum_CalculateAndCombine(std::uint32_t* checksum, const void* data, std::uint32_t byteCount)
	{
		const auto value = NXBasics::XB_GetMemoryChecksum(data, byteCount);
		*checksum = (*checksum ^ value) + 0x1234EDCBu;
	}
}
#pragma once

#include "XCrtFileAccess.h"

#include <cstring>

namespace NXBasics
{
	// Original function: FUN_00410b46.
	inline void XB_Tool_ZeroMemory(void* memory, std::size_t byteCount)
	{
		std::memset(memory, 0, byteCount);
	}

	// Original function: FUN_00410c1f; loads the supplied DWORD as binary32.
	inline double XB_BinaryConvert_T32uToFloat(std::uint32_t bits)
	{
		float value;
		std::memcpy(&value, &bits, sizeof(value));

		return value;
	}

	// Original function: FUN_00410b59
	inline char* StringDuplicate(const char* source)
	{
		if (source == nullptr)
		{
			return nullptr;
		}

		const std::size_t byteCount = std::strlen(source) + 1;
		auto* copy = static_cast<char*>(Runtime_AllocateWithNewHandler(byteCount, 1));

		// Remaster: the allocation wrapper may return null after its handler declines retry.

		if (copy == nullptr)
			return nullptr;

		std::memcpy(copy, source, byteCount);

		return copy;
	}
}
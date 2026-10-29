#pragma once

#include <cstdint>

namespace NC2Logic
{
	struct SCleanMapGuid
	{
		std::uint8_t bytes[16];
		bool IsValid() const;
		bool IsEqualTo(const SCleanMapGuid& other) const;
	};

	static_assert(sizeof(SCleanMapGuid) == 16);
}
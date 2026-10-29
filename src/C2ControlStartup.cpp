#include "C2ControlStartup.h"

#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_00406b33
	SGameControlStartupData::SGameControlStartupData()
	{
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_00406b4a
	void SGameControlStartupData::Reset()
	{
		std::memset(this, 0, sizeof(*this));
	}
}
#pragma once

#include <cstdint>

namespace NNetwork::Synchronization
{
	// Main-thread host policy only; frequency commands still pass through the game command barrier.
	void Reset(std::uint32_t now);
	std::uint32_t GetBudget();
	void Update(std::uint32_t now);
}

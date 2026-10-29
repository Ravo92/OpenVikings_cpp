#pragma once

#include <cstdint>

namespace NC2Logic::RemasterAILog
{
	// Diagnostic state is process-local and never participates in saves, RNG or network decisions.
	void Initialize() noexcept;
	void Shutdown() noexcept;
	bool Enabled(std::uint32_t player, int level = 1) noexcept;
	void ResetWorld(const char* reason) noexcept;
	void Event(std::uint32_t player, const char* topic, std::int32_t subject, const char* format, ...) noexcept;
	void Update(std::uint32_t player) noexcept;
}
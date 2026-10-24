#pragma once

#include <cstdint>
#include <string>

namespace NRemaster::Logging
{
	enum class ECategory : std::uint8_t { Network, AI, Renderer, Count };
	// Called once with WinMain's arguments; the captured data working directory is the game root.
	void Initialize(const char* commandLine) noexcept;
	bool Enabled(ECategory category) noexcept;
	bool Write(ECategory category, std::string message) noexcept;
	void Event(ECategory category, const char* event, const char* format, ...) noexcept;
	// Stop producers first. This closes admission, drains the FIFO, flushes files and joins the writer.
	void Shutdown() noexcept;
}

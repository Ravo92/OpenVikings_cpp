#pragma once

#include <cstdint>

namespace NRemaster::ChestDiagnostics
{
	// Opt-in renderer diagnostics; follow the most recently opened chest without changing map state.
	void BeginOpen(std::uint32_t humanId, std::uint32_t playerId, const void* point) noexcept;
	void EndOpen(const void* point, std::int32_t transitionResult) noexcept;
	void ObserveDraw(const void* display, const void* point, bool staticPart, bool backgroundPart, bool submitted) noexcept;
}
#pragma once

#include <cstdint>

namespace NXBasics
{
	class CPalette;
}

namespace NC2E2
{
	struct SE2DynamicDataMapMoveableData;
}

namespace NRemaster::HumanSprites
{
	void Initialize(bool enabled, bool experimental = false);
	void Shutdown();
	bool Supports(std::uint32_t tribe, std::uint32_t job);
	std::uint32_t ResolveAppearanceTribe(std::uint32_t tribe, std::uint32_t job, std::int32_t player);
	void PrepareNativeFallback(NC2E2::SE2DynamicDataMapMoveableData& dynamic, std::uint32_t tribe, std::uint32_t job);
	std::uint32_t NativeFallbackJob(std::uint32_t tribe, std::uint32_t job);
	void PrepareNativeFallbackPalettes(std::uint32_t tribe, std::uint32_t job, std::int32_t player, NXBasics::CPalette& body, NXBasics::CPalette& head);
	bool PrepareEgyptianSoldierPalette(std::uint32_t tribe, std::uint32_t job, NXBasics::CPalette& body);
	bool SelectFrame(const void* moveable, NC2E2::SE2DynamicDataMapMoveableData& dynamic, std::uint32_t tribe, std::uint32_t job, std::uint32_t good, bool atomicActive, std::uint32_t action, std::uint32_t done, std::uint32_t total);
	NXBasics::CPalette* GetPalette(const NC2E2::SE2DynamicDataMapMoveableData& dynamic, std::int32_t player, NXBasics::CPalette* fallback);
}
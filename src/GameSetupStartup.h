#pragma once

#include <cstdint>

namespace NRemaster::GameSetup
{
	struct SPlayerChoices;

	namespace Startup
	{
		void Reset();
		void Prepare(const void* mapGuid, const SPlayerChoices& choices, bool enabled);
		std::uint32_t ResolveStartupTribe(std::uint32_t player, std::uint32_t originalTribe);
		void ResolveStartupHouse(std::uint32_t player, std::uint32_t& engineType, std::uint32_t& stage, std::uint32_t& point);

		class CStartupScope
		{
		  public:
			explicit CStartupScope(const void* mapGuid);
			~CStartupScope();
			CStartupScope(const CStartupScope&) = delete;
			CStartupScope& operator=(const CStartupScope&) = delete;
		};
	}
}
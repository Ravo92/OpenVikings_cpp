#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SGameRandomState
	{
		std::uint32_t currentState;
		std::uint32_t seed;
	};

	static_assert(sizeof(SGameRandomState) == 8);
	static_assert(offsetof(SGameRandomState, currentState) == 0);
	static_assert(offsetof(SGameRandomState, seed) == 4);
	std::uint32_t Random_Get(SGameRandomState& random);
}

namespace NXBasics
{
	class CRandom
	{
	public:
		explicit CRandom(std::uint32_t seed);
		void SetSeed(std::uint32_t seed);
		std::uint32_t Get()
		{
			return NC2Logic::Random_Get(mState);
		}

	private:
		NC2Logic::SGameRandomState mState;
	};

	static_assert(sizeof(CRandom) == 8);
}

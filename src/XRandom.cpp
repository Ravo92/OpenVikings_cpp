#include "XNumericConstants.h"
#include "XRandom.h"

namespace NXBasics
{
	// Original function: FUN_00410c2d.
	CRandom::CRandom(std::uint32_t seed)
	{
		SetSeed(seed);
	}

	// Original function: FUN_00410c3f.
	void CRandom::SetSeed(std::uint32_t seed)
	{
		// Complete original SetSeed::bits region 004F36CC..004F374B.
		static constexpr std::uint32_t bits[] = { 12, 23, 10, 25, 8, 27, 6, 29, 4, 30, 1, 22, 9, 13, 21, 0, 17, 26, 5, 15, 18, 28, 11, 2, 14, 3, 24, 7, 19, 16, 20, 31 };
		std::uint32_t value = seed & NXBasics::IntegerRange::Int32Maximum;
		std::uint32_t setBits = 0;

		for (std::uint32_t bit = 0; bit < 31; ++bit)
		{
			if ((value & (1u << bit)) != 0)
				++setBits;
		}

		if (setBits < 8)
			for (std::uint32_t index = 0; index < 8u - setBits; ++index)
			{
				value |= 1u << bits[index];
			}

		if (setBits > 24)
			for (std::uint32_t index = 0; index < 32u - setBits; ++index)
			{
				value &= ~(1u << bits[index]);
			}

		mState.seed = value;
		mState.currentState = value != 0 ? value & NXBasics::IntegerRange::Int32Maximum : 1u;
	}
}

// Shared scalar generator; independent of the game-state owner layout.
namespace NC2Logic
{
	// Original functions: FUN_00410ccd, FUN_00410cd2; NXBasics::CRandom::Get/l_GetLong.
	std::uint32_t Random_Get(SGameRandomState& random)
	{
		const std::uint32_t high = (random.currentState >> 16) * 0x41A7u;
		std::uint32_t value = (random.currentState & NXBasics::IntegerRange::UInt16Maximum) * 0x41A7u + ((high & NXBasics::IntegerRange::Int16Maximum) << 16);

		if (value > NXBasics::IntegerRange::Int32Maximum)
			value = (value & NXBasics::IntegerRange::Int32Maximum) + 1u;

		value += high >> 15;

		if (value > NXBasics::IntegerRange::Int32Maximum)
			value = (value & NXBasics::IntegerRange::Int32Maximum) + 1u;

		random.currentState = value;

		return value;
	}

}
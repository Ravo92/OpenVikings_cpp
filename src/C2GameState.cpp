#include "C2GameState.h"
#include "XNumericConstants.h"

namespace NC2Logic
{

	// Original function: FUN_00407570; CGame::Random_GetNumber.
	std::uint32_t Game_Random_GetNumber()
	{

		return Random_Get(gGameRandomState);
	}

	// Original function: FUN_0040aed5.
	std::uint32_t Logic_EntryPoint_Random_GetNumber()
	{

		return Game_Random_GetNumber();
	}

	// Original function: FUN_0040757a; CGame::UniqueId_Generate.
	std::uint32_t Game_UniqueId_Generate()
	{

		return ++gGameStaticVars.uniqueIdCounter;
	}
}
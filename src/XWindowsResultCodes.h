#pragma once

// Windows COM result values with the original integer literal types.
// Values and integer literal types are retained from the Windows reconstruction.
namespace NXBasics
{
	namespace ComResult
	{
		inline constexpr auto Failure = 0x80004005u;
		inline constexpr auto NotInitialized = 0x800401F0u;
	}

}
#include "XBTokenList.h"
#include "XCrtFileAccess.h"

#include <cstring>

namespace NXBasics
{
	// Original function: FUN_0042b66e
	CTokenList::CTokenList(const SToken* tokens) : mTokens(tokens)
	{
	}

	// Original function: FUN_0042b679
	std::uint32_t CTokenList::FindToken(const char* name)
	{

		return XB_Token_FindTokenInArray(mTokens, name);
	}

	// Original function: FUN_0042b689
	std::uint32_t XB_Token_FindTokenInArray(const SToken* tokens, const char* name)
	{
		for (const SToken* entry = tokens;; ++entry)
		{
			if (entry->name == nullptr || std::strlen(entry->name) == 0)
				return 0;

			if (Runtime_CompareStringsIgnoreCase(entry->name, name) == 0)
			{

				return entry->identifier;
			}
		}
	}
}
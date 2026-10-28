#pragma once

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	struct SToken
	{
		const char* name;
		std::uint32_t identifier;
	};

	static_assert(sizeof(SToken) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SToken, identifier) == sizeof(void*));

	class CTokenList
	{
	public:
		explicit CTokenList(const SToken* tokens);
		std::uint32_t FindToken(const char* name);

	private:
		const SToken* mTokens;
	};

	static_assert(sizeof(CTokenList) == sizeof(void*));

	std::uint32_t XB_Token_FindTokenInArray(const SToken* tokens, const char* name);
}
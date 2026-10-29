#pragma once

#include <cstdint>

namespace NXSysMisc
{
	void __cdecl XWS_AbortBox(const char* message);
	void __cdecl XWS_AssertBox(const char* message, const char* sourceFile, std::int32_t line);
	void __cdecl XWS_DirectDrawErrorBox(std::int32_t result, const char* sourceFile, std::int32_t line);

	// Original function: FUN_0047a9ea. The wrapper name is inferred.
	void __cdecl XWS_WarningBoxV(const char* format, ...);

	// Original function: FUN_0047aa20
	void __cdecl XWS_WarningBox(const char* message);
}
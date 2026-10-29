#pragma once

#include "GameProfile.h"

#include <Windows.h>

namespace NRemaster
{
	// Shared remaster entry; each launcher supplies its fixed game variant.
	int RunGame(EGameVariant variant, HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand);
}
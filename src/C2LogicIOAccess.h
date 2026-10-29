#pragma once

#include "OriginalStrings.h"

namespace NC2Logic
{
	const char* Player_GetSystemPlayerFileSystemName();

	// Original function: FUN_00440297
	inline const char* IO_Tool_GetPlayerDataDirectoryNamePtr()
	{

		return OriginalStrings::SaveDirectory;
	}

	// Original function: FUN_0044029d
	inline const char* IO_Tool_GetUserMapDirectoryNamePtr()
	{

		return OriginalStrings::UserMapDirectory;
	}
}
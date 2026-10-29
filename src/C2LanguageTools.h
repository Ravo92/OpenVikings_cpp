#pragma once

#include <cstdint>

namespace NC2Logic
{
	// Original function: FUN_00405ac8
	void LanguageTool_SetCurrentLanguage(std::int32_t languageId);
	std::int32_t LanguageTool_GetCurrentLanguage();

	// Original function: FUN_00405adb
	bool LanguageTool_GetFilename(const char* fileNamePattern, char* fileName, bool allowCompiledIni);

	// Original function: FUN_00405c5f
	bool LanguageTool_GetDirectoryName(const char* directoryPattern, char* directoryName, const char* referenceFileName);
}
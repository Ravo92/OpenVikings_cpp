#include "C2LanguageTools.h"
#include "OriginalStrings.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XTextEncoding.h"

#include <cstdint>
#include <cstring>

namespace NC2Logic
{
	namespace
	{
		// Original pointer table: 004f3388
		constexpr const char* lsLanguageDataArray[20] =
		{
			"ger",
			"eng",
			"fra",
			"ita",
			"cze",
			"rus",
			"pol",
			"spa",
			"por",
			"hun",
			"l10",
			"l11",
			"l12",
			"l13",
			"l14",
			"l15",
			"l16",
			"l17",
			"l18",
			"l19"
		};

		// Original data: 0050f754
		std::int32_t lsLanguageTypeCurrent = 0;

		constexpr std::int32_t kEnglishLanguageId = 1;
		constexpr std::int32_t kGermanLanguageId = 0;
	}

	// Original function: FUN_00405ac8
	void LanguageTool_SetCurrentLanguage(std::int32_t languageId)
	{
		if (languageId >= 0 && languageId < 20)
		{
			lsLanguageTypeCurrent = languageId;
			NXBasics::Text_SetLegacyCodePage(languageId == 5 ? 1251u : languageId == 4 || languageId == 6 || languageId == 9 ? 1250u : 1252u);
		}
	}

	// Original function: FUN_00405adb
	bool LanguageTool_GetFilename(const char* fileNamePattern, char* fileName, bool allowCompiledIni)
	{
		const auto tryLanguage = [fileNamePattern, fileName, allowCompiledIni] (const std::int32_t& languageId) -> bool
		{
			NXBasics::Runtime_PrintString(fileName, fileNamePattern, lsLanguageDataArray[languageId]);

			if (NXBasics::CFile::FileSystem_Tool_FileExists(fileName, true))
				return true;

			if (!allowCompiledIni)
				return false;

			// Re-read the language selection before the compiled-file attempt.
			NXBasics::Runtime_PrintString(fileName, fileNamePattern, lsLanguageDataArray[languageId]);

			const std::size_t length = std::strlen(fileName);

			if (length <= 4)
				return false;

			char* extension = fileName + length - 4;

			if (NXBasics::Runtime_CompareStringsIgnoreCase(extension, ".ini") != 0)
				return false;

			std::strcpy(extension, ".cif");
			return NXBasics::CFile::FileSystem_Tool_FileExists(fileName, true);
		};

		if (tryLanguage(lsLanguageTypeCurrent))
			return true;

		if (tryLanguage(kEnglishLanguageId))
			return true;

		return tryLanguage(kGermanLanguageId);
	}

	// Original function: FUN_00405c5f
	bool LanguageTool_GetDirectoryName(const char* directoryPattern, char* directoryName, const char* referenceFileName)
	{
		char testFileName[260];

		const auto tryLanguage = [&] (const std::int32_t& languageId) -> bool
		{
			NXBasics::Runtime_PrintString(testFileName, directoryPattern, lsLanguageDataArray[languageId]);
			std::strcat(testFileName, OriginalStrings::WindowsRootPath);
			std::strcat(testFileName, referenceFileName);

			if (!NXBasics::CFile::FileSystem_Tool_FileExists(testFileName, true))
				return false;

			// Re-read the language selection when producing the result.
			NXBasics::Runtime_PrintString(directoryName, directoryPattern, lsLanguageDataArray[languageId]);
			return true;
		};

		if (tryLanguage(lsLanguageTypeCurrent))
			return true;

		if (tryLanguage(kEnglishLanguageId))
			return true;

		if (tryLanguage(kGermanLanguageId))
			return true;

		NXBasics::Runtime_PrintString(directoryName, directoryPattern, lsLanguageDataArray[kEnglishLanguageId]);
		return false;
	}
}
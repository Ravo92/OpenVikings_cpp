#include "C2LanguageTools.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2StaticStringsManager.h"
#include "C2StringIO.h"
#include "XStringArray.h"

#include <cstring>

namespace NC2Logic
{
	namespace
	{
		// Complete original filename-pointer table: 005008E8..00500913.
		constexpr const char* staticStringFileNames[11] = {
			nullptr,
			"data\\text\\%s\\strings\\gameobjects\\goods.ini",
			"data\\text\\%s\\strings\\gameobjects\\tribes.ini",
			"data\\text\\%s\\strings\\gameobjects\\jobs.ini",
			"data\\text\\%s\\strings\\gameobjects\\houses.ini",
			"data\\text\\%s\\strings\\gameobjects\\experiences.ini",
			"data\\text\\%s\\strings\\gameobjects\\vehicletypes.ini",
			"data\\text\\%s\\strings\\gameobjects\\vehicletasks.ini",
			"data\\text\\%s\\strings\\gameobjects\\militarymodes.ini",
			"data\\text\\%s\\strings\\gameobjects\\misclogicstrings.ini",
			"data\\text\\%s\\strings\\gameobjects\\chesttypes.ini"
		};
	}

	// Original function: FUN_004170b5
	CStaticStringsManager::CStaticStringsManager()
	{
		static_assert(sizeof(gStaticStrings) == 44);
		gStaticStringsManager = this;
		std::memset(this, 0, 1);
		std::memset(gStaticStrings, 0, sizeof(gStaticStrings));
		char fileName[260];

		for (unsigned int index = 0; index < 11; ++index)
		{
			if (staticStringFileNames[index] == nullptr)
				continue;

			LanguageTool_GetFilename(staticStringFileNames[index], fileName, true);
			gStaticStrings[index] = StringIO_CreateOutOfIniFile(fileName);
		}
	}

	// Original function: FUN_00417121; previously reconstructed in the lifetime module.
	CStaticStringsManager::~CStaticStringsManager()
	{
		for (auto*& strings : gStaticStrings)
		{
			NC2Lifetime::DeleteStorable(strings);
			strings = nullptr;
		}

		gStaticStringsManager = nullptr;
	}
}
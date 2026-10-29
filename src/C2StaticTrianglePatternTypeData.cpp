#include "C2DataIds.h"
#include "C2EngineTerrainData.h"
#include "C2LifetimeGlobals.h"
#include "C2StaticTrianglePatternTypeData.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"

#include <cstring>
#include <new>

namespace NC2Logic
{
	SStaticTrianglePatternTypeStorage gStaticTrianglePatternTypeStorage{ };
	SStaticTrianglePatternTypeData*& gStaticTrianglePatternTypes = gStaticTrianglePatternTypeStorage.records;
	CStaticTrianglePatternTypeDataManager*& gStaticTrianglePatternTypeDataManager = gStaticTrianglePatternTypeStorage.owner;

	namespace
	{
		// Original function: FUN_004159b4, an empty token-list exit callback.
		// The runtime callback interface requires an int; its return is ignored.
		int __cdecl TrianglePatternTokens_OnExit()
		{

			return 0;
		}

		NXBasics::CTokenList& TrianglePatternTokens()
		{
			auto& storage = gStaticTrianglePatternTypeStorage;

			if ((storage.tokenInitFlags & 1) == 0)
			{
				storage.tokenInitFlags |= 1;
				storage.tokens[11] = { nullptr, 0 };
				storage.tokens[0] = { "type", NC2DataIds::TrianglePatternToken::Type };
				storage.tokens[1] = { "humancanwalkon", SwitchIds::TrianglePatternToken::Humancanwalkon };
				storage.tokens[2] = { "housecanbebuildon", SwitchIds::TrianglePatternToken::Housecanbebuildon };
				storage.tokens[3] = { "biocangrowon", NC2DataIds::TrianglePatternToken::BioCanGrowOn };
				storage.tokens[4] = { "biocanplanton", SwitchIds::TrianglePatternToken::Biocanplanton };
				storage.tokens[5] = { "island", SwitchIds::TrianglePatternToken::IsLand };
				storage.tokens[6] = { "iswater", SwitchIds::TrianglePatternToken::IsWater };
				storage.tokens[7] = { "isborder", SwitchIds::TrianglePatternToken::IsBorder };
				storage.tokens[8] = { "moveresistance", SwitchIds::TrianglePatternToken::MoveResistance };
				storage.tokens[9] = { "debugname", SwitchIds::TrianglePatternToken::Debugname };
				storage.tokens[10] = { "debugcolor", SwitchIds::TrianglePatternToken::DebugColor };
			}

			if ((storage.tokenInitFlags & 2) == 0)
			{
				storage.tokenInitFlags |= 2;
				::new (storage.tokenList) NXBasics::CTokenList(storage.tokens);
				NXBasics::Runtime_AtExit(TrianglePatternTokens_OnExit);
			}

			return *std::launder(reinterpret_cast<NXBasics::CTokenList*>(storage.tokenList));
		}
	}

	// Original function: FUN_00415596.
	CStaticTrianglePatternTypeDataManager::CStaticTrianglePatternTypeDataManager()
	{
		gStaticTrianglePatternTypeDataManager = this;
		std::memset(this, 0, 1);
		gStaticTrianglePatternTypes = nullptr;
		auto* records = static_cast<SStaticTrianglePatternTypeData*>(NXBasics::Runtime_AllocateWithNewHandler(528, 1));

		if (records != nullptr)
			for (std::uint32_t index = 0; index < 11; ++index)
			{
				::new (records + index) SStaticTrianglePatternTypeData;
			}

		gStaticTrianglePatternTypes = records;
		// Windows continues unconditionally after the conditional color construction.
		std::memset(records, 0, 528);
		std::strcpy(records[0].debugName, "none");
		records[0].debugColor.SetColors(60, 60, 60);
		records[0].isBorder = 1;

		NXBasics::CIniFileReader reader("data\\logic\\trianglepatterntypes.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
		{

			return;
		}

		for (char* group = reader.GetFirstGroup("trianglepatterntype"); group != nullptr; group = reader.GetNextGroup("trianglepatterntype"))
		{
			if (!reader.SearchCommand("type"))
				continue;

			const auto type = static_cast<std::uint32_t>(reader.GetInt());

			if (type == 0 || type >= 11)
				continue;

			auto& record = gStaticTrianglePatternTypes[type];
			reader.SetCurrentLineToGroupStart();

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				switch (TrianglePatternTokens().FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::TrianglePatternToken::Humancanwalkon:
						record.humanCanWalkOn = reader.GetInt() != 0;
						break;
					case SwitchIds::TrianglePatternToken::Housecanbebuildon:
						record.houseCanBeBuiltOn = reader.GetInt() != 0;
						break;
					case SwitchIds::TrianglePatternToken::BioCanGrowOn:
						record.bioCanGrowOn = reader.GetInt() != 0;
						break;
					case SwitchIds::TrianglePatternToken::Biocanplanton:
						record.bioCanPlantOn = reader.GetInt() != 0;
						break;
					case SwitchIds::TrianglePatternToken::IsLand:
						record.isLand = reader.GetInt() != 0;
						break;
					case SwitchIds::TrianglePatternToken::IsWater:
						record.isWater = reader.GetInt() != 0;
						break;
					case SwitchIds::TrianglePatternToken::IsBorder:
						record.isBorder = reader.GetInt() != 0;
						break;
					case SwitchIds::TrianglePatternToken::MoveResistance:
						record.moveResistance = static_cast<std::uint32_t>(reader.GetInt());
						break;
					case SwitchIds::TrianglePatternToken::Debugname:
						std::strncpy(record.debugName, reader.GetString(), 29);
						break;
					case SwitchIds::TrianglePatternToken::DebugColor:
					{
						const auto red = static_cast<std::uint8_t>(reader.GetInt());
						const auto green = static_cast<std::uint8_t>(reader.GetInt());
						const auto blue = static_cast<std::uint8_t>(reader.GetInt());
						record.debugColor.SetColors(red, green, blue);
						break;
					}
				}
			}
		}
	}

	// Original function: FUN_004159b5; moved from the lifetime module.
	CStaticTrianglePatternTypeDataManager::~CStaticTrianglePatternTypeDataManager()
	{
		NXBasics::Runtime_Free(gStaticTrianglePatternTypes);
		gStaticTrianglePatternTypes = nullptr;
		gStaticTrianglePatternTypeDataManager = nullptr;
	}
}
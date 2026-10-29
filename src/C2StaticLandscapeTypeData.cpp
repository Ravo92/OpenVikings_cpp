#include "C2DataIds.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2StaticLandscapeTypeData.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"

#include <cstring>
#include <new>

namespace NC2Logic
{
	SStaticLandscapeTypeStorage gStaticLandscapeTypeStorage{ };
	SStaticLandscapeTypeData*& gStaticLandscapeTypeData = gStaticLandscapeTypeStorage.records;
	CStaticLandscapeTypeDataManager*& gStaticLandscapeTypeDataManager = gStaticLandscapeTypeStorage.owner;

	namespace
	{
		// Original function: FUN_00415dda, an empty token-list exit callback.
		// The runtime callback interface requires an int; its return is ignored.
		int __cdecl LandscapeTokens_OnExit()
		{

			return 0;
		}

		NXBasics::CTokenList& LandscapeTokens()
		{
			auto& storage = gStaticLandscapeTypeStorage;

			if ((storage.tokenInitFlags & 1) == 0)
			{
				storage.tokenInitFlags |= 1;
				storage.tokens[9] = { nullptr, 0 };
				storage.tokens[0] = { "type", NC2DataIds::LandscapeToken::Type };
				storage.tokens[1] = { "name", SwitchIds::LogicLandscapeToken::Name };
				storage.tokens[2] = { "allowedonland", SwitchIds::LogicLandscapeToken::Allowedonland };
				storage.tokens[3] = { "allowedonwater", SwitchIds::LogicLandscapeToken::Allowedonwater };
				storage.tokens[4] = { "allowedoneverything", SwitchIds::LogicLandscapeToken::Allowedoneverything };
				storage.tokens[5] = { "maximumvalency", SwitchIds::LogicLandscapeToken::MaximumValency };
				storage.tokens[6] = { "playeridallowed", SwitchIds::LogicLandscapeToken::Playeridallowed };
				storage.tokens[7] = { "transition", SwitchIds::LogicLandscapeToken::Transition };
				storage.tokens[8] = { "debugcolor", SwitchIds::LogicLandscapeToken::DebugColor };
			}

			if ((storage.tokenInitFlags & 2) == 0)
			{
				storage.tokenInitFlags |= 2;
				::new (storage.tokenList) NXBasics::CTokenList(storage.tokens);
				NXBasics::Runtime_AtExit(LandscapeTokens_OnExit);
			}

			return *std::launder(reinterpret_cast<NXBasics::CTokenList*>(storage.tokenList));
		}
	}

	// Original function: FUN_004159d0.
	CStaticLandscapeTypeDataManager::CStaticLandscapeTypeDataManager()
	{
		gStaticLandscapeTypeDataManager = this;
		std::memset(this, 0, 1);
		gStaticLandscapeTypeData = nullptr;
		auto* records = static_cast<SStaticLandscapeTypeData*>(NXBasics::Runtime_AllocateWithNewHandler(13728, 1));

		if (records != nullptr)
			for (std::uint32_t index = 0; index < 88; ++index)
			{
				::new (records + index) SStaticLandscapeTypeData;
			}

		gStaticLandscapeTypeData = records;
		// Windows continues unconditionally after the conditional color construction.
		std::memset(records, 0, 13728);
		std::strcpy(records[0].name, "none");
		records[0].debugColor.SetColors(100, 100, 100);

		NXBasics::CIniFileReader reader("data\\logic\\landscapetypes.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
		{

			return;
		}

		for (char* group = reader.GetFirstGroup("landscapetype"); group != nullptr; group = reader.GetNextGroup("landscapetype"))
		{
			if (!reader.SearchCommand("type"))
				continue;

			const auto type = static_cast<std::uint32_t>(reader.GetInt());

			if (type == 0 || type >= 88)
				continue;

			auto& record = gStaticLandscapeTypeData[type];
			reader.SetCurrentLineToGroupStart();

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				switch (LandscapeTokens().FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::LogicLandscapeToken::Name:
						std::strncpy(record.name, reader.GetString(), 29);
						break;
					case SwitchIds::LogicLandscapeToken::Allowedonland:
						record.allowedOnLand = reader.GetInt() != 0;
						break;
					case SwitchIds::LogicLandscapeToken::Allowedonwater:
						record.allowedOnWater = reader.GetInt() != 0;
						break;
					case SwitchIds::LogicLandscapeToken::Allowedoneverything:
						record.allowedOnEverything = reader.GetInt() != 0;
						break;
					case SwitchIds::LogicLandscapeToken::MaximumValency:
						record.maximumValency = static_cast<std::uint32_t>(reader.GetInt());
						break;
					case SwitchIds::LogicLandscapeToken::Playeridallowed:
						record.playerIdAllowed = reader.GetInt() != 0;
						break;
					case SwitchIds::LogicLandscapeToken::Transition:
					{
						if (record.transitionCount == 5)
							break;

						auto& transition = record.transitions[record.transitionCount];
						transition.type = static_cast<std::uint32_t>(reader.GetInt());
						transition.targetLandscapeType = static_cast<std::uint32_t>(reader.GetInt());
						transition.valencyOperation = static_cast<std::uint32_t>(reader.GetInt());
						transition.valencyValue = static_cast<std::uint32_t>(reader.GetInt());
						transition.resultValue = static_cast<std::uint32_t>(reader.GetInt());
						record.transitionMask |= 1u << (transition.type & 31u);
						++record.transitionCount;
						break;
					}
					case SwitchIds::LogicLandscapeToken::DebugColor:
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

	// Original function: FUN_00415ddb; moved from the lifetime module.
	CStaticLandscapeTypeDataManager::~CStaticLandscapeTypeDataManager()
	{
		NXBasics::Runtime_Free(gStaticLandscapeTypeData);
		gStaticLandscapeTypeData = nullptr;
		gStaticLandscapeTypeDataManager = nullptr;
	}
}
#include "C2DataIds.h"
#include "C2Debug.h"
#include "C2EngineLandscapeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2StaticGoodTypeData.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"

#include <cstring>
#include <new>

namespace NC2Logic
{
	SStaticGoodTypeStorage gStaticGoodTypeStorage{ };
	SStaticGoodTypeData*& gStaticGoodTypeData = gStaticGoodTypeStorage.records;
	CStaticGoodTypeDataManager*& gStaticGoodTypeDataManager = gStaticGoodTypeStorage.owner;

	// Original function: FUN_0044CEEF; Windows outlined unchecked static-data reference.
	const SStaticGoodTypeData* StaticGoodTypeData_GetPtr(std::uint32_t good)
	{

		return gStaticGoodTypeData + good;
	}

	namespace
	{
		// Original function: FUN_00415549, an empty token-list exit callback.
		// The runtime callback interface requires an int; its return is ignored.
		int __cdecl GoodTokens_OnExit()
		{

			return 0;
		}

		NXBasics::CTokenList& GoodTokens()
		{
			auto& storage = gStaticGoodTypeStorage;

			if ((storage.tokenInitFlags & 1) == 0)
			{
				storage.tokenInitFlags |= 1;
				storage.tokens[0] = { "type", NC2DataIds::GoodToken::Type };
				storage.tokens[1] = { "name", SwitchIds::GoodToken::Name };
				storage.tokens[2] = { "landscapetype", SwitchIds::GoodToken::LandscapeType };
				storage.tokens[3] = { "weaponType", SwitchIds::GoodToken::WeaponType };
				storage.tokens[4] = { "armorType", SwitchIds::GoodToken::ArmorType };
				storage.tokens[5] = { "isInputGoodFlag", SwitchIds::GoodToken::IsInputGoodFlag };
				storage.tokens[6] = { "isProducedInHouseFlag", SwitchIds::GoodToken::IsProducedInHouseFlag };
				storage.tokens[7] = { "isProducedOnMapFlag", SwitchIds::GoodToken::IsProducedOnMapFlag };
				storage.tokens[8] = { "isBioLandscapeFlag", SwitchIds::GoodToken::IsBioLandscapeFlag };
				storage.tokens[9] = { "landscapeToHarvest", SwitchIds::GoodToken::LandscapeToHarvest };
				storage.tokens[10] = { "landscapeToPickup", SwitchIds::GoodToken::LandscapeToPickup };
				storage.tokens[11] = { "landscapeToStore", SwitchIds::GoodToken::LandscapeToStore };
				storage.tokens[12] = { "atomicForHarvesting", SwitchIds::GoodToken::AtomicForHarvesting };
				storage.tokens[13] = { "atomicForCultivating", SwitchIds::GoodToken::AtomicForCultivating };
				storage.tokens[14] = { "atomicForPlanting", SwitchIds::GoodToken::AtomicForPlanting };
				storage.tokens[15] = { "atomicForProduction", SwitchIds::GoodToken::AtomicForProduction };
				storage.tokens[16] = { "productionInputGoods", SwitchIds::GoodToken::ProductionInputGoods };
				storage.tokens[17] = { nullptr, 0 };
			}

			if ((storage.tokenInitFlags & 2) == 0)
			{
				storage.tokenInitFlags |= 2;
				::new (storage.tokenList) NXBasics::CTokenList(storage.tokens);
				NXBasics::Runtime_AtExit(GoodTokens_OnExit);
			}

			return *std::launder(reinterpret_cast<NXBasics::CTokenList*>(storage.tokenList));
		}
	}

	// Original function: FUN_00414fee.
	CStaticGoodTypeDataManager::CStaticGoodTypeDataManager()
	{
		gStaticGoodTypeDataManager = this;
		std::memset(this, 0, 1);
		gStaticGoodTypeData = nullptr;
		gStaticGoodTypeData = static_cast<SStaticGoodTypeData*>(NXBasics::Runtime_AllocateWithNewHandler(13728, 1));
		std::memset(gStaticGoodTypeData, 0, 13728);
		std::strcpy(gStaticGoodTypeData[0].name, "none");
		NXBasics::CIniFileReader reader("data\\logic\\goodtypes.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
		{

			return;
		}

		for (char* group = reader.GetFirstGroup("goodtype"); group != nullptr; group = reader.GetNextGroup("goodtype"))
		{
			if (!reader.SearchCommand("type"))
				continue;

			const std::int32_t type = reader.GetInt();

			if (type <= 0 || type >= 66)
				continue;

			auto& record = gStaticGoodTypeData[type];
			record.engineLandscapeId = -1;
			record.landscapeType = 0;
			reader.SetCurrentLineToGroupStart();

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				switch (GoodTokens().FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::GoodToken::Name:
						std::strncpy(record.name, reader.GetString(), 29);
						break;
					case SwitchIds::GoodToken::LandscapeType:
						record.landscapeType = reader.GetInt();
						break;
					case SwitchIds::GoodToken::WeaponType:
						if (record.weaponType == 0)
							record.weaponType = reader.GetInt();

						break;
					case SwitchIds::GoodToken::ArmorType:
						if (record.armorType == 0)
							record.armorType = reader.GetInt();

						break;
					case SwitchIds::GoodToken::IsInputGoodFlag:
						record.isInputGood = reader.GetInt() != 0;
						break;
					case SwitchIds::GoodToken::IsProducedInHouseFlag:
						record.isProducedInHouse = reader.GetInt() != 0;
						break;
					case SwitchIds::GoodToken::IsProducedOnMapFlag:
						record.isProducedOnMap = reader.GetInt() != 0;
						break;
					case SwitchIds::GoodToken::IsBioLandscapeFlag:
						record.isBioLandscape = reader.GetInt() != 0;
						break;
					case SwitchIds::GoodToken::LandscapeToHarvest:
						record.landscapeToHarvest = reader.GetInt();
						break;
					case SwitchIds::GoodToken::LandscapeToPickup:
						record.landscapeToPickup = reader.GetInt();
						break;
					case SwitchIds::GoodToken::LandscapeToStore:
						record.landscapeToStore = reader.GetInt();
						break;
					case SwitchIds::GoodToken::AtomicForHarvesting:
						record.atomicForHarvesting = reader.GetInt();
						break;
					case SwitchIds::GoodToken::AtomicForCultivating:
						record.atomicForCultivating = reader.GetInt();
						break;
					case SwitchIds::GoodToken::AtomicForPlanting:
						record.atomicForPlanting = reader.GetInt();
						break;
					case SwitchIds::GoodToken::AtomicForProduction:
						record.atomicForProduction = reader.GetInt();
						break;
					case SwitchIds::GoodToken::ProductionInputGoods:
						while (reader.GetTypeOfNextParameter() == 3)
						{
							if (record.productionInputCount >= 10 || record.distinctProductionInputCount >= 10)
								break;

							const std::int32_t input = reader.GetInt();
							record.productionInputs[record.productionInputCount] = input;
							++record.productionInputCount;
							std::uint32_t index = 0;

							while (index < record.distinctProductionInputCount && record.distinctProductionInputs[index] != input)
								++index;

							++record.productionInputAmounts[index];

							if (index >= record.distinctProductionInputCount)
							{
								record.distinctProductionInputs[index] = input;
								++record.distinctProductionInputCount;
							}
						}

						break;
				}
			}

			if (record.landscapeType == 0)
				C2Debug_WarningBox("a landscape type must be defined for each good type!!!");
		}
	}

	// Original function: FUN_0041554a; moved from the lifetime module.
	CStaticGoodTypeDataManager::~CStaticGoodTypeDataManager()
	{
		NXBasics::Runtime_Free(gStaticGoodTypeData);
		gStaticGoodTypeData = nullptr;
		gStaticGoodTypeDataManager = nullptr;
	}

	// Original function: FUN_00415565; moved from game startup.
	void CStaticGoodTypeDataManager::Misc_GetEngineLandscapeIds()
	{
		// Windows visits records 0..55 although its allocation contains 66 records.

		for (std::uint32_t index = 0; index <= 55; ++index)
		{
			auto& record = gStaticGoodTypeData[index];

			if (record.landscapeType != 0)
				record.engineLandscapeId = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(record.landscapeType, true);
		}
	}
}
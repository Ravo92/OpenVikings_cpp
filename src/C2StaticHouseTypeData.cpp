#include "C2DataIds.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2StaticHouseTypeData.h"
#include "SwitchIdentifiers.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"

#include <cstring>
#include <new>

namespace NC2Logic
{
	SStaticHouseTypeStorage gStaticHouseTypeStorage{ };
	SStaticHouseTypeData*& gStaticHouseTypeData = gStaticHouseTypeStorage.records;
	CStaticHouseTypeDataManager*& gStaticHouseTypeDataManager = gStaticHouseTypeStorage.owner;

	namespace
	{
		// Original function: FUN_00414f1a, an empty token-list exit callback.
		// The runtime callback interface requires an int; its return is ignored.
		int __cdecl HouseTokens_OnExit()
		{
			return 0;
		}

		NXBasics::CTokenList& HouseTokens()
		{
			auto& storage = gStaticHouseTypeStorage;
			if ((storage.tokenInitFlags & 1) == 0)
			{
				storage.tokenInitFlags |= 1;
				storage.tokens[0] = { "logictype", NC2DataIds::HouseToken::LogicType };
				storage.tokens[1] = { "logicmaintype", SwitchIds::HouseTypeToken::Logicmaintype };
				storage.tokens[2] = { "logicworker", SwitchIds::HouseTypeToken::LogicWorker };
				storage.tokens[3] = { "logicstock", SwitchIds::HouseTypeToken::LogicStock };
				storage.tokens[4] = { "logicproduction", SwitchIds::HouseTypeToken::LogicProduction };
				storage.tokens[5] = { "logichomesize", SwitchIds::HouseTypeToken::LogicHomeSize };
				storage.tokens[6] = { "logicschoolsize", SwitchIds::HouseTypeToken::LogicSchoolSize };
				storage.tokens[7] = { "logicignorecontinentsflag", SwitchIds::HouseTypeToken::Logicignorecontinentsflag };
				storage.tokens[8] = { "logicvehicletype", SwitchIds::HouseTypeToken::LogicVehicleType };
				storage.tokens[9] = { "logiccanenabledefencemode", SwitchIds::HouseTypeToken::Logiccanenabledefencemode };
				storage.tokens[10] = { "logicbuildonbiopattern", SwitchIds::HouseTypeToken::Logicbuildonbiopattern };
				storage.tokens[11] = { "debugname", SwitchIds::HouseTypeToken::Debugname };
				storage.tokens[12] = { "debugcolor", SwitchIds::HouseTypeToken::DebugColor };
				storage.tokens[13] = { nullptr, 0 };
			}
			if ((storage.tokenInitFlags & 2) == 0)
			{
				storage.tokenInitFlags |= 2;
				::new (storage.tokenList) NXBasics::CTokenList(storage.tokens);
				NXBasics::Runtime_AtExit(HouseTokens_OnExit);
			}
			return *std::launder(reinterpret_cast<NXBasics::CTokenList*>(storage.tokenList));
		}
	}

	// Original function: FUN_004149b2.
	CStaticHouseTypeDataManager::CStaticHouseTypeDataManager()
	{
		gStaticHouseTypeDataManager = this;
		std::memset(this, 0, 1);
		gStaticHouseTypeData = nullptr;
		auto* records = static_cast<SStaticHouseTypeData*>(NXBasics::Runtime_AllocateWithNewHandler(86460, 1));
		if (records != nullptr)
			for (std::uint32_t index = 0; index < 55; ++index) ::new (records + index) SStaticHouseTypeData;
		gStaticHouseTypeData = records;
		// Windows continues unconditionally after the conditional color construction.
		std::memset(records, 0, 86460);
		std::strcpy(records[0].debugName, "none");
		records[0].debugColor.SetColors(100, 100, 100);
		NXBasics::CIniFileReader reader("data\\logic\\housetypes.ini", nullptr, false, nullptr, nullptr);
		if (!reader.IsLoaded()) return;
		for (char* group = reader.GetFirstGroup("logichousetype"); group != nullptr; group = reader.GetNextGroup("logichousetype"))
		{
			if (!reader.SearchCommand("logictype")) continue;
			const auto type = static_cast<std::uint32_t>(reader.GetInt());
			if (type == 0 || type >= 55) continue;
			auto& record = gStaticHouseTypeData[type];
			std::memset(record.stockIndices, NXBasics::IntegerRange::UInt8MaximumSigned, sizeof(record.stockIndices));
			reader.SetCurrentLineToGroupStart();
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				switch (HouseTokens().FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::HouseTypeToken::Logicmaintype: record.mainType = reader.GetInt(); break;
					case SwitchIds::HouseTypeToken::LogicHomeSize: if (record.mainType == 2) record.homeSize = reader.GetInt(); break;
					case SwitchIds::HouseTypeToken::LogicSchoolSize: if (record.mainType == 4 && (type == 0x26 || type == 0x27)) record.schoolSize = reader.GetInt(); break;
					case SwitchIds::HouseTypeToken::Logicignorecontinentsflag: record.ignoreContinents = reader.GetInt() != 0; break;
					case SwitchIds::HouseTypeToken::LogicVehicleType: record.vehicleType = reader.GetInt(); break;
					case SwitchIds::HouseTypeToken::Logiccanenabledefencemode: record.canEnableDefenceMode = reader.GetInt() != 0; break;
					case SwitchIds::HouseTypeToken::Logicbuildonbiopattern: record.buildOnBioPattern = reader.GetInt() != 0; break;
					case SwitchIds::HouseTypeToken::Debugname: std::strncpy(record.debugName, reader.GetString(), 49); break;
					case SwitchIds::HouseTypeToken::DebugColor:
						{
							const auto red = static_cast<std::uint8_t>(reader.GetInt());
							const auto green = static_cast<std::uint8_t>(reader.GetInt());
							const auto blue = static_cast<std::uint8_t>(reader.GetInt());
							record.debugColor.SetColors(red, green, blue);
							break;
						}
					case SwitchIds::HouseTypeToken::LogicWorker:
						if (record.workerCount != 10)
						{
							const std::int32_t jobType = reader.GetInt();
							NC2Lifetime::Write<std::int32_t>(&record, static_cast<std::uint32_t>(record.workerCount) * NC2Layout::RecordSize::StaticHouseWorkerUnsigned + NC2Layout::StaticHouseTypeOffset::WorkersUnsigned, jobType);
							const std::int32_t secondValue = reader.GetInt();
							NC2Lifetime::Write<std::int32_t>(&record, static_cast<std::uint32_t>(record.workerCount) * NC2Layout::RecordSize::StaticHouseWorkerUnsigned + NC2Layout::StaticHouseTypeOffset::FirstWorkerMaximumUnsigned, secondValue);
							record.workerCount = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(record.workerCount) + 1u);
						}
						break;
					case SwitchIds::HouseTypeToken::LogicStock:
						if (record.stockCount < 66)
						{
							const std::int32_t goodType = reader.GetInt();
							std::int32_t maximumAmount = reader.GetInt();
							const std::uint8_t flag = reader.GetInt() != 0;
							if (goodType < 0 || goodType >= 66) break;
							if (maximumAmount > 1000) maximumAmount = 1000;
							const auto offset = record.stockCount * 12u + 0x64u;
							NC2Lifetime::Write<std::int32_t>(&record, offset, goodType);
							NC2Lifetime::Write<std::int32_t>(&record, offset + NC2Layout::StaticHouseStockOffset::MaximumAmountUnsigned, maximumAmount);
							NC2Lifetime::Write<std::uint8_t>(&record, offset + NC2Layout::StaticHouseStockOffset::IsOutputStockUnsigned, flag);
							// Remaster: only validated good IDs may update the reverse index.
							NC2Lifetime::Write<std::uint32_t>(&record, NC2Layout::StaticHouseTypeOffset::StockIndicesUnsigned + static_cast<std::uint32_t>(goodType) * NC2Layout::RecordSize::HouseTypeIndexUnsigned, record.stockCount);
							++record.stockCount;
						}
						break;
					case SwitchIds::HouseTypeToken::LogicProduction:
						if (record.productionCount < 20)
						{
							const std::int32_t goodType = reader.GetInt();
							if (goodType < 0 || goodType >= 66) break;
							NC2Lifetime::Write<std::int32_t>(&record, NC2Layout::StaticHouseTypeOffset::ProductionGoodsUnsigned + record.productionCount * NC2Layout::RecordSize::HouseTypeIndexUnsigned, goodType);
							NC2Lifetime::Write<std::uint32_t>(&record, NC2Layout::StaticHouseTypeOffset::ProductionIndicesUnsigned + static_cast<std::uint32_t>(goodType) * NC2Layout::RecordSize::HouseTypeIndexUnsigned, record.productionCount);
							++record.productionCount;
						}
						break;
				}
			}
		}
	}

	// Original function: FUN_00414f1b; moved from the lifetime module.
	CStaticHouseTypeDataManager::~CStaticHouseTypeDataManager()
	{
		NXBasics::Runtime_Free(gStaticHouseTypeData);
		gStaticHouseTypeData = nullptr;
		gStaticHouseTypeDataManager = nullptr;
	}

	// Original function: FUN_00414f36.
	std::int32_t CStaticHouseTypeDataManager::GetHouseType(std::int32_t jobType, std::int32_t goodType)
	{
		for (std::uint32_t type = 0; type < 55; ++type)
		{
			const auto& record = gStaticHouseTypeData[type];
			for (std::int32_t worker = 0; worker < record.workerCount; ++worker)
			{
				if (NC2Lifetime::Read<std::int32_t>(&record, NC2Layout::StaticHouseTypeOffset::WorkersUnsigned + static_cast<std::uint32_t>(worker) * NC2Layout::RecordSize::StaticHouseWorkerUnsigned) != jobType) continue;
				for (std::uint32_t slot = 0; slot < record.stockCount; ++slot)
				{
					const auto offset = 0x64u + slot * 12u;
					if (NC2Lifetime::Read<std::uint8_t>(&record, offset + NC2Layout::StaticHouseStockOffset::IsOutputStockUnsigned) == 0 && NC2Lifetime::Read<std::int32_t>(&record, offset) == goodType) return static_cast<std::int32_t>(type);
				}
			}
		}
		return 0;
	}

	// Original function: FUN_00414fa7.
	std::int32_t CStaticHouseTypeDataManager::GetHouseType_NoStock(std::int32_t goodType)
	{
		for (std::uint32_t type = 0; type < 55; ++type)
		{
			const auto& record = gStaticHouseTypeData[type];
			if (record.mainType == 1) continue;
			for (std::uint32_t slot = 0; slot < record.stockCount; ++slot)
			{
				const auto offset = 0x64u + slot * 12u;
				if (NC2Lifetime::Read<std::uint8_t>(&record, offset + NC2Layout::StaticHouseStockOffset::IsOutputStockUnsigned) == 0 && NC2Lifetime::Read<std::int32_t>(&record, offset) == goodType) return static_cast<std::int32_t>(type);
			}
		}
		return 0;
	}
}

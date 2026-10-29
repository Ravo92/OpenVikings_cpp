#include "C2DataIds.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2StaticVehicleTypeData.h"
#include "SwitchIdentifiers.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"

#include <cstring>
#include <new>

namespace NC2Logic
{
	SStaticVehicleTypeStorage gStaticVehicleTypeStorage{ };
	SStaticVehicleTypeData*& gStaticVehicleTypeData = gStaticVehicleTypeStorage.records;
	CStaticVehicleTypeDataManager*& gStaticVehicleTypeDataManager = gStaticVehicleTypeStorage.owner;

	namespace
	{
		// Original function: FUN_00414996, an empty token-list exit callback.
		// The runtime callback interface requires an int; its return is ignored.
		int __cdecl VehicleTokens_OnExit()
		{

			return 0;
		}

		NXBasics::CTokenList& VehicleTokens()
		{
			auto& storage = gStaticVehicleTypeStorage;

			if ((storage.tokenInitFlags & 1) == 0)
			{
				storage.tokenInitFlags |= 1;
				storage.tokens[15] = { nullptr, 0 };
				storage.tokens[0] = { "type", NC2DataIds::VehicleToken::Type };
				storage.tokens[1] = { "name", SwitchIds::VehicleTypeToken::Name };
				storage.tokens[2] = { "logicsize", NC2DataIds::LogicMapChunk::MapSize };
				storage.tokens[3] = { "debugcolor", SwitchIds::VehicleTypeToken::DebugColor };
				storage.tokens[4] = { "debugradius", SwitchIds::VehicleTypeToken::DebugRadius };
				storage.tokens[5] = { "stockslots", SwitchIds::VehicleTypeToken::StockSlots };
				storage.tokens[6] = { "logicgood", SwitchIds::VehicleTypeToken::LogicGood };
				storage.tokens[7] = { "stockvector", SwitchIds::VehicleTypeToken::StockVector };
				storage.tokens[8] = { "passengerslots", SwitchIds::VehicleTypeToken::PassengerSlots };
				storage.tokens[9] = { "logicpassenger", SwitchIds::VehicleTypeToken::LogicPassenger };
				storage.tokens[10] = { "logiccommander", SwitchIds::VehicleTypeToken::LogicCommander };
				storage.tokens[11] = { "passengervector", SwitchIds::VehicleTypeToken::PassengerVector };
				storage.tokens[12] = { "logicdragginganimaltribe", SwitchIds::VehicleTypeToken::LogicDraggingAnimalTribe };
				storage.tokens[13] = { "logictransformvehicleType", NC2DataIds::VehicleToken::LogicTransformVehicleType };
				storage.tokens[14] = { "vehicleslots", SwitchIds::VehicleTypeToken::VehicleSlots };
			}

			if ((storage.tokenInitFlags & 2) == 0)
			{
				storage.tokenInitFlags |= 2;
				::new (storage.tokenList) NXBasics::CTokenList(storage.tokens);
				NXBasics::Runtime_AtExit(VehicleTokens_OnExit);
			}

			return *std::launder(reinterpret_cast<NXBasics::CTokenList*>(storage.tokenList));
		}
	}

	// Original function: FUN_00414462.
	CStaticVehicleTypeDataManager::CStaticVehicleTypeDataManager()
	{
		gStaticVehicleTypeDataManager = this;
		std::memset(this, 0, 1);
		gStaticVehicleTypeData = nullptr;
		auto* records = static_cast<SStaticVehicleTypeData*>(NXBasics::Runtime_AllocateWithNewHandler(3332, 1));

		if (records != nullptr)
			for (std::uint32_t index = 0; index < 7; ++index)
			{
				::new (records + index) SStaticVehicleTypeData;
			}

		gStaticVehicleTypeData = records;
		// Windows continues unconditionally after the conditional color construction.
		std::memset(records, 0, 3332);
		std::strcpy(records[0].name, "none");
		records[0].debugColor.SetColors(100, 100, 100);
		records[0].debugRadius = 10;
		NXBasics::CIniFileReader reader("data\\logic\\vehicletypes.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
		{

			return;
		}

		for (char* group = reader.GetFirstGroup("vehicletype"); group != nullptr; group = reader.GetNextGroup("vehicletype"))
		{
			if (!reader.SearchCommand("type"))
				continue;

			const auto type = static_cast<std::uint32_t>(reader.GetInt());

			if (type == 0 || type >= 7)
				continue;

			auto& record = gStaticVehicleTypeData[type];
			std::memset(record.goodTypeIndices, NXBasics::IntegerRange::UInt8MaximumSigned, sizeof(record.goodTypeIndices));
			reader.SetCurrentLineToGroupStart();

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				switch (VehicleTokens().FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::VehicleTypeToken::Name:
						std::strncpy(record.name, reader.GetString(), 29);
						break;
					case SwitchIds::VehicleTypeToken::LogicSize:
						record.logicSize = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::StockSlots:
						record.stockSlots = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::DebugRadius:
						record.debugRadius = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::PassengerSlots:
						record.passengerSlots = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::LogicCommander:
						record.commanderType = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::VehicleSlots:
						record.vehicleSlots = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::LogicDraggingAnimalTribe:
						record.draggingAnimalTribe = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::LogicTransformVehicleType:
						record.transformVehicleType = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::StockVector:
						record.stockVectorX = reader.GetInt();
						record.stockVectorY = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::PassengerVector:
						record.passengerVectorX = reader.GetInt();
						record.passengerVectorY = reader.GetInt();
						break;
					case SwitchIds::VehicleTypeToken::LogicGood:
					{
						const std::int32_t goodType = reader.GetInt();

						if (goodType < 1 || goodType > 55)
							break;

						record.allowedGoodTypes[goodType] = 1;
						record.goodTypeIndices[goodType] = NXBasics::Runtime_TimeSigned32(record.goodTypeCount);
						++record.goodTypeCount;
						break;
					}
					case SwitchIds::VehicleTypeToken::LogicPassenger:
					{
						const std::int32_t passengerType = reader.GetInt();
						// Passenger permissions are indexed by the 56 human job types.

						if (passengerType < 0 || passengerType >= 56)
							break;

						NC2Lifetime::Write<std::uint8_t>(&record, NC2Layout::StaticVehicleTypeOffset::AllowedPassengerTypesUnsigned + static_cast<std::uint32_t>(passengerType), 1);
						break;
					}
					case SwitchIds::VehicleTypeToken::DebugColor:
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

	// Original function: FUN_00414997; moved from the lifetime module.
	CStaticVehicleTypeDataManager::~CStaticVehicleTypeDataManager()
	{
		NXBasics::Runtime_Free(gStaticVehicleTypeData);
		gStaticVehicleTypeData = nullptr;
		gStaticVehicleTypeDataManager = nullptr;
	}
}
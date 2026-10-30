#include "C2Debug.h"
#include "C2GameControlNetwork.h"
#include "C2GameEngineStartup.h"
#include "C2GameState.h"
#include "C2GuideAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicLifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticHouseTypeData.h"
#include "C2TributeAccess.h"
#include "C2UserMessageAccess.h"
#include "C2UserNetworkCommandAccess.h"
#include "C2VehicleAccess.h"
#include "C2WallRoadConstruction.h"
#include "C2WallRoadPathfinder.h"
#include "InGameGuiManager.h"
#include "NetworkShutdown.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XList.h"
#include "XNumericConstants.h"
#include <cstddef>
#include <cstring>

namespace NC2Logic
{
	// These original callees remain separate reconstruction targets.
	extern void __fastcall House_Misc_Destroy_House(void*, void*);
	extern bool __fastcall House_Construction_UpgradeStart(void*, void*);
	extern bool __fastcall House_Construction_UpgradeStop(void*, void*);
	extern void Logic_EntryPoint_FreeMapMoveable(std::uint32_t);
	extern std::int32_t Logic_EntryPoint_House_Set(std::uint32_t, std::uint32_t, std::uint32_t, const void*, bool, std::uint32_t);
	extern void MapTool_WallGate_Remove(std::uint32_t, std::uint32_t, std::uint32_t);
	extern void MapTool_WallGate_SetIntoWall(std::uint32_t, const void*);
	extern void MapTool_WallGate_SetState(std::uint32_t, const void*, bool);
	extern void Tool_GlobalDefendMode_Start(std::uint32_t);
	extern void Tool_GlobalDefendMode_Stop(std::uint32_t);
	extern void __fastcall GameControl_Player_LeavesGame(void*, void*, std::uint32_t, bool, bool);
	extern void __fastcall GameControl_Player_LeftGameModifyObjects(void*, void*, std::uint32_t, bool, bool);

	// Original function: FUN_0040ad55; Windows passes the four-byte point by value.
	static std::uint32_t MapMigPoint_CopyValue(const void* point)
	{

		return NC2Lifetime::Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
	}

	// Original function: FUN_00410fcc; both jump tables were read from Windows PE bytes.
	void __fastcall UserNetworkCommandManager_Execute(void* manager, void* /*unusedEdx*/, const void* command)
	{
		using namespace NC2Lifetime;

		if (command == nullptr)
			return;

		const auto opcode = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode);

		if (opcode > 1 && opcode < 0x58)
		{
			if (CHumanArray::sTheObjectPtr == nullptr || Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId) >= static_cast<std::uint32_t>(CHumanArray::sTheObjectPtr->mCapacity))
				return;

			auto* human = CHumanArray::mStaticVars + Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId) * CHumanArray::RuntimeElementSize;

			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) != Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision))
				return;

			if (opcode == 2)
			{
				Logic_EntryPoint_FreeMapMoveable(Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId));
			}
			else
			{
				Human_DoAddNewUserCommand(human, command);
			}

			return;
		}

		if (opcode > 0x59 && opcode < 0x62)
		{
			if (CHouseArray::sTheObjectPtr == nullptr || Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId) >= static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mCapacity))
				return;

			auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId) * CHouseArray::RuntimeElementSize;

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) != Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision))
				return;

			switch (opcode)
			{
				case SwitchIds::UserCommand::DestroyHouse:
					House_Misc_Destroy_House(house, nullptr);
					break;
				case SwitchIds::UserCommand::StartHouseUpgrade:
					House_Construction_UpgradeStart(house, nullptr);
					break;
				case SwitchIds::UserCommand::StopHouseUpgrade:
					House_Construction_UpgradeStop(house, nullptr);
					break;
				case SwitchIds::UserCommand::SetHomeQualityEnhancerAllowed:
					House_HomeQualityEnhancer_SetAllowedFlag(house, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2) != 0);
					break;
				case SwitchIds::UserCommand::SetHouseProductionStopped:
					House_Production_SetStoppedFlag(house, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1) != 0);
					break;
				case SwitchIds::UserCommand::ChangeHouseProductionCounter:
				{
					auto delta = Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2);

					if (Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1) == 0)
						delta = 0u - delta;

					House_Production_ChangeCounter(house, static_cast<std::int32_t>(delta));
					break;
				}
				case SwitchIds::UserCommand::SetHouseStockMinimum:
					House_Stock_SetMinimumValue(house, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
					break;
				case SwitchIds::UserCommand::SetHouseDefenceMode:
					House_SetDefenceModeFlag(house, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1) != 0);
					break;
				default:
					House_DoNewUserCommand(house, command);
					break;
			}

			return;
		}

		if (opcode > 0x63 && opcode < 0x74)
		{
			if (CVehicleArray::sTheObjectPtr == nullptr || Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId) >= static_cast<std::uint32_t>(CVehicleArray::sTheObjectPtr->mCapacity))
				return;

			auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId) * CVehicleArray::RuntimeElementSize;

			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId) != Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision))
				return;

			if (opcode == 0x64)
			{
				Logic_EntryPoint_FreeMapMoveable(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId));
			}
			else
			{
				Vehicle_DoAddNewUserCommand(vehicle, command);
			}

			return;
		}

		switch (opcode)
		{
			case SwitchIds::UserCommand::PlaceBuildItem:
			{
				const auto player = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId);
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::HouseBuildCommandOffset::Position));
				const auto item = Read<std::uint32_t>(command, NC2Layout::HouseBuildCommandOffset::SpecialItem);
				const auto itemType = item >> 16;
				bool usedItem = false;

				if (itemType != 0)
				{
					if (!GameControl_SpecialItem_Use(gGameControl, player, itemType, item & NXBasics::IntegerRange::UInt16Maximum))
						return;

					usedItem = true;
				}

				const auto id = Logic_EntryPoint_House_Set(player, Read<std::uint32_t>(command, NC2Layout::HouseBuildCommandOffset::HouseType), Read<std::uint32_t>(command, NC2Layout::HouseBuildCommandOffset::DevelopmentStage), &point, usedItem, 0);

				if (id == -1 || itemType != 4)
					return;

				auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;
				const auto* type = House_GetHouseTypeDataPtr(house);

				for (std::uint32_t stock = 0; stock < type->stockCount; ++stock)
				{
					if (type->stock[stock].isOutputStock == 0)
						continue;

					const auto good = static_cast<std::uint32_t>(type->stock[stock].goodType);
					const auto maximum = House_Stock_GetMaximumAmountByStockId(house, stock);
					House_Stock_ModifyAmount(house, nullptr, good, maximum, false);
				}

				return;
			}
			case SwitchIds::UserCommand::BuildRoad:
			{
				const auto player = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId);
				std::uint32_t start, end;
				MapMigPoint_AssignLong(&start, Read<std::uint32_t>(command, NC2Layout::ConstructionLineCommandOffset::FirstPoint));
				MapMigPoint_AssignLong(&end, Read<std::uint32_t>(command, NC2Layout::ConstructionLineCommandOffset::SecondPoint));
				RoadBuildManager_SetRoadBuildFlag(gRoadBuildManager, nullptr, player, &start);
				CMapPathfinderSpecialWallRoadElement path;
				Tool_RoadBuildManager_GetPossibleVectorList(player, &path, &start, &end);

				for (auto direction = path.GetNextDirection(); direction >= 0; direction = path.GetNextDirection())
				{
					MapMigPoint_MoveCursorUnwrapped(start, static_cast<std::uint32_t>(direction));
					RoadBuildManager_SetRoadBuildFlag(gRoadBuildManager, nullptr, player, &start);
				}

				return;
			}
			case SwitchIds::UserCommand::ClearRoadBuildArea:
			{
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::MapAreaCommandOffset::CenterPoint));
				RoadBuildManager_ClearRoadBuildFlagArea(gRoadBuildManager, nullptr, Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId), &point, Read<std::uint32_t>(command, NC2Layout::MapAreaCommandOffset::Radius));
				return;
			}
			case SwitchIds::UserCommand::BuildWall:
			{
				const auto player = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId);
				std::uint32_t start, end;
				MapMigPoint_AssignLong(&start, Read<std::uint32_t>(command, NC2Layout::ConstructionLineCommandOffset::FirstPoint));
				MapMigPoint_AssignLong(&end, Read<std::uint32_t>(command, NC2Layout::ConstructionLineCommandOffset::SecondPoint));
				WallBuildManager_SetWallBuildFlag(gWallBuildManager, nullptr, player, &start);
				CMapPathfinderSpecialWallRoadElement path;
				MapTool_Wall_Build_GetPossibleVectorList(player, &path, &start, &end);

				for (auto direction = path.GetNextDirection(); direction >= 0; direction = path.GetNextDirection())
				{
					MapMigPoint_MoveCursorUnwrapped(start, static_cast<std::uint32_t>(direction));
					WallBuildManager_SetWallBuildFlag(gWallBuildManager, nullptr, player, &start);
				}

				return;
			}
			case SwitchIds::UserCommand::ClearWallBuildArea:
			{
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::MapAreaCommandOffset::CenterPoint));
				WallBuildManager_ClearWallBuildFlagArea(gWallBuildManager, nullptr, Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId), &point, Read<std::uint32_t>(command, NC2Layout::MapAreaCommandOffset::Radius));
				return;
			}
			case SwitchIds::UserCommand::RemoveWallGate:
			{
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::MapAreaCommandOffset::CenterPoint));
				MapTool_WallGate_Remove(Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId), MapMigPoint_CopyValue(&point), Read<std::uint32_t>(command, NC2Layout::MapAreaCommandOffset::Radius));
				return;
			}
			case SwitchIds::UserCommand::SetWallGate:
			{
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::MapPointCommandOffset::Position));
				MapTool_WallGate_SetIntoWall(Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId), &point);
				return;
			}
			case SwitchIds::UserCommand::SetWallGateState:
			{
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::WallGateStateCommandOffset::Position));
				MapTool_WallGate_SetState(Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId), &point, Read<std::uint32_t>(command, NC2Layout::WallGateStateCommandOffset::Open) != 0);
				return;
			}
			case SwitchIds::UserCommand::InsertGuide:
			{
				std::uint32_t point;
				MapMigPoint_AssignLong(&point, Read<std::uint32_t>(command, NC2Layout::MapPointCommandOffset::Position));
				GuideManager_InsertGuideAtPosition(gGuideManager, nullptr, Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId), &point, 0);
				return;
			}
			case SwitchIds::UserCommand::RemoveGuide:
			{
				const auto id = Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectId);
				const auto* guide = CGuideArray::mStaticVars + id * CGuideArray::RuntimeElementSize;

				if (Read<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(guide, NC2Layout::GuideOffset::UniqueId) == Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision))
					GuideManager_RemoveGuide(gGuideManager, nullptr, id);

				return;
			}
			case SwitchIds::UserCommand::SetDiplomacy:
				GameControl_Diplomacy_SetState(gGameControl, nullptr, Read<std::uint32_t>(command, NC2Layout::DiplomacyCommandOffset::FirstPlayer), Read<std::uint32_t>(command, NC2Layout::DiplomacyCommandOffset::SecondPlayer), Read<std::uint32_t>(command, NC2Layout::DiplomacyCommandOffset::State));
				return;
			case SwitchIds::UserCommand::PayTribute:
				TributeData_Pay(static_cast<std::byte*>(gTributeManager) + Read<std::uint32_t>(command, NC2Layout::TributeCommandOffset::TributeId) * NC2Layout::RecordSize::TributeUnsigned, nullptr);
				return;
			case SwitchIds::UserCommand::EquipShoesForPlayer:
			case SwitchIds::UserCommand::EquipWoodenToolsForPlayer:
			case SwitchIds::UserCommand::EquipIronToolsForPlayer:
			case SwitchIds::UserCommand::EquipMeadForPlayer:
			{
				const auto player = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId);
				CHumanIterator iterator;

				for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
				{
					auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (!Human_IsAgeAdult(human) || !Human_IsSexMale(human))
						continue;

					if (opcode == SwitchIds::UserCommand::EquipShoesForPlayer)
					{
						if (Human_HasEquippedShoes(human))
							continue;
					}
					else if (opcode == SwitchIds::UserCommand::EquipWoodenToolsForPlayer || opcode == SwitchIds::UserCommand::EquipIronToolsForPlayer)
					{
						if (Human_IsJobSoldier(human) || Human_IsJobHero(human) || Human_HasEquippedTools(human))
							continue;
					}
					else if (Human_IsJobHero(human))
					{
						continue;
					}

					const auto parameter = opcode == SwitchIds::UserCommand::EquipMeadForPlayer ? 0x2Bu : 0x1Eu + opcode - 0x81u;
					Human_DoAddNewUserCommand(human, 0x53, player, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), parameter, opcode == SwitchIds::UserCommand::EquipMeadForPlayer ? NXBasics::IntegerRange::UInt32Maximum : 0u);
				}

				return;
			}
			case SwitchIds::UserCommand::GameAssistantInput:
				if (gGameAssistantManager != nullptr)
					GameAssistantManager_UserInput_Save(gGameAssistantManager, Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId), Read<std::int32_t>(command, NC2Layout::GameAssistantCommandOffset::Action), Read<std::int32_t>(command, NC2Layout::GameAssistantCommandOffset::Parameter));

				return;
			case SwitchIds::UserCommand::SetGlobalDefendMode:
				if (Read<std::uint32_t>(command, NC2Layout::GlobalDefendCommandOffset::Enabled) != 0)
				{
					Tool_GlobalDefendMode_Start(Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId));
				}
				else
				{
					Tool_GlobalDefendMode_Stop(Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId));
				}

				return;
			case SwitchIds::UserCommand::SetGameSpeed:
				GameTime_SetSpeed(static_cast<double>(Read<std::uint32_t>(command, NC2Layout::GameSpeedCommandOffset::Speed)));
				return;
			case SwitchIds::UserCommand::PlayerLeavesGame:
				GameControl_Player_LeavesGame(gGameControl, nullptr, Read<std::uint32_t>(command, NC2Layout::PlayerDepartureCommandOffset::PlayerId), Read<std::uint32_t>(command, NC2Layout::PlayerDepartureCommandOffset::OriginalFlag) != 0, Read<std::uint32_t>(command, NC2Layout::PlayerDepartureCommandOffset::RemoveObjects) != 0);
				return;
			case SwitchIds::UserCommand::ModifyObjectsAfterPlayerLeaves:
				GameControl_Player_LeftGameModifyObjects(gGameControl, nullptr, Read<std::uint32_t>(command, NC2Layout::PlayerDepartureCommandOffset::PlayerId), Read<std::uint32_t>(command, NC2Layout::PlayerDepartureCommandOffset::OriginalFlag) != 0, Read<std::uint32_t>(command, NC2Layout::PlayerDepartureCommandOffset::RemoveObjects) != 0);
				return;
			case SwitchIds::UserCommand::SubmitGameChecksum:
			{
				const auto player = GameTime_Network_GetPlayerByNetworkId(Read<std::uint32_t>(command, NC2Layout::GameChecksumCommandOffset::NetworkPlayerId));

				if (player == -1)
					return;

				const auto baseOffset = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::NetworkChecksumPlayerUnsigned;
				auto selected = NXBasics::IntegerRange::UInt32Maximum;

				for (std::uint32_t sample = 0; sample < NC2Runtime::NetworkCommandLimits::SamplesPerPlayer; ++sample)
				{
					const auto offset = baseOffset + sample * NC2Layout::RecordSize::NetworkChecksumSampleUnsigned;

					if (Read<std::uint32_t>(manager, offset) == 0 || Read<std::uint32_t>(manager, offset + NC2Layout::NetworkChecksumSampleOffset::Tick) != Read<std::uint32_t>(command, NC2Layout::GameChecksumCommandOffset::Tick))
						continue;

					selected = sample;
					char message[300];
					NXBasics::Runtime_PrintString(message, ">> NET: CHECKSUM RECEIVED TWICE! (%d:%d)", player, Read<std::uint32_t>(command, NC2Layout::GameChecksumCommandOffset::Tick) / NC2Runtime::NetworkCommandLimits::TicksPerSecondUnsigned);
					NC2LogicExternals::External_Misc_SendToUserStringMessage(message, false);
					break;
				}

				if (selected == NXBasics::IntegerRange::UInt32Maximum)
				{
					auto oldestTick = NXBasics::IntegerRange::UInt32Maximum;

					for (std::uint32_t sample = 0; sample < NC2Runtime::NetworkCommandLimits::SamplesPerPlayer; ++sample)
					{
						const auto tick = Read<std::uint32_t>(manager, baseOffset + sample * NC2Layout::RecordSize::NetworkChecksumSampleUnsigned + NC2Layout::NetworkChecksumSampleOffset::Tick);

						if (tick > oldestTick)
							continue;

						oldestTick = tick;
						selected = sample;
					}

					if (selected == NXBasics::IntegerRange::UInt32Maximum)
						return;
				}

				const auto offset = baseOffset + selected * NC2Layout::RecordSize::NetworkChecksumSampleUnsigned;
				Write(manager, offset, Read<std::uint32_t>(command, NC2Layout::GameChecksumCommandOffset::Checksum));
				Write(manager, offset + NC2Layout::NetworkChecksumSampleOffset::Tick, Read<std::uint32_t>(command, NC2Layout::GameChecksumCommandOffset::Tick));
				Write(manager, offset + NC2Layout::NetworkChecksumSampleOffset::NetworkPlayerId, Read<std::uint32_t>(command, NC2Layout::GameChecksumCommandOffset::NetworkPlayerId));
				return;
			}
			case SwitchIds::UserCommand::SetNetworkFrequency:
			{
				auto frequency = Read<std::int32_t>(command, NC2Layout::NetworkFrequencyCommandOffset::Frequency);

				if (frequency < NC2Runtime::NetworkCommandLimits::MinimumFrequency)
					frequency = NC2Runtime::NetworkCommandLimits::MinimumFrequency;

				if (frequency > NC2Runtime::NetworkCommandLimits::MaximumFrequency)
					frequency = NC2Runtime::NetworkCommandLimits::MaximumFrequency;

				gGameTime.networkFrequency = static_cast<std::uint32_t>(frequency);
				return;
			}
			default:

				return;
		}
	}

	// Original function: FUN_00411829; decoded directly from Windows PE bytes.
	void __fastcall UserNetworkCommandManager_SendCommands(void* manager, void* /*unusedEdx*/)
	{
		if (gGameStaticVars.networkGame == 0)
			return;

		auto* system = reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(manager) + NC2Layout::UserNetworkCommandManagerOffset::SystemQueue);
		auto* deferred = reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(manager) + NC2Layout::UserNetworkCommandManagerOffset::DeferredQueue);
		auto* ordinary = reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(manager) + NC2Layout::UserNetworkCommandManagerOffset::OrdinaryQueue);
		auto count = ordinary->GetCount() + deferred->GetCount() + system->GetCount();

		if (count > NC2Runtime::NetworkCommandLimits::PacketCommandsUnsigned)
			count = NC2Runtime::NetworkCommandLimits::PacketCommandsUnsigned;

		const auto size = count * NC2Layout::RecordSize::UserNetworkCommand + NC2Layout::RecordSize::NetworkCommandPacketHeaderUnsigned;
		auto* packet = static_cast<std::byte*>(NXBasics::Runtime_Allocate(size));

		if (packet == nullptr)
		{

			return;
		}

		std::memcpy(packet, &count, NC2Layout::RecordSize::NetworkCommandPacketHeader);
		auto* destination = packet + NC2Layout::NetworkCommandPacketOffset::Commands;

		for (std::uint32_t index = 0; index < count; ++index)
		{
			auto* list = ordinary;

			if (deferred->GetCount() != 0)
				list = deferred;

			if (system->GetCount() != 0)
				list = system;

			void* command = list->l_Base_GetStartElement();
			list->l_Base_RemoveElement(command);
			std::memcpy(destination, command, NC2Layout::RecordSize::UserNetworkCommandSigned);
			destination += NC2Layout::RecordSize::UserNetworkCommandSigned;
			NXBasics::Runtime_Free(command);
		}

		NNetwork::SendDynamicNetworkCommands(&gDynamicNetworkData, packet, size);
		NXBasics::Runtime_Free(packet);
	}

	// Original function: FUN_004118f6; command counts in received slots are unchecked.
	bool __fastcall UserNetworkCommandManager_ReceiveCommands(void* manager, void* /*unusedEdx*/)
	{
		using namespace NC2Lifetime;

		if (gGameStaticVars.networkGame == 0)
			return true;

		NNetwork::UpdateDynamicNetwork(&gDynamicNetworkData, false);
		NNetwork::SDynamicSlot* slots = nullptr;
		std::uint32_t slotCount = 0;

		if (!NNetwork::TakeDynamicNetworkCommands(&gDynamicNetworkData, &slots, &slotCount))
			return false;

		for (std::uint32_t slot = 0; slot < slotCount; ++slot)
		{
			if (slots[slot].playerId == NXBasics::IntegerRange::UInt32Maximum)
				continue;

			if (slots[slot].payloadByteCount < NC2Layout::RecordSize::NetworkCommandPacketHeaderUnsigned)
			{
				NXBasics::Runtime_Free(slots);
				return false;
			}

			const auto count = Read<std::uint32_t>(slots[slot].data, NC2Layout::NetworkCommandPacketOffset::Count);

			if (count > (slots[slot].payloadByteCount - NC2Layout::RecordSize::NetworkCommandPacketHeaderUnsigned) / NC2Layout::RecordSize::UserNetworkCommand)
			{
				NXBasics::Runtime_Free(slots);
				return false;
			}

			const auto* command = slots[slot].data + NC2Layout::NetworkCommandPacketOffset::Commands;

			for (std::uint32_t index = 0; index < count; ++index, command += NC2Layout::RecordSize::UserNetworkCommandSigned)
			{
				// Frequency is global deterministic state; only the session host may select it.

				if (Read<std::uint8_t>(command, 0) == SwitchIds::UserCommand::SetNetworkFrequency && slots[slot].playerId != NNetwork::Network_GetHostPlayerId())
					continue;

				UserNetworkCommandManager_Execute(manager, nullptr, command);
			}
		}

		if (slots != nullptr)
			NXBasics::Runtime_Free(slots);

		std::uint32_t players[NC2Runtime::NetworkCommandLimits::Players];
		std::uint32_t playerCount = 0;

		for (std::uint32_t player = 0; player < NC2Runtime::NetworkCommandLimits::Players; ++player)
		{
			if (GameControl_Player_IsNetworkPlayer(gGameControl, static_cast<std::int32_t>(player)))
				players[playerCount++] = player;
		}

		if (playerCount == 0)
			return true;

		for (std::uint32_t sample = 0; sample < NC2Runtime::NetworkCommandLimits::SamplesPerPlayer; ++sample)
		{
			const auto baseOffset = players[0] * NC2Layout::RecordSize::NetworkChecksumPlayerUnsigned + sample * NC2Layout::RecordSize::NetworkChecksumSampleUnsigned;
			const auto checksum = Read<std::uint32_t>(manager, baseOffset);
			const auto tick = Read<std::uint32_t>(manager, baseOffset + NC2Layout::NetworkChecksumSampleOffset::Tick);

			if (checksum == 0)
				continue;

			std::uint32_t matched = 1;
			std::uint32_t selectedTick = NXBasics::IntegerRange::UInt32Maximum;

			for (std::uint32_t player = 1; player < playerCount; ++player)
			{
				for (std::uint32_t otherSample = 0; otherSample < NC2Runtime::NetworkCommandLimits::SamplesPerPlayer; ++otherSample)
				{
					const auto offset = players[player] * NC2Layout::RecordSize::NetworkChecksumPlayerUnsigned + otherSample * NC2Layout::RecordSize::NetworkChecksumSampleUnsigned;
					const auto otherChecksum = Read<std::uint32_t>(manager, offset);
					const auto otherTick = Read<std::uint32_t>(manager, offset + NC2Layout::NetworkChecksumSampleOffset::Tick);

					if (otherChecksum == 0 || otherTick != tick)
						continue;

					if (otherChecksum == checksum)
					{
						++matched;
					}
					else
					{
						NNetwork::GetDynamicNetworkLocalPlayerId(&gDynamicNetworkData);
						C2Debug_WarningBoxV("Checksums do not match!\n\nReceived checksums:\n\nSlot %d: %8.8x - Gametick %d\nSlot %d: %8.8x - Gametick %d", players[0], checksum, tick, players[player], otherChecksum, otherTick);
						selectedTick = tick;
					}
				}
			}
			// Duplicate records count independently, as in the Windows loop.

			if (matched == playerCount)
			{
				selectedTick = tick;
				Write(manager, NC2Layout::UserNetworkCommandManagerOffset::LastVerifiedTick, tick);
			}

			if (selectedTick != NXBasics::IntegerRange::UInt32Maximum)
			{
				for (std::uint32_t player = 0; player < playerCount; ++player)
				{
					for (std::uint32_t otherSample = 0; otherSample < NC2Runtime::NetworkCommandLimits::SamplesPerPlayer; ++otherSample)
					{
						const auto offset = players[player] * NC2Layout::RecordSize::NetworkChecksumPlayerUnsigned + otherSample * NC2Layout::RecordSize::NetworkChecksumSampleUnsigned;

						if (Read<std::uint32_t>(manager, offset) != 0 && Read<std::uint32_t>(manager, offset + NC2Layout::NetworkChecksumSampleOffset::Tick) == selectedTick)
							std::memset(static_cast<std::byte*>(manager) + offset, 0, NC2Layout::RecordSize::NetworkChecksumSample);
					}
				}
			}
		}

		const auto currentTick = gGameTime.tick;

		if (currentTick > NC2Runtime::NetworkCommandLimits::ChecksumExpiryTicks)
		{
			for (std::uint32_t player = 0; player < playerCount; ++player)
			{
				for (std::uint32_t sample = 0; sample < NC2Runtime::NetworkCommandLimits::SamplesPerPlayer; ++sample)
				{
					const auto offset = players[player] * NC2Layout::RecordSize::NetworkChecksumPlayerUnsigned + sample * NC2Layout::RecordSize::NetworkChecksumSampleUnsigned;
					const auto tick = Read<std::uint32_t>(manager, offset + NC2Layout::NetworkChecksumSampleOffset::Tick);

					if (Read<std::uint32_t>(manager, offset) == 0 || tick >= currentTick - NC2Runtime::NetworkCommandLimits::ChecksumExpiryTicksUnsigned)
						continue;

					char message[300];
					NXBasics::Runtime_PrintString(message, ">> NET: TOO OLD CHECKSUM! (%d-%d)", players[player], tick / NC2Runtime::NetworkCommandLimits::TicksPerSecondUnsigned);
					NC2LogicExternals::External_Misc_SendToUserStringMessage(message, false);
					std::memset(static_cast<std::byte*>(manager) + offset, 0, NC2Layout::RecordSize::NetworkChecksumSample);
				}
			}
		}

		return true;
	}

	// Original function: FUN_00410f00; Windows receives a manager in ECX.
	void __fastcall UserNetworkCommandManager_AddCommand(void* manager, void* /*unusedEdx*/, const void* command, bool systemCommand, bool deferred)
	{
		if (*static_cast<const std::uint8_t*>(command) == 0)
			return;

		if (gGameStaticVars.networkGame == 0)
		{
			UserNetworkCommandManager_Execute(manager, nullptr, command);
			return;
		}

		if (gGameStaticVars.networkWait != 0 && !systemCommand)
			return;

		const auto offset = systemCommand ? NC2Layout::UserNetworkCommandManagerOffset::SystemQueueUnsigned : deferred ? NC2Layout::UserNetworkCommandManagerOffset::DeferredQueueUnsigned
		                                                                                                               : NC2Layout::UserNetworkCommandManagerOffset::OrdinaryQueueUnsigned;
		auto* list = reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(manager) + offset);

		if (!systemCommand && !deferred)
		{
			NXBasics::CListBase::Iterator iterator(*list);

			for (const void* queued = iterator.First(); queued != nullptr; queued = iterator.Next())
			{
				if (std::memcmp(queued, command, NC2Layout::RecordSize::UserNetworkCommandSigned) == 0)
					return;
			}
		}

		void* queued = NXBasics::Runtime_Allocate(NC2Layout::RecordSize::UserNetworkCommandSigned);

		if (queued != nullptr)
			std::memset(queued, 0, NC2Layout::RecordSize::UserNetworkCommandSigned);

		if (queued == nullptr)
		{

			return;
		}

		std::memcpy(queued, command, NC2Layout::RecordSize::UserNetworkCommandSigned);
		list->InsertAtEnd(queued);
	}
}
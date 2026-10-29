#include "C2GameMapStorage.h"
#include "C2ChecksumTools.h"
#include "C2GameChecksum.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapSectorAttachment.h"
#include "C2MapWalkSectorManager.h"
#include "C2MultiPlayerGoalAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2WallRoadConstruction.h"
#include "SwitchIdentifiers.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_004081cc.
	std::uint32_t __fastcall AIManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		std::uint32_t checksum = 1234567u;
		for (std::uint32_t player = 0; player < NC2Runtime::AIManagerLimits::Players; ++player)
		{
			if (Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer) == 0) continue;
			Tool_Checksum_CalculateAndCombine(&checksum, Read<const void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned), NC2Layout::ObjectSize::AISinglePlayerHandler);
			Tool_Checksum_Combine(&checksum, AIHighLevelHandler_Checksum_Calculate(Read<const void*>(manager, NC2Layout::AIManagerOffset::HighLevelHandlersUnsigned + player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned), nullptr));
		}
		return checksum;
	}

	// Original function: FUN_00409b39.
	std::uint32_t __fastcall GameControl_Checksum_Calculate(void* control, void* /*unusedEdx*/)
	{
		std::byte saved[20][104];
		for (std::uint32_t player = 0; player < 20; ++player)
		{
			auto* statistics = static_cast<std::byte*>(control) + player * NC2Layout::RecordSize::GameControlPlayer + 0x49ACu;
			std::memcpy(saved[player], statistics, 104);
			std::memset(statistics, 0, 104);
		}
		const auto checksum = Tool_Checksum_Calculate(control, 0x5D204);
		for (std::uint32_t player = 0; player < 20; ++player) std::memcpy(static_cast<std::byte*>(control) + player * NC2Layout::RecordSize::GameControlPlayer + 0x49ACu, saved[player], 104);
		return checksum;
	}

	// Original function: FUN_004171d9.
	std::uint32_t __fastcall MissionOnscreenInfoManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		return Tool_Checksum_Calculate(manager, 0xFA0);
	}
	// Original function: FUN_004176ba.
	std::uint32_t __fastcall GameAssistantManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		return Tool_Checksum_Calculate(manager, 0x460);
	}
	// Original function: FUN_0041980a.
	std::uint32_t __fastcall HappinessManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		return Tool_Checksum_Calculate(manager, 0xAA0);
	}

	// Original function: FUN_00419c8f.
	std::uint32_t __fastcall EmbeddedSavegameManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		auto checksum = Tool_Checksum_Calculate(manager, 4);
		const auto bytes = Read<std::uint32_t>(manager, NC2Layout::EmbeddedSavegameOffset::Size);
		if (bytes != 0) Tool_Checksum_Combine(&checksum, Tool_Checksum_Calculate(Read<const void*>(manager, NC2Layout::EmbeddedSavegameOffset::Data), bytes));
		return checksum;
	}

	// Original function: FUN_00419ec7.
	std::uint32_t __fastcall TributeManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		return Tool_Checksum_Calculate(manager, 0x960);
	}

	// Original function: FUN_0041ac6d.
	std::uint32_t __fastcall PlayerStatisticManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		std::uint32_t checksum = 0x76543210u;
		Tool_Checksum_CalculateAndCombine(&checksum, manager, 8);
		for (std::uint32_t player = 0; player < 20; ++player)
		{
			const auto* history = Read<const void*>(manager, NC2Layout::PlayerStatisticManagerOffset::HistoryByPlayerUnsigned + player * NC2Layout::RecordSize::StatisticPlayerPointerUnsigned);
			if (history != nullptr) Tool_Checksum_CalculateAndCombine(&checksum, history, 0x2DB40);
			else
			{
				const auto* current = Read<const void*>(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentByPlayerUnsigned + player * NC2Layout::RecordSize::StatisticPlayerPointerUnsigned);
				if (current != nullptr) Tool_Checksum_CalculateAndCombine(&checksum, current, 0x138);
				else Tool_Checksum_Combine(&checksum, 0xFF000000u);
			}
		}
		return checksum;
	}

	// Original function: FUN_0041b037.
	std::uint32_t __fastcall GuideManager_Checksum_Calculate(void* manager, void* /*unusedEdx*/)
	{
		auto checksum = Array_Checksum_Calculate(CGuideArray::sTheObjectPtr, nullptr);
		Tool_Checksum_CalculateAndCombine(&checksum, manager, 4);
		return checksum;
	}

	// Original function: FUN_0041cef3; shared Windows tail-jump entry, not separate array-specific bodies.
	std::uint32_t __fastcall Array_Checksum_Calculate(void* array, void* /*unusedEdx*/)
	{
		return static_cast<CArrayHandler*>(array)->Checksum_Calculate();
	}
	// Original function: FUN_0041d08d.
	std::uint32_t __fastcall MissionManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		return Tool_Checksum_Calculate(manager, NC2Layout::ObjectSize::MissionManager);
	}

	// Original function: FUN_00424d9d; the native map object argument is unused.
	void __fastcall GameMap_Checksum_Calculate(void* /*map*/, void* /*unusedEdx*/, SGameChecksum* output)
	{
		output->components[4] = Tool_Checksum_Calculate(gGameMapStaticStorage, 0x30);
		output->components[5] = Tool_Checksum_Calculate(Read<const void*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointStorage), Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount) * 3u);
		output->components[6] = Tool_Checksum_Calculate(Read<const void*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage), Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 14u);
		output->components[7] = Tool_Checksum_Calculate(gGameMapStaticStorage + NC2Layout::GameMapOffset::ContinentCount, 0xFA4);
	}

	// Original function: FUN_00428736.
	std::uint32_t Game_Checksum_Calculate()
	{
		SGameChecksum output;
		Game_Checksum_Calculate(&output);
		return output.checksum;
	}

	// Original function: FUN_00428754.
	void Game_Checksum_Calculate(SGameChecksum* output)
	{
		std::memset(output, 0, sizeof(*output));
		auto* part = output->components;
		part[0] = gGameTime.tick;
		part[1] = Tool_Checksum_Calculate(&gGameRandomState, 8);
		part[2] = gGameStaticVars.uniqueIdCounter;
		part[3] = GameControl_Checksum_Calculate(gGameControl, nullptr);
		GameMap_Checksum_Calculate(gGameMap, nullptr, output);
		part[8] = Array_Checksum_Calculate(CHumanArray::sTheObjectPtr, nullptr);
		part[9] = Array_Checksum_Calculate(CAnimalArray::sTheObjectPtr, nullptr);
		part[10] = Array_Checksum_Calculate(CVehicleArray::sTheObjectPtr, nullptr);
		part[11] = Array_Checksum_Calculate(gMapMoveableArrayStorage.manager, nullptr);
		part[12] = Array_Checksum_Calculate(CHouseArray::sTheObjectPtr, nullptr);
		part[13] = GuideManager_Checksum_Calculate(gGuideManager, nullptr);
		part[14] = Array_Checksum_Calculate(gMapPathfinderManager, nullptr);
		part[15] = Array_Checksum_Calculate(gMapLandscapeAutoTransitionManager, nullptr);
		part[16] = Array_Checksum_Calculate(gMapBioRegrowManager, nullptr);
		part[17] = Array_Checksum_Calculate(gDelayedActionManager, nullptr);
		part[18] = MapWalkSectorManager_Checksum_Calculate();
		part[19] = GameMapSectorAttachmentManager_Checksum_Calculate(CGameMapSectorAttachmentManager::sTheObjectPtr, nullptr);
		part[20] = MapFishManager_Checksum_Calculate(gMapFishManager, nullptr);
		part[21] = WallBuildManager_Checksum_Calculate(gWallBuildManager);
		part[22] = RoadBuildManager_Checksum_Calculate(gRoadBuildManager);
		part[23] = PlayerStatisticManager_Checksum_Calculate(gPlayerStatisticManager, nullptr);
		part[24] = MissionManager_Checksum_Calculate(gMissionManager, nullptr);
		part[25] = TributeManager_Checksum_Calculate(gTributeManager, nullptr);
		part[26] = EmbeddedSavegameManager_Checksum_Calculate(gEmbeddedSavegameManager, nullptr);
		part[27] = AIManager_Checksum_Calculate(gAIManager, nullptr);
		part[28] = HappinessManager_Checksum_Calculate(gHappinessManager, nullptr);
		part[29] = MultiPlayerGoalManager_Checksum_Calculate(gMultiPlayerGoalManager);
		part[30] = GameAssistantManager_Checksum_Calculate(gGameAssistantManager, nullptr);
		part[31] = MissionOnscreenInfoManager_Checksum_Calculate(gMissionOnscreenInfoManager, nullptr);
		output->checksum = Tool_Checksum_Calculate(part, 0xA8);
		if (output->checksum == 0) output->checksum = 1;
	}

	// Original function: FUN_00428b2f.
	std::uint32_t __fastcall AIHighLevelHandler_Checksum_Calculate(const void* handler, void* /*unusedEdx*/)
	{
		auto checksum = Tool_Checksum_Calculate(handler, NC2Layout::AIHighLevelOffset::GuideManager);
		Tool_Checksum_Combine(&checksum, AIGuideManager_Checksum_Calculate(static_cast<const std::byte*>(handler) + NC2Layout::AIHighLevelOffset::GuideManager, nullptr));
		return checksum;
	}
	// Original function: FUN_004295b7.
	std::uint32_t __fastcall AIGuideManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		return Tool_Checksum_Calculate(manager, NC2Runtime::AIGuideLimits::SerializedBytes);
	}
	// Original function: FUN_0044e7af.
	std::uint32_t __fastcall MapFishManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		return Tool_Checksum_Calculate(manager, 0x1774);
	}

	// Original function: FUN_0044ed88.
	std::uint32_t __fastcall GameMapSectorAttachmentManager_Checksum_Calculate(const void* manager, void* /*unusedEdx*/)
	{
		auto checksum = Tool_Checksum_Calculate(manager, 8);
		const auto count = Read<std::uint32_t>(manager, NC2Layout::SectorAttachmentManagerOffset::GridHeight) * Read<std::uint32_t>(manager, NC2Layout::SectorAttachmentManagerOffset::GridWidth);
		Tool_Checksum_CalculateAndCombine(&checksum, Read<const void*>(manager, NC2Layout::SectorAttachmentManagerOffset::SectorData), count * 0x540u);
		Tool_Checksum_CalculateAndCombine(&checksum, Read<const void*>(manager, NC2Layout::SectorAttachmentManagerOffset::SectorAuxiliaryData), count * 0xFCu);
		return checksum;
	}

	// Original function: FUN_004509f4.
	std::uint32_t MapWalkSectorManager_Checksum_Calculate()
	{
		const auto& data = CMapWalkSectorManager::mStaticVars;
		auto checksum = Tool_Checksum_Calculate(&data, 8);
		const auto bytes = static_cast<std::uint32_t>(data.gridWidth) * static_cast<std::uint32_t>(data.gridHeight) * 0x34u;
		Tool_Checksum_CalculateAndCombine(&checksum, data.landSectors, bytes);
		Tool_Checksum_CalculateAndCombine(&checksum, data.waterSectors, bytes);
		return checksum;
	}
}

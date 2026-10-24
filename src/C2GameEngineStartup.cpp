#include "RemasterAILog.h"
#include "GameSetupStartup.h"
#include "ApplicationGlobals.h"
#include "CivilizationBuildings.h"
#include "C2CallbackManager.h"
#include "C2CampaignStaticDataManager.h"
#include "C2CleanMapsManager.h"
#include "C2E2AmbientSoundManagerAccess.h"
#include "C2E2AnimalSoundManager.h"
#include "C2E2LevelOfDetailManager.h"
#include "C2E2StaticDataAnimalSounds.h"
#include "C2E2StaticDataHouses.h"
#include "C2E2StaticDataHumanMisc.h"
#include "C2E2StaticDataMapMoveables.h"
#include "C2E2StaticDataSoundFX.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDisplayManager.h"
#include "C2EngineLandscapeAccess.h"
#include "C2EngineLifetimeAccess.h"
#include "C2EngineMapIO.h"
#include "C2EngineMapTerrainAccess.h"
#include "C2GameControlStartup.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapIO.h"
#include "C2GameTime.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2PlayerStatisticAccess.h"
#include "C2RuntimeConstants.h"
#include "C2SavegameIO.h"
#include "C2SavegamesManager.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2StaticAtomicAnimationData.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticHouseTypeData.h"
#include "C2StaticLandscapeTypeData.h"
#include "C2StaticStringsManager.h"
#include "C2StaticTrianglePatternTypeData.h"
#include "C2StaticTribeTypeData.h"
#include "C2StaticVehicleTypeData.h"
#include "MasterControlProgram.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"

#include <cstring>
#include <Windows.h>

namespace
{
	template<class Constructor, class... Arguments> void ConstructOriginal(std::uint32_t bytes, Constructor constructor, Arguments... arguments)
	{
		if (void* allocation = NXBasics::Runtime_AllocateWithNewHandler(bytes, 1)) constructor(allocation, nullptr, arguments...);
	}

	template<class Object> void ConstructNative(std::uint32_t bytes)
	{
		if (void* allocation = NXBasics::Runtime_AllocateWithNewHandler(bytes, 1)) ::new (allocation) Object();
	}
}

namespace NC2Logic
{
	using namespace NC2Lifetime;
	// Original function: FUN_0040743A.
	void Game_WorkOnStartUpData()
	{
		const auto type = Read<std::uint32_t>(&gGameControlStartupData, NC2Layout::GameControlStartupOffset::Mode);
		if (type == SwitchIds::GameStartupMode::SingleMap || type == SwitchIds::GameStartupMode::MultiplayerMap || type == SwitchIds::GameStartupMode::SkirmishMap)
		{
			const auto guid = Read<SCleanMapGuid>(&gGameControlStartupData, type == SwitchIds::GameStartupMode::SingleMap ? NC2Layout::GameControlStartupOffset::SingleMapGuidUnsigned : NC2Layout::GameControlStartupOffset::MultiplayerMapGuidUnsigned);
			const auto* map = gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(guid);
			if (map == nullptr) return;
			NRemaster::GameSetup::Startup::CStartupScope setupScope(type == SwitchIds::GameStartupMode::MultiplayerMap || type == SwitchIds::GameStartupMode::SkirmishMap ? &guid : nullptr);
			if (map->archiveFilename[0] == '\0') GameIO_Cleanmap_Load(map->mapRoot, true, true, type == SwitchIds::GameStartupMode::MultiplayerMap);
			else GameIO_Cleanmap_LoadX(nullptr, map->archiveFilename, true, true, type == SwitchIds::GameStartupMode::MultiplayerMap);
		}
		else if (type == SwitchIds::GameStartupMode::Savegame)
		{
			const auto* savegame = gSavegamesManager->GetSavegameInfoPtrByUniqueId(Read<std::uint32_t>(&gGameControlStartupData, NC2Layout::GameControlStartupOffset::SavegameId));
			if (savegame != nullptr) GameIO_Savegame_Load(savegame->filename, false);
		}
		else if (type == SwitchIds::GameStartupMode::MultiplayerContinue) GameIO_Savegame_Load("sys_continue", true);
	}

	// Unreconstructed original subconstructors. No replacement bodies are supplied.
	extern void* __fastcall GameMap_Construct(void*, void* unusedEdx, std::int16_t, std::int16_t);
	extern void* __fastcall MissionManager_Construct(void*, void* unusedEdx);
	extern void* __fastcall AIManager_Construct(void*, void* unusedEdx);


	// Original function: FUN_00411ba6
	static void* __fastcall GameMapModifyManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gGameMapModifyManager = manager;
		gGameMapModifyNestingDepth = 0;
		return manager;
	}

	// Original function: FUN_00406b70
	void* Game_Construct(void* game)
	{
		RemasterAILog::ResetWorld("game constructed; fresh map or save world");
		gGame = game;
		std::memset(&gGameStaticVars, 0, sizeof(gGameStaticVars));
		// Fixed Windows seed has thirteen set bits; neither adjustment branch runs.
		gGameRandomState.seed = NC2Runtime::RandomSeed::Game;
		gGameRandomState.currentState = NC2Runtime::RandomSeed::Game;
		gGameStaticVars.uniqueIdCounter = 1;
		ConstructNative<CStaticStringsManager>(1);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CCampaignStaticDataManager>(0x218);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CCleanMapsManager>(0x30);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CSavegamesManager>(0x20);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CStaticLandscapeTypeDataManager>(1);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CStaticTrianglePatternTypeDataManager>(1);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CStaticGoodTypeDataManager>(1);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CStaticHouseTypeDataManager>(1);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CStaticVehicleTypeDataManager>(1);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CStaticAtomicAnimationDataManager>(1);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructNative<CStaticTribeTypeDataManager>(1);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, StaticJobTypeDataManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, StaticWeaponTypeDataManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, StaticArmorTypeDataManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, StaticAnimalTypeDataManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, StaticDependenciesManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, StaticHumanJobExperienceTypeDataManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		static_assert(sizeof(NC2Logic::CCallbackManager) == 416);
		ConstructNative<NC2Logic::CCallbackManager>(0x1A0);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, GameMapModifyManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(NC2Layout::ObjectSize::UserNetworkCommandManager, UserNetworkCommandManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		return game;
	}

	// Original function: FUN_00406f7d
	void Game_StartUp(std::int16_t width, std::int16_t height, bool enableTime, NXBasics::CIniFileReader* iniFile, bool networkGame)
	{
		if (gGameInitialized != 0)
		{
			std::byte savedStartupData[288];
			std::memcpy(savedStartupData, &gGameControlStartupData, sizeof(savedStartupData));
			Game_ShutDown();
			std::memcpy(&gGameControlStartupData, savedStartupData, sizeof(savedStartupData));
		}
		gGameRandomState.seed = NC2Runtime::RandomSeed::Game;
		gGameRandomState.currentState = NC2Runtime::RandomSeed::Game;
		gGameStaticVars.uniqueIdCounter = 1;
		NRemaster::CivilizationBuildings::End();
		GameTime_Reset(false);
		gGameStaticVars.networkGame = static_cast<std::uint8_t>(networkGame);
		if (networkGame || Read<std::uint32_t>(&gGameControlStartupData, NC2Layout::GameControlStartupOffset::Mode) == SwitchIds::GameStartupMode::SkirmishMap) NRemaster::CivilizationBuildings::Begin();
		ConstructOriginal(NC2Layout::ObjectSize::GameControl, GameControl_Construct, iniFile);
		ConstructOriginal(1, GameMap_Construct, width, height);
		ConstructOriginal(NC2Layout::ObjectSize::MissionManager, MissionManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::AIManager, AIManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::MapMoveableArray, MapMoveableArray_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::MapPathfinderManager, MapPathfinderManager_Construct);
		static_assert(sizeof(NC2Logic::CHumanArray) == 48);
		ConstructNative<NC2Logic::CHumanArray>(0x30);
		ConstructOriginal(NC2Layout::ObjectSize::HumanMerchantArray, HumanMerchantArray_Construct);
		static_assert(sizeof(NC2Logic::CAnimalArray) == 48);
		ConstructNative<NC2Logic::CAnimalArray>(0x30);
		static_assert(sizeof(NC2Logic::CVehicleArray) == 48);
		ConstructNative<NC2Logic::CVehicleArray>(0x30);
		ConstructOriginal(NC2Layout::ObjectSize::MarkerArray, MarkerArray_Construct);
		static_assert(sizeof(NC2Logic::CHouseArray) == 48);
		ConstructNative<NC2Logic::CHouseArray>(0x30);
		ConstructOriginal(NC2Layout::ObjectSize::GuideManager, GuideManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::PlayerStatisticManager, PlayerStatisticManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::DelayedActionManager, DelayedActionManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::TributeManager, TributeManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::EmbeddedSavegameManager, EmbeddedSavegameManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::HappinessManager, HappinessManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::MultiPlayerGoalManager, MultiPlayerGoalManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::GameAssistantManager, GameAssistantManager_Construct);
		ConstructOriginal(NC2Layout::ObjectSize::MissionOnscreenInfoManager, MissionOnscreenInfoManager_Construct);
		CCleanMapsManager::Tool_GenerateNewGuid(&gGameStaticVars.mapGuid);
		gGameStaticVars.mapType = 2;
		GameTime_Reset(enableTime);
		gGameStaticVars.autosave.intervalTicks = 0;
		gGameInitialized = 1;
	}

	// Original function: FUN_004075ab; distinct tail-forwarding entry.
	void Misc_EngineIsInitialised()
	{
		CStaticGoodTypeDataManager::Misc_GetEngineLandscapeIds();
	}

	// Original function: FUN_0040796b; Mac l_GameTime_Reset.
	void GameTime_Reset(bool enabled)
	{
		std::memset(&gGameTime, 0, sizeof(gGameTime));
		gGameTime.tick = 1;
		gGameTime.lastRealTime = static_cast<double>(timeGetTime());
		gGameTime.retainedSpeed = 12.0;
		GameTime_SetSpeed(enabled ? 12.0 : 0.0);
		if (enabled) GameTime_ResetNetworkSynchronization();
	}

	// Original function: FUN_00407be0
	void GameTime_SetSpeed(double speed)
	{
		// The original JAE branch treats an unordered old speed like a small speed.
		if (!(gGameTime.speed >= 0.1)) gGameTime.lastRealTime = static_cast<double>(timeGetTime());
		if (gGameTime.speed > 1.0) gGameTime.retainedSpeed = gGameTime.speed;
		gGameTime.speed = speed;
		gGameTime.tickDuration = speed > 0.1 ? 1000.0 / speed : 0.0;
		gGameTime.maximizeSpeed = 0;
	}

	// Original function: FUN_00407c5b; the network-game flag suppresses this operation.
	void GameTime_MaximizeGameSpeed_SetFlag(bool enabled)
	{
		if (gGameStaticVars.networkGame != 0) return;
		GameTime_SetSpeed(enabled ? 1.0 : 12.0);
		gGameTime.maximizeSpeed = static_cast<std::uint8_t>(enabled);
	}

	// Original function: FUN_00411bcc
	void GameMapModifyManager_StartGameMapModify()
	{
		if (++gGameMapModifyNestingDepth == 1) CCallbackManager::sTheObjectPtr->ExecuteCallback(0x31, false, 0, 0, 0);
	}

	// Original function: FUN_00411bef
	void GameMapModifyManager_StopGameMapModify()
	{
		if (gGameMapModifyNestingDepth == 0) return;
		if (gGameMapModifyNestingDepth == 1) CCallbackManager::sTheObjectPtr->ExecuteCallback(0x32, true, 0, 0, 0);
		--gGameMapModifyNestingDepth;
	}

	// Original function: FUN_00411c18
	void GameMapModifyManager_ForceUpdate()
	{
		if (gGameMapModifyNestingDepth == 0) return;
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x32, true, 0, 0, 0);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x31, false, 0, 0, 0);
	}
}

namespace NC2E2
{
	using namespace NC2Lifetime;
	// Unreconstructed original subconstructors. No replacement bodies are supplied.
	extern void* __fastcall E2StaticDataTransitionsManager_Construct(void*, void* unusedEdx);
	extern void* __fastcall E2StaticDataParticelManager_Construct(void*, void* unusedEdx);
	extern void* __fastcall E2StaticDataGoodManager_Construct(void*, void* unusedEdx);
	extern void* __fastcall E2StaticDataParticelEffectManager_Construct(void*, void* unusedEdx);
	extern void* __fastcall MacroMapManager_Construct(void*, void* unusedEdx);

	// The remaining original callbacks keep their verified callback-manager ABI.
	extern bool __cdecl E2Manager_sl_Callback_ObjectDisplayStateInLogicChanged(NC2Logic::TCallbackTypes, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
	extern bool __cdecl E2Manager_sl_Callback_Misc(NC2Logic::TCallbackTypes, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

	// Original function: FUN_0047dcc4
	void* E2Manager_Construct(void* manager)
	{
		gE2Manager = manager;
		std::memset(manager, 0, 1);
		std::memset(gStaticMapModifiedBounds, 0, sizeof(gStaticMapModifiedBounds));
		gDesktopOwnedBobBitmap = nullptr;
		ConstructOriginal(NC2Layout::ObjectSize::E2StaticDataBobManagersManager, E2StaticDataBobManagersManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(NC2Layout::ObjectSize::E2StaticDataTexturesManager, E2StaticDataTexturesManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, E2StaticDataPalettesManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(NC2Layout::ObjectSize::E2StaticDataLandscapesManager, E2StaticDataLandscapesManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, E2StaticDataHousesManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(NC2Layout::ObjectSize::E2StaticDataPatternManager, E2StaticDataPatternManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		static_assert(sizeof(NC2E2::CE2StaticDataSoundFXManager) == 1304);
		ConstructNative<NC2E2::CE2StaticDataSoundFXManager>(0x518);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, E2StaticDataTransitionsManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, E2StaticDataParticelManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(NC2Layout::ObjectSize::E2StaticDataMapMoveablesManager, E2StaticDataMapMoveablesManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, E2StaticDataHumanMiscManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, E2StaticDataGoodManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		static_assert(sizeof(NC2E2::CE2StaticDataAnimalSoundManager) == 8);
		ConstructNative<NC2E2::CE2StaticDataAnimalSoundManager>(0x8);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, E2StaticDataMiscLandscapeManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, E2StaticDataParticelEffectManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		static_assert(sizeof(NC2E2::CLevelOfDetailManager) == 1);
		ConstructNative<NC2E2::CLevelOfDetailManager>(1);
		ConstructOriginal(NC2Layout::ObjectSize::MacroMapManager, MacroMapManager_Construct);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x2, E2Manager_CallbackSomethingInLogicXtructed, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x3, E2Manager_CallbackSomethingInLogicXtructed, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x4, E2Manager_CallbackSomethingInLogicXtructed, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x5, E2Manager_CallbackSomethingInLogicXtructed, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x6, E2Manager_CallbackSomethingInLogicXtructed, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x7, E2Manager_CallbackSomethingInLogicXtructed, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x8, E2Manager_CallbackSomethingInLogicXtructed, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x9, E2Manager_CallbackSomethingInLogicXtructed, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0xA, E2Manager_CallbackSomethingInLogicXtructed, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0xE, E2Manager_sl_Callback_ObjectDisplayStateInLogicChanged, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0xF, E2Manager_sl_Callback_ObjectDisplayStateInLogicChanged, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x10, E2Manager_sl_Callback_ObjectDisplayStateInLogicChanged, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x11, E2Manager_sl_Callback_ObjectDisplayStateInLogicChanged, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x12, E2Manager_sl_Callback_MapIO, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x13, E2Manager_sl_Callback_MapIO, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x14, E2Manager_sl_Callback_MapIO, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x15, E2Manager_sl_Callback_MapIO, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x16, E2Manager_sl_Callback_MapIO, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x31, E2Manager_sl_Callback_MapEdited, 0, 0x2);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x32, E2Manager_sl_Callback_MapEdited, 0, 0x2);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x33, E2Manager_sl_Callback_MapEdited, 0, 0x2);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0xD, E2Manager_sl_Callback_MapEdited, 0, 0x2);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x17, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x1C, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x21, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x22, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x1F, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x20, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x23, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x27, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x24, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x1D, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x18, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x25, E2Manager_sl_Callback_Misc, 0, 0xA);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x26, E2Manager_sl_Callback_Misc, 0, 0xA);
		static_assert(sizeof(NC2E2::C2DEngineDisplayManager) == 24);
		ConstructNative<NC2E2::C2DEngineDisplayManager>(0x18);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(NC2Layout::ObjectSize::E2ParticelManager, E2ParticelManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(NC2Layout::ObjectSize::AmbientSoundManager, AmbientSoundManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		static_assert(sizeof(NC2E2::CE2AnimalSoundManager) == 140);
		ConstructNative<NC2E2::CE2AnimalSoundManager>(0x8C);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(1, WorldMusic_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(NC2Layout::ObjectSize::E2HumanSoundManager, E2HumanSoundManager_Construct);
		NC2LogicExternals::External_Misc_ProgressBar_Update();
		ConstructOriginal(NC2Layout::ObjectSize::E2MapWeatherManagerSectorManager, E2MapWeatherManagerSectorManager_Construct);
		return manager;
	}
}


namespace NC2Logic
{
	// Original function: FUN_0040aebd; distinct float-to-double forwarding entry.
	void GameTime_SetSpeed(float speed)
	{
		GameTime_SetSpeed(static_cast<double>(speed));
	}
}

#pragma once
#include "C2GameState.h"

namespace NC2Logic
{
	class CIoHelper;
	extern void* gGame;
	void Game_ShutDown();
	void Game_Destruct(void* game);
	void GameControlStartupData_Clear(void* data);
	void UserNetworkCommandManager_EmptyQueue(void* manager);
	void* __fastcall UserNetworkCommandManager_Construct(void* manager, void* unusedEdx);
	void GameMap_Destruct(void* map);
	void* __fastcall MissionOnscreenInfoManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall GameAssistantManager_Construct(void* manager, void* unusedEdx);
	void GameAssistantManager_UserInput_Save(void* manager, std::int32_t player, std::int32_t command, std::int32_t value);
	bool GameAssistantManager_l_Assistant_IsPlayerIdOk(const void* manager, std::int32_t player);
	void* __fastcall MultiPlayerGoalManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall HappinessManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall EmbeddedSavegameManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall TributeManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall DelayedActionManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall HumanMerchantArray_Construct(void* manager, void* unusedEdx);
	void* __fastcall MarkerArray_Construct(void* manager, void* unusedEdx);
	void* __fastcall MapMoveableArray_Construct(void* manager, void* unusedEdx);
	void* __fastcall MapPathfinderManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall GuideManager_Construct(void* manager, void* unusedEdx);
	void __fastcall GuideManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall GuideManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall StaticDependenciesManager_Construct(void* manager, void* unusedEdx);
}
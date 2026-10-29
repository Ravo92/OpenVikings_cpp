#pragma once
#include <cstdint>

namespace NC2Logic
{
	class CIoHelper;
	void __fastcall GameControl_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall GameControl_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall MissionManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall MissionManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall MissionOnscreenInfoManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall MissionOnscreenInfoManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall GameAssistantManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall GameAssistantManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall MultiPlayerGoalManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall MultiPlayerGoalManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall HappinessManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall HappinessManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall TributeManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall TributeManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall MissionManager_Construct(void* manager, void* unusedEdx);
	void __fastcall MarkerArray_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall MarkerArray_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall MapMoveableArray_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall MapMoveableArray_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall MapPathfinderManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall MapPathfinderManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall DelayedActionManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall DelayedActionManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall HumanMerchantArray_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall HumanMerchantArray_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall PlayerStatisticManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall PlayerStatisticManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	void __fastcall EmbeddedSavegameManager_IO_Savegame_DoSave(void* manager, void* unusedEdx, CIoHelper* io);
	void* __fastcall EmbeddedSavegameManager_ConstructFromSavegame(void* manager, void* unusedEdx, CIoHelper* io);
	bool EmbeddedSavegameManager_Savegame_DoEmbed(void* manager, const char* filename);
	bool EmbeddedSavegameManager_Savegame_RestoreEmbedded(void* manager, const char* filename);
}
#pragma once

#include <cstdint>

namespace NXBasics
{
	class CIniFileReader;
}

namespace NC2Logic
{
	void* Game_Construct(void* game);
	void Game_WorkOnStartUpData();
	void Game_StartUp(std::int16_t width, std::int16_t height, bool enableTime, NXBasics::CIniFileReader* iniFile, bool networkGame);
	void Misc_EngineIsInitialised();
	void GameTime_Reset(bool enabled);
	void GameTime_SetSpeed(double speed);
	void GameTime_SetSpeed(float speed);
	void GameTime_MaximizeGameSpeed_SetFlag(bool enabled);
	void GameMapModifyManager_StartGameMapModify();
	void GameMapModifyManager_StopGameMapModify();
	void GameMapModifyManager_ForceUpdate();
}

namespace NC2E2
{
	void* E2Manager_Construct(void* manager);
}
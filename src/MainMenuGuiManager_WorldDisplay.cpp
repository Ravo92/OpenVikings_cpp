#include "C2EngineDisplayAccess.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2PlayerCenterPosition.h"
#include "MainMenuGuiManager.h"

namespace NC2MainMenuGuiManager
{
	// Original function: FUN_004d317c.
	void CGuiManager::EngineWorldDisplayGuiElement_CenterDisplay() const
	{
		std::uint32_t mapPoint = 0;

		if (NC2InGameGuiEntryPoints::C2IGG_EntryPoint_PlayerCenterPosition_Get(0, &mapPoint))
		{
			const void* worldDisplayElement = NC2Lifetime::Read<void*>(this, NC2Layout::MainMenuManagerOffset::WorldDisplayElement);
			NC2E2::DE_SetWantedPositionToMapPoint(NC2Lifetime::Read<void*>(worldDisplayElement, NC2Layout::WorldGuiElementOffset::Display), &mapPoint);
		}
	}
}
#pragma once

#include <cstdint>

namespace NC2E2
{
	void __fastcall E2ParticelManager_SortParticelAndDrawBottom(void* manager, void* unusedEdx, void* display);
	void E2ParticelManager_DrawParticelLine(void* manager, void* display, std::int32_t row);
	void E2ParticelManager_l_DrawParticel(void* manager, void* display, void* particle);
	void LogicParticelObject_Move(void* particle);
	void LogicParticelObject_CalculateValency(void* particle);
	void LogicParticelObject_DoOneMovementTick(void* particle, void* display);
	void ParticelPosition_ToMapPoint(const void* position, void* point);
	void ParticelObject_RefreshGroundHeight(void* particle);
}
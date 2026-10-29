#pragma once
#include <cstdint>

namespace NC2Logic
{
	void* __fastcall GameMapStaticVars_ConstructMembers(void* data, void* unusedEdx);
	void* __fastcall House_ConstructMembers(void* house, void* unusedEdx);
	void* __fastcall Human_ConstructMembers(void* human, void* unusedEdx);
	void* __fastcall Human_Task_ConstructMembers(void* task, void* unusedEdx);
	void* __fastcall Human_ConstructMilitaryMembers(void* data, void* unusedEdx);
	void* __fastcall Vehicle_ConstructMembers(void* vehicle, void* unusedEdx);
	void* __fastcall Vehicle_ConstructVars(void* data, void* unusedEdx);
	void* __fastcall Vehicle_Task_ConstructMembers(void* task, void* unusedEdx);
	void* __fastcall Animal_ConstructStateMembers(void* state, void* unusedEdx);
	void* __fastcall ConstructPointMembers(void* data, void* unusedEdx);

	void* SearchNearbyMapPoints_ConstructMembers(void* data);
	void* SearchDamagedWall_ConstructMembers(void* data);
}
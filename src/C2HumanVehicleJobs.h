#pragma once
#include <cstdint>

namespace NC2Logic
{
	void Human_l_StartTask_ExecuteJob_Carpenter(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_TryToCollect(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_Collect(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_BuildVehicle(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_TryToBuild(void* human);
	void Human_l_AttachToVehicleBuildHouse(void* human, std::int32_t house);
	bool Human_l_IsHouseTypeOnPoint(const void* human, const void* point, std::uint32_t type);
	void* Human_FindVehicleConstructionSiteNearWorkCenter(const void* human, std::uint32_t type);
	bool Human_FindVehicleBuildPointNearWorkCenter(void* human, std::uint32_t type, void* realPosition, void* workPosition);
	std::uint32_t Human_l_FindVehicleBuildPointNearWorkCenter(const void* human, std::int32_t engineType, std::uint32_t stage, bool ignoreContinents, std::uint32_t continent, void* realPosition, void* workPosition, const void* candidate, std::uint32_t size);
}
#pragma once
#include <cstdint>

namespace NC2InGameGuiWorldWindow
{
	void* WorldObserveDisplayWindow_Construct(void* window);
	void WorldObserveDisplayWindow_l_Follow_Start(void* window);
	void WorldObserveDisplayWindow_l_Follow_Stop(void* window);
	void WorldObserveDisplayWindow_l_Follow_Validate(void* window);
	void WorldObserveDisplayWindow_l_Follow_Human(void* window, std::int32_t id);
	void WorldObserveDisplayWindow_l_Follow_Vehicle(void* window, std::int32_t id);
	void WorldObserveDisplayWindow_l_Follow_House(void* window, std::int32_t id);
}
#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2E2
{
	// Physical Windows map buffers and complete static owner region.
	struct SEngineMapMacroPointStorage
	{
		std::uint16_t patternA, patternB;
		std::uint8_t brightness, vertexColor, flags, transitions[4];
		std::byte unresolved0B;
	};

	struct SEngineMapMicroPointStorage
	{
		std::uint16_t landscapeId;
		std::uint8_t flags;
		std::byte unresolved03;
	};

	struct SEngineMapDirtyBoundsStorage
	{
		std::int16_t first[2], last[2];
	};

	struct SEngineMapStaticStorage
	{
		SEngineMapMacroPointStorage* macroPoints;
		SEngineMapMicroPointStorage* microPoints;
		SEngineMapDirtyBoundsStorage heightDirtyBounds, transitionDirtyBounds;
		std::uint8_t missingResource;
		std::byte unresolved19[3];
	};

	void EngineMap_MapMacroPoint_TrianglePattern_SetType(const void* point, bool triangleA, std::uint32_t pattern);
	void EngineMap_Highlighting_Clear();
	const void* Engine2d_EntryPoint_Tool_GetPatternDescriptionPtr(const void* packedMacroPoint, bool firstTriangle);
	void Engine2d_EntryPoint_Tool_GetPatternDescriptionsAroundPoint(const void* packedMicroPoint, const void** descriptions);
	void* __fastcall EngineMap_Construct(void* manager, void* unusedEdx);
	bool __cdecl EngineMap_l_HeightChanged_Callback(std::uint32_t type, std::uintptr_t userData, std::uintptr_t parameter1, std::uintptr_t parameter2, std::uintptr_t parameter3);
	bool __cdecl EngineMap_l_Transitions_Callback(std::uint32_t type, std::uintptr_t userData, std::uintptr_t parameter1, std::uintptr_t parameter2, std::uintptr_t parameter3);
	void EngineMap_l_HeightChanged_CalculateBrightness(const void* packedPoint);
	std::uint32_t EngineMap_Brightness_Get(const void* packedMicroPoint);
	void EngineMap_Transition_Set(const void* packedPoint, std::int32_t transitionId, std::uint32_t layer, bool triangleA);
	void EngineMap_l_Transitions_Check(const void* packedPoint);
	void EngineMap_OverlayPattern_RoadChanged(const void* packedMicroPoint, bool enabled);
	void EngineMap_OverlayPattern_HouseAreaChanged(const void* packedMicroPoint, bool enabled);
	void EngineMap_l_OverlayPattern_RecalculateMacroGridData(const void* packedMicroPoint);
	bool __cdecl E2Manager_sl_Callback_MapEdited(std::uint32_t type, std::uintptr_t userData, std::uintptr_t parameter1, std::uintptr_t parameter2, std::uintptr_t parameter3);
	bool EngineMap_Misc_GetTrianglePatternDescriptions_AroundWay(const void* packedMicroPoint, std::int32_t direction, const void** descriptions);
	bool EngineMap_Misc_GetTrianglePatternDescriptions_AroundPoint(const void* packedMicroPoint, const void** descriptions);
}

namespace NC2E2
{
	void EngineMap_VertexColorIndex_Set(const void* point, std::int32_t color);
	void Engine2d_EntryPoint_Edit_VertexColorIndex_Set(std::uint32_t point, std::int32_t radius, std::int32_t color, bool landOnly);
}

namespace NC2LogicExternals
{
	void External_VertexColor_SetArea(const void* point, std::uint32_t radius, std::int32_t color, bool landOnly);
}
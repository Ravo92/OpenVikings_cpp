#include "ApplicationGlobals.h"
#include "C2CallbackManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDisplayManager.h"
#include "C2EngineMapTerrainAccess.h"
#include "C2EngineTerrainData.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanInitialization.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticTrianglePatternTypeData.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <algorithm>
#include <cstring>

namespace
{
	using NC2Lifetime::Read;
	using NC2Lifetime::Write;
	using Point = std::int16_t[2];

	const void* GetMacro(const void* point)
	{

		return NC2Logic::GameMap_GetConstMapMacroPointPtr(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y));
	}

	using TriangleGetter = const void* (*)(const void*);

	// The two original around-way routines use the same geometric branches.
	bool GetAroundWay(const void* micro, std::int32_t direction, const void** output, TriangleGetter getA, TriangleGetter getB)
	{
		output[0] = output[1] = nullptr;
		Point point;
		NC2Logic::MapMagPoint_ConstructFromMig(point, micro, false);

		if (!NC2Logic::MapMagPoint_IsInsideMap(point))
			return false;

		std::int16_t x, y;
		NC2Logic::VE_MapCoords_GetMicroGridMapOffset(Read<std::int16_t>(micro, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(micro, NC2Layout::MapCoordinatePairOffset::Y), &x, &y);
		const int offset = x | (y << 1);
		using namespace NC2Logic;

		switch (offset)
		{
			case SwitchIds::MicroGridOffset::Origin:
				switch (direction)
				{
					case SwitchIds::HexagonDirection::East:
						output[0] = getB(point);
						MapMagPoint_MoveCursorUnwrapped(point, 5);
						output[1] = getA(point);
						break;
					case SwitchIds::HexagonDirection::SouthEast:
						output[0] = getB(point);
						output[1] = getA(point);
						break;
					case SwitchIds::HexagonDirection::SouthWest:
						output[0] = getA(point);
						MapMagPoint_MoveCursorUnwrappedWest(point);
						output[1] = getB(point);
						break;
					case SwitchIds::HexagonDirection::West:
						MapMagPoint_MoveCursorUnwrappedWest(point);
						output[0] = getB(point);
						MapMagPoint_MoveCursorUnwrapped(point, 5);
						output[1] = getA(point);
						break;
					case SwitchIds::HexagonDirection::NorthWest:
						MapMagPoint_MoveCursorUnwrapped(point, 4);
						output[0] = getA(point);
						output[1] = getB(point);
						break;
					case SwitchIds::HexagonDirection::NorthEast:
						MapMagPoint_MoveCursorUnwrapped(point, 4);
						output[0] = getB(point);
						MapMagPoint_MoveCursorUnwrappedEast(point);
						output[1] = getA(point);
						break;
				}

				break;
			case SwitchIds::MicroGridOffset::East:
				if (direction == 0 || direction == 3)
				{
					output[0] = getB(point);
					MapMagPoint_MoveCursorUnwrapped(point, 5);
					output[1] = getA(point);
				}
				else if (direction == 1 || direction == 2)
				{
					output[0] = output[1] = getB(point);
				}
				else if (direction == 4 || direction == 5)
				{
					MapMagPoint_MoveCursorUnwrapped(point, 5);
					output[0] = output[1] = getA(point);
				}

				break;
			case SwitchIds::MicroGridOffset::SouthWest:
				if (direction == 0 || direction == 1)
				{
					output[0] = output[1] = getA(point);
				}
				else if (direction == 2 || direction == 5)
				{
					output[0] = getA(point);
					MapMagPoint_MoveCursorUnwrappedWest(point);
					output[1] = getB(point);
				}
				else if (direction == 3 || direction == 4)
				{
					MapMagPoint_MoveCursorUnwrappedWest(point);
					output[0] = output[1] = getB(point);
				}

				break;
			case SwitchIds::MicroGridOffset::SouthEast:
				if (direction == 0 || direction == 5)
				{
					output[0] = output[1] = getB(point);
				}
				else if (direction == 1 || direction == 4)
				{
					output[0] = getA(point);
					output[1] = getB(point);
				}
				else if (direction == 2 || direction == 3)
				{
					output[0] = output[1] = getA(point);
				}

				break;
		}

		return output[0] != nullptr && output[1] != nullptr;
	}
}

namespace NC2Logic
{
	// Original function: FUN_0040b0a5.
	bool Logic_EntryPoint_Edit_MapMacroPoint_TrianglePattern_SetType(const void* point, bool triangleA, std::uint32_t pattern)
	{
		if (gGameMapModifyNestingDepth == 0)
			return false;

		return GameMap_MapMacroPoint_TrianglePattern_SetType(point, triangleA, pattern);
	}

	// Original function: FUN_0040b0f2; the native entry discards the setter result.
	void Logic_EntryPoint_Edit_MapMicroPoint_Road_Set(const void* point, bool enabled)
	{
		if (gGameMapModifyNestingDepth != 0)
			GameMap_MapMicroPoint_Road_Set(point, enabled);
	}

	// Original function: FUN_004238a6; compare the full pattern ID before storing its low byte.
	bool GameMap_MapMacroPoint_TrianglePattern_SetType(const void* point, bool triangleA, std::uint32_t pattern)
	{
		auto* macro = GameMap_GetMapMacroPointPtr(point);

		if (macro == nullptr || MapMagPoint_IsBorderPoint(point))
			return false;

		const auto offset = triangleA ? 0u : 1u;

		if (Read<std::uint8_t>(macro, offset) == pattern)
			return false;

		Write<std::uint8_t>(macro, offset, static_cast<std::uint8_t>(pattern));
		GameMapModifyManager_GameMapModifiedMacro(11, point, 0);

		return true;
	}

	// Original function: FUN_0042430f.
	const void* GameMap_GetTrianglePatternDescriptionA(const void* point)
	{
		if (!MapMagPoint_IsInsideMap(point))
			return nullptr;

		return gStaticTrianglePatternTypes + Read<std::uint8_t>(GetMacro(point), NC2Layout::MapMacroPointOffset::PatternA);
	}

	// Original function: FUN_00424339.
	const void* GameMap_GetTrianglePatternDescriptionB(const void* point)
	{
		if (!MapMagPoint_IsInsideMap(point))
			return nullptr;

		return gStaticTrianglePatternTypes + Read<std::uint8_t>(GetMacro(point), NC2Layout::MapMacroPointOffset::PatternB);
	}

	// Original function: FUN_004240b0; map-edit callback 12 updates the road overlay.
	bool GameMap_MapMicroPoint_Road_Set(const void* point, bool enabled)
	{
		auto* micro = GameMap_GetMapMicroPointPtr(point);

		if (micro == nullptr || MapMigPoint_IsBorderPoint(point))
			return false;

		const auto flags = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags);

		if (((flags >> 1) & 1u) == static_cast<std::uint8_t>(enabled))
			return false;

		Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, (flags & NXBasics::BitMask::ByteExceptBit1) | (static_cast<std::uint8_t>(enabled) << 1));
		GameMapModifyManager_GameMapModified(12, point, static_cast<std::uint32_t>(enabled));

		return true;
	}

	// Original function: FUN_00424108.
	bool GameMap_Misc_GetTrianglePatternDescriptions_AroundWay(const void* micro, std::int32_t direction, const void** descriptions)
	{

		return GetAroundWay(micro, direction, descriptions, GameMap_GetTrianglePatternDescriptionA, GameMap_GetTrianglePatternDescriptionB);
	}

	// Original function: FUN_00424364.
	bool GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(const void* micro, const void** descriptions)
	{
		std::memset(descriptions, 0, 24);

		return GameMap_Misc_GetTrianglePatternDescriptions_AroundWay(micro, 1, descriptions) && GameMap_Misc_GetTrianglePatternDescriptions_AroundWay(micro, 3, descriptions + 2) && GameMap_Misc_GetTrianglePatternDescriptions_AroundWay(micro, 5, descriptions + 4);
	}

	// Original function: FUN_00424862.
	bool GameMap_Exploration_HasBeenSeenOnce(std::uint32_t player, const void* micro)
	{
		if (player >= 16)
			return true;

		if (!MapMigPoint_IsInsideMap(micro))
			return false;

		return (Read<std::uint16_t>(GameMap_GetConstMapMicroPointPtr(micro), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) != 0;
	}

	// Original function: FUN_00411c48.
	void GameMapModifyManager_GameMapModified(std::uint32_t type, const void* micro, std::uint32_t parameter)
	{
		if (gGameMapModifyNestingDepth != 0 && MapMigPoint_IsInsideMap(micro))
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x33, true, type, MapMigPoint_GetLong(micro), parameter);
	}

	// Original function: FUN_00411c84.
	void GameMapModifyManager_GameMapModifiedMacro(std::uint32_t type, const void* macro, std::uint32_t parameter)
	{
		Point micro;
		MapMigPoint_ConstructFromMag(micro, macro, false);
		GameMapModifyManager_GameMapModified(type, micro, parameter);
	}
}

namespace NC2E2
{
	// Original function: FUN_00482551; graphics change precedes the logic callback.
	void EngineMap_MapMacroPoint_TrianglePattern_SetType(const void* point, bool triangleA, std::uint32_t pattern)
	{
		auto* data = const_cast<void*>(EngineMap_GetConstMapMacroPointPtr(point));

		if (data == nullptr || NC2Logic::MapMagPoint_IsBorderPoint(point))
			return;

		const auto offset = triangleA ? 0u : 2u;

		if (Read<std::uint16_t>(data, offset) == pattern)
		{

			return;
		}

		Write<std::uint16_t>(data, offset, static_cast<std::uint16_t>(pattern));
		NC2Logic::GameMapModifyManager_GameMapModifiedMacro(0x33, point, 0);
		NC2Logic::Logic_EntryPoint_Edit_MapMacroPoint_TrianglePattern_SetType(point, triangleA, static_cast<const SE2StaticDataPatternData*>(gStaticPatternStorage.entries)[pattern].logicType);
	}

	// Original function: FUN_004832dc; clear flags in descending index order.
	void EngineMap_Highlighting_Clear()
	{
		auto index = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) - 1u);

		while (index >= 0)
		{
			void* data = Read<void*>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MicroPoints);
			const auto offset = static_cast<std::uint32_t>(index) * 4u + 2u;
			Write<std::uint8_t>(data, offset, Read<std::uint8_t>(data, offset) & NXBasics::BitMask::ByteExceptBits0Through1);
			--index;
		}
	}

	// Original function: FUN_0048114e.
	const void* Engine2d_EntryPoint_Tool_GetPatternDescriptionPtr(const void* packedMacroPoint, bool firstTriangle)
	{
		if (NC2Logic::MapMagPoint_IsBorderPoint(packedMacroPoint))
			return nullptr;

		const void* macro = EngineMap_GetConstMapMacroPointPtr(packedMacroPoint);
		const auto index = Read<std::uint16_t>(macro, firstTriangle ? NC2Layout::EngineMapMacroPointOffset::PatternAUnsigned : NC2Layout::EngineMapMacroPointOffset::PatternBUnsigned);

		return static_cast<const SE2StaticDataPatternData*>(gStaticPatternStorage.entries) + index;
	}

	// Original function: FUN_00481188; preserves the around-point helper's output initialization and call order.
	void Engine2d_EntryPoint_Tool_GetPatternDescriptionsAroundPoint(const void* packedMicroPoint, const void** descriptions)
	{
		EngineMap_Misc_GetTrianglePatternDescriptions_AroundPoint(packedMicroPoint, descriptions);
	}

	// Original function: FUN_0048320e.
	const void* EngineMap_GetTrianglePatternDescriptionA(const void* point)
	{
		if (!NC2Logic::MapMagPoint_IsInsideMap(point) || Read<std::uint8_t>(GetMacro(point), NC2Layout::MapMacroPointOffset::PatternA) == 0)
			return nullptr;

		return static_cast<const SE2StaticDataPatternData*>(gStaticPatternStorage.entries) + Read<std::uint16_t>(EngineMap_GetConstMapMacroPointPtr(point), NC2Layout::EngineMapMacroPointOffset::PatternA);
	}

	// Original function: FUN_00483243.
	const void* EngineMap_GetTrianglePatternDescriptionB(const void* point)
	{
		if (!NC2Logic::MapMagPoint_IsInsideMap(point) || Read<std::uint8_t>(GetMacro(point), NC2Layout::MapMacroPointOffset::PatternB) == 0)
			return nullptr;

		return static_cast<const SE2StaticDataPatternData*>(gStaticPatternStorage.entries) + Read<std::uint16_t>(EngineMap_GetConstMapMacroPointPtr(point), NC2Layout::EngineMapMacroPointOffset::PatternB);
	}

	// Original function: FUN_00483007.
	bool EngineMap_Misc_GetTrianglePatternDescriptions_AroundWay(const void* micro, std::int32_t direction, const void** descriptions)
	{

		return GetAroundWay(micro, direction, descriptions, EngineMap_GetTrianglePatternDescriptionA, EngineMap_GetTrianglePatternDescriptionB);
	}

	// Original function: FUN_0048327a.
	bool EngineMap_Misc_GetTrianglePatternDescriptions_AroundPoint(const void* micro, const void** descriptions)
	{
		std::memset(descriptions, 0, 24);

		return EngineMap_Misc_GetTrianglePatternDescriptions_AroundWay(micro, 1, descriptions) && EngineMap_Misc_GetTrianglePatternDescriptions_AroundWay(micro, 3, descriptions + 2) && EngineMap_Misc_GetTrianglePatternDescriptions_AroundWay(micro, 5, descriptions + 4);
	}

	// Original function: FUN_00482241.
	void* __fastcall EngineMap_Construct(void* manager, void* /*unusedEdx*/)
	{
		gEngineMap = manager;
		std::memset(gEngineMapStaticStorage, 0, sizeof(gEngineMapStaticStorage));
		const auto macroCount = Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount), microCount = Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount);
		auto* macro = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(macroCount * NC2Layout::RecordSize::EngineMapMacroPointUnsigned, 1));
		Write(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MacroPoints, macro);
		std::memset(macro, 0, macroCount * NC2Layout::RecordSize::EngineMapMacroPointUnsigned);
		auto* micro = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(microCount * NC2Layout::RecordSize::EngineMapMicroPointUnsigned, 1));
		Write(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MicroPoints, micro);
		std::memset(micro, 0, microCount * NC2Layout::RecordSize::EngineMapMicroPointUnsigned);

		for (std::uint32_t i = 0; i < microCount; ++i)
		{
			Write<std::uint16_t>(micro, i * NC2Layout::RecordSize::EngineMapMicroPointUnsigned, NXBasics::IntegerRange::UInt16MaximumSigned);
		}

		Point point{ };

		for (point[1] = 0; point[1] < static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroHeight()); ++point[1])
		{
			for (point[0] = 0; point[0] < static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroWidth()); ++point[0])
			{
				EngineMap_l_HeightChanged_CalculateBrightness(point);
			}
		}

		for (std::uint32_t i = 0; i < macroCount; ++i)
		{
			for (std::uint32_t layer = 0; layer < 2; ++layer)
			{
				Write<std::uint8_t>(macro, i * NC2Layout::RecordSize::EngineMapMacroPointUnsigned + NC2Layout::EngineMapMacroPointOffset::Transitions + layer * NC2Runtime::EngineMapEncoding::TrianglesPerLayer, NXBasics::IntegerRange::UInt8MaximumSigned);
				Write<std::uint8_t>(macro, i * NC2Layout::RecordSize::EngineMapMacroPointUnsigned + NC2Layout::EngineMapMacroPointOffset::TransitionB + layer * NC2Runtime::EngineMapEncoding::TrianglesPerLayer, NXBasics::IntegerRange::UInt8MaximumSigned);
			}
		}

		for (std::uint32_t type = SwitchIds::CallbackType::BeginMapChanges; type <= SwitchIds::CallbackType::MapPointChanged; ++type)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(type, EngineMap_l_HeightChanged_Callback, 0, 10);
		}

		for (std::uint32_t type = SwitchIds::CallbackType::BeginMapChanges; type <= SwitchIds::CallbackType::MapPointChanged; ++type)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(type, EngineMap_l_Transitions_Callback, 0, 10);
		}

		return manager;
	}

	// Original function: FUN_0048274a.
	void EngineMap_l_HeightChanged_CalculateBrightness(const void* packedPoint)
	{
		using namespace NC2Logic;

		if (!MapMagPoint_IsInsideMap(packedPoint))
			return;

		auto* macro = const_cast<void*>(EngineMap_GetConstMapMacroPointPtr(packedPoint));
		GetMacro(packedPoint);
		Write<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::Brightness, 0);

		if (MapMagPoint_IsBorderPoint(packedPoint))
			return;

		Point micro;
		MapMigPoint_ConstructFromMag(micro, packedPoint, false);
		const void* descriptions[6];

		if (!GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(micro, descriptions))
			return;

		for (const void* description : descriptions)
		{
			if (Read<std::uint8_t>(description, NC2Layout::TrianglePatternTypeOffset::IsBorder) != 0)
			{

				return;
			}
		}

		Point cursor;
		std::memcpy(cursor, packedPoint, 4);
		const auto h0 = Read<std::uint8_t>(GetMacro(cursor), NC2Layout::MapMacroPointOffset::Height);
		MapMagPoint_MoveCursorWrappedWest(cursor);
		GetMacro(cursor);
		MapMagPoint_MoveCursorWrapped(cursor, 0);
		GetMacro(cursor);
		MapMagPoint_MoveCursorWrappedEast(cursor);
		GetMacro(cursor);
		MapMagPoint_MoveCursorWrapped(cursor, 2);
		GetMacro(cursor);
		MapMagPoint_MoveCursorWrapped(cursor, 2);
		const auto hFar = Read<std::uint8_t>(GetMacro(cursor), NC2Layout::MapMacroPointOffset::Height);
		MapMagPoint_MoveCursorWrappedWest(cursor);
		const auto hBelow = Read<std::uint8_t>(GetMacro(cursor), NC2Layout::MapMacroPointOffset::Height);
		const int slopeBelow = hBelow * 2 - h0 - hFar;
		MapMagPoint_MoveCursorWrapped(cursor, 4);
		const auto hFinal = Read<std::uint8_t>(GetMacro(cursor), NC2Layout::MapMacroPointOffset::Height);
		int brightness = (h0 * 2 - hFinal - hBelow - slopeBelow) * 3 / 2 + 127;

		if (brightness > 200)
			brightness = (brightness - 200) / 4 + 200;

		if (brightness < 60)
			brightness = 60 - (60 - brightness) / 4;

		Write<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::Brightness, static_cast<std::uint8_t>(std::clamp(brightness, 4, 255)));
	}

	// Original function: FUN_004828e5; C2DEngineMap::Brightness_Get.
	std::uint32_t EngineMap_Brightness_Get(const void* packedMicroPoint)
	{
		const auto x = Read<std::int16_t>(packedMicroPoint, NC2Layout::MapCoordinatePairOffset::X), y = Read<std::int16_t>(packedMicroPoint, NC2Layout::MapCoordinatePairOffset::Y);

		if (x < 4 || x >= NC2Logic::GameMap_GetWidth() - 4 || y < 4 || y >= NC2Logic::GameMap_GetHeight() - 4)
			return 0;

		Point macro, offset;
		NC2Logic::VE_MapCoords_MicroGridMapToMacroGridMapPoint(x, y, &macro[0], &macro[1]);
		NC2Logic::VE_MapCoords_GetMicroGridMapOffset(x, y, &offset[0], &offset[1]);
		const auto first = Read<std::uint8_t>(EngineMap_GetConstMapMacroPointPtr(macro), NC2Layout::EngineMapMacroPointOffset::Brightness);

		switch (static_cast<std::uint16_t>(offset[0]) | (static_cast<std::uint16_t>(offset[1]) << 1))
		{
			case SwitchIds::MicroGridOffset::Origin:
				return first;
			case SwitchIds::MicroGridOffset::East:
				++macro[0];
				break;
			case SwitchIds::MicroGridOffset::SouthWest:
				if ((macro[1] & 1) == 0)
					--macro[0];

				++macro[1];
				break;
			default:
				if ((macro[1] & 1) != 0)
					++macro[0];

				++macro[1];
				break;
		}

		return (first + Read<std::uint8_t>(EngineMap_GetConstMapMacroPointPtr(macro), NC2Layout::EngineMapMacroPointOffset::Brightness)) / 2u;
	}

	namespace
	{
		bool ChangedCallback(std::uint32_t type, std::uint32_t parameter1, std::uint32_t parameter2, std::uint32_t offset, bool heights)
		{
			if (type == SwitchIds::CallbackType::BeginMapChanges)
			{
				Write<std::uint16_t>(gEngineMapStaticStorage, offset + NC2Layout::EngineDirtyBoundsOffset::LastY, 0);
				Write<std::uint16_t>(gEngineMapStaticStorage, offset + NC2Layout::EngineDirtyBoundsOffset::LastPoint, 0);
				Write<std::uint16_t>(gEngineMapStaticStorage, offset + NC2Layout::EngineDirtyBoundsOffset::FirstY, NC2Runtime::EngineMapDirtyBounds::EmptyMinimum);
				Write<std::uint16_t>(gEngineMapStaticStorage, offset, NC2Runtime::EngineMapDirtyBounds::EmptyMinimum);
			}
			else if (type == SwitchIds::CallbackType::MapPointChanged && (heights ? parameter1 == 13 || parameter1 == 11 : parameter1 == 0x33))
			{
				Point point;
				NC2Logic::MapMigPoint_AssignLong(point, parameter2);

				for (unsigned i = 0; i != 2; ++i)
				{
					if (point[i] < Read<std::int16_t>(gEngineMapStaticStorage, offset + i * NC2Layout::RecordSize::EngineMapCoordinate))
						Write(gEngineMapStaticStorage, offset + i * NC2Layout::RecordSize::EngineMapCoordinate, point[i]);

					if (point[i] > Read<std::int16_t>(gEngineMapStaticStorage, offset + NC2Layout::EngineDirtyBoundsOffset::LastPoint + i * NC2Layout::RecordSize::EngineMapCoordinate))
						Write(gEngineMapStaticStorage, offset + NC2Layout::EngineDirtyBoundsOffset::LastPoint + i * NC2Layout::RecordSize::EngineMapCoordinate, point[i]);
				}
			}
			else if (type == SwitchIds::CallbackType::EndMapChanges && Read<std::int16_t>(gEngineMapStaticStorage, offset) != NC2Runtime::EngineMapDirtyBounds::EmptyMinimum)
			{
				Point first, last, point{ };
				NC2Logic::MapMagPoint_ConstructFromMig(first, gEngineMapStaticStorage + offset, false);
				NC2Logic::MapMagPoint_ConstructFromMig(last, gEngineMapStaticStorage + offset + NC2Layout::EngineDirtyBoundsOffset::LastPoint, false);

				for (unsigned i = 0; i != 2; ++i)
				{
					first[i] = static_cast<std::int16_t>(first[i] - 3);
					last[i] = static_cast<std::int16_t>(last[i] + 3);
				}

				NC2Logic::MapMagPoint_CutInsideMap(first);
				NC2Logic::MapMagPoint_CutInsideMap(last);

				for (point[1] = first[1]; point[1] <= last[1]; ++point[1])
				{
					for (point[0] = first[0]; point[0] <= last[0]; ++point[0])
					{
						if (heights)
						{
							EngineMap_l_HeightChanged_CalculateBrightness(point);
						}
						else
						{
							EngineMap_l_Transitions_Check(point);
						}
					}
				}
			}

			return true;
		}
	}

	// Original function: FUN_00482614.
	bool __cdecl EngineMap_l_HeightChanged_Callback(std::uint32_t type, std::uintptr_t /*userData*/, std::uintptr_t callbackValueParameter1, std::uintptr_t callbackValueParameter2, std::uintptr_t /*parameter3*/)
	{
		std::uint32_t parameter1 = static_cast<std::uint32_t>(callbackValueParameter1);
		std::uint32_t parameter2 = static_cast<std::uint32_t>(callbackValueParameter2);

		return ChangedCallback(type, parameter1, parameter2, NC2Layout::EngineMapStaticOffset::HeightDirtyBounds, true);
	}

	// Original function: FUN_004829da.
	void EngineMap_Transition_Set(const void* point, std::int32_t transitionId, std::uint32_t layer, bool triangleA)
	{
		auto* macro = const_cast<void*>(EngineMap_GetConstMapMacroPointPtr(point));

		if (macro == nullptr || layer >= 2)
		{

			return;
		}

		std::uint8_t encoded = NXBasics::IntegerRange::UInt8MaximumSigned;

		if (transitionId != -1 && transitionId < gStaticTransitionStorage.count)
		{
			const auto& type = gStaticTransitionStorage.pointTypes[gStaticTransitionStorage.entries[transitionId].pointTypeId];
			Point cursor, micro;
			const void* descriptions[3][6];
			NC2Logic::MapMigPoint_ConstructFromMag(micro, point, false);
			EngineMap_Misc_GetTrianglePatternDescriptions_AroundPoint(micro, descriptions[0]);
			std::memcpy(cursor, point, 4);

			if (triangleA)
			{
				NC2Logic::MapMagPoint_MoveCursorUnwrapped(cursor, 1);
			}
			else
			{
				NC2Logic::MapMagPoint_MoveCursorUnwrappedEast(cursor);
			}

			NC2Logic::MapMigPoint_ConstructFromMag(micro, cursor, false);
			EngineMap_Misc_GetTrianglePatternDescriptions_AroundPoint(micro, descriptions[1]);

			if (triangleA)
			{
				NC2Logic::MapMagPoint_MoveCursorUnwrappedWest(cursor);
			}
			else
			{
				NC2Logic::MapMagPoint_MoveCursorUnwrapped(cursor, 2);
			}

			NC2Logic::MapMigPoint_ConstructFromMag(micro, cursor, false);
			EngineMap_Misc_GetTrianglePatternDescriptions_AroundPoint(micro, descriptions[2]);
			unsigned present = 0;

			for (unsigned corner = 0; corner != 3; ++corner)
			{
				bool found = false;

				for (const void* description : descriptions[corner])
				{
					if (description == nullptr)
						continue;

					const auto* pattern = static_cast<const SE2StaticDataPatternData*>(description);

					for (unsigned i = 0; i < pattern->groupCount && !found; ++i)
					{
						for (unsigned j = 0; j < type.groupCount; ++j)
						{
							if (pattern->groupIds[i] == type.groupIds[j])
							{
								found = true;
								break;
							}
						}
					}

					if (found)
						break;
				}

				if (found)
					present |= 1u << corner;
			}

			if (present != 0 && present != 7)
			{
				// Complete transition-shape table 004F4628..004F465F.
				static constexpr std::int32_t shapes[14] = { -1, 5, 3, 1, 4, 0, 2, -1, 5, 4, 0, 3, 1, 2 };
				encoded = static_cast<std::uint8_t>(transitionId * 6 + shapes[present + (triangleA ? 0 : 7)]);
			}
		}

		const auto offset = 7u + layer * 2u + (triangleA ? 0u : 1u);

		if (Read<std::uint8_t>(macro, offset) != encoded)
		{
			Write(macro, offset, encoded);
			NC2Logic::GameMapModifyManager_GameMapModifiedMacro(0x34, point, 0);
		}
	}

	// Original function: FUN_00482c00.
	bool __cdecl EngineMap_l_Transitions_Callback(std::uint32_t type, std::uintptr_t /*userData*/, std::uintptr_t callbackValueParameter1, std::uintptr_t callbackValueParameter2, std::uintptr_t /*parameter3*/)
	{
		std::uint32_t parameter1 = static_cast<std::uint32_t>(callbackValueParameter1);
		std::uint32_t parameter2 = static_cast<std::uint32_t>(callbackValueParameter2);

		return ChangedCallback(type, parameter1, parameter2, NC2Layout::EngineMapStaticOffset::TransitionDirtyBounds, false);
	}

	// Original function: FUN_00482d30.
	void EngineMap_l_Transitions_Check(const void* point)
	{
		const auto* macro = EngineMap_GetConstMapMacroPointPtr(point);

		for (std::uint32_t layer = 0; layer < 2; ++layer)
		{
			const auto a = Read<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::Transitions + layer * NC2Layout::RecordSize::MacroTransitionLayer);

			if (a != NXBasics::IntegerRange::UInt8MaximumSigned)
				EngineMap_Transition_Set(point, a / 6, layer, true);

			const auto b = Read<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::TransitionB + layer * NC2Layout::RecordSize::MacroTransitionLayer);

			if (b != NXBasics::IntegerRange::UInt8MaximumSigned)
				EngineMap_Transition_Set(point, b / 6, layer, false);
		}
	}

	// Original function: FUN_00482d8a.
	void EngineMap_OverlayPattern_RoadChanged(const void* micro, bool enabled)
	{
		auto* entry = const_cast<void*>(EngineMap_GetConstMapMicroPointPtr(micro));
		Write<std::uint8_t>(entry, NC2Layout::EngineMapMicroPointOffset::Flags, Read<std::uint8_t>(entry, NC2Layout::EngineMapMicroPointOffset::Flags) & NXBasics::BitMask::ByteExceptBits2Through3);

		if (enabled)
			Write<std::uint8_t>(entry, NC2Layout::EngineMapMicroPointOffset::Flags, Read<std::uint8_t>(entry, NC2Layout::EngineMapMicroPointOffset::Flags) | ((NC2Logic::Logic_EntryPoint_Random_GetNumber() & 1u) != 0 ? 4u : 8u));

		EngineMap_l_OverlayPattern_RecalculateMacroGridData(micro);
	}

	// Original function: FUN_00482dc0; clearing a house area consumes no random value.
	void EngineMap_OverlayPattern_HouseAreaChanged(const void* micro, bool enabled)
	{
		auto* entry = const_cast<void*>(EngineMap_GetConstMapMicroPointPtr(micro));
		Write<std::uint8_t>(entry, NC2Layout::EngineMapMicroPointOffset::Flags, Read<std::uint8_t>(entry, NC2Layout::EngineMapMicroPointOffset::Flags) & NXBasics::BitMask::ByteExceptBits4Through5);

		if (enabled)
			Write<std::uint8_t>(entry, NC2Layout::EngineMapMicroPointOffset::Flags, Read<std::uint8_t>(entry, NC2Layout::EngineMapMicroPointOffset::Flags) | ((NC2Logic::Logic_EntryPoint_Random_GetNumber() & 1u) != 0 ? 0x10u : 0x20u));

		EngineMap_l_OverlayPattern_RecalculateMacroGridData(micro);
	}

	// Original function: FUN_00482df6.
	void EngineMap_l_OverlayPattern_RecalculateMacroGridData(const void* micro)
	{
		using namespace NC2Logic;
		Point macro;
		MapMagPoint_ConstructFromMig(macro, micro, false);
		std::int16_t rectangle[4];
		MapMagRectangle_Construct(rectangle, static_cast<std::int16_t>(macro[0] - 2), static_cast<std::int16_t>(macro[1] - 2), 5, 5);
		const std::uint32_t directions[2][5] = { { 2, 0, 1, 3, 3 }, { 0, 0, 2, 3, 1 } };
		const auto lastX = static_cast<std::int16_t>(rectangle[0] + rectangle[2] - 1), lastY = static_cast<std::int16_t>(rectangle[1] + rectangle[3] - 1);

		for (macro[1] = rectangle[1]; macro[1] <= lastY; ++macro[1])
		{
			for (macro[0] = rectangle[0]; macro[0] <= lastX; ++macro[0])
			{
				auto* entry = const_cast<void*>(EngineMap_GetConstMapMacroPointPtr(macro));
				Write<std::uint8_t>(entry, NC2Layout::EngineMapMacroPointOffset::Flags, Read<std::uint8_t>(entry, NC2Layout::EngineMapMacroPointOffset::Flags) & NXBasics::BitMask::ByteExceptBits0Through1Signed);

				for (unsigned triangle = 0; triangle != 2; ++triangle)
				{
					Point cursor;
					MapMigPoint_ConstructFromMag(cursor, macro, false);
					bool present = (Read<std::uint8_t>(EngineMap_GetConstMapMicroPointPtr(cursor), NC2Layout::EngineMapMicroPointOffset::Flags) & NXBasics::BitMask::Bits2Through5) != 0;

					for (auto direction : directions[triangle])
					{
						MapMigPoint_MoveCursorWrapped(cursor, direction);
						present |= (Read<std::uint8_t>(EngineMap_GetConstMapMicroPointPtr(cursor), NC2Layout::EngineMapMicroPointOffset::Flags) & NXBasics::BitMask::Bits2Through5) != 0;
					}

					if (present)
						Write<std::uint8_t>(entry, NC2Layout::EngineMapMacroPointOffset::Flags, Read<std::uint8_t>(entry, NC2Layout::EngineMapMacroPointOffset::Flags) | (1u << triangle));
				}
			}
		}
	}

	// Original function: FUN_0047eaf3; complete dirty region 00554D98..00554D9F.
	bool __cdecl E2Manager_sl_Callback_MapEdited(std::uint32_t type, std::uintptr_t /*userData*/, std::uintptr_t callbackValueParameter1, std::uintptr_t callbackValueParameter2, std::uintptr_t callbackValueParameter3)
	{
		std::uint32_t parameter1 = static_cast<std::uint32_t>(callbackValueParameter1);
		std::uint32_t parameter2 = static_cast<std::uint32_t>(callbackValueParameter2);
		std::uint32_t parameter3 = static_cast<std::uint32_t>(callbackValueParameter3);
		std::int16_t rectangle[4];

		if (type == SwitchIds::CallbackType::BeginMapChanges)
		{
			Write<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MinimumX, 32000);
			Write<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MinimumY, 32000);
			Write<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MaximumX, 0);
			Write<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MaximumY, 0);
		}
		else if (type == SwitchIds::CallbackType::MapPointChanged)
		{
			Point point;
			NC2Logic::MapMigPoint_AssignLong(point, parameter2);
			// Windows 0047EB3D..0047EB41 invalidates the cached image for static landscapes.

			if (parameter1 == 0x33 || parameter1 == 13 || parameter1 == 12 || (parameter1 == 0x32 && parameter3 != 0) || parameter1 == 0x34 || parameter1 == 0x35)
			{
				for (unsigned axis = 0; axis != 2; ++axis)
				{
					if (point[axis] < Read<std::int16_t>(gStaticMapModifiedBounds, axis * NC2Layout::RecordSize::DirtyBoundsCoordinate))
						Write(gStaticMapModifiedBounds, axis * NC2Layout::RecordSize::DirtyBoundsCoordinate, point[axis]);

					if (point[axis] > Read<std::int16_t>(gStaticMapModifiedBounds, axis * NC2Layout::RecordSize::DirtyBoundsCoordinate + NC2Layout::MapDirtyBoundsOffset::MaximumPoint))
						Write(gStaticMapModifiedBounds, axis * NC2Layout::RecordSize::DirtyBoundsCoordinate + NC2Layout::MapDirtyBoundsOffset::MaximumPoint, point[axis]);
				}
			}

			if (parameter1 == 12)
				EngineMap_OverlayPattern_RoadChanged(point, parameter3 != 0);
		}
		else if (type == SwitchIds::CallbackType::EndMapChanges && Read<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MinimumX) != 32000)
		{
			rectangle[0] = static_cast<std::int16_t>(Read<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MinimumX) - 2);
			rectangle[1] = static_cast<std::int16_t>(Read<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MinimumY) - 2);
			rectangle[2] = static_cast<std::int16_t>(Read<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MaximumX) - Read<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MinimumX) + 4);
			rectangle[3] = static_cast<std::int16_t>(Read<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MaximumY) - Read<std::int16_t>(gStaticMapModifiedBounds, NC2Layout::MapDirtyBoundsOffset::MinimumY) + 4);
			NC2Logic::MapMigRectangle_CutInsideMap(rectangle);
			C2DEngineDisplayManager::sTheObjectPtr->InformThatStaticMapChanged(rectangle, NXBasics::IntegerRange::UInt32Maximum);
		}
		else if (type == 13)
		{
			rectangle[0] = static_cast<std::int16_t>(parameter2);
			rectangle[1] = static_cast<std::int16_t>(parameter2 >> 16);
			rectangle[2] = static_cast<std::int16_t>(parameter3);
			rectangle[3] = static_cast<std::int16_t>(parameter3 >> 16);
			NC2Logic::MapMigRectangle_CutInsideMap(rectangle);
			C2DEngineDisplayManager::sTheObjectPtr->InformThatStaticMapChanged(rectangle, parameter1);
		}

		return true;
	}

	// Original function: FUN_004829b4.
	void EngineMap_VertexColorIndex_Set(const void* point, std::int32_t color)
	{
		auto* macro = const_cast<void*>(EngineMap_GetConstMapMacroPointPtr(point));

		if (macro == nullptr)
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(macro, NC2Layout::EngineMapMacroPointOffset::VertexColorIndex, static_cast<std::uint8_t>(color));
		NC2Logic::GameMapModifyManager_GameMapModifiedMacro(0x35, point, 0);
	}

	// Original function: FUN_00481068; rings exclude the supplied outer radius.
	void Engine2d_EntryPoint_Edit_VertexColorIndex_Set(std::uint32_t point, std::int32_t radius, std::int32_t color, bool landOnly)
	{
		using namespace NC2Logic;
		Logic_EntryPoint_Edit_Start();
		auto set = [&]()
		{
			if (landOnly)
			{
				if (MapMagPoint_IsBorderPoint(&point))
					return;

				std::uint32_t micro;
				MapMigPoint_ConstructFromMag(&micro, &point, false);

				if (NC2Lifetime::Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&micro), NC2Layout::MapMicroPointOffset::ObjectType) != 1)
				{

					return;
				}
			}

			EngineMap_VertexColorIndex_Set(&point, color);
		};
		set();

		for (std::int32_t ring = 1; ring < radius; ++ring)
		{
			MapMagPoint_MoveCursorUnwrapped(&point, 4);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::int32_t step = 0; step < ring; ++step)
				{
					MapMagPoint_MoveCursorUnwrapped(&point, direction);

					if (MapMagPoint_IsInsideMap(&point))
						set();
				}
			}
		}

		Logic_EntryPoint_Edit_Stop();
	}
}

namespace NC2LogicExternals
{
	// Original function: FUN_004809ee; micro radius is halved before entering the macro-grid editor.
	void External_VertexColor_SetArea(const void* point, std::uint32_t radius, std::int32_t color, bool landOnly)
	{
		std::uint32_t macro;
		NC2Logic::MapMagPoint_ConstructFromMig(&macro, point, false);
		NC2E2::Engine2d_EntryPoint_Edit_VertexColorIndex_Set(macro, static_cast<std::int32_t>(radius >> 1), color, landOnly);
	}
}
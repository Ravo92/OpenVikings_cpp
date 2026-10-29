#include "C2GameMapStorage.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2GameEngineStartup.h"
#include "C2CallbackManager.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		// Complete callback accumulation region 00512550..00512579.
		std::int16_t gWalkFlagChangedArea[4]{ };
		std::int16_t gMaxMoveableSizeChangedArea[4]{ };
		std::int16_t gContinentChangedArea[4]{ };
		std::uint8_t gContinentsNeedUpdate = 0;
		std::byte gUnknown512569{ };
		std::int16_t gResistanceChangedArea[4]{ };
		std::int16_t gLandscapeValidationChangedArea[4]{ };

		void ResetChangedArea(std::int16_t* area)
		{
			area[3] = 0; area[2] = 0; area[1] = 32000; area[0] = 32000;
		}
		void IncludeChangedPoint(std::int16_t* area, std::uint32_t packed)
		{
			const auto x = Read<std::int16_t>(&packed, NC2Layout::MapCoordinatePairOffset::X), y = Read<std::int16_t>(&packed, NC2Layout::MapCoordinatePairOffset::Y);
			if (x < area[0]) area[0] = x;
			if (x > area[2]) area[2] = x;
			if (y < area[1]) area[1] = y;
			if (y > area[3]) area[3] = y;
		}
		void ExpandChangedArea(std::int16_t* area, std::int32_t lower, std::int32_t upper)
		{
			area[0] = static_cast<std::int16_t>(area[0] - lower); area[1] = static_cast<std::int16_t>(area[1] - lower);
			MapMigPoint_ValidatePosition(area);
			area[2] = static_cast<std::int16_t>(area[2] + upper); area[3] = static_cast<std::int16_t>(area[3] + upper);
			MapMigPoint_ValidatePosition(area + 2);
		}
		template<class F> void VisitChangedArea(const std::int16_t* area, F visit)
		{
			std::int16_t point[2] = { 0, area[1] };
			for (; point[1] <= area[3]; ++point[1])
				for (point[0] = area[0]; point[0] <= area[2]; ++point[0]) visit(point);
		}
		std::uint32_t ContinentTypeUnchecked(std::uint32_t continent)
		{
			return Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::ContinentsUnsigned + continent * NC2Layout::RecordSize::GameMapContinentUnsigned);
		}
	}

	// Original function: FUN_00424AE9.
	bool __cdecl GameMap_l_ValidateLandscapes_Callback(TCallbackTypes type, std::uintptr_t, std::uintptr_t callbackValueModification, std::uintptr_t callbackValuePoint, std::uintptr_t)
	{
		std::uint32_t modification = static_cast<std::uint32_t>(callbackValueModification);
		std::uint32_t point = static_cast<std::uint32_t>(callbackValuePoint);
		if (type == 0x31) ResetChangedArea(gLandscapeValidationChangedArea);
		else if (type == 0x33 && (modification == 10 || modification == 11)) IncludeChangedPoint(gLandscapeValidationChangedArea, point);
		else if (type == 0x32 && gLandscapeValidationChangedArea[0] != 32000)
		{
			ExpandChangedArea(gLandscapeValidationChangedArea, 35, 35);
			VisitChangedArea(gLandscapeValidationChangedArea, [] (const void* position)
			{
				void* micro = GameMap_GetMapMicroPointPtr(position);
				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0) return;
				const auto engine = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(position);
				const auto valency = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0u;
				const auto* area = NC2LogicExternals::External_Landscape_GetAreaPtr(engine, valency, 1);
				const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) : 0u;
				if (!GameMap_Misc_IsLandscapeAllowedOnPattern(position, landscape)) GameMap_MapMicroPoint_Landscape_Clear(position);
				else if (area != nullptr)
				{
					const auto currentLandscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) : 0u;
					if (!area->Check_CanLandscapeBeSetOnPattern(position, currentLandscape)) GameMap_MapMicroPoint_Landscape_Clear(position);
				}
			});
		}
		return true;
	}

	// Original function: FUN_00450C1D.
	bool __cdecl GameMap_l_MaxMoveableSize_Callback(TCallbackTypes type, std::uintptr_t, std::uintptr_t callbackValueModification, std::uintptr_t callbackValuePoint, std::uintptr_t)
	{
		std::uint32_t modification = static_cast<std::uint32_t>(callbackValueModification);
		std::uint32_t point = static_cast<std::uint32_t>(callbackValuePoint);
		if (type == 0x31) ResetChangedArea(gMaxMoveableSizeChangedArea);
		else if (type == 0x33 && (modification == 14 || modification == 11)) IncludeChangedPoint(gMaxMoveableSizeChangedArea, point);
		else if (type == 0x32 && gMaxMoveableSizeChangedArea[0] != 32000)
		{
			ExpandChangedArea(gMaxMoveableSizeChangedArea, 10, 10);
			VisitChangedArea(gMaxMoveableSizeChangedArea, GameMap_l_MaxMoveableSize_UpdatePoint);
		}
		return true;
	}

	// Original function: FUN_00450D4F.
	void GameMap_l_MaxMoveableSize_UpdatePoint(const void* position)
	{
		if (MapMigPoint_IsBorderPoint(position)) return;
		auto* micro = GameMap_GetMapMicroPointPtr(position);
		std::uint32_t size = 0;
		const auto continent = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId);
		if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 && continent != 0)
		{
			size = 1;
			auto neighbor = Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates);
			MapMigPoint_MoveCursorUnwrapped(neighbor, 4);
			// Windows repeats this same neighbor read six times; it does not advance the cursor here.
			for (std::uint32_t count = 0; count < 6; ++count)
				if (MapMigPoint_IsInsideMap(&neighbor))
				{
					const auto candidate = Read<std::uint8_t>(GameMap_GetMapMicroPointPtr(&neighbor), NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u;
					if (candidate > size) size = candidate;
				}
			--size;
			for (auto candidate = size; candidate <= 7; ++candidate)
			{
				if (!GameMap_l_MaxMoveableSize_CheckForSize(Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates), candidate, continent)) break;
				size = candidate;
			}
		}
		Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags, (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::ByteExceptBits0Through2) | (size & 7u));
	}

	// Original function: FUN_00450E08; the continent-type load has no effect on the predicate in Windows.
	bool GameMap_l_MaxMoveableSize_CheckForSize(std::uint32_t point, std::uint32_t size, std::uint32_t continent)
	{
		if (size == 0) return true;
		const volatile std::uint32_t continentType = ContinentTypeUnchecked(continent);
		(void) continentType;
		MapMigPoint_MoveCursorUnwrapped(&point, 4, size);
		for (std::uint32_t direction = 0; direction < 6; ++direction)
			for (std::uint32_t step = 0; step < size; ++step)
			{
				if (!MapMigPoint_IsInsideMap(&point)) return false;
				const auto* micro = GameMap_GetConstMapMicroPointPtr(&point);
				if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) != continent) return false;
				MapMigPoint_MoveCursorUnwrapped(point, direction);
			}
		return true;
	}

	// Original function: FUN_00450E8C.
	bool __cdecl GameMap_l_WalkFlagSet_Callback(TCallbackTypes type, std::uintptr_t, std::uintptr_t callbackValueModification, std::uintptr_t callbackValuePoint, std::uintptr_t)
	{
		std::uint32_t modification = static_cast<std::uint32_t>(callbackValueModification);
		std::uint32_t point = static_cast<std::uint32_t>(callbackValuePoint);
		if (type == 0x31) ResetChangedArea(gWalkFlagChangedArea);
		else if (type == 0x33 && (modification == 11 || modification == 12)) IncludeChangedPoint(gWalkFlagChangedArea, point);
		else if (type == 0x32 && gWalkFlagChangedArea[0] != 32000)
		{
			ExpandChangedArea(gWalkFlagChangedArea, 5, 5);
			VisitChangedArea(gWalkFlagChangedArea, GameMap_l_WalkFlagSet_Update);
		}
		return true;
	}

	// Original function: FUN_00450FBE.
	void GameMap_l_WalkFlagSet_Update(const void* point)
	{
		if (!MapMigPoint_IsInsideMap(point)) return;
		GameMap_l_WalkFlagSet_ClearConnections(Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		GameMap_l_WalkFlagSet_CalculateConnections(point);
	}

	// Original function: FUN_00450FEE.
	void GameMap_l_WalkFlagSet_ClearConnections(std::uint32_t point)
	{
		if (MapMigPoint_IsBorderPoint(&point)) return;
		Write<std::uint8_t>(GameMap_GetMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::WalkingDirections, 0);
		MapMigPoint_MoveCursorUnwrapped(point, 4);
		for (std::uint32_t direction = 0; direction < 6; ++direction)
		{
			// An outside-map cursor does not advance in this native loop.
			if (!MapMigPoint_IsInsideMap(&point)) continue;
			auto* micro = GameMap_GetMapMicroPointPtr(&point);
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) & ~(1u << ((direction + 1u) % 6u)));
			MapMigPoint_MoveCursorUnwrapped(point, direction);
		}
	}

	// Original function: FUN_00451059.
	void GameMap_l_WalkFlagSet_CalculateConnections(const void* point)
	{
		if (MapMigPoint_IsBorderPoint(point)) return;
		for (std::uint32_t direction = 0; direction < 6; ++direction)
		{
			auto neighbor = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
			MapMigPoint_MoveCursorUnwrapped(neighbor, direction);
			if (!MapMigPoint_IsInsideMap(&neighbor) || MapMigPoint_IsBorderPoint(&neighbor)) continue;
			const void* wayPatterns[2];
			if (!GameMap_Misc_GetTrianglePatternDescriptions_AroundWay(point, direction, wayPatterns)) continue;
			if (Read<std::uint8_t>(wayPatterns[0], NC2Layout::TrianglePatternTypeOffset::HumanCanWalkOn) == 0 && Read<std::uint8_t>(wayPatterns[1], NC2Layout::TrianglePatternTypeOffset::HumanCanWalkOn) == 0)
			{
				const void* patterns[6];
				if (!GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(point, patterns)) continue;
				bool water = true;
				for (const auto* pattern : patterns) if (Read<std::uint8_t>(pattern, NC2Layout::TrianglePatternTypeOffset::IsWater) == 0) water = false;
				if (!water || !GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(&neighbor, patterns)) continue;
				for (const auto* pattern : patterns) if (Read<std::uint8_t>(pattern, NC2Layout::TrianglePatternTypeOffset::IsWater) == 0) water = false;
				if (!water) continue;
			}
			GameMap_l_WalkFlagSet_SetConnection(Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates), direction);
		}
	}

	// Original function: FUN_00451183.
	void GameMap_l_WalkFlagSet_SetConnection(std::uint32_t point, std::uint32_t direction)
	{
		auto* micro = GameMap_GetMapMicroPointPtr(&point);
		Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) | (1u << direction));
		MapMigPoint_MoveCursorUnwrapped(point, direction);
		micro = GameMap_GetMapMicroPointPtr(&point);
		Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) | (1u << VE_HexagonDirection_OppositeDirection(direction)));
	}

	// Original function: FUN_004511C5.
	bool __cdecl GameMap_l_ResistanceSet_Callback(TCallbackTypes type, std::uintptr_t, std::uintptr_t callbackValueModification, std::uintptr_t callbackValuePoint, std::uintptr_t)
	{
		std::uint32_t modification = static_cast<std::uint32_t>(callbackValueModification);
		std::uint32_t point = static_cast<std::uint32_t>(callbackValuePoint);
		if (type == 0x31) ResetChangedArea(gResistanceChangedArea);
		else if (type == 0x33 && (modification == 11 || modification == 12)) IncludeChangedPoint(gResistanceChangedArea, point);
		else if (type == 0x32 && gResistanceChangedArea[0] != 32000)
		{
			ExpandChangedArea(gResistanceChangedArea, 5, 5);
			VisitChangedArea(gResistanceChangedArea, GameMap_l_ResistanceSet_Calculate);
		}
		return true;
	}

	// Original function: FUN_004512F7.
	void GameMap_l_ResistanceSet_Calculate(const void* point)
	{
		if (!MapMigPoint_IsInsideMap(point)) return;
		auto* micro = GameMap_GetMapMicroPointPtr(point);
		const auto continent = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId);
		if (continent != 0 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & 2u) != 0)
		{
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags, (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::ByteExceptBits4Through6) | 8u);
			return;
		}
		const auto continentType = continent == 0 ? 0u : ContinentTypeUnchecked(continent);
		const void* patterns[6];
		if (continent == 0 || !GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(point, patterns))
		{
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::ByteExceptBits3Through6);
			return;
		}
		const bool land = continentType == 1;
		std::uint32_t sum = 0, count = 0;
		for (const auto* pattern : patterns)
			if (!land || Read<std::uint8_t>(pattern, NC2Layout::TrianglePatternTypeOffset::IsLand) != 0)
			{
				sum += Read<std::uint32_t>(pattern, NC2Layout::TrianglePatternTypeOffset::MoveResistance); ++count;
			}
		if (count == 0)
		{
			Write<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::ContinentsUnsigned + continent * NC2Layout::RecordSize::GameMapContinentUnsigned, 0);
			return;
		}
		Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags, (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::ByteExceptBits3Through6) | (((sum / count) & 15u) << 3));
	}

	// Original function: FUN_004513C1.
	bool __cdecl GameMap_l_Continents_Callback(TCallbackTypes type, std::uintptr_t, std::uintptr_t callbackValueModification, std::uintptr_t callbackValuePoint, std::uintptr_t)
	{
		std::uint32_t modification = static_cast<std::uint32_t>(callbackValueModification);
		std::uint32_t point = static_cast<std::uint32_t>(callbackValuePoint);
		if (type == 0x31)
		{
			ResetChangedArea(gContinentChangedArea); gContinentsNeedUpdate = 0;
		}
		else if (type == 0x33 && modification == 11)
		{
			IncludeChangedPoint(gContinentChangedArea, point); gContinentsNeedUpdate = 1;
		}
		else if (type == 0x32 && gContinentsNeedUpdate != 0 && gContinentChangedArea[0] != 32000)
		{
			ExpandChangedArea(gContinentChangedArea, 5, 6);
			const auto area = static_cast<std::uint32_t>((static_cast<std::int32_t>(gContinentChangedArea[0]) - gContinentChangedArea[2]) * (static_cast<std::int32_t>(gContinentChangedArea[1]) - gContinentChangedArea[3]));
			const auto limit = (Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 3u) >> 2;
			if (area > limit) GameMap_l_Continents_RecalculateAll();
			else GameMap_l_Continents_UpdateArea(gContinentChangedArea, gContinentChangedArea + 2);
		}
		return true;
	}

	// Original function: FUN_00411bcc
	void GameMapModifyManager_StartGameMapModify()
	{
		if (++gGameMapModifyNestingDepth == 1) CCallbackManager::sTheObjectPtr->ExecuteCallback(0x31, false, 0, 0, 0);
	}

	// Original function: FUN_00411bef
	void GameMapModifyManager_StopGameMapModify()
	{
		if (gGameMapModifyNestingDepth == 0) return;
		if (gGameMapModifyNestingDepth == 1) CCallbackManager::sTheObjectPtr->ExecuteCallback(0x32, true, 0, 0, 0);
		--gGameMapModifyNestingDepth;
	}

	// Original function: FUN_00411c18
	void GameMapModifyManager_ForceUpdate()
	{
		if (gGameMapModifyNestingDepth == 0) return;
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x32, true, 0, 0, 0);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x31, false, 0, 0, 0);
	}
}

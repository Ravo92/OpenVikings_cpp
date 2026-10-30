#include "C2DataIds.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2PlayerCenterPosition.h"
#include "InGameGuiManager.h"
#include "WorldDisplayElementAccess.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"

#include <cstddef>
#include <new>

namespace NC2Logic
{
	// Original function: FUN_00439e61.
	bool Tool_Misc_GetPlayerCenterPosition(std::uint32_t playerId, std::uint32_t* mapPoint)
	{
		using namespace NC2Lifetime;
		const auto houseAt = [](std::int32_t id)
		{

			return CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;
		};
		CHouseIterator preferredHouses;

		for (auto id = preferredHouses.GetFirstHouseId(playerId); id != -1; id = preferredHouses.GetNextHouseId())
		{
			const auto* house = houseAt(id);
			const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

			if (type == 7 || type == 8 || type == 9 || type == 1)
			{
				*mapPoint = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint);
				return true;
			}
		}

		std::uint32_t sumX = 0, sumY = 0, count = 0, continent = NXBasics::IntegerRange::UInt32Maximum;
		bool firstHouse = true;
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(playerId); id != -1; id = houses.GetNextHouseId())
		{
			const auto* position = houseAt(id) + 0x1C;
			sumX += static_cast<std::uint32_t>(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X));
			sumY += static_cast<std::uint32_t>(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y));
			++count;

			if (firstHouse)
			{
				continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId);
				firstHouse = false;
			}
		}

		bool firstHuman = continent == NXBasics::IntegerRange::UInt32Maximum;
		CHumanIterator humans;

		for (auto id = humans.GetFirstHumanId(playerId); id != -1; id = humans.GetNextHumanId())
		{
			const auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
			const auto* moveable = static_cast<const std::byte*>(gMapMoveableArrayStorage.entries) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;

			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0)
				continue;

			const auto* position = moveable + 0x12;
			sumX += static_cast<std::uint32_t>(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X));
			sumY += static_cast<std::uint32_t>(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y));
			++count;

			if (firstHuman)
			{
				continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId);
				firstHuman = false;
			}
		}

		if (count == 0)
			return false;
		// Windows DIV treats the wrapped sums as unsigned, then narrows to WORDs.
		*mapPoint = static_cast<std::uint16_t>(sumX / count) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(sumY / count)) << 16);
		const auto acceptable = [&](const void* point)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

			return (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) == continent;
		};

		if (acceptable(mapPoint))
			return true;

		const auto limit = static_cast<std::uint32_t>(GameMap_GetWidth() > GameMap_GetHeight() ? GameMap_GetWidth() : GameMap_GetHeight());

		for (std::uint32_t radius = 1; radius < limit; ++radius)
		{
			MapMigPoint_MoveCursorUnwrapped(mapPoint, 4, 1);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::uint32_t step = 0; step < radius; ++step)
				{
					MapMigPoint_MoveCursorUnwrapped(mapPoint, direction, 1);

					if (MapMigPoint_IsInsideMap(mapPoint) && acceptable(mapPoint))
						return true;
				}
			}
		}

		return false;
	}
}

namespace NC2InGameGuiEntryPoints
{
	// Original function: FUN_004a2f64.
	void C2IGG_EntryPoint_MainDisplay_SetToMapPosition(const void* mapPoint)
	{
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(manager->GetDesktop());
		void* display = NC2Lifetime::Read<void*>(manager->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display);
		NC2E2::DE_SetWantedPositionToMapPoint(display, static_cast<const std::uint32_t*>(mapPoint));
	}

	// Original function: FUN_004a2f86.
	void C2IGG_EntryPoint_MainDisplay_SetControllingPlayerCenterPosition()
	{
		std::uint32_t mapPoint = 0;
		const auto playerId = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();

		if (C2IGG_EntryPoint_PlayerCenterPosition_Get(static_cast<std::uint32_t>(playerId), &mapPoint))
			C2IGG_EntryPoint_MainDisplay_SetToMapPosition(&mapPoint);
	}

	namespace
	{
		// Complete Windows lazy region 005690D0..005690D7; no owning allocation.
		struct SPlayerCenterTokens
		{
			alignas(NXBasics::CTokenList) std::byte tokenList[sizeof(NXBasics::CTokenList)];
			std::uint8_t initFlags;
			std::byte padding[3];
		};

		SPlayerCenterTokens gPlayerCenterTokens{ };
		static_assert(sizeof(SPlayerCenterTokens) == (sizeof(void*) == 8 ? 16 : 8));
		const NXBasics::SToken gPlayerCenterTokenTable[] = { { "startposition", NC2DataIds::PlayerCenterPositionToken::StartPosition }, { nullptr, 0 } };

		// Original function: FUN_004a307b; empty exit callback, return ignored.
		int __cdecl PlayerCenterTokens_OnExit()
		{

			return 0;
		}
	}

	// Original function: FUN_004a2fb7.
	bool C2IGG_EntryPoint_PlayerCenterPosition_Get(std::uint32_t playerId, std::uint32_t* mapPoint)
	{
		auto* reader = NC2Logic::gGameStaticVars.retainedMapIni;

		if (reader != nullptr && reader->SearchGroup("misc_startpositions"))
		{
			while (reader->GetNextCommandLineInGroup() != nullptr)
			{
				if ((gPlayerCenterTokens.initFlags & 1) == 0)
				{
					gPlayerCenterTokens.initFlags |= 1;
					::new (gPlayerCenterTokens.tokenList) NXBasics::CTokenList(gPlayerCenterTokenTable);
					NXBasics::Runtime_AtExit(PlayerCenterTokens_OnExit);
				}

				auto* tokens = reinterpret_cast<NXBasics::CTokenList*>(gPlayerCenterTokens.tokenList);

				if (tokens->FindToken(reader->GetCurrentName()) != NC2DataIds::PlayerCenterPositionToken::StartPosition)
					continue;

				const std::uint32_t entryPlayer = static_cast<std::uint32_t>(reader->GetInt());
				const std::int32_t x = reader->GetInt();
				const std::int32_t y = reader->GetInt();

				if (entryPlayer == playerId)
				{
					// SMapMigPoint::Set(x,y,false) stores the two low words.
					*mapPoint = static_cast<std::uint16_t>(x) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(y)) << 16);
					return true;
				}
			}
		}

		return NC2Logic::Tool_Misc_GetPlayerCenterPosition(playerId, mapPoint);
	}
}
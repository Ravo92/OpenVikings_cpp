#include "ApplicationGlobals.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2GuideAccess.h"
#include "C2HouseAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticHouseTypeData.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* Guide(std::int32_t id)
		{

			return CGuideArray::mStaticVars + static_cast<std::intptr_t>(id) * CGuideArray::RuntimeElementSize;
		}

		const void* Position(const void* guide)
		{

			return static_cast<const std::byte*>(guide) + NC2Layout::GuideOffset::MapPosition;
		}

		std::int32_t Distance(const void* a, const void* b)
		{

			return VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::Y));
		}

		void AddGood(void* guide, std::uint32_t good, std::uint32_t amount)
		{
			Write(guide, NC2Layout::GuideOffset::GoodsCountsUnsigned + good * NC2Layout::RecordSize::GuideGoodCountUnsigned, Read<std::uint32_t>(guide, NC2Layout::GuideOffset::GoodsCountsUnsigned + good * NC2Layout::RecordSize::GuideGoodCountUnsigned) + amount);
		}

		// Original function: FUN_0041b137.
		bool __cdecl slConnectGuidesAndSetGoods(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto* guide = callback->userData;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			const auto kind = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);

			if (kind == SwitchIds::MapMicroObjectType::Guide)
			{
				const auto id = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId);
				auto* other = Guide(id);

				if (id != Read<std::int32_t>(guide, NC2Layout::GuideOffset::GuideId) && Read<std::uint32_t>(other, NC2Layout::GuideOffset::PlayerId) == Read<std::uint32_t>(guide, NC2Layout::GuideOffset::PlayerId) && static_cast<std::uint32_t>(Distance(Position(other), Position(guide))) < 40u)
					Guide_AddConnectionToGuide(guide, id);
			}
			else if (kind == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0)
			{
				if (static_cast<std::uint32_t>(Distance(point, Position(guide))) >= 40u)
					return false;

				const auto good = gStaticDependencyGoodByLandscape[Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData)];

				if (good != 0)
					AddGood(guide, static_cast<std::uint32_t>(good), Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency));
			}
			else if (kind == SwitchIds::MapMicroObjectType::House)
			{
				if (static_cast<std::uint32_t>(Distance(point, Position(guide))) >= 40u)
					return false;

				auto* house = CHouseArray::mStaticVars + Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId) * CHouseArray::RuntimeElementSize;

				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || House_GetHouseMainType(house) == 2 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != Read<std::uint32_t>(guide, NC2Layout::GuideOffset::PlayerId))
					return false;

				const auto* data = House_GetHouseTypeDataPtr(house);

				for (std::uint32_t index = 0; index < data->stockCount; ++index)
				{
					AddGood(guide, data->stock[index].goodType, House_Stock_RequestAmountOfGoods(house, data->stock[index].goodType, false));
				}
			}

			return false;
		}

		// Original function: FUN_004476f6. This scan omits distance filtering and
		// excludes flagged stock slots; connection recalculation uses a distinct leaf.
		bool __cdecl slScanGoodsAround(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto* guide = callback->userData;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			const auto kind = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);

			if (kind == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0)
			{
				const auto good = gStaticDependencyGoodByLandscape[Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData)];

				if (good != 0)
					AddGood(guide, static_cast<std::uint32_t>(good), Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency));
			}
			else if (kind == SwitchIds::MapMicroObjectType::House)
			{
				auto* house = CHouseArray::mStaticVars + Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId) * CHouseArray::RuntimeElementSize;

				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || House_GetHouseMainType(house) == 2 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != Read<std::uint32_t>(guide, NC2Layout::GuideOffset::PlayerId))
					return false;

				const auto* data = House_GetHouseTypeDataPtr(house);

				for (std::uint32_t index = 0; index < data->stockCount; ++index)
				{
					if (data->stock[index].isOutputStock == 0)
						AddGood(guide, data->stock[index].goodType, House_Stock_RequestAmountOfGoods(house, data->stock[index].goodType, true));
				}
			}

			return false;
		}
	}

	// Original function: FUN_0041b061.
	bool __fastcall GuideManager_InsertGuideAtPosition(void* manager, void*, std::uint32_t player, const void* point, std::uint32_t flags)
	{
		if (!Tool_Guide_IsAllowedToBeBuild(player, point, (flags & NXBasics::IntegerRange::UInt8Maximum) != 0))
			return false;

		const auto id = CGuideArray::sTheObjectPtr->AllocateGuide(player, point);

		if (id == -1)
			return false;

		GuideManager_RecalculateGuideConnections(manager, id);

		return true;
	}

	// Original function: FUN_0041b0a5.
	void __fastcall GuideManager_RemoveGuide(void* manager, void*, std::uint32_t id)
	{
		auto* guide = Guide(static_cast<std::int32_t>(id));
		Guide_DetachFromMap(guide);
		Guide_SetLinkIdInvalid(guide);
		Write<std::int32_t>(guide, NC2Layout::GuideOffset::GuideId, -1);

		while (Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount) != 0)
			GuideManager_RecalculateGuideConnections(manager, Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionGuideIds));

		CGuideArray::sTheObjectPtr->FreeGuide(static_cast<std::int32_t>(id));
	}

	// Original function: FUN_0041b0f1; goods are not cleared in this routine.
	void GuideManager_RecalculateGuideConnections(void* manager, std::int32_t id)
	{
		auto* guide = Guide(id);
		Guide_DisconnectFromGuides(guide);
		CMapPathfinderElement::SearchOverMap(Position(guide), 40, true, slConnectGuidesAndSetGoods, guide, true, Read<std::int32_t>(guide, NC2Layout::GuideOffset::PlayerId));
		const auto linkId = Read<std::uint32_t>(manager, NC2Layout::GuideManagerOffset::NextLinkSystemId);
		Guide_SetLinkId(guide, linkId);
		Write(manager, NC2Layout::GuideManagerOffset::NextLinkSystemId, linkId + 1u);
	}

	// Original function: FUN_0041b936; the map-change passes retain Y/X scan order.
	bool __cdecl GuideManager_l_Guides_Callback(TCallbackTypes type, std::uintptr_t, std::uintptr_t callbackValueParameter1, std::uintptr_t callbackValueParameter2, std::uintptr_t)
	{
		std::uint32_t parameter1 = static_cast<std::uint32_t>(callbackValueParameter1);
		std::uint32_t parameter2 = static_cast<std::uint32_t>(callbackValueParameter2);
		auto& vars = gGuideManagerStaticVars;

		if (type == 0x33)
		{
			if (parameter1 != 14 && parameter1 != 11)
				return true;

			std::uint32_t point;
			MapMigPoint_AssignLong(&point, parameter2);

			for (std::uint32_t axis = 0; axis < 2; ++axis)
			{
				const auto value = Read<std::int16_t>(&point, axis * NC2Layout::RecordSize::MapCoordinateUnsigned);

				if (value < vars.modifiedAreaMinimum[axis])
					vars.modifiedAreaMinimum[axis] = value;

				if (value > vars.modifiedAreaMaximum[axis])
					vars.modifiedAreaMaximum[axis] = value;
			}
		}
		else if (type == 0x31)
		{
			vars.modifiedAreaMinimum[0] = vars.modifiedAreaMinimum[1] = 0x7D00;
			vars.modifiedAreaMaximum[0] = vars.modifiedAreaMaximum[1] = 0;
		}
		else if (type == 0x32)
		{
			if (vars.modifiedAreaMinimum[0] == 0x7D00)
				return true;

			for (std::uint32_t axis = 0; axis < 2; ++axis)
			{
				vars.modifiedAreaMinimum[axis] = static_cast<std::int16_t>(vars.modifiedAreaMinimum[axis] - 40);
			}

			MapMigPoint_ValidatePosition(vars.modifiedAreaMinimum);

			for (std::uint32_t axis = 0; axis < 2; ++axis)
			{
				vars.modifiedAreaMaximum[axis] = static_cast<std::int16_t>(vars.modifiedAreaMaximum[axis] + 40);
			}

			MapMigPoint_ValidatePosition(vars.modifiedAreaMaximum);

			for (std::uint32_t pass = 0; pass < 2; ++pass)
			{
				for (std::int16_t y = vars.modifiedAreaMinimum[1]; y <= vars.modifiedAreaMaximum[1]; ++y)
				{
					for (std::int16_t x = vars.modifiedAreaMinimum[0]; x <= vars.modifiedAreaMaximum[0]; ++x)
					{
						const std::int16_t point[2] = { x, y };
						const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

						if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::Guide)
							continue;

						const auto id = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId);

						if (pass == 0)
						{
							Guide_SetLinkIdInvalid(Guide(id));
							Guide_DisconnectFromGuides(Guide(id));
						}
						else
						{
							GuideManager_RecalculateGuideConnections(gGuideManager, id);
						}
					}
				}
			}
		}
		else if (type == 1 && parameter1 % 12u == 0)
		{
			vars.scanGuideId = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(vars.scanGuideId) + 1u);

			while (vars.scanGuideId <= CGuideArray::sTheObjectPtr->mLastScanIndex)
			{
				auto* guide = Guide(vars.scanGuideId);

				if (Read<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag) != 0)
				{
					Guide_ScanForGoods(guide);
					return true;
				}

				vars.scanGuideId = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(vars.scanGuideId) + 1u);
			}

			vars.scanGuideId = -1;
		}

		return true;
	}

	// Original function: FUN_00426885.
	std::int32_t CGuideArray::AllocateGuide(std::uint32_t player, const void* point)
	{
		const auto id = Element_Allocate();

		if (id == -1)
			return -1;

		if (!Guide_Init(Guide(id), id, player, point))
		{
			FreeGuide(id);
			return -1;
		}

		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x0C, true, static_cast<std::uint32_t>(id), 1, 0);

		return id;
	}

	// Original function: FUN_004268dd; notify before freeing the slot and detaching.
	void CGuideArray::FreeGuide(std::int32_t id)
	{
		auto* guide = Guide(id);

		if (Read<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag) == 0)
		{

			return;
		}

		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x0C, false, static_cast<std::uint32_t>(id), 0, 0);

		if (Element_Free(id, true))
			Guide_Exit(guide);
	}

	// Original function: FUN_00447283. Windows excludes a guide at packed zero
	// from its spacing comparison; retain this native exception.
	bool Tool_Guide_IsAllowedToBeBuild(std::uint32_t player, const void* point, bool checkSpacing)
	{
		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

		if (!MapMigPoint_IsInsideMap(point) || GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId)) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) == 0)
			return false;

		const auto kind = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);

		if (kind == SwitchIds::MapMicroObjectType::House || kind == SwitchIds::MapMicroObjectType::Guide || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & 1u) != 0)
			return false;

		if (!checkSpacing)
			return true;

		auto* array = CGuideArray::sTheObjectPtr;

		for (auto id = array->mFirstScanIndex; id <= array->mLastScanIndex; ++id)
		{
			auto* guide = Guide(id);

			if (Read<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(guide, NC2Layout::GuideOffset::PlayerId) != player)
				continue;

			std::uint32_t zero;
			MapMigPoint_AssignLong(&zero, 0);

			if (Read<std::uint32_t>(Position(guide), NC2Layout::MapCoordinatePairOffset::Coordinates) != zero && static_cast<std::uint32_t>(Distance(point, Position(guide))) < 16u)
				return false;
		}

		return true;
	}

	// Original function: FUN_0044737d.
	bool Guide_AddConnectionToGuide(void* guide, std::int32_t id)
	{
		if (Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount) >= NC2Runtime::GuideRoutingLimits::ConnectionsPerGuide)
			return false;

		auto* other = Guide(id);

		if (!Guide_TryConnectingFromGuide(other, Read<std::int32_t>(guide, NC2Layout::GuideOffset::GuideId)))
			return false;

		const auto count = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::ConnectionCount);
		Write(guide, NC2Layout::GuideOffset::ConnectionGuideIdsUnsigned + count * NC2Layout::RecordSize::GuideConnectionUnsigned, id);
		Write(guide, NC2Layout::GuideOffset::ConnectionsUnsigned + count * NC2Layout::RecordSize::GuideConnectionUnsigned, Distance(Position(other), Position(guide)));
		Write(guide, NC2Layout::GuideOffset::ConnectionCount, count + 1u);

		return true;
	}

	// Original function: FUN_004473e9; a full neighbour replaces its furthest link.
	bool Guide_TryConnectingFromGuide(void* guide, std::int32_t id)
	{
		const auto distance = Distance(Position(Guide(id)), Position(guide));
		const auto count = Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount);

		if (count < NC2Runtime::GuideRoutingLimits::ConnectionsPerGuide)
		{
			Write(guide, NC2Layout::GuideOffset::ConnectionGuideIdsUnsigned + static_cast<std::uint32_t>(count) * NC2Layout::RecordSize::GuideConnectionUnsigned, id);
			Write(guide, NC2Layout::GuideOffset::ConnectionsUnsigned + static_cast<std::uint32_t>(count) * NC2Layout::RecordSize::GuideConnectionUnsigned, distance);
			Write(guide, NC2Layout::GuideOffset::ConnectionCount, static_cast<std::uint32_t>(count) + 1u);
			return true;
		}

		std::int32_t selected = -1, maximum = distance;

		for (std::int32_t index = 0; index < count; ++index)
		{
			const auto current = Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionsUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::GuideConnectionUnsigned);

			if (current > distance && current > maximum)
			{
				selected = index;
				maximum = current;
			}
		}

		if (selected == -1)
			return false;

		const auto offset = NC2Layout::GuideOffset::ConnectionsUnsigned + static_cast<std::uint32_t>(selected) * NC2Layout::RecordSize::GuideConnectionUnsigned;
		Guide_RemoveConnectionToGuide(Guide(Read<std::int32_t>(guide, offset + NC2Layout::GuideConnectionOffset::GuideId)), Read<std::int32_t>(guide, NC2Layout::GuideOffset::GuideId));
		Write(guide, offset, distance);
		Write(guide, offset + NC2Layout::GuideConnectionOffset::GuideId, id);

		return true;
	}

	// Original function: FUN_004474ae; native preconditions require a matching
	// reciprocal link. The count decrement also occurs when no match is found.
	void Guide_RemoveConnectionToGuide(void* guide, std::int32_t id)
	{
		const auto count = Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount);

		for (std::int32_t index = 0; index < count; ++index)
		{
			if (Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionGuideIdsUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::GuideConnectionUnsigned) != id)
				continue;

			auto* destination = static_cast<std::byte*>(guide) + NC2Layout::GuideOffset::ConnectionsUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::GuideConnectionUnsigned;
			std::memmove(destination, destination + NC2Layout::RecordSize::GuideConnection, static_cast<std::uint32_t>(count - index - 1) * NC2Layout::RecordSize::GuideConnectionUnsigned);
			break;
		}

		Write(guide, NC2Layout::GuideOffset::ConnectionCount, static_cast<std::uint32_t>(count) - 1u);
	}

	// Original function: FUN_004474f9.
	void Guide_DisconnectFromGuides(void* guide)
	{
		for (std::int32_t index = 0; index < Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount); ++index)
		{
			Guide_RemoveConnectionToGuide(Guide(Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionGuideIdsUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::GuideConnectionUnsigned)), Read<std::int32_t>(guide, NC2Layout::GuideOffset::GuideId));
		}

		Write<std::uint32_t>(guide, NC2Layout::GuideOffset::ConnectionCount, 0);
	}

	// Original function: FUN_0044752f.
	void Guide_Exit(void* guide)
	{
		Guide_DisconnectFromGuides(guide);
		Guide_DetachFromMap(guide);
		Write<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag, 0);
	}

	// Original function: FUN_00447543; allocation is marked before validation.
	bool Guide_Init(void* guide, std::int32_t id, std::uint32_t player, const void* point)
	{
		if (Read<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag) != 0)
			return false;

		std::memset(guide, 0, CGuideArray::ElementSize);
		Write<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag, 1);

		if (!Tool_Guide_IsAllowedToBeBuild(player, point, false))
			return false;

		Write(guide, NC2Layout::GuideOffset::GuideId, id);
		Write(guide, NC2Layout::GuideOffset::MapPosition, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write(guide, NC2Layout::GuideOffset::PlayerId, player);
		const auto revision = Game_UniqueId_Generate();
		Write<std::int32_t>(guide, NC2Layout::GuideOffset::LinkSystemId, -1);
		Write(guide, NC2Layout::GuideOffset::UniqueId, revision);
		GameMapModifyManager_StartGameMapModify();
		GameMap_MapMicroPoint_Landscape_Clear(point);
		GameMapModifyManager_StopGameMapModify();
		Guide_AttachToMap(guide);

		return true;
	}

	// Original function: FUN_004475b3.
	void Guide_AttachToMap(void* guide)
	{
		const auto id = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::GuideId);
		auto* micro = GameMap_GetMapMicroPointPtr(Position(guide));

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Empty)
		{
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType, 3);
			Write<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId, static_cast<std::uint16_t>(id));
		}
	}

	// Original function: FUN_004475cf; the native routine does not compare ids.
	void Guide_DetachFromMap(void* guide)
	{
		auto* micro = GameMap_GetMapMicroPointPtr(Position(guide));

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::Guide)
		{

			return;
		}

		micro = GameMap_GetMapMicroPointPtr(Position(guide));

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Empty || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Guide)
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType, 0);
	}

	// Original function: FUN_004475f6.
	void Guide_SetLinkId(void* guide, std::uint32_t linkId)
	{
		if (Read<std::int32_t>(guide, NC2Layout::GuideOffset::GuideId) == -1)
		{

			return;
		}

		Write(guide, NC2Layout::GuideOffset::LinkSystemId, linkId);

		for (std::int32_t index = 0; index < Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount); ++index)
		{
			auto* other = Guide(Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionGuideIdsUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::GuideConnectionUnsigned));

			if (Read<std::uint32_t>(other, NC2Layout::GuideOffset::LinkSystemId) != linkId)
				Guide_SetLinkId(other, linkId);
		}
	}

	// Original function: FUN_0044763e.
	void Guide_SetLinkIdInvalid(void* guide)
	{
		if (Read<std::int32_t>(guide, NC2Layout::GuideOffset::LinkSystemId) == -1)
		{

			return;
		}

		Write<std::int32_t>(guide, NC2Layout::GuideOffset::LinkSystemId, -1);

		for (std::int32_t index = 0; index < Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount); ++index)
		{
			Guide_SetLinkIdInvalid(Guide(Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionGuideIdsUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::GuideConnectionUnsigned)));
		}
	}

	// Original function: FUN_004476c6.
	void Guide_ScanForGoods(void* guide)
	{
		std::memset(static_cast<std::byte*>(guide) + NC2Layout::GuideOffset::GoodsCounts, 0, NC2Layout::RecordSize::GuideGoodsCounts);
		CMapPathfinderElement::SearchOverMap(Position(guide), 40, true, slScanGoodsAround, guide, true, Read<std::int32_t>(guide, NC2Layout::GuideOffset::PlayerId));
	}
}
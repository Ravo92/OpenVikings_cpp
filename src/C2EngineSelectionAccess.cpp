#include "C2DefaultInteraction.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineGuideDrawing.h"
#include "C2EngineHouseDrawing.h"
#include "C2EngineLifetimeAccess.h"
#include "C2EngineMoveableDrawing.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XBobRendering.h"
#include "XNumericConstants.h"

#include <cstring>

namespace NC2E2
{
	using namespace NC2Lifetime;
	namespace
	{
		const void* Human(std::uint32_t id)
		{
			return NC2Logic::CHumanArray::mStaticVars + id * 0xC58u;
		}
		const void* House(std::uint32_t id)
		{
			return NC2Logic::CHouseArray::mStaticVars + id * 0x7E0u;
		}
		const void* Guide(std::uint32_t id)
		{
			return NC2Logic::CGuideArray::mStaticVars + id * 0x184u;
		}
		const void* Landscape(std::uint32_t id)
		{
			return static_cast<const std::byte*>(gStaticLandscapeEntries) + static_cast<std::size_t>(id) * sizeof(SE2StaticDataLandscape);
		}

		bool IsHit(const S2DEngineDisplaySearchObjectBobData& bobs, const std::int32_t* mouse, const std::int32_t* base)
		{
			for (std::uint32_t i = 0; i < bobs.count; ++i)
			{
				const auto& bob = bobs.bobs[i];
				const std::int32_t relative[2]{ mouse[0] - base[0] - bob.offsetX, mouse[1] - base[1] - bob.offsetY };
				if (NXBasics::BobManager_IsBobHit(bob.manager, bob.id, relative)) return true;
			}
			return false;
		}

		void SetMoveable(void* output, std::uint32_t type, std::uint32_t id, const void* object, std::uint32_t point, std::uint32_t moveableId, const S2DEngineDisplaySearchObjectBobData& bobs)
		{
			Write(output, NC2Layout::DisplayObjectReferenceOffset::Type, type);
			Write(output, NC2Layout::DisplayObjectReferenceOffset::MapPosition, point);
			const auto offset = type == SwitchIds::MapObjectUnderCursor::Human ? NC2Layout::DisplayObjectReferenceOffset::HumanIdUnsigned : type == SwitchIds::MapObjectUnderCursor::Animal ? NC2Layout::DisplayObjectReferenceOffset::AnimalIdUnsigned : NC2Layout::DisplayObjectReferenceOffset::VehicleIdUnsigned;
			Write(output, offset, id);
			Write(output, offset + NC2Layout::ObjectReferenceOffset::Revision, Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::UniqueId));
			Write(output, NC2Layout::DisplayObjectReferenceOffset::MapMoveableId, moveableId);
			Write(output, NC2Layout::DisplayObjectReferenceOffset::MapMoveableUniqueId, NC2Logic::MapMoveable_GetUniqueId(NC2Logic::gMapMoveableArrayStorage.entries + moveableId * NC2Layout::RecordSize::MapMoveable));
			Write(output, NC2Layout::DisplayObjectReferenceOffset::PlayerId, Read<std::int32_t>(object, type == 3 ? NC2Layout::VehicleOffset::PlayerId : NC2Layout::ActorPlayerOffset::HumanOrAnimalPlayerId));
			std::memcpy(static_cast<std::byte*>(output) + NC2Layout::DisplayObjectReferenceOffset::BobData, &bobs, sizeof(bobs));
		}
	}

	// Original function: FUN_00484061; sprite hits precede the 20-pixel fallback.
	void DE_SearchObject_FindOnWorldPosition(const void* display, const std::int32_t* mouse, void* output)
	{
		std::memset(output, 0, NC2Layout::RecordSize::DisplaySearchObject);
		DisplayObjectReference_Reset(output);
		std::uint32_t first = 0, last = 0;
		const std::int32_t firstPixel[2]{ mouse[0] - 200, mouse[1] - 200 }, lastPixel[2]{ mouse[0] + 200, mouse[1] + 300 };
		DisplayDescription_WorldPixelToMapMigCoordinates(display, firstPixel, &first);
		DisplayDescription_WorldPixelToMapMigCoordinates(display, lastPixel, &last);
		std::uint32_t priority = 0, fallbackPriority = 0, bestDistance = 400;
		alignas(4) std::byte fallback[NC2Layout::RecordSize::DisplaySearchObject]{ };
		const auto player = Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId);
		for (std::int32_t y = static_cast<std::int16_t>(first >> 16); y <= static_cast<std::int16_t>(last >> 16); ++y)
		{
			for (std::int32_t x = static_cast<std::int16_t>(first); x <= static_cast<std::int16_t>(last); ++x)
			{
				const auto point = static_cast<std::uint32_t>(static_cast<std::uint16_t>(x)) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(y)) << 16);
				const auto* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(&point);
				if ((Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) & 4u) != 0 && player < 16 && (Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) == 0) continue;
				const auto* engineMicro = EngineMap_GetConstMapMicroPointPtr(&point);
				std::int32_t base[2];
				DisplayDescription_MapMigToWorldPixelCoordinates(display, point, base, true);
				S2DEngineDisplaySearchObjectBobData bobs;
				const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);
				const auto phase = static_cast<std::uint32_t>(x + y * 2);
				if (type == 1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0 && priority <= 20 && Read<std::uint16_t>(engineMicro, NC2Layout::EngineMapMicroPointOffset::LandscapeId) != NXBasics::IntegerRange::UInt16MaximumSigned && Read<std::uint32_t>(Landscape(Read<std::uint16_t>(engineMicro, NC2Layout::EngineMapMicroPointOffset::LandscapeId)), NC2Layout::StaticLandscapeOffset::LogicType) != 1)
				{
					DrawAtom_Landscape_GetBobData(Landscape(Read<std::uint16_t>(engineMicro, NC2Layout::EngineMapMicroPointOffset::LandscapeId)), Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency), phase, &bobs);
					if (IsHit(bobs, mouse, base))
					{
						Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::Type, SwitchIds::MapObjectUnderCursor::Landscape);
						Write(output, NC2Layout::DisplayObjectReferenceOffset::MapPosition, point);
						Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::LandscapeId, Read<std::uint16_t>(engineMicro, NC2Layout::EngineMapMicroPointOffset::LandscapeId));
						Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::LandscapeVariant, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency));
						Write<std::int32_t>(output, NC2Layout::DisplayObjectReferenceOffset::PlayerId, Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId));
						std::memcpy(static_cast<std::byte*>(output) + NC2Layout::DisplayObjectReferenceOffset::BobData, &bobs, sizeof(bobs));
						priority = 20;
					}
				}
				if (type == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 1u) != 0)
				{
					const auto houseId = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId);
					if (priority <= 50)
					{
						S2DEngineDrawAtomHouseSignData signs;
						DrawAtom_House_GetSignData(houseId, &signs);
						for (auto i = static_cast<std::int32_t>(signs.count) - 1; i >= 0; --i)
						{
							const auto& sign = signs.signs[i];
							if (sign.human == -1) continue;
							DrawAtom_Landscape_GetBobData(Landscape(sign.landscape), 1, phase, &bobs);
							for (std::uint32_t j = 0; j < bobs.count; ++j)
							{
								bobs.bobs[j].offsetX += sign.x; bobs.bobs[j].offsetY += sign.y;
							}
							if (!IsHit(bobs, mouse, base)) continue;
							const auto* human = Human(sign.human);
							SetMoveable(output, 1, sign.human, human, point, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId), bobs); priority = 50;
						}
					}
					if (priority <= 30)
					{
						DrawAtom_House_GetBobData(houseId, &bobs);
						if (IsHit(bobs, mouse, base))
						{
							Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::Type, SwitchIds::MapObjectUnderCursor::House);
							Write(output, NC2Layout::DisplayObjectReferenceOffset::MapPosition, point);
							Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::HouseId, houseId);
							Write(output, NC2Layout::DisplayObjectReferenceOffset::HouseUniqueId, Read<std::uint32_t>(House(houseId), NC2Layout::HouseOffset::UniqueId));
							Write(output, NC2Layout::DisplayObjectReferenceOffset::PlayerId, Read<std::int32_t>(House(houseId), NC2Layout::HouseOffset::PlayerId));
							std::memcpy(static_cast<std::byte*>(output) + NC2Layout::DisplayObjectReferenceOffset::BobData, &bobs, sizeof(bobs));
							priority = 30;
						}
					}
				}
				if (type == 3 && priority <= 25)
				{
					const auto guideId = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId);
					DrawAtom_Guide_GetBobData(guideId, display, &bobs);
					if (IsHit(bobs, mouse, base))
					{
						Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::Type, SwitchIds::MapObjectUnderCursor::Guide);
						Write(output, NC2Layout::DisplayObjectReferenceOffset::MapPosition, point);
						Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::GuideId, guideId);
						Write(output, NC2Layout::DisplayObjectReferenceOffset::GuideUniqueId, Read<std::uint32_t>(Guide(guideId), NC2Layout::GuideOffset::UniqueId));
						Write(output, NC2Layout::DisplayObjectReferenceOffset::PlayerId, Read<std::int32_t>(Guide(guideId), NC2Layout::GuideOffset::PlayerId));
						std::memcpy(static_cast<std::byte*>(output) + NC2Layout::DisplayObjectReferenceOffset::BobData, &bobs, sizeof(bobs));
						priority = 25;
					}
				}
				NC2Logic::CMapMoveableIterator iterator;
				for (auto moveableId = iterator.GetFirstMapMoveableId(&point); moveableId != -1; moveableId = iterator.GetNextMapMoveableId())
				{
					const auto* moveable = NC2Logic::gMapMoveableArrayStorage.entries + moveableId * NC2Layout::RecordSize::MapMoveable;
					DrawAtom_MapMoveable_GetBobData(moveableId, display, &bobs, player);
					auto objectType = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType), id = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId), objectPoint = point;
					const void* object = nullptr;
					auto objectMoveable = static_cast<std::uint32_t>(moveableId);
					const bool marker = objectType == 4;
					if (objectType == 1)
					{
						object = Human(id); if (NC2Logic::Human_Misc_IsHidden(object)) continue;
					}
					else if (objectType == 2)
					{
						object = NC2Logic::CAnimalArray::mStaticVars + id * 0x98u; if (Read<std::uint8_t>(object, NC2Layout::AnimalOffset::InsideHouseFlag) != 0) continue;
					}
					else if (objectType == 3) object = NC2Logic::CVehicleArray::mStaticVars + id * 0x54Cu;
					else if (marker)
					{
						const auto* data = static_cast<const std::byte*>(NC2Logic::gMarkerEntries) + id * 0x18u;
						if (Read<std::uint32_t>(data, NC2Layout::MarkerOffset::Type) != 1 || Read<std::int32_t>(data, NC2Layout::MarkerOffset::HumanId) == -1) continue;
						id = Read<std::uint32_t>(data, NC2Layout::MarkerOffset::HumanId); object = Human(id); objectType = 1;
						objectPoint = Read<std::uint32_t>(NC2Logic::Human_GetMapPositionRef(object), NC2Layout::MapCoordinatePairOffset::Coordinates); objectMoveable = Read<std::uint32_t>(object, NC2Layout::HumanOffset::MapMoveableId);
					}
					else continue;
					const auto objectPriority = objectType == 2 ? 40u : 50u;
					if (priority <= objectPriority && IsHit(bobs, mouse, base))
					{
						SetMoveable(output, objectType, id, object, objectPoint, objectMoveable, bobs); priority = objectPriority;
					}
					if (priority != 0 || marker) continue;
					const auto dx = static_cast<std::uint32_t>(mouse[0]) - static_cast<std::uint32_t>(base[0]), dy = static_cast<std::uint32_t>(mouse[1]) - static_cast<std::uint32_t>(base[1]);
					const auto distance = dx * dx + dy * dy;
					if (distance >= bestDistance || objectPriority < fallbackPriority) continue;
					SetMoveable(fallback, objectType, id, object, point, moveableId, bobs); fallbackPriority = objectPriority; bestDistance = distance;
				}
			}
		}
		if (Read<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::None && fallbackPriority != 0) std::memcpy(output, fallback, sizeof(fallback));
		if (Read<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::None)
		{
			Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::Type, SwitchIds::MapObjectUnderCursor::Position);
			std::uint32_t point;
			DisplayDescription_WorldPixelToMapMigCoordinates(display, mouse, &point); Write(output, NC2Layout::DisplayObjectReferenceOffset::MapPosition, point);
			if (NC2Logic::MapMigPoint_IsInsideMap(&point))
			{
				const auto flags = Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::TerrainFlags);
				if ((flags & 12u) != 0)
				{
					const auto* manager = (flags & 4u) != 0 ? NC2Logic::gRoadBuildManager : NC2Logic::gWallBuildManager;
					Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::Type, (flags & 4u) != 0 ? SwitchIds::MapObjectUnderCursor::RoadBuildPlaceUnsigned : SwitchIds::MapObjectUnderCursor::WallBuildPlaceUnsigned);
					Write(output, NC2Layout::DisplayObjectReferenceOffset::PlayerId, NC2Logic::RoadBuildManager_GetRoadBuildFlagPlayerId(manager, &point));
				}
			}
		}
		const auto owner = Read<std::int32_t>(output, NC2Layout::DisplayObjectReferenceOffset::PlayerId);
		Write<std::uint32_t>(output, NC2Layout::DisplayObjectReferenceOffset::MarkType, owner != -1 && static_cast<std::uint32_t>(owner) != player ? NC2Runtime::DisplayObjectMarkType::OtherPlayer : NC2Runtime::DisplayObjectMarkType::LocalOrUnowned);
	}
}

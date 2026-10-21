#include "C2AnimalAccess.h"
#include "XTextEncoding.h"
#include "C2E2AnimalSoundManager.h"
#include "C2E2StaticDataHumanMisc.h"
#include "C2E2StaticDataPalettes.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineLandscapeDrawing.h"
#include "C2EngineMoveableDrawing.h"
#include "C2EngineObjectData.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanEquipment.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "InGameGuiSupportAccess.h"
#include "RemasterHumanSprites.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XNumericConstants.h"
#include "XPalette.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <windows.h>

namespace NC2E2
{
	// Original function: FUN_0047e936; callback bits select which state bytes are written.
	bool __cdecl E2Manager_sl_Callback_ObjectDisplayStateInLogicChanged(std::uint32_t type, std::uint32_t, std::uint32_t id, std::uint32_t flags, std::uint32_t)
	{
		using namespace NC2Lifetime;
		switch (type)
		{
			case SwitchIds::CallbackType::HumanDisplayStateChanged:
				{
					auto* data = gDynamicDataHumans.entries + id * NC2Layout::RecordSize::DynamicHumanUnsigned;
					if ((flags & 2u) != 0) Write<std::uint8_t>(data, NC2Layout::DynamicHumanOffset::SelectedFlag, flags & 1u);
					if ((flags & 8u) != 0) Write<std::uint8_t>(data, NC2Layout::DynamicHumanOffset::HighlightedFlag, (flags >> 2) & 1u);
					if ((flags & NXBasics::BitMask::Bit5) != 0) Write<std::uint8_t>(data, NC2Layout::DynamicHumanOffset::HasUserMessageFlag, (flags >> 4) & 1u);
					break;
				}
			case SwitchIds::CallbackType::AnimalDisplayStateChanged: case SwitchIds::CallbackType::VehicleDisplayStateChanged:
				{
					auto* data = (type == 0xF ? gDynamicDataAnimals.entries : gDynamicDataVehicles.entries) + id * NC2Layout::RecordSize::DynamicAnimalVehicleUnsigned;
					if ((flags & 2u) != 0) Write<std::uint8_t>(data, NC2Layout::DynamicAnimalVehicleOffset::SelectedFlag, flags & 1u);
					if ((flags & 8u) != 0) Write<std::uint8_t>(data, NC2Layout::DynamicAnimalVehicleOffset::HighlightedFlag, (flags >> 2) & 1u);
					break;
				}
			case SwitchIds::CallbackType::HouseDisplayStateChanged:
				if ((flags & 2u) != 0) Write<std::uint8_t>(gDynamicDataHouses.entries + id * 0xD0u, NC2Layout::DynamicHouseOffset::SelectedFlag, flags & 1u);
				break;
			default: break;
		}
		return true;
	}

	namespace
	{
		using NC2Lifetime::Read;
		using NC2Lifetime::Write;

		std::uint32_t ParentJob(std::uint32_t job)
		{
			return Read<std::uint32_t>(NC2Logic::gStaticJobTypeData, job * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::BaseJob);
		}

		std::uint32_t DrawingDirection(const void* moveable)
		{
			return Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 ? Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::Direction) : 2u;
		}

		const SE2StaticDataMapMoveableWalkData* GetCachedWalk(const void* moveable, SE2DynamicDataMapMoveableData& dynamic, std::uint32_t tribe, std::uint32_t job, std::uint32_t good)
		{
			const auto speed = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingTicksPerStep);
			if (dynamic.walk != nullptr && dynamic.walkTribe == tribe && dynamic.walkJob == job && dynamic.walkGood == good && dynamic.walkSpeed == speed) return dynamic.walk;
			dynamic.walk = nullptr;
			auto lookupJob = job;
			auto lookupGood = good;
			while (true)
			{
				dynamic.walk = StaticMapMoveables_GetWalkDataPtr(gE2StaticDataMapMoveablesManager, tribe, lookupJob, lookupGood, speed);
				if (dynamic.walk != nullptr)
				{
					dynamic.walkTribe = tribe;
					dynamic.walkJob = job;
					dynamic.walkGood = good;
					dynamic.walkSpeed = speed;
					return dynamic.walk;
				}
				const auto parent = ParentJob(lookupJob);
				if (parent != 0) lookupJob = parent;
				else if (lookupGood != 0) lookupGood = 0;
				else return nullptr;
			}
		}

		const SE2StaticDataMapMoveableAnimationData* FindAnimation(std::uint32_t tribe, std::uint32_t job, std::uint32_t action, std::uint32_t subId)
		{
			while (true)
			{
				if (const auto* animation = StaticMapMoveables_GetAnimationDataPtr(gE2StaticDataMapMoveablesManager, tribe, job, action, subId)) return animation;
				job = ParentJob(job);
				if (job == 0) return nullptr;
			}
		}

		void* Remaster_GetSaracenWoodcutterHead(const SE2DynamicDataMapMoveableData& dynamic, std::uint32_t tribe, std::uint32_t job, std::uint32_t action)
		{
			if (tribe != 4 || job != SwitchIds::HumanJob::Collector || action != 24 || dynamic.headBobManager == nullptr || dynamic.headBobId == NXBasics::IntegerRange::UInt32Maximum || NXBasics::BobManager_DoesBobExists(dynamic.headBobManager, dynamic.headBobId)) return nullptr;
			// Original Head_41 stops before woodcutting; Head_40 contains the matching native frames.
			const auto* graphics = static_cast<const SHumanJobBaseGraphics*>(StaticHumanMisc_JobGraphicsBase_GetDataPtr(tribe, job));
			for (std::uint32_t index = 0; index < graphics->graphics.headCount; ++index)
			{
				auto* head = graphics->graphics.heads[index].headBobManager;
				if (head != nullptr && NXBasics::BobManager_DoesBobExists(head, dynamic.headBobId)) return head;
			}
			return nullptr;
		}

		std::uint32_t Remaster_GetVikingMasonMaterialRanges(std::uint32_t tribe, std::uint32_t job, std::uint32_t action, std::uint32_t done, std::uint32_t total)
		{
			if (tribe != 1 || job != SwitchIds::HumanJob::Mason || (action != 54 && action != 55) || total == 0) return 0;
			const auto* animation = FindAnimation(tribe, job, action, 0);
			if (animation == nullptr || animation->animationMode != NC2Runtime::MoveableAnimationMode::Compound) return 0;
			const auto progress = (done * 10000u) / total;
			const auto* parts = reinterpret_cast<const SE2StaticDataMapMoveableAnimationPart*>(static_cast<std::uintptr_t>(animation->headSequenceOrPartAddress));
			for (std::uint32_t index = 0; index < animation->bodySequenceOrPartCount; ++index)
			{
				const auto& part = parts[index];
				if (progress < part.startPercent * 100u || progress >= part.endPercent * 100u) continue;
				if (part.type == NC2Runtime::MoveableAnimationPartType::Walk)
				{
					// Original compound parts carry rubble (3), stone blocks (26) or marble (27).
					return part.walkGood == 3 || part.walkGood == 26 || part.walkGood == 27 ? (1u << 14) | (1u << 15) : 0;
				}
				if (part.type == NC2Runtime::MoveableAnimationPartType::AtomicAnimation)
				{
					// The original Artist examine/hit sprites put their stone in accessory range 13.
					return part.atomicAction == 54 && (part.atomicSubId == 1 || part.atomicSubId == 2) ? 1u << 13 : 0;
				}
			}
			return 0;
		}

		void SetFrame(SE2DynamicDataMapMoveableData& dynamic, std::uint32_t bodySequence, std::uint32_t headSequence, std::uint32_t frame)
		{
			dynamic.bodyBobId = bodySequence + frame;
			dynamic.headBobId = headSequence == NXBasics::IntegerRange::UInt32Maximum ? dynamic.bodyBobId : headSequence + frame;
		}

		void SetAnimationFrame(SE2DynamicDataMapMoveableData& dynamic, const SE2StaticDataMapMoveableAnimationData& animation, const SMapMoveableFrameList& frames, std::uint32_t done, std::uint32_t total)
		{
			std::uint32_t index = 0;
			if (animation.animationMode == NC2Runtime::MoveableAnimationMode::ProgressFrames)
			{
				index = (frames.count * done) / total;
				if (index >= frames.count) index = frames.count - 1u;
			}
			else if (animation.animationMode == NC2Runtime::MoveableAnimationMode::CyclicFrames) index = done % frames.count;
			SetFrame(dynamic, animation.bodySequenceOrPartCount, animation.headSequenceOrPartAddress, frames.frames[index]);
		}

		std::int32_t AddOffset(std::int32_t first, std::int32_t second)
		{
			return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(first) + static_cast<std::uint32_t>(second));
		}

		std::int32_t InterpolateOffset(std::int32_t start, std::int32_t end, std::uint32_t progress)
		{
			const auto product = (static_cast<std::uint32_t>(end) - static_cast<std::uint32_t>(start)) * progress;
			return AddOffset(start, NXBasics::Runtime_TimeSigned32(product) / 10000);
		}
	}

	// Original function: FUN_0049b30b.
	std::uint32_t DrawAtom_Tool_GetSelectedBrightness()
	{
		const auto time = ::timeGetTime() & 1023u;
		const auto value = time >= 512u ? 512u - (time & 511u) : time;
		return static_cast<std::uint32_t>(256 + NXBasics::Runtime_TimeSigned32(value - 256u) / 2);
	}

	namespace
	{
		std::uint32_t AtomicLength(const void* object, std::uint32_t typeOffset)
		{
			return Read<std::uint32_t>(NC2Logic::gStaticAtomicAnimationData, Read<std::uint32_t>(object, typeOffset) * NC2Layout::RecordSize::AtomicAnimation + NC2Layout::AtomicAnimationOffset::LengthInTicks);
		}
		std::byte* HumanDynamic(const void* human)
		{
			return gDynamicDataHumans.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) * NC2Layout::RecordSize::DynamicHumanUnsigned;
		}
		NXBasics::CPalette* HumanBody(std::byte* dynamic)
		{
			return reinterpret_cast<NXBasics::CPalette*>(dynamic + NC2Layout::DynamicHumanOffset::BodyPalette);
		}
		NXBasics::CPalette* HumanHead(std::byte* dynamic)
		{
			return reinterpret_cast<NXBasics::CPalette*>(dynamic + NC2Layout::DynamicHumanOffset::HeadPalette);
		}
		const std::byte* Vehicle(std::uint32_t id)
		{
			return NC2Logic::CVehicleArray::mStaticVars + id * NC2Logic::CVehicleArray::ElementSize;
		}
		void WalkingOffset(const void* moveable, const void* display, std::int32_t& x, std::int32_t& y, std::int32_t* brightness)
		{
			if (display == nullptr || Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) == 0 || Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) != 0) return;
			auto direction = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingDirection);
			if (direction >= 8u) return;
			auto progress = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress);
			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag) != 0)
			{
				progress = NC2Runtime::WalkingProgress::FullStep - progress;
				direction = NC2Logic::VE_HexagonDirection_OppositeDirection(direction);
			}
			const auto startPoint = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
			auto targetPoint = startPoint;
			NC2Logic::MapMigPoint_MoveCursorUnwrapped(targetPoint, direction);
			std::int32_t start[2], target[2];
			DisplayDescription_MapMigToWorldPixelCoordinates(display, startPoint, start, true);
			DisplayDescription_MapMigToWorldPixelCoordinates(display, targetPoint, target, true);
			x = AddOffset(x, InterpolateOffset(0, AddOffset(target[0], NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(start[0]))), progress));
			y = AddOffset(y, InterpolateOffset(0, AddOffset(target[1], NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(start[1]))), progress));
			if (brightness != nullptr) *brightness = InterpolateOffset(*brightness, Display_DE_Hightlighting_GetMapBrightness(display, &targetPoint), progress);
		}
		std::int32_t DynamicBrightness(const void* dynamic, std::uint32_t selectedOffset, std::int32_t brightness)
		{
			if (Read<std::uint8_t>(dynamic, selectedOffset) != 0) brightness = static_cast<std::int32_t>(DrawAtom_Tool_GetSelectedBrightness() >> 1);
			if (Read<std::uint8_t>(dynamic, selectedOffset + NC2Layout::DynamicSelectionStateOffset::HighlightedDisplacement) != 0) brightness = 256;
			return brightness;
		}
		std::uint32_t HealthBob(std::uint32_t hitpoints, std::uint32_t maximum)
		{
			const auto percentage = NXBasics::Runtime_TimeSigned32(hitpoints * 100u) / NXBasics::Runtime_TimeSigned32(maximum);
			return 21u - static_cast<std::uint32_t>(std::clamp(percentage, 0, 100) / 10);
		}
	}

	// Original function: FUN_004955e7; bounded descriptor append, distinct from sprite drawing.
	bool S2DEngineDisplaySearchObjectBobData::AddBob(void* manager, std::uint32_t id, std::int32_t offsetX, std::int32_t offsetY)
	{
		if (count == NC2Runtime::DisplaySelectionLimits::BobsPerHit || !NXBasics::BobManager_DoesBobExists(manager, id)) return false;
		bobs[count++] = { manager, id, offsetX, offsetY };
		return true;
	}

	// Original function: FUN_004972de; descriptor animation differs from drawing.
	void DrawAtom_Landscape_GetBobData(const void* graphicData, std::uint32_t variant, std::uint32_t phase, S2DEngineDisplaySearchObjectBobData* output)
	{
		std::memset(output, 0, sizeof(*output));
		const auto* graphic = static_cast<const SE2StaticDataLandscape*>(graphicData);
		if (graphic->frameGroupCount == 0) return;
		std::int32_t group = static_cast<std::int32_t>(graphic->frameGroupCount) - 1;
		while (group >= 0 && Read<std::uint32_t>(graphic->frameGroups, static_cast<std::uint32_t>(group) * NC2Layout::RecordSize::StaticLandscapeFramesUnsigned) > variant) --group;
		if (group < 0) return;
		const auto* frames = reinterpret_cast<const std::byte*>(graphic->frameGroups) + static_cast<std::uint32_t>(group) * NC2Layout::RecordSize::StaticLandscapeFramesUnsigned;
		std::uint32_t value = graphic->loopAnimation != 0 ? phase + NC2Logic::gGameTime.tick : 0;
		if (graphic->loopAnimation != 0 && gE2ParticelGeneratorStorage.useRealTimeAnimationClock != 0) value = ::timeGetTime() / 83u + phase;
		const auto* ids = Read<const std::uint32_t*>(frames, NC2Layout::StaticLandscapeFramesOffset::Frames);
		output->AddBob(graphic->bobManagers[0], ids[value % Read<std::uint32_t>(frames, NC2Layout::StaticLandscapeFramesOffset::Count)], 0, 0);
	}

	// Original function: FUN_004951a4; only body/head sprites contribute to sort data.
	void DrawAtom_MapMoveable_GetBobData(std::uint32_t moveableId, const void* display, S2DEngineDisplaySearchObjectBobData* output, std::int32_t playerId)
	{
		std::memset(output, 0, sizeof(*output));
		const auto* moveable = NC2Logic::gMapMoveableArrayStorage.entries + moveableId * NC2Layout::RecordSize::MapMoveable;
		std::int32_t walkingX = 0, walkingY = 0, offsetX = 0, offsetY = 0;
		WalkingOffset(moveable, display, walkingX, walkingY, nullptr);
		const auto type = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType), id = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
		void* dynamic = nullptr;
		SE2DynamicDataMapMoveableData remasterFrames{};
		if (type == 1)
		{
			const auto* human = NC2Logic::CHumanArray::mStaticVars + id * NC2Logic::CHumanArray::ElementSize;
			dynamic = HumanDynamic(human);
			auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
			const auto tribe = NRemaster::HumanSprites::ResolveAppearanceTribe(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), job, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));
			if (NRemaster::HumanSprites::Supports(tribe, job))
			{
				remasterFrames = *static_cast<SE2DynamicDataMapMoveableData*>(dynamic); NRemaster::HumanSprites::PrepareNativeFallback(remasterFrames, tribe, job); dynamic = &remasterFrames;
				if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & (1u << 14)) != 0) job = NRemaster::HumanSprites::NativeFallbackJob(tribe, job);
			}
			sl_DoUpdateMapMoveableDynamicBobData(moveable, dynamic, tribe, job, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), Read<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag) != 0, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction), Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationFrame), AtomicLength(human, NC2Layout::HumanOffset::AtomicAnimationId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationStartCount), NC2Logic::Human_Misc_IsHidden(human), display, offsetX, offsetY, nullptr, false);
			const auto action = Read<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag) != 0 ? Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) : 0u;
			if (auto* head = Remaster_GetSaracenWoodcutterHead(*static_cast<SE2DynamicDataMapMoveableData*>(dynamic), tribe, job, action))
			{
				remasterFrames = *static_cast<SE2DynamicDataMapMoveableData*>(dynamic); remasterFrames.headBobManager = head; dynamic = &remasterFrames;
			}
		}
		else if (type == 2)
		{
			const auto* animal = NC2Logic::CAnimalArray::mStaticVars + id * NC2Logic::CAnimalArray::ElementSize;
			dynamic = gDynamicDataAnimals.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId) * NC2Layout::RecordSize::DynamicAnimalVehicleUnsigned;
			sl_DoUpdateMapMoveableDynamicBobData(moveable, dynamic, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType), 0, Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationRunningFlag) != 0, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAction), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationFrame), NC2Logic::Animal_GetCurrentAtomicAnimationLengthInTicks(animal), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationStartCount), Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::InsideHouseFlag) != 0, display, offsetX, offsetY, nullptr, false);
		}
		else if (type == 3)
		{
			const auto* vehicle = Vehicle(id);
			dynamic = gDynamicDataVehicles.entries + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId) * NC2Layout::RecordSize::DynamicAnimalVehicleUnsigned;
			const auto vehicleType = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);
			const bool special = (vehicleType == 3 || vehicleType == 4) && Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) != 0;
			sl_DoUpdateMapMoveableDynamicBobData(moveable, dynamic, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::TribeType), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType), special ? 5u : 0u, Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationRunningFlag) != 0, special ? 4u : Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationFrame), NC2Logic::Vehicle_GetCurrentAtomicAnimationLengthInTicks(vehicle), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationStartCount), false, display, offsetX, offsetY, nullptr, false);
		}
		else if (type == 4)
		{
			const auto* marker = static_cast<const std::byte*>(NC2Logic::gMarkerEntries) + id * 0x18u;
			if (Read<std::uint32_t>(marker, NC2Layout::MarkerOffset::Type) != 1 || NC2Logic::Marker_PlayerId_Get(marker) == -1 || NC2Logic::Marker_PlayerId_Get(marker) != playerId) return;
			const auto landscapeId = StaticHumanMisc_InhabitantSign_GetLandscapeId(NC2Logic::Marker_PlayerId_Get(marker), 9);
			if (landscapeId != -1) DrawAtom_Landscape_GetBobData(static_cast<const SE2StaticDataLandscape*>(gStaticLandscapeEntries) + landscapeId, 1, NC2Logic::gGameTime.tick, output);
			return;
		}
		else return;
		const auto& frames = *static_cast<const SE2DynamicDataMapMoveableData*>(dynamic);
		if (frames.bodyBobManager != nullptr && frames.bodyBobId != NXBasics::IntegerRange::UInt32Maximum) output->AddBob(frames.bodyBobManager, frames.bodyBobId, AddOffset(offsetX, walkingX), AddOffset(offsetY, walkingY));
		if (frames.headBobManager != nullptr && frames.headBobId != NXBasics::IntegerRange::UInt32Maximum) output->AddBob(frames.headBobManager, frames.headBobId, AddOffset(offsetX, walkingX), AddOffset(offsetY, walkingY));
	}

	// Original function: FUN_004951a3; the original destructor is an empty RET.
	static void __cdecl HumanGroupWhiteColor_AtExit()
	{}
	// Original function: FUN_004951a2; the original destructor is an empty RET.
	static void __cdecl HumanGroupBlackColor_AtExit()
	{}
	// Original function: FUN_00493656; full type-specific sprite and status dispatch.
	void DrawAtom_MapMoveable_DoDraw(std::uint32_t moveableId, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, const void* display, bool centered, std::int32_t playerId)
	{
		const auto* moveable = NC2Logic::gMapMoveableArrayStorage.entries + moveableId * NC2Layout::RecordSize::MapMoveable;
		std::int32_t brightness = 128;
		if (display != nullptr)
		{
			brightness = Display_DE_Hightlighting_GetMapBrightness(display, moveable + NC2Layout::MapMoveableOffset::MapPosition);
			WalkingOffset(moveable, display, x, y, &brightness);
			brightness = AddOffset(128, AddOffset(brightness, -128) / 2);
		}
		const auto type = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType), objectId = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
		if (type == 4)
		{
			const auto* marker = static_cast<const std::byte*>(NC2Logic::gMarkerEntries) + objectId * 0x18u;
			const auto markerType = Read<std::uint32_t>(marker, NC2Layout::MarkerOffset::Type);
			auto drawGraphic = [&] (std::int32_t id, std::int32_t offsetY)
			{
				if (id != -1) EngineGraphic_Draw(static_cast<std::uint32_t>(id), 1, bitmap, x, AddOffset(y, offsetY), NC2Logic::gGameTime.tick, AddOffset(brightness, brightness), -1, false);
			};
			if (markerType == 4)
			{
				static const auto incense = E2StaticDataLandscapesManager_GetLandscapeId("fx fire incense");
				drawGraphic(incense, 0);
				static const auto fire = E2StaticDataLandscapesManager_GetLandscapeId("fx fire small");
				drawGraphic(fire, 0);
				static const auto star = E2StaticDataLandscapesManager_GetLandscapeId("fx star");
				drawGraphic(star, 0);
				static const auto fog = E2StaticDataLandscapesManager_GetLandscapeId("fx fog waterfall");
				drawGraphic(fog, 0);
			}
			else if (markerType == 3)
			{
				static const auto work = E2StaticDataLandscapesManager_GetLandscapeId("player01 work extern 01");
				drawGraphic(work, 0);
			}
			else if (markerType == 2)
			{
				static const auto flag = E2StaticDataLandscapesManager_GetLandscapeId("fx marker");
				drawGraphic(flag, -60);
			}
			else if (markerType == 1)
			{
				const auto humanId = Read<std::int32_t>(marker, NC2Layout::MarkerOffset::HumanId);
				if (humanId != -1) brightness = DynamicBrightness(gDynamicDataHumans.entries + static_cast<std::uint32_t>(humanId) * NC2Layout::RecordSize::DynamicHumanUnsigned, NC2Layout::DynamicHumanOffset::SelectedFlag, brightness);
				if (NC2Logic::Marker_PlayerId_Get(marker) != -1 && NC2Logic::Marker_PlayerId_Get(marker) == playerId)
				{
					const auto id = StaticHumanMisc_InhabitantSign_GetLandscapeId(NC2Logic::Marker_PlayerId_Get(marker), 9);
					if (id != -1) drawGraphic(id, 0);
				}
			}
			return;
		}
		if (type == 3)
		{
			const auto* vehicle = Vehicle(objectId);
			auto* dynamic = gDynamicDataVehicles.entries + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId) * NC2Layout::RecordSize::DynamicAnimalVehicleUnsigned;
			brightness = DynamicBrightness(dynamic, NC2Layout::DynamicAnimalVehicleOffset::SelectedFlag, brightness);
			const auto marking = display != nullptr ? Display_DE_ObjectsToMark_Vehicle_GetMarkType(display, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId)) : 0u;
			const auto* commander = NC2Logic::Vehicle_Passengers_GetAttachedCommanderPtr(vehicle);
			if (commander != nullptr && (NC2Logic::Human_IsJobTrader(commander) || NC2Logic::Human_IsJobCarrier(commander)) && Read<std::uint8_t>(commander, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
			{
				const auto attachedType = Read<std::uint32_t>(Vehicle(Read<std::uint32_t>(commander, NC2Layout::HumanOffset::WorkAttachmentVehicleId)), NC2Layout::VehicleOffset::LogicType);
				if (attachedType == 1 || attachedType == 2)
				{
					auto* humanDynamic = HumanDynamic(commander);
					Write<void*>(humanDynamic, NC2Layout::DisplayAtomInformationOffset::Animation, nullptr);
					Write<void*>(humanDynamic, NC2Layout::DisplayAtomInformationOffset::Walk, nullptr);
					const bool atomic = Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationRunningFlag) != 0;
					sl_DoDrawMapMoveable(moveable, humanDynamic, Read<std::uint32_t>(commander, NC2Layout::HumanOffset::TribeType), 25, attachedType == 1 ? 59u : 60u, atomic, atomic ? (attachedType == 1 ? 2u : 3u) : 1u, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationFrame), NC2Logic::Vehicle_GetCurrentAtomicAnimationLengthInTicks(vehicle), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationStartCount), HumanBody(humanDynamic), HumanHead(humanDynamic), bitmap, x, y, brightness, marking, Read<std::int32_t>(commander, NC2Layout::HumanOffset::PlayerId), false, centered, false, nullptr, -1, -1, display, true);
					return;
				}
			}
			const auto vehicleType = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);
			const auto player = Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId);
			auto* palette = gStaticPalettes.palettes256[Read<std::uint32_t>(dynamic, NC2Layout::DynamicAnimalVehicleOffset::PaletteId)].palette;
			if ((vehicleType == 3 || vehicleType == 4) && player != -1) palette = EnginePalette_GetPlayerShipPalette(player);
			const bool special = (vehicleType == 3 || vehicleType == 4) && Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) != 0;
			sl_DoDrawMapMoveable(moveable, dynamic, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::TribeType), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType), special ? 5u : 0u, Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationRunningFlag) != 0, special ? 4u : Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationFrame), NC2Logic::Vehicle_GetCurrentAtomicAnimationLengthInTicks(vehicle), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAnimationStartCount), palette, palette, bitmap, x, y, brightness, marking, player, false, centered, false, nullptr, -1, -1, display, false);
			if (player != -1 && vehicleType != 3 && vehicleType != 4)
				NXBasics::BobManager_PrintBob(Read<void*>(gE2StaticDataBobManagersManager, NC2Layout::StaticBobCacheManagerOffset::GuiBubblesBobManager), HealthBob(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::Hitpoints), vehicleType == 5 ? 3000u : 1000u), bitmap, x, AddOffset(y, -56), EnginePalette_GetPlayerPalette(player));
			return;
		}
		if (type == 2)
		{
			const auto* animal = NC2Logic::CAnimalArray::mStaticVars + objectId * NC2Logic::CAnimalArray::ElementSize;
			if (!centered && Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::InsideHouseFlag) != 0) return;
			auto* dynamic = gDynamicDataAnimals.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId) * NC2Layout::RecordSize::DynamicAnimalVehicleUnsigned;
			brightness = DynamicBrightness(dynamic, NC2Layout::DynamicAnimalVehicleOffset::SelectedFlag, brightness);
			const auto marking = display != nullptr ? Display_DE_ObjectsToMark_Animal_GetMarkType(display, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalId)) : 0u;
			auto* palette = gStaticPalettes.palettes256[Read<std::uint32_t>(dynamic, NC2Layout::DynamicAnimalVehicleOffset::PaletteId)].palette;
			const auto player = Read<std::int32_t>(animal, NC2Layout::AnimalOffset::PlayerId);
			sl_DoDrawMapMoveable(moveable, dynamic, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType), 0, Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationRunningFlag) != 0, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAction), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationFrame), NC2Logic::Animal_GetCurrentAtomicAnimationLengthInTicks(animal), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationStartCount), palette, palette, bitmap, x, y, brightness, marking, player, Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::InsideHouseFlag) != 0, centered, false, nullptr, -1, -1, display, false);
			if (player != 20) NXBasics::BobManager_PrintBob(Read<void*>(gE2StaticDataBobManagersManager, NC2Layout::StaticBobCacheManagerOffset::GuiBubblesBobManager), HealthBob(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::Hitpoints), NC2Logic::Animal_HitPoints_GetMaximum(animal)), bitmap, x, AddOffset(y, -56), EnginePalette_GetPlayerPalette(player));
			if (!centered && display != nullptr && Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) != 0 && Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 49)
			{
				const auto offset = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) * 4u - 0x1Cu;
				auto* sounds = CE2AnimalSoundManager::sTheObjectPtr;
				Write(sounds, offset, Read<std::uint32_t>(sounds, offset) + 1u);
			}
			return;
		}
		if (type != 1) return;
		const auto* human = NC2Logic::CHumanArray::mStaticVars + objectId * NC2Logic::CHumanArray::ElementSize;
		if (!centered && NC2Logic::Human_Misc_IsHidden(human)) return;
		auto* dynamic = HumanDynamic(human);
		brightness = DynamicBrightness(dynamic, NC2Layout::DynamicHumanOffset::SelectedFlag, brightness);
		const auto marking = display != nullptr ? Display_DE_ObjectsToMark_Human_GetMarkType(display, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)) : 0u;
		auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		if ((NC2Logic::Human_IsJobTrader(human) || NC2Logic::Human_IsJobCarrier(human)) && (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) == 0 || centered))
		{
			job = 6;
			const auto vehicleId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId);
			if (vehicleId != NXBasics::IntegerRange::UInt32Maximum)
			{
				std::int32_t patch = -1;
				if (Read<std::uint32_t>(Vehicle(vehicleId), NC2Layout::VehicleOffset::LogicType) == 1) patch = StaticHumanMisc_RandomPalette_GetPatchId("good_handcart");
				const auto vehicleType = Read<std::uint32_t>(Vehicle(Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId)), NC2Layout::VehicleOffset::LogicType);
				if (vehicleType == 2 || vehicleType == 6) patch = StaticHumanMisc_RandomPalette_GetPatchId("good_oxcart");
				if (patch != -1) StaticHumanMisc_RandomPalette_Execute(static_cast<std::uint32_t>(patch), HumanBody(dynamic), HumanHead(dynamic), true);
			}
		}
		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), flags = Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags);
		const auto player = Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId);
		const bool specialHero = tribe == 2 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 47;
		const auto* effect = (NC2Logic::Human_IsJobHero(human) || (flags & (1u << 19)) != 0) && !specialHero ? Tool_PlayerColors_GetColorRef(player) : nullptr;
		auto* bodyPalette = HumanBody(dynamic);
		std::optional<NXBasics::CPalette> materialPalette;
		std::optional<NXBasics::CPalette> carriedHeadPalette;
		auto* headPalette = HumanHead(dynamic);
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);
		if (tribe == 3 && good == 2 && gStaticGoodData != nullptr && Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) == NXBasics::IntegerRange::UInt32Maximum)
		{
			// Remaster: render carried clay with its native material patch instead of a stale material color.
			const auto patch = Read<std::int32_t>(gStaticGoodData, good * NC2Layout::RecordSize::GoodGraphicsUnsigned + NC2Layout::GoodGraphicsOffset::PalettePatchId);
			if (patch != -1)
			{
				materialPalette.emplace(*bodyPalette); carriedHeadPalette.emplace(*headPalette);
				StaticHumanMisc_RandomPalette_Execute(static_cast<std::uint32_t>(patch), &*materialPalette, &*carriedHeadPalette, true);
				bodyPalette = &*materialPalette; headPalette = &*carriedHeadPalette;
			}
		}
		const auto action = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction);
		const bool atomicActive = Read<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag) != 0;
		if (tribe == 1 && job == SwitchIds::HumanJob::Mason && atomicActive && !NC2Logic::Human_Misc_IsHidden(human) && Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) == 0 && Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) == 0)
		{
			const auto ranges = Remaster_GetVikingMasonMaterialRanges(tribe, job, action, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationFrame), AtomicLength(human, NC2Layout::HumanOffset::AtomicAnimationId));
			const auto greyId = ranges == 0 ? -1 : E2StaticDataPalettesManager_GetPalette16Id("colors grey");
			if (greyId != -1)
			{
				const auto& grey = gStaticPalettes.palettes16[greyId];
				if (grey.palette256Id >= 0 && grey.palette256Id < gStaticPalettes.palette256Count && grey.colorRange < 16 && gStaticPalettes.palettes256[grey.palette256Id].palette != nullptr)
				{
					// Remaster: use a draw-local palette; appearance, saved palettes and gameplay RNG stay intact.
					materialPalette.emplace(*bodyPalette);
					const auto* source = reinterpret_cast<const std::byte*>(gStaticPalettes.palettes256[grey.palette256Id].palette) + NC2Layout::PaletteOffset::Colors + grey.colorRange * NC2Runtime::HumanPaletteRange::BytesUnsigned;
					for (std::uint32_t range = 13; range < 16; ++range)
						if ((ranges & (1u << range)) != 0) std::memcpy(reinterpret_cast<std::byte*>(&*materialPalette) + NC2Layout::PaletteOffset::Colors + range * NC2Runtime::HumanPaletteRange::BytesUnsigned, source, NC2Runtime::HumanPaletteRange::Bytes);
					bodyPalette = &*materialPalette;
				}
			}
		}
		sl_DoDrawMapMoveable(moveable, dynamic, tribe, job, good, atomicActive, action, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationFrame), AtomicLength(human, NC2Layout::HumanOffset::AtomicAnimationId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationStartCount), bodyPalette, headPalette, bitmap, x, y, brightness, marking, player, NC2Logic::Human_Misc_IsHidden(human), centered, (flags & (1u << 14)) != 0, effect, -1, -1, display, false);
		auto iconY = AddOffset(y, -56);
		const bool tallSoldier = tribe == 3 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 32;
		if (tallSoldier) iconY = AddOffset(iconY, -16);
		auto icon = [&] (std::uint32_t bob)
		{
			NXBasics::BobManager_PrintBob_UsingShadedAlpha(Read<void*>(gE2StaticDataBobManagersManager, NC2Layout::StaticBobCacheManagerOffset::GuiBubblesBobManager), bob, bitmap, x, iconY, gStaticPalettes.palettes256[gStaticPalettes.guiBubblesPaletteId].palette, brightness);
		};
		auto experience = [&] (std::uint32_t value)
		{
			const auto level = (std::min) (value, 100u) / 25u; if (level > 0)
			{
				icon(10u - level); iconY = AddOffset(iconY, -16);
			}
		};
		if (NC2Logic::Human_IsJobSoldier(human) && !centered && !tallSoldier) experience(NC2Logic::Human_Tool_Gui_GetExperienceForEquippedWeapon(human));
		if (player == playerId && !centered && Read<std::uint8_t>(dynamic, NC2Layout::DynamicHumanOffset::SelectedFlag) != 0)
		{
			NXBasics::BobManager_PrintBob(Read<void*>(gE2StaticDataBobManagersManager, NC2Layout::StaticBobCacheManagerOffset::GuiBubblesBobManager), HealthBob(Read<std::uint32_t>(human, NC2Layout::HumanOffset::Hitpoints), NC2Logic::Human_GetHumanAttributeMaximum_Hitpoints(human)), bitmap, x, iconY, EnginePalette_GetPlayerPalette(player));
			iconY = AddOffset(iconY, -20);
		}
		if (player == playerId && !centered)
		{
			if (Read<std::uint8_t>(dynamic, NC2Layout::DynamicHumanOffset::SelectedFlag) != 0) experience(static_cast<std::uint32_t>(NC2Logic::Human_JobEfficiency_CalculateExperienceFactor(human)));
			// Read-only critical-need DWORDs at 004F3B68/004F3B6C are both 1000.
			std::int32_t need = -1;
			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) < 1000) need = 4;
			else if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) < 1000) need = 0;
			else if (Read<std::uint8_t>(dynamic, NC2Layout::DynamicHumanOffset::HasUserMessageFlag) != 0) need = 5;
			else if (NC2Logic::Human_UserControl_DoesWantToMarry(human) || NC2Logic::Human_UserControl_DoesWantToGetChild(human)) need = 2;
			if (need != -1)
			{
				icon(static_cast<std::uint32_t>(need)); iconY = AddOffset(iconY, -16);
			}
		}
		if ((flags & (1u << 7)) != 0 && (!NC2Logic::Human_IsJobHero(human) || specialHero))
		{
			icon(10); iconY = AddOffset(iconY, -16);
		}
		if (!centered && Read<std::uint8_t>(human, NC2Layout::HumanOffset::AssistantHasCommandHistoryFlag) != 0) icon(22);
		if (display == nullptr || centered || Read<void*>(display, NC2Layout::EngineDisplayOffset::Font) == nullptr || player != playerId || NC2InGameGuiGroups::gObjectGroupManager == nullptr) return;
		const auto group = static_cast<const NC2InGameGuiGroups::CObjectGroupManager*>(NC2InGameGuiGroups::gObjectGroupManager)->Misc_GetGroupIdOfHuman(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		if (group == -1) return;
		static const NXBasics::SColorRGB white(255, 255, 255), black(0, 0, 0);
		static std::uint8_t initialized = 0;
		if ((initialized & 1u) == 0)
		{
			initialized |= 1u; NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(HumanGroupWhiteColor_AtExit));
		}
		if ((initialized & 2u) == 0)
		{
			initialized |= 2u; NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(HumanGroupBlackColor_AtExit));
		}
		auto* font = Read<NXBasics::CFont*>(display, NC2Layout::EngineDisplayOffset::Font);
		const auto labelX = AddOffset(x, 8), labelY = AddOffset(y, -5);
		bitmap->Text_PrintV(font, &black, AddOffset(labelX, -1), labelY, "%d", AddOffset(group, 1));
		bitmap->Text_PrintV(font, &black, AddOffset(labelX, 1), labelY, "%d", AddOffset(group, 1));
		bitmap->Text_PrintV(font, &black, labelX, AddOffset(labelY, -1), "%d", AddOffset(group, 1));
		bitmap->Text_PrintV(font, &black, labelX, AddOffset(labelY, 1), "%d", AddOffset(group, 1));
		bitmap->Text_PrintV(font, &white, labelX, labelY, "%d", AddOffset(group, 1));
	}

	// Original function: FUN_004803d5; creation callback, including savegame preservation.
	void E2DynamicDataHumanManager_HumanXtructed(std::uint32_t humanId, bool created, bool fromSavegame)
	{
		if (!created) return;
		auto* dynamic = gDynamicDataHumans.entries + humanId * NC2Layout::RecordSize::DynamicHumanUnsigned;
		const auto* human = NC2Logic::CHumanArray::mStaticVars + humanId * NC2Logic::CHumanArray::ElementSize;
		if (!fromSavegame) std::memset(dynamic, 0, NC2Layout::RecordSize::DynamicHuman);
		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
		const auto* graphics = StaticHumanMisc_JobGraphicsBase_GetDataPtr(tribe, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType));
		const auto bodyCount = Read<std::uint32_t>(graphics, NC2Layout::HumanJobGraphicsOffset::BodyCount);
		if (bodyCount != 0)
		{
			const auto index = NC2Logic::Logic_EntryPoint_Random_GetNumber() % bodyCount;
			Write(dynamic, NC2Layout::DynamicHumanOffset::BodyVariant, Read<std::uint32_t>(graphics, NC2Layout::HumanJobGraphicsOffset::BodyChoicesHex + index * NC2Layout::RecordSize::HumanBodyGraphicsUnsigned));
			Write(dynamic, NC2Layout::DisplayAtomInformationOffset::BodyBobManager, Read<void*>(graphics, NC2Layout::HumanJobGraphicsOffset::BodyBobManagers + index * NC2Layout::RecordSize::HumanBodyGraphicsUnsigned));
			Write(dynamic, NC2Layout::DisplayAtomInformationOffset::ShadowBobManager, Read<void*>(graphics, NC2Layout::HumanJobGraphicsOffset::ShadowBobManagers + index * NC2Layout::RecordSize::HumanBodyGraphicsUnsigned));
		}
		const auto headCount = Read<std::uint32_t>(graphics, NC2Layout::HumanJobGraphicsOffset::HeadCount);
		if (headCount != 0)
		{
			const auto index = NC2Logic::Logic_EntryPoint_Random_GetNumber() % headCount;
			Write(dynamic, NC2Layout::DynamicHumanOffset::HeadVariant, Read<std::uint32_t>(graphics, NC2Layout::HumanJobGraphicsOffset::HeadChoices + index * NC2Layout::RecordSize::HumanHeadGraphicsUnsigned));
			Write(dynamic, NC2Layout::DisplayAtomInformationOffset::HeadBobManager, Read<void*>(graphics, NC2Layout::HumanJobGraphicsOffset::HeadBobManagers + index * NC2Layout::RecordSize::HumanHeadGraphicsUnsigned));
		}
		if (!fromSavegame)
		{
			auto* body = reinterpret_cast<NXBasics::CPalette*>(dynamic + NC2Layout::DynamicHumanOffset::BodyPalette);
			auto* head = reinterpret_cast<NXBasics::CPalette*>(dynamic + NC2Layout::DynamicHumanOffset::HeadPalette);
			const auto bodyPaletteCount = Read<std::uint32_t>(graphics, NC2Layout::HumanJobBaseGraphicsOffset::BodyPaletteCount);
			if (bodyPaletteCount != 0)
			{
				const auto index = NC2Logic::Logic_EntryPoint_Random_GetNumber() % bodyPaletteCount;
				const auto paletteId = Read<std::uint32_t>(graphics, NC2Layout::HumanJobBaseGraphicsOffset::BodyPaletteIds + index * NC2Layout::RecordSize::HumanGraphicsVariantIdUnsigned);
				*body = *gStaticPalettes.palettes256[paletteId].palette;
			}
			const auto headPaletteCount = Read<std::uint32_t>(graphics, NC2Layout::HumanJobBaseGraphicsOffset::HeadPaletteCount);
			if (headPaletteCount != 0)
			{
				const auto index = NC2Logic::Logic_EntryPoint_Random_GetNumber() % headPaletteCount;
				const auto paletteId = Read<std::uint32_t>(graphics, NC2Layout::HumanJobBaseGraphicsOffset::HeadPaletteIds + index * NC2Layout::RecordSize::HumanGraphicsVariantIdUnsigned);
				*head = *gStaticPalettes.palettes256[paletteId].palette;
			}
			const auto playerPatch = StaticHumanMisc_ColorIds_GetPlayerRandomPatchId(Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), NC2Logic::Human_IsSexFemale(human));
			if (playerPatch != -1) StaticHumanMisc_RandomPalette_Execute(static_cast<std::uint32_t>(playerPatch), body, head, false);
			const auto patchCount = Read<std::uint32_t>(graphics, NC2Layout::HumanJobBaseGraphicsOffset::RandomPatchCount);
			if (patchCount != 0)
			{
				const auto index = NC2Logic::Logic_EntryPoint_Random_GetNumber() % patchCount;
				StaticHumanMisc_RandomPalette_Execute(Read<std::uint32_t>(graphics, NC2Layout::HumanJobBaseGraphicsOffset::RandomPatchIds + index * NC2Layout::RecordSize::HumanGraphicsVariantIdUnsigned), body, head, false);
			}
			const auto* name = StaticHumanMisc_TribeHumanNames_GetRandomNamePtr(tribe, NC2Logic::Human_IsSexFemale(human));
			NXBasics::Text_Copy(reinterpret_cast<char*>(dynamic + NC2Layout::DynamicHumanOffset::Name), 0x63, name);
		}
		Write<std::uint32_t>(dynamic, NC2Layout::DisplayAtomInformationOffset::BodyBobId, NXBasics::IntegerRange::UInt32Maximum);
		Write<std::uint32_t>(dynamic, NC2Layout::DisplayAtomInformationOffset::HeadBobId, NXBasics::IntegerRange::UInt32Maximum);
	}

	// Original function: FUN_00480576; keep the appearance variant when the new job supports it.
	void E2DynamicDataHumanManager_HumanJobChanged(std::uint32_t humanId, std::uint32_t oldJob, std::uint32_t newJob)
	{
		if (oldJob == 0) return;
		const auto* human = NC2Logic::CHumanArray::mStaticVars + humanId * NC2Logic::CHumanArray::ElementSize;
		auto* dynamic = gDynamicDataHumans.entries + humanId * NC2Layout::RecordSize::DynamicHumanUnsigned;
		if (const auto* graphics = StaticHumanMisc_JobGraphicsChange_GetDataPtr(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), newJob))
		{
			const auto bodyCount = Read<std::uint32_t>(graphics, NC2Layout::HumanJobGraphicsOffset::BodyCount);
			if (bodyCount != 0)
			{
				std::uint32_t index = 0;
				while (index < bodyCount && Read<std::uint32_t>(graphics, NC2Layout::HumanJobGraphicsOffset::BodyChoicesHex + index * NC2Layout::RecordSize::HumanBodyGraphicsUnsigned) != Read<std::uint32_t>(dynamic, NC2Layout::DynamicHumanOffset::BodyVariant)) ++index;
				if (index == bodyCount) index = 0;
				Write(dynamic, NC2Layout::DisplayAtomInformationOffset::BodyBobManager, Read<void*>(graphics, NC2Layout::HumanJobGraphicsOffset::BodyBobManagers + index * NC2Layout::RecordSize::HumanBodyGraphicsUnsigned));
				Write(dynamic, NC2Layout::DisplayAtomInformationOffset::ShadowBobManager, Read<void*>(graphics, NC2Layout::HumanJobGraphicsOffset::ShadowBobManagers + index * NC2Layout::RecordSize::HumanBodyGraphicsUnsigned));
			}
			const auto headCount = Read<std::uint32_t>(graphics, NC2Layout::HumanJobGraphicsOffset::HeadCount);
			if (headCount != 0)
			{
				std::uint32_t index = 0;
				while (index < headCount && Read<std::uint32_t>(graphics, NC2Layout::HumanJobGraphicsOffset::HeadChoices + index * NC2Layout::RecordSize::HumanHeadGraphicsUnsigned) != Read<std::uint32_t>(dynamic, NC2Layout::DynamicHumanOffset::HeadVariant)) ++index;
				if (index == headCount) index = 0;
				Write(dynamic, NC2Layout::DisplayAtomInformationOffset::HeadBobManager, Read<void*>(graphics, NC2Layout::HumanJobGraphicsOffset::HeadBobManagers + index * NC2Layout::RecordSize::HumanHeadGraphicsUnsigned));
			}
			const auto patch = Read<std::uint32_t>(graphics, NC2Layout::HumanJobChangeGraphicsOffset::RandomPatchId);
			if (patch != NXBasics::IntegerRange::UInt32Maximum) StaticHumanMisc_RandomPalette_Execute(patch, HumanBody(dynamic), HumanHead(dynamic), false);
		}
		if (NC2Logic::Human_IsJobSoldier(human)) E2DynamicDataHumanManager_HumanArmorChanged(humanId, NC2Logic::Human_Equipment_GetArmorType(human));
	}

	// Original function: FUN_00480676; armor patches follow the job patch.
	void E2DynamicDataHumanManager_HumanArmorChanged(std::uint32_t humanId, std::uint32_t armor)
	{
		char name[100];
		NXBasics::Runtime_PrintString(name, "human_armor_%3.3d", NXBasics::Runtime_TimeSigned32(armor));
		const auto patch = StaticHumanMisc_RandomPalette_GetPatchId(name);
		if (patch == -1) return;
		auto* dynamic = gDynamicDataHumans.entries + humanId * NC2Layout::RecordSize::DynamicHumanUnsigned;
		StaticHumanMisc_RandomPalette_Execute(static_cast<std::uint32_t>(patch), HumanBody(dynamic), HumanHead(dynamic), false);
	}

	// Original function: FUN_0049b3ed; the original destructor is an empty RET.
	static void __cdecl MarkingWhiteColor_AtExit()
	{}
	// Original function: FUN_0049b3ec; the original destructor is an empty RET.
	static void __cdecl MarkingGreyColor_AtExit()
	{}
	// Original function: FUN_0049b3eb; the original destructor is an empty RET.
	static void __cdecl MarkingFallbackColor_AtExit()
	{}

	// Original function: FUN_0049b337; original static colours have no destructor work.
	const NXBasics::SColorRGB* DrawAtom_Tool_GetMarkingColorRef(std::uint32_t markingType, std::int32_t playerId)
	{
		static const NXBasics::SColorRGB white(255, 255, 255);
		static const NXBasics::SColorRGB grey(170, 170, 170);
		static const NXBasics::SColorRGB fallback(255, 0, 255);
		static std::uint8_t initialized = 0;
		if ((initialized & 1u) == 0)
		{
			initialized |= 1u; NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(MarkingWhiteColor_AtExit));
		}
		if ((initialized & 2u) == 0)
		{
			initialized |= 2u; NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(MarkingGreyColor_AtExit));
		}
		if ((initialized & 4u) == 0)
		{
			initialized |= 4u; NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(MarkingFallbackColor_AtExit));
		}
		if (markingType == 1) return &white;
		if (markingType == 2) return &grey;
		if (markingType == 3) return Tool_PlayerColors_GetColorRef(playerId);
		return &fallback;
	}

	// Original function: FUN_0049445e; shared human, animal and vehicle drawing.
	bool sl_DoDrawMapMoveable(const void* moveable, void* dynamicData, std::uint32_t tribe, std::uint32_t job, std::uint32_t good, bool atomicActive, std::uint32_t action, std::uint32_t done, std::uint32_t total, std::uint32_t atomicAnimationStartCount, NXBasics::CPalette* bodyPalette, NXBasics::CPalette* headPalette, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness, std::uint32_t markingType, std::int32_t playerId, bool overrideAction, bool centered, bool transparent, const NXBasics::SColorRGB* effectColor, std::int32_t bodyRange, std::int32_t headRange, const void* display, bool ignorePathfinder)
	{
		const auto originalTribe = tribe;
		std::optional<SE2DynamicDataMapMoveableData> remasterFrames;
		if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 1)
		{
			tribe = NRemaster::HumanSprites::ResolveAppearanceTribe(tribe, job, playerId);
			if (NRemaster::HumanSprites::Supports(tribe, job))
			{
				remasterFrames.emplace(*static_cast<SE2DynamicDataMapMoveableData*>(dynamicData)); NRemaster::HumanSprites::PrepareNativeFallback(*remasterFrames, tribe, job); dynamicData = &*remasterFrames;
				if (transparent) job = NRemaster::HumanSprites::NativeFallbackJob(tribe, job);
			}
		}
		SMapMoveableAnimationOverlays overlays{ };
		std::int32_t offsetX, offsetY;
		sl_DoUpdateMapMoveableDynamicBobData(moveable, dynamicData, tribe, job, good, atomicActive, action, done, total, atomicAnimationStartCount, overrideAction, display, offsetX, offsetY, centered ? nullptr : &overlays, ignorePathfinder);
		if (auto* head = Remaster_GetSaracenWoodcutterHead(*static_cast<SE2DynamicDataMapMoveableData*>(dynamicData), tribe, job, atomicActive && !overrideAction ? action : 0u))
		{
			auto corrected = *static_cast<SE2DynamicDataMapMoveableData*>(dynamicData); corrected.headBobManager = head; remasterFrames = corrected; dynamicData = &*remasterFrames;
		}
		auto& dynamic = *static_cast<SE2DynamicDataMapMoveableData*>(dynamicData);
		auto* selectedPalette = NRemaster::HumanSprites::GetPalette(dynamic, playerId, bodyPalette);
		std::optional<NXBasics::CPalette> fallbackBody, fallbackHead;
		if (remasterFrames && tribe != originalTribe && selectedPalette == bodyPalette)
		{
			fallbackBody.emplace(*bodyPalette); fallbackHead.emplace(*headPalette);
			NRemaster::HumanSprites::PrepareNativeFallbackPalettes(tribe, job, playerId, *fallbackBody, *fallbackHead);
			selectedPalette = &*fallbackBody; headPalette = &*fallbackHead;
		}
		bodyPalette = selectedPalette;
		const auto drawX = centered ? x : AddOffset(x, offsetX);
		const auto drawY = centered ? y : AddOffset(y, offsetY);
		if (markingType != 0)
		{
			const auto* color = DrawAtom_Tool_GetMarkingColorRef(markingType, playerId);
			if (dynamic.bodyBobManager != nullptr && dynamic.bodyBobId != NXBasics::IntegerRange::UInt32Maximum)
				for (std::int32_t row = -1; row <= 1; ++row) NXBasics::BobManager_PrintBob_FX_DrawOutline(dynamic.bodyBobManager, dynamic.bodyBobId, bitmap, drawX, AddOffset(drawY, row), color, 1);
			if (dynamic.headBobManager != nullptr && dynamic.headBobId != NXBasics::IntegerRange::UInt32Maximum)
				for (std::int32_t row = -1; row <= 1; ++row) NXBasics::BobManager_PrintBob_FX_DrawOutline(dynamic.headBobManager, dynamic.headBobId, bitmap, drawX, AddOffset(drawY, row), color, 1);
		}
		const auto drawOverlays = [&] (std::uint32_t count, const SE2StaticDataMapMoveableAnimationPart* const* parts)
		{
			const auto* moveablePoint = static_cast<const std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition;
			const auto* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(moveablePoint);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2) return;
			std::int32_t anchorX = 0, anchorY = 0;
			if (display != nullptr)
			{
				std::int32_t moveablePixel[2], housePixel[2];
				Display_MapMigToLocalDisplayPixelCoordinates(display, moveablePoint, moveablePixel, false);
				const auto houseId = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 ? static_cast<std::uint32_t>(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId)) : NXBasics::IntegerRange::UInt32Maximum;
				const auto* housePoint = NC2Logic::CHouseArray::mStaticVars + houseId * NC2Logic::CHouseArray::ElementSize + 0x1C;
				Display_MapMigToLocalDisplayPixelCoordinates(display, housePoint, housePixel, false);
				anchorX = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(housePixel[0]) - static_cast<std::uint32_t>(moveablePixel[0]));
				anchorY = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(housePixel[1]) - static_cast<std::uint32_t>(moveablePixel[1]));
			}
			for (std::int32_t index = 0; index < NXBasics::Runtime_TimeSigned32(count); ++index)
			{
				const auto& part = *parts[index];
				const auto overlayX = AddOffset(AddOffset(x, anchorX), part.overlayOffsetX);
				const auto overlayY = AddOffset(AddOffset(y, anchorY), part.overlayOffsetY);
				const auto overlayBrightness = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(brightness) * 2u);
				if (part.type == NC2Runtime::MoveableAnimationPartType::LandscapeOverlay)
					EngineGraphic_Draw(static_cast<std::uint32_t>(part.overlayLandscapeId), 1, bitmap, overlayX, overlayY, NC2Logic::gGameTime.tick, overlayBrightness, -1, false);
				else if (part.type == NC2Runtime::MoveableAnimationPartType::HouseBobOverlay)
				{
					const auto houseId = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 ? static_cast<std::int32_t>(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId)) : -1;
					DrawAtom_House_Tool_DoDrawBob(houseId, bitmap, overlayX, overlayY, overlayBrightness, static_cast<std::uint32_t>(part.overlayBobId));
				}
			}
		};
		if (!centered) drawOverlays(overlays.behindCount, overlays.behind);
		bool drawn = false;
		if (dynamic.shadowBobManager != nullptr && dynamic.bodyBobId != NXBasics::IntegerRange::UInt32Maximum)
		{
			NXBasics::BobManager_PrintBob_Shadow(dynamic.shadowBobManager, dynamic.bodyBobId, bitmap, drawX, drawY);
			drawn = true;
		}
		if (effectColor != nullptr)
		{
			const std::int32_t glowX[8] = { -6, 6, 0, 0, -2, -2, 2, 2 };
			const std::int32_t glowY[8] = { 0, 0, -6, 6, -2, 2, -2, 2 };
			const std::uint32_t strength[8] = { 40, 40, 40, 40, 64, 64, 64, 64 };
			const auto drawGlow = [&] (const void* manager, std::uint32_t bobId, std::int32_t range)
			{
				if (manager == nullptr || bobId == NXBasics::IntegerRange::UInt32Maximum) return;
				for (std::uint32_t index = 0; index < 8; ++index)
				{
					if (range == -1)
						NXBasics::BobManager_PrintBob_FX_DrawArea(manager, bobId, bitmap, AddOffset(drawX, glowX[index]), AddOffset(drawY, glowY[index]), effectColor, static_cast<std::int32_t>(strength[index]));
					else
					{
						const auto firstIndex = static_cast<std::uint8_t>((static_cast<std::uint32_t>(range) & 15u) << 4);
						NXBasics::BobManager_PrintBob_Glow_AddColor_OnlyBobByteRange(manager, bobId, bitmap, AddOffset(drawX, glowX[index]), AddOffset(drawY, glowY[index]), effectColor, strength[index], firstIndex, static_cast<std::uint8_t>(firstIndex + 15u));
					}
				}
			};
			if (bodyRange == -1 && headRange == -1)
			{
				drawGlow(dynamic.bodyBobManager, dynamic.bodyBobId, -1);
				drawGlow(dynamic.headBobManager, dynamic.headBobId, -1);
			}
			else
			{
				if (bodyRange != -1) drawGlow(bodyRange < 16 ? dynamic.bodyBobManager : dynamic.headBobManager, bodyRange < 16 ? dynamic.bodyBobId : dynamic.headBobId, bodyRange);
				if (headRange != -1) drawGlow(headRange < 16 ? dynamic.bodyBobManager : dynamic.headBobManager, headRange < 16 ? dynamic.bodyBobId : dynamic.headBobId, headRange);
			}
		}
		if (dynamic.bodyBobManager != nullptr && dynamic.bodyBobId != NXBasics::IntegerRange::UInt32Maximum)
		{
			if (NXBasics::BobManager_GetBobType(dynamic.bodyBobManager, dynamic.bodyBobId) == 4)
				NXBasics::BobManager_PrintBob_UsingShadedAlpha(dynamic.bodyBobManager, dynamic.bodyBobId, bitmap, drawX, drawY, bodyPalette, brightness);
			else if (!transparent)
				NXBasics::BobManager_PrintBob_Shade_SSE2(dynamic.bodyBobManager, dynamic.bodyBobId, bitmap, drawX, drawY, bodyPalette, brightness);
			else
				NXBasics::BobManager_PrintBob_UsingTransparency(dynamic.bodyBobManager, dynamic.bodyBobId, bitmap, 128u, drawX, drawY, bodyPalette);
			drawn = true;
		}
		if (dynamic.headBobManager != nullptr && dynamic.headBobId != NXBasics::IntegerRange::UInt32Maximum)
		{
			if (!transparent)
				NXBasics::BobManager_PrintBob_Shade_SSE2(dynamic.headBobManager, dynamic.headBobId, bitmap, drawX, drawY, headPalette, brightness);
			else
				NXBasics::BobManager_PrintBob_UsingTransparency(dynamic.headBobManager, dynamic.headBobId, bitmap, 128u, drawX, drawY, headPalette);
			drawn = true;
		}
		if (!centered) drawOverlays(overlays.frontCount, overlays.front);
		return drawn;
	}

	// Original function: FUN_00494ae6; frame selection shared by drawing and depth descriptors.
	void sl_DoUpdateMapMoveableDynamicBobData(const void* moveable, void* dynamicData, std::uint32_t tribe, std::uint32_t job, std::uint32_t good, bool atomicActive, std::uint32_t action, std::uint32_t done, std::uint32_t total, std::uint32_t atomicAnimationStartCount, bool overrideAction, const void* display, std::int32_t& offsetX, std::int32_t& offsetY, SMapMoveableAnimationOverlays* overlays, bool ignorePathfinder)
	{
		auto& dynamic = *static_cast<SE2DynamicDataMapMoveableData*>(dynamicData);
		offsetX = 0; offsetY = 0;
		if (overrideAction)
		{
			atomicAnimationStartCount = NXBasics::IntegerRange::UInt32Maximum;
			atomicActive = true;
			action = 2;
			done = 0;
			total = 1;
		}
		if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 1 && NRemaster::HumanSprites::Supports(tribe, job))
		{
			if (NRemaster::HumanSprites::SelectFrame(moveable, dynamic, tribe, job, good, atomicActive, action, done, total)) return;
			// Unavailable prototype poses use native art from the same civilization, retaining the actual atomic clock.
			job = NRemaster::HumanSprites::NativeFallbackJob(tribe, job);
		}
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) != 0)
		{
			const auto* walk = GetCachedWalk(moveable, dynamic, tribe, job, good);
			if (walk == nullptr) return;
			auto direction = DrawingDirection(moveable);
			if (walk->turnFrames[direction].frames == nullptr)
			{
				SetFrame(dynamic, static_cast<std::uint32_t>(walk->bodySequence), static_cast<std::uint32_t>(walk->headSequence), walk->walkFrames[direction].frames[0]);
				return;
			}
			auto turnDone = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningElapsedTicks);
			const auto turnTotal = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningTicksPerDirection);
			const auto target = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurnTargetDirection);
			const bool vertical = Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::IncludeVerticalDirectionsFlag) != 0;
			const auto nextDirection = NC2Logic::VE_HexagonDirection_NextDirection(direction, target, vertical);
			if (NC2Logic::VE_HexagonDirection_DirectionToTurn(direction, target, vertical) < 0)
			{
				direction = nextDirection;
				turnDone = turnTotal - turnDone - 1u;
			}
			const auto& frames = walk->turnFrames[direction];
			auto index = (frames.count * turnDone) / turnTotal;
			if (index >= frames.count) index = frames.count - 1u;
			SetFrame(dynamic, static_cast<std::uint32_t>(walk->bodySequence), static_cast<std::uint32_t>(walk->headSequence), frames.frames[index]);
			return;
		}
		if (!atomicActive)
		{
			if (!ignorePathfinder && Read<std::uint32_t>(NC2Logic::MapMoveable_Pathfinder_GetPathfinderPtr(moveable), NC2Layout::MapPathfinderOffset::SearchState) == NC2Runtime::PathfinderSearchState::SearchRequired) return;
			const auto* walk = GetCachedWalk(moveable, dynamic, tribe, job, good);
			if (walk == nullptr) return;
			const auto& frames = walk->walkFrames[DrawingDirection(moveable)];
			SetFrame(dynamic, static_cast<std::uint32_t>(walk->bodySequence), static_cast<std::uint32_t>(walk->headSequence), frames.frames[NC2Logic::gGameTime.tick % frames.count]);
			return;
		}
		if (dynamic.animation == nullptr || dynamic.animationTribe != tribe || dynamic.animationJob != job || dynamic.animationAction != action || dynamic.animationStartCount != atomicAnimationStartCount)
		{
			dynamic.animation = nullptr;
			dynamic.animation = FindAnimation(tribe, job, action, 0);
			if (dynamic.animation != nullptr)
			{
				dynamic.animationTribe = tribe;
				dynamic.animationJob = job;
				dynamic.animationAction = action;
				dynamic.animationStartCount = atomicAnimationStartCount;
			}
		}
		const auto* animation = dynamic.animation;
		if (animation == nullptr) return;
		if (animation->animationMode == NC2Runtime::MoveableAnimationMode::ProgressFrames || animation->animationMode == NC2Runtime::MoveableAnimationMode::CyclicFrames)
		{
			const auto& frames = animation->frames.frames != nullptr ? animation->frames : animation->directionFrames[DrawingDirection(moveable)];
			SetAnimationFrame(dynamic, *animation, frames, done, total);
			return;
		}
		if (animation->animationMode != NC2Runtime::MoveableAnimationMode::Compound) return;
		const auto partCount = animation->bodySequenceOrPartCount;
		const auto* parts = reinterpret_cast<const SE2StaticDataMapMoveableAnimationPart*>(static_cast<std::uintptr_t>(animation->headSequenceOrPartAddress));
		const auto animationProgress = (done * 10000u) / total;
		if (display != nullptr && Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
		{
			const auto* point = static_cast<const std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition;
			const auto* micro = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			{
				const auto houseId = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId);
				const auto* housePoint = NC2Logic::CHouseArray::mStaticVars + houseId * NC2Logic::CHouseArray::ElementSize + 0x1C;
				std::int32_t moveablePixel[2], housePixel[2];
				Display_MapMigToLocalDisplayPixelCoordinates(display, point, moveablePixel, false);
				Display_MapMigToLocalDisplayPixelCoordinates(display, housePoint, housePixel, false);
				offsetX = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(housePixel[0]) - static_cast<std::uint32_t>(moveablePixel[0]));
				offsetY = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(housePixel[1]) - static_cast<std::uint32_t>(moveablePixel[1]));
			}
		}
		std::int32_t startX = 0, startY = 0;
		for (std::uint32_t index = 0; index < partCount; ++index)
		{
			const auto& part = parts[index];
			const auto partStartProgress = part.startPercent * 100u;
			const auto partEndProgress = part.endPercent * 100u;
			if (animationProgress < partStartProgress) continue;
			if (animationProgress >= partEndProgress)
			{
				if (part.type == NC2Runtime::MoveableAnimationPartType::Walk)
				{
					startX = part.walkEndOffsetX;
					startY = part.walkEndOffsetY;
				}
				continue;
			}
			const auto progress = (animationProgress * 10000u - partStartProgress * 10000u) / (partEndProgress - partStartProgress);
			if (part.type == NC2Runtime::MoveableAnimationPartType::HouseBobOverlay || part.type == NC2Runtime::MoveableAnimationPartType::LandscapeOverlay) continue;
			if (part.type == NC2Runtime::MoveableAnimationPartType::Walk)
			{
				auto lookupJob = job;
				const SE2StaticDataMapMoveableWalkData* walk;
				while (true)
				{
					walk = StaticMapMoveables_GetWalkDataPtr(gE2StaticDataMapMoveablesManager, tribe, lookupJob, static_cast<std::uint32_t>(part.walkGood), Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingTicksPerStep));
					if (walk != nullptr) break;
					lookupJob = ParentJob(lookupJob);
					if (lookupJob == 0) break;
				}
				if (walk != nullptr)
				{
					const auto& frames = walk->walkFrames[part.walkDirection];
					SetFrame(dynamic, static_cast<std::uint32_t>(walk->bodySequence), static_cast<std::uint32_t>(walk->headSequence), frames.frames[NC2Logic::gGameTime.tick % frames.count]);
					startX = InterpolateOffset(startX, part.walkEndOffsetX, progress);
					startY = InterpolateOffset(startY, part.walkEndOffsetY, progress);
				}
			}
			else if (part.type == NC2Runtime::MoveableAnimationPartType::AtomicAnimation)
			{
				const auto* atomic = FindAnimation(tribe, job, static_cast<std::uint32_t>(part.atomicAction), static_cast<std::uint32_t>(part.atomicSubId));
				if (atomic != nullptr)
				{
					const auto direction = part.atomicDirection == -1 ? DrawingDirection(moveable) : static_cast<std::uint32_t>(part.atomicDirection);
					const auto& frames = atomic->frames.frames != nullptr ? atomic->frames : atomic->directionFrames[direction];
					SetAnimationFrame(dynamic, *atomic, frames, progress, 10000);
				}
			}
			offsetX = AddOffset(offsetX, startX);
			offsetY = AddOffset(offsetY, startY);
			break;
		}
		if (overlays == nullptr) return;
		bool behind = true;
		for (std::uint32_t index = 0; index < partCount; ++index)
		{
			const auto& part = parts[index];
			if (animationProgress < part.startPercent * 100u || animationProgress >= part.endPercent * 100u) continue;
			if (part.type == NC2Runtime::MoveableAnimationPartType::Walk || part.type == NC2Runtime::MoveableAnimationPartType::AtomicAnimation) behind = false;
			else if (part.type == NC2Runtime::MoveableAnimationPartType::HouseBobOverlay || part.type == NC2Runtime::MoveableAnimationPartType::LandscapeOverlay)
			{
				auto& count = behind ? overlays->behindCount : overlays->frontCount;
				auto* entries = behind ? overlays->behind : overlays->front;
				if (count != NC2Runtime::MoveableAnimationLimits::OverlaysPerLayer) entries[count++] = &part;
			}
		}
	}
}

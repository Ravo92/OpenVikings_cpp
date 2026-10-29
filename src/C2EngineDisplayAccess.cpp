#include "ApplicationGlobals.h"
#include "C2E2LevelOfDetailManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDisplayManager.h"
#include "C2EngineLandscapeDrawing.h"
#include "C2EngineParticleDrawing.h"
#include "C2EngineWeatherDrawing.h"
#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "CDAudioManager.h"
#include "GouraudShader.h"
#include "NetworkSessionAccess.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XBobManager.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <windows.h>


// Untranslated original routines. The unused EDX parameter preserves the
// ECX receiver and callee-popped stack of their Windows __thiscall interfaces.
extern bool __cdecl C2IGG_EntryPoint_InputManager_Highlighting_Update(const std::int16_t* rectangle);
extern void __cdecl DrawAtom_FishManager_DoDraw(void* display);
extern void __fastcall Display_l_DE_Debug_DrawMapMoveablesDebugInfos(void* display);
extern void __fastcall Display_l_DE_Debug_DrawGridLines(void* display);

namespace NC2E2
{
	// Original function: FUN_0047c6fb; thirty packed map points at stride 0x48.
	static void* DisplaySecondaryObjectReferences_Construct(void* references)
	{
		for (std::uint32_t index = 0; index < NC2Runtime::DisplaySelectionLimits::SecondaryMarks; ++index)
		{
			NC2Lifetime::Write<std::uint16_t>(references, index * NC2Layout::RecordSize::DisplayMarkedObject + NC2Layout::DisplayMarkingOffset::FirstSecondaryMapPositionY, 0);
			NC2Lifetime::Write<std::uint16_t>(references, index * NC2Layout::RecordSize::DisplayMarkedObject + NC2Layout::DisplayMarkingOffset::FirstSecondaryMapPositionX, 0);
		}
		return references;
	}

	// Original function: FUN_0047cc5b.
	void DE_Highlighting_SetState(void* display, bool enabled)
	{
		if (NC2Lifetime::Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::HighlightingEnabled) == static_cast<std::uint8_t>(enabled)) return;
		NC2Lifetime::Write<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::HighlightingEnabled, enabled);
		NC2Lifetime::Write<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag, 1);
	}

	// Original function: FUN_0047ce8e; a missing point returns zero.
	std::uint32_t DE_Hightlighting_GetHighlightBrightness(const void* display, const void* point)
	{
		if (NC2Lifetime::Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::HighlightingEnabled) != 0)
		{
			const void* data = EngineMap_GetConstMapMicroPointPtr(point);
			if (data == nullptr) return 0;
			const auto flags = NC2Lifetime::Read<std::uint8_t>(data, NC2Layout::EngineMapMicroPointOffset::Flags);
			if ((flags & 1u) != 0) return ((flags & 2u) | 1u) << 6;
		}
		return 128;
	}

	// Original function: FUN_00480fe3; unchecked native array access.
	const void* Engine2d_EntryPoint_GetPatternDataPtr(std::uint32_t type)
	{
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(gStaticPatternStorage.entries) + type * 0x70u);
	}
	// Original function: FUN_00480ff1; unchecked native array access.
	const void* Engine2d_EntryPoint_GetLandscapeDataPtr(std::uint32_t type)
	{
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(gStaticLandscapeStorage.entries) + type * 0x8Cu);
	}

	// Original function: FUN_00481a47; two point copies precede the dimensions.
	void* S2DEngineDisplayRectangle_Copy(void* destination, const void* source)
	{
		NC2Lifetime::Write(destination, NC2Layout::EngineDisplayRectangleOffset::WorldX, NC2Lifetime::Read<std::uint32_t>(source, NC2Layout::EngineDisplayRectangleOffset::WorldX));
		NC2Lifetime::Write(destination, NC2Layout::EngineDisplayRectangleOffset::WorldY, NC2Lifetime::Read<std::uint32_t>(source, NC2Layout::EngineDisplayRectangleOffset::WorldY));
		NC2Lifetime::Write(destination, NC2Layout::EngineDisplayRectangleOffset::BitmapX, NC2Lifetime::Read<std::uint32_t>(source, NC2Layout::EngineDisplayRectangleOffset::BitmapX));
		NC2Lifetime::Write(destination, NC2Layout::EngineDisplayRectangleOffset::BitmapY, NC2Lifetime::Read<std::uint32_t>(source, NC2Layout::EngineDisplayRectangleOffset::BitmapY));
		NC2Lifetime::Write(destination, NC2Layout::EngineDisplayRectangleOffset::Width, NC2Lifetime::Read<std::uint32_t>(source, NC2Layout::EngineDisplayRectangleOffset::Width));
		NC2Lifetime::Write(destination, NC2Layout::EngineDisplayRectangleOffset::Height, NC2Lifetime::Read<std::uint32_t>(source, NC2Layout::EngineDisplayRectangleOffset::Height));
		return destination;
	}

	// Original function: FUN_004821b3; wrapped multiply/add precedes signed shift.
	std::int32_t DED_MapHeightToPixelHeight(const void* description, std::int16_t height)
	{
		return NXBasics::Runtime_TimeSigned32(NC2Lifetime::Read<std::uint32_t>(description, NC2Layout::DisplayDescriptionOffset::MicroHeightFixed) * static_cast<std::uint32_t>(height) + NC2Runtime::DisplayProjection::RoundingBias) >> NC2Runtime::DisplayProjection::FractionBits;
	}

	// Original function: FUN_0048245d.
	const void* EngineMap_GetConstMapMacroPointPtr(std::int16_t x, std::int16_t y)
	{
		if (x < 0 || x >= static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroWidth()) || y < 0 || y >= static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroHeight())) return nullptr;
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(NC2Lifetime::Read<void*>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MacroPoints)) + (static_cast<std::uint32_t>(static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroWidth())) * static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(x)) * NC2Layout::RecordSize::EngineMapMacroPointUnsigned);
	}
	// Original function: FUN_004824a5.
	const void* EngineMap_GetConstMapMicroPointPtr(std::int16_t x, std::int16_t y)
	{
		if (x < 0 || x >= NC2Logic::GameMap_GetWidth() || y < 0 || y >= NC2Logic::GameMap_GetHeight()) return nullptr;
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(NC2Lifetime::Read<void*>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MicroPoints)) + (static_cast<std::uint32_t>(NC2Logic::GameMap_GetWidth()) * static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(x)) * NC2Layout::RecordSize::EngineMapMicroPointUnsigned);
	}

	// Original function: FUN_00483f80.
	bool DisplayObjectReference_Tool_IsHouse(const void* reference, std::uint32_t id)
	{
		return NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::House && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::HouseId) == id && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::HouseUniqueId) == NC2Lifetime::Read<std::uint32_t>(NC2Logic::CHouseArray::mStaticVars, id * NC2Logic::CHouseArray::ElementSize + NC2Layout::HouseOffset::UniqueIdUnsigned);
	}
	// Original function: FUN_00483fad.
	bool DisplayObjectReference_Tool_IsHuman(const void* reference, std::uint32_t id)
	{
		return NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::Human && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::HumanId) == id && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::HumanUniqueId) == NC2Lifetime::Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars, id * NC2Logic::CHumanArray::ElementSize + NC2Layout::HumanOffset::UniqueIdUnsigned);
	}
	// Original function: FUN_00483fda.
	bool DisplayObjectReference_Tool_IsAnimal(const void* reference, std::uint32_t id)
	{
		return NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::Animal && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::AnimalId) == id && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::AnimalUniqueId) == NC2Lifetime::Read<std::uint32_t>(NC2Logic::CAnimalArray::mStaticVars, id * NC2Logic::CAnimalArray::ElementSize + NC2Layout::AnimalOffset::UniqueIdUnsigned);
	}
	// Original function: FUN_00484007.
	bool DisplayObjectReference_Tool_IsVehicle(const void* reference, std::uint32_t id)
	{
		return NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::Vehicle && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::VehicleId) == id && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::VehicleUniqueId) == NC2Lifetime::Read<std::uint32_t>(NC2Logic::CVehicleArray::mStaticVars, id * NC2Logic::CVehicleArray::ElementSize + NC2Layout::VehicleOffset::UniqueIdUnsigned);
	}
	// Original function: FUN_00484034.
	bool DisplayObjectReference_Tool_IsGuide(const void* reference, std::uint32_t id)
	{
		return NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::Guide && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::GuideId) == id && NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::GuideUniqueId) == NC2Lifetime::Read<std::uint32_t>(NC2Logic::CGuideArray::mStaticVars, id * NC2Logic::CGuideArray::ElementSize + NC2Layout::GuideOffset::UniqueIdUnsigned);
	}

	// Original function: FUN_00484b95; distinct native forwarding entry.
	static bool DisplayMarkedObject_Tool_IsHouse(const void* reference, std::uint32_t id)
	{
		return DisplayObjectReference_Tool_IsHouse(reference, id);
	}
	// Original function: FUN_00484ba1.
	static bool DisplayMarkedObject_Tool_IsHuman(const void* reference, std::uint32_t id)
	{
		return DisplayObjectReference_Tool_IsHuman(reference, id);
	}
	// Original function: FUN_00484bad.
	static bool DisplayMarkedObject_Tool_IsAnimal(const void* reference, std::uint32_t id)
	{
		return DisplayObjectReference_Tool_IsAnimal(reference, id);
	}
	// Original function: FUN_00484bb9.
	static bool DisplayMarkedObject_Tool_IsVehicle(const void* reference, std::uint32_t id)
	{
		return DisplayObjectReference_Tool_IsVehicle(reference, id);
	}
	// Original function: FUN_00484bc5.
	static bool DisplayMarkedObject_Tool_IsGuide(const void* reference, std::uint32_t id)
	{
		return DisplayObjectReference_Tool_IsGuide(reference, id);
	}

	// Original function: FUN_0047cfea.
	void Display_DE_ObjectsToMark_AddSecondaryHuman(void* display, std::uint32_t markType, std::uint32_t humanId)
	{
		using namespace NC2Lifetime;
		const auto count = Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount);
		if (count == NC2Runtime::DisplaySelectionLimits::SecondaryMarks) return;
		auto* reference = static_cast<std::byte*>(display) + NC2Layout::EngineDisplayOffset::SecondaryMarkedObjects + count * NC2Layout::RecordSize::DisplayMarkedObject;
		Write(reference, NC2Layout::DisplayObjectReferenceOffset::MarkType, markType);
		DisplayObjectReference_Reset(reference);
		Write<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::Type, SwitchIds::MapObjectUnderCursor::Human);
		Write(reference, NC2Layout::DisplayObjectReferenceOffset::HumanId, humanId);
		Write(reference, NC2Layout::DisplayObjectReferenceOffset::HumanUniqueId, Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars + humanId * NC2Logic::CHumanArray::ElementSize, NC2Layout::HumanOffset::UniqueId));
		Write(display, NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount, count + 1u);
	}

	// Original function: FUN_0047d06d.
	void Display_DE_ObjectsToMark_AddSecondaryHouse(void* display, std::uint32_t markType, std::uint32_t houseId)
	{
		using namespace NC2Lifetime;
		const auto count = Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount);
		if (count == NC2Runtime::DisplaySelectionLimits::SecondaryMarks) return;
		auto* reference = static_cast<std::byte*>(display) + NC2Layout::EngineDisplayOffset::SecondaryMarkedObjects + count * NC2Layout::RecordSize::DisplayMarkedObject;
		Write(reference, NC2Layout::DisplayObjectReferenceOffset::MarkType, markType);
		DisplayObjectReference_Reset(reference);
		Write<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::Type, SwitchIds::MapObjectUnderCursor::House);
		Write(reference, NC2Layout::DisplayObjectReferenceOffset::HouseId, houseId);
		Write(reference, NC2Layout::DisplayObjectReferenceOffset::HouseUniqueId, Read<std::uint32_t>(NC2Logic::CHouseArray::mStaticVars + houseId * NC2Logic::CHouseArray::ElementSize, NC2Layout::HouseOffset::UniqueId));
		Write(display, NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount, count + 1u);
	}
	static std::uint32_t GetMoveableMarkType(const void* display, std::uint32_t id, std::uint32_t type, std::uint32_t idOffset, const std::byte* objects, std::uint32_t stride)
	{
		using NC2Lifetime::Read;
		const auto matches = [=] (const void* reference)
		{
			return type == SwitchIds::MapObjectUnderCursor::Human ? DisplayMarkedObject_Tool_IsHuman(reference, id) : type == SwitchIds::MapObjectUnderCursor::Animal ? DisplayMarkedObject_Tool_IsAnimal(reference, id) : DisplayMarkedObject_Tool_IsVehicle(reference, id);
		};
		const void* primary = Read<void*>(display, NC2Layout::EngineDisplayOffset::PrimaryMarkedObject);
		if (primary != nullptr && matches(primary)) return Read<std::uint32_t>(primary, NC2Layout::DisplayObjectReferenceOffset::MarkType);
		const auto count = Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount);
		for (std::int32_t index = 0; index < count; ++index)
		{
			const auto* reference = static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::SecondaryMarkedObjects + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::DisplayMarkedObject;
			if (matches(reference)) return Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::MarkType);
		}
		return 0;
	}

	// Original function: FUN_0047d0ef; only the primary house reference checks its generation.
	std::uint32_t Display_DE_ObjectsToMark_House_GetMarkType(const void* display, std::uint32_t houseId)
	{
		using NC2Lifetime::Read;
		const auto* primary = Read<void*>(display, NC2Layout::EngineDisplayOffset::PrimaryMarkedObject);
		if (primary != nullptr && DisplayMarkedObject_Tool_IsHouse(primary, houseId)) return Read<std::uint32_t>(primary, NC2Layout::DisplayObjectReferenceOffset::MarkType);
		for (std::int32_t index = 0; index < Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount); ++index)
		{
			const auto* reference = static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::SecondaryMarkedObjects + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::DisplayMarkedObject;
			if (Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::Type) == SwitchIds::MapObjectUnderCursor::House && Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::HouseId) == houseId) return Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::MarkType);
		}
		return 0;
	}

	// Original function: FUN_0047d14d; reference predicates also compare generation.
	std::uint32_t Display_DE_ObjectsToMark_Human_GetMarkType(const void* display, std::uint32_t humanId)
	{
		return GetMoveableMarkType(display, humanId, SwitchIds::MapObjectUnderCursor::Human, NC2Layout::DisplayObjectReferenceOffset::HumanId, NC2Logic::CHumanArray::mStaticVars, NC2Logic::CHumanArray::ElementSize);
	}
	// Original function: FUN_0047d1c7.
	std::uint32_t Display_DE_ObjectsToMark_Animal_GetMarkType(const void* display, std::uint32_t animalId)
	{
		return GetMoveableMarkType(display, animalId, SwitchIds::MapObjectUnderCursor::Animal, NC2Layout::DisplayObjectReferenceOffset::AnimalId, NC2Logic::CAnimalArray::mStaticVars, NC2Logic::CAnimalArray::ElementSize);
	}
	// Original function: FUN_0047d241.
	std::uint32_t Display_DE_ObjectsToMark_Vehicle_GetMarkType(const void* display, std::uint32_t vehicleId)
	{
		return GetMoveableMarkType(display, vehicleId, SwitchIds::MapObjectUnderCursor::Vehicle, NC2Layout::DisplayObjectReferenceOffset::VehicleId, NC2Logic::CVehicleArray::mStaticVars, NC2Logic::CVehicleArray::ElementSize);
	}

	// Original function: FUN_0047d2bb; both guide references validate the generation at +0x10.
	std::uint32_t Display_DE_ObjectsToMark_Guide_GetMarkType(const void* display, std::uint32_t guideId)
	{
		using NC2Lifetime::Read;
		const auto matches = [=] (const void* reference)
		{
			return DisplayMarkedObject_Tool_IsGuide(reference, guideId);
		};
		const auto* primary = Read<void*>(display, NC2Layout::EngineDisplayOffset::PrimaryMarkedObject);
		if (primary != nullptr && matches(primary)) return Read<std::uint32_t>(primary, NC2Layout::DisplayObjectReferenceOffset::MarkType);
		for (std::int32_t index = 0; index < Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount); ++index)
		{
			const auto* reference = static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::SecondaryMarkedObjects + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::DisplayMarkedObject;
			if (matches(reference)) return Read<std::uint32_t>(reference, NC2Layout::DisplayObjectReferenceOffset::MarkType);
		}
		return 0;
	}

	// Original function: FUN_004824ea; C2DEngineMap::GetConstMapMacroPointPtr.
	const void* EngineMap_GetConstMapMacroPointPtr(const void* point)
	{
		if (!NC2Logic::MapMagPoint_IsInsideMap(point)) return nullptr;
		std::int16_t coordinates[2];
		std::memcpy(coordinates, point, sizeof(coordinates));
		const auto index = static_cast<std::uint32_t>(static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroWidth())) * static_cast<std::uint32_t>(coordinates[1]) + static_cast<std::uint32_t>(coordinates[0]);
		void* entries;
		std::memcpy(&entries, gEngineMapStaticStorage, sizeof(entries));
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(entries) + index * 12u);
	}
	// Original function: FUN_0048251f; C2DEngineMap::GetConstMapMicroPointPtr.
	const void* EngineMap_GetConstMapMicroPointPtr(const void* point)
	{
		if (!NC2Logic::MapMigPoint_IsInsideMap(point)) return nullptr;
		std::int16_t coordinates[2];
		std::memcpy(coordinates, point, sizeof(coordinates));
		const auto index = static_cast<std::uint32_t>(NC2Logic::GameMap_GetWidth()) * static_cast<std::uint32_t>(coordinates[1]) + static_cast<std::uint32_t>(coordinates[0]);
		void* entries;
		std::memcpy(&entries, gEngineMapStaticStorage + NC2Layout::EngineMapStaticOffset::MicroPoints, sizeof(entries));
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(entries) + index * 4u);
	}
	// Original function: FUN_00481198.
	const void* Engine2d_EntryPoint_Tool_GetEngineMacroPointDataPtr(const void* point)
	{
		return EngineMap_GetConstMapMacroPointPtr(point);
	}
	// Original function: FUN_004811a3.
	const void* Engine2d_EntryPoint_Tool_GetEngineMicroPointDataPtr(const void* point)
	{
		return EngineMap_GetConstMapMicroPointPtr(point);
	}

	// Original function: FUN_004986ab.
	static void Display_InformGuiRedraw(void* display)
	{
		std::uint8_t enabled;
		std::memcpy(&enabled, static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::ActiveDisplayFlag, sizeof(enabled));
		if (enabled != 0) NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0x19, true, 0, 0, 0);
	}

	// Original function: FUN_0048b442; existing complete human-misc storage.
	const void* StaticHumanMisc_ColorIds_GetColorRef(std::uint32_t playerColorId)
	{
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(gStaticHumanMiscData) + NC2Layout::StaticHumanMiscOffset::PlayerColorsUnsigned + playerColorId * NC2Layout::RecordSize::StaticHumanColorUnsigned);
	}

	// Original function: FUN_0048b441; the original destructor is an empty RET.
	static void __cdecl InvalidPlayerColor_AtExit()
	{}
	// Original function: FUN_0048b440; the original destructor is an empty RET.
	static void __cdecl NeutralPlayerColor_AtExit()
	{}
	// Original function: FUN_0048b3a1; the two exit callbacks are empty.
	const NXBasics::SColorRGB* StaticHumanMisc_ColorIds_GetPlayerColorRef(std::int32_t playerId)
	{
		// Four-byte colors at 00568964 and 00568974; lazy flag bits at 0056895C.
		static const NXBasics::SColorRGB invalid(255, 0, 255);
		static const NXBasics::SColorRGB neutral(80, 80, 80);
		static std::uint8_t initialized = 0;
		if ((initialized & 1u) == 0)
		{
			initialized |= 1u; NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(InvalidPlayerColor_AtExit));
		}
		if ((initialized & 2u) == 0)
		{
			initialized |= 2u; NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(NeutralPlayerColor_AtExit));
		}
		if (playerId == 20) return &neutral;
		if (playerId >= 20) return &invalid;
		const void* player = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(NC2Logic::gGameControl) + static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer);
		std::uint8_t active;
		std::memcpy(&active, player, sizeof(active));
		if (active == 0) return &invalid;
		std::int32_t color;
		std::memcpy(&color, static_cast<const std::byte*>(player) + 0x10, sizeof(color));
		if (color < 0 || color >= 10) color = 9;
		return static_cast<const NXBasics::SColorRGB*>(StaticHumanMisc_ColorIds_GetColorRef(static_cast<std::uint32_t>(color)));
	}

	// Original function: FUN_00490edf; a distinct original forwarding entry.
	const NXBasics::SColorRGB* Tool_PlayerColors_GetColorRef(std::int32_t playerId)
	{
		return StaticHumanMisc_ColorIds_GetPlayerColorRef(playerId);
	}

	// Original function: FUN_0048089e.
	const void* Engine2d_EntryPoint_GetHouseDataPtr(std::uint32_t type)
	{
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(gStaticHouseEntries) + static_cast<std::size_t>(type) * NC2Layout::RecordSize::EngineHouseType + NC2Layout::EngineHouseTypeOffset::LogicTypes);
	}

	std::int32_t StaticDataHouses_GetHouseId(std::uint32_t tribe, std::uint32_t logicType, std::uint32_t* stage);
	// Original function: FUN_004808be; distinct Windows forwarding entry, semantic name from its verified target.
	std::int32_t Engine2d_EntryPoint_GetHouseId(std::uint32_t tribe, std::uint32_t logicType, std::uint32_t* stage)
	{
		return StaticDataHouses_GetHouseId(tribe, logicType, stage);
	}

	// Original owning pointer: 00554DA0. Windows establishes the ownership;
	// the exact original symbol name is unavailable in the Mac reference.
	NXBasics::CBitmap* gDesktopOwnedBobBitmap = nullptr;

	// Original function: FUN_0047e5f4; height is unused by the Windows routine.
	void Inform_DesktopXTructed(std::int32_t width, std::int32_t height, std::uint32_t depth)
	{
		(void) height;
		if (width != 0)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CBitmap), 1);
			NXBasics::CBitmap* bitmap = allocation == nullptr ? nullptr : ::new (allocation) NXBasics::CBitmap(100, 100, NXBasics::ConvertBitsToTDepth(static_cast<std::int32_t>(depth)));
			// Remaster: retain the existing owner on failure and release it on replacement.
			if (bitmap == nullptr) return;
			NXBasics::CBitmap* previous = gDesktopOwnedBobBitmap;
			gDesktopOwnedBobBitmap = bitmap;
			NXBasics::CBobManager::mStaticVars = bitmap;
			delete previous;
		}
		else
		{
			NXBasics::CBitmap* bitmap = gDesktopOwnedBobBitmap;
			NXBasics::CBobManager::mStaticVars = nullptr;
			if (bitmap != nullptr)
				delete bitmap;
			gDesktopOwnedBobBitmap = nullptr;
		}
	}

	// Original function: FUN_0047e650.
	void Inform_ControllingPlayerChanged(std::int32_t playerId)
	{
		C2DEngineDisplayManager::sTheObjectPtr->InformThatControllingPlayerChanged(playerId);
		gWorldMusicState.controllingPlayer = static_cast<std::uint32_t>(playerId);
	}

	static void DisplayDescription_WriteDword(void* description, std::size_t offset, std::uint32_t bits)
	{
		std::memcpy(static_cast<std::byte*>(description) + offset, &bits, sizeof(bits));
	}

	static std::uint32_t DisplayDescription_ShiftRightSigned(std::uint32_t bits, std::uint32_t shift)
	{
		shift &= 31u;

		if (shift == 0)
			return bits;

		const std::uint32_t shifted = bits >> shift;

		if ((bits & NXBasics::IntegerRange::Bit31) != 0)
			return shifted | (NXBasics::IntegerRange::UInt32Maximum << (32u - shift));

		return shifted;
	}

	static std::uint32_t DisplayDescription_HalveSigned(std::uint32_t bits)
	{
		// CDQ; SUB EAX, EDX; SAR EAX, 1.
		return DisplayDescription_ShiftRightSigned(bits + (bits >> 31), 1);
	}

	static std::uint32_t DisplayDescription_FixedToPixels(std::uint32_t bits)
	{
		// Preserve the original 32-bit addition and arithmetic right shift.
		return DisplayDescription_ShiftRightSigned(bits + NC2Runtime::DisplayProjection::RoundingBias, NC2Runtime::DisplayProjection::FractionBits);
	}

	static std::uint32_t DisplayDescription_ConvertScaledFloat(float value, float scale)
	{
		// Two float significands fit in a double product without float rounding.
		// The caller consumes only the low DWORD of the signed 64-bit result.
		const double scaledValue = static_cast<double>(value) * static_cast<double>(scale);
		return static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(scaledValue));
	}

	// Original function: FUN_00481e72
	void* DisplayDescription_Construct(void* description)
	{
		DisplayDescription_AssignNewTriangleValues(description, NC2Runtime::DisplayProjection::DefaultVerticalSpacing, NC2Runtime::DisplayProjection::DefaultHorizontalSpacing, NC2Runtime::DisplayProjection::DefaultHeightScale);
		return description;
	}

	// Original function: FUN_00481e9c
	void DisplayDescription_AssignNewTriangleValues(void* description, float verticalSpacing, float horizontalSpacing, float heightScale)
	{
		// The original also replaces unordered values with the lower bound.
		if (!(verticalSpacing > NC2Runtime::DisplayProjection::MinimumVerticalSpacing))
			verticalSpacing = NC2Runtime::DisplayProjection::MinimumVerticalSpacing;

		if (!(horizontalSpacing > NC2Runtime::DisplayProjection::MinimumHorizontalSpacing))
			horizontalSpacing = NC2Runtime::DisplayProjection::MinimumHorizontalSpacing;

		if (!(heightScale > NC2Runtime::DisplayProjection::MinimumHeightScale))
			heightScale = NC2Runtime::DisplayProjection::MinimumHeightScale;

		auto* bytes = static_cast<std::byte*>(description);

		std::memcpy(bytes + NC2Layout::DisplayDescriptionOffset::HeightScale, &heightScale, sizeof(heightScale));
		std::memcpy(bytes + NC2Layout::DisplayDescriptionOffset::VerticalSpacing, &verticalSpacing, sizeof(verticalSpacing));
		std::memcpy(bytes + NC2Layout::DisplayDescriptionOffset::HorizontalSpacing, &horizontalSpacing, sizeof(horizontalSpacing));

		DisplayDescription_CalculateResultingVars(description);
	}

	// Original function: FUN_00481efa
	void DisplayDescription_CalculateResultingVars(void* description)
	{
		float values[3];
		std::memcpy(values, description, sizeof(values));

		const float verticalSpacing = values[0];
		const float horizontalSpacing = values[1];
		const float heightScale = values[2];

		// Integer values below retain their original two's-complement bit patterns.
		const std::uint32_t microHorizontalFixed = DisplayDescription_ConvertScaledFloat(horizontalSpacing, NC2Runtime::DisplayProjection::FixedPointScale);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroHorizontalFixed, microHorizontalFixed);

		const std::uint32_t microVerticalFixed = DisplayDescription_ConvertScaledFloat(verticalSpacing, NC2Runtime::DisplayProjection::FixedPointScale);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroVerticalFixed, microVerticalFixed);

		const std::uint32_t heightFixed = DisplayDescription_ConvertScaledFloat(heightScale, NC2Runtime::DisplayProjection::FixedPointScale);

		const std::uint32_t microHalfHorizontalFixed = DisplayDescription_HalveSigned(microHorizontalFixed);
		const std::uint32_t microHalfVerticalFixed = DisplayDescription_HalveSigned(microVerticalFixed);
		const std::uint32_t heightPixels = DisplayDescription_FixedToPixels(heightFixed);

		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroHalfHorizontalFixed, microHalfHorizontalFixed);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroHalfVerticalFixed, microHalfVerticalFixed);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroHorizontalPixels, DisplayDescription_FixedToPixels(microHorizontalFixed));

		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroHeightFixed, heightFixed);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroVerticalPixels, DisplayDescription_FixedToPixels(microVerticalFixed));
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroHeightPixels, heightPixels);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroHalfHorizontalPixels, DisplayDescription_FixedToPixels(microHalfHorizontalFixed));
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MicroHalfVerticalPixels, DisplayDescription_FixedToPixels(microHalfVerticalFixed));

		const std::uint32_t macroHorizontalFixed = DisplayDescription_ConvertScaledFloat(horizontalSpacing, NC2Runtime::DisplayProjection::DoubledFixedPointScale);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroHorizontalFixed, macroHorizontalFixed);

		const std::uint32_t macroVerticalFixed = DisplayDescription_ConvertScaledFloat(verticalSpacing, NC2Runtime::DisplayProjection::DoubledFixedPointScale);
		const std::uint32_t macroHalfHorizontalFixed = DisplayDescription_HalveSigned(macroHorizontalFixed);
		const std::uint32_t macroHalfVerticalFixed = DisplayDescription_HalveSigned(macroVerticalFixed);

		// The doubled-spacing block retains the original height scale.
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroHeightFixed, heightFixed);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroVerticalFixed, macroVerticalFixed);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroHalfHorizontalFixed, macroHalfHorizontalFixed);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroHalfVerticalFixed, macroHalfVerticalFixed);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroHorizontalPixels, DisplayDescription_FixedToPixels(macroHorizontalFixed));
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroVerticalPixels, DisplayDescription_FixedToPixels(macroVerticalFixed));
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroHeightPixels, heightPixels);
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroHalfVerticalPixels, DisplayDescription_FixedToPixels(macroHalfVerticalFixed));
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::MacroHalfHorizontalPixels, DisplayDescription_FixedToPixels(macroHalfHorizontalFixed));

		const std::int32_t mapWidth = NC2Logic::GameMap_GetWidth();
		const std::uint32_t worldWidthFixed = static_cast<std::uint32_t>(mapWidth) * microHorizontalFixed;
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::WorldWidthPixels, DisplayDescription_FixedToPixels(worldWidthFixed));

		const std::int32_t mapHeight = NC2Logic::GameMap_GetHeight();
		const std::uint32_t worldHeightFixed = static_cast<std::uint32_t>(mapHeight) * microVerticalFixed;
		DisplayDescription_WriteDword(description, NC2Layout::DisplayDescriptionOffset::WorldHeightPixels, DisplayDescription_FixedToPixels(worldHeightFixed));
	}

	static std::uint32_t DisplayDescription_ReadDword(const void* description, std::size_t offset)
	{
		std::uint32_t bits;
		std::memcpy(&bits, static_cast<const std::byte*>(description) + offset, sizeof(bits));
		return bits;
	}

	static std::int32_t DisplayDescription_SignedDword(std::uint32_t bits)
	{
		std::int32_t value;
		std::memcpy(&value, &bits, sizeof(value));
		return value;
	}

	static std::int32_t DisplayDescription_DivideSigned(std::uint32_t numerator, std::uint32_t denominator)
	{
		// Keep runtime operands so the Win32 compiler emits the original signed IDIV.
		// Zero and INT_MIN / -1 retain the processor's division exception.
		const volatile std::int32_t dividend = DisplayDescription_SignedDword(numerator);
		const volatile std::int32_t divisor = DisplayDescription_SignedDword(denominator);
		return dividend / divisor;
	}

	// Original function: FUN_0047cfc6.
	std::int32_t DE_GetDisplayedCenterWorldXPosition(const void* display)
	{
		const std::uint32_t halfWidth = DisplayDescription_HalveSigned(DisplayDescription_ReadDword(display, NC2Layout::EngineDisplayOffset::BufferWidth));
		return DisplayDescription_SignedDword(halfWidth + DisplayDescription_ReadDword(display, NC2Layout::EngineDisplayOffset::WorldPixelX));
	}

	// Original function: FUN_0047cfd8.
	std::int32_t DE_GetDisplayedCenterWorldYPosition(const void* display)
	{
		const std::uint32_t halfHeight = DisplayDescription_HalveSigned(DisplayDescription_ReadDword(display, NC2Layout::EngineDisplayOffset::BufferHeight));
		return DisplayDescription_SignedDword(halfHeight + DisplayDescription_ReadDword(display, NC2Layout::EngineDisplayOffset::WorldPixelY));
	}

	// Original function: FUN_0048201b
	void DisplayDescription_MakeValidWorldPixelPosition(const void* description, std::int32_t* point)
	{
		if (point[0] < 0)
			point[0] = 0;

		const std::uint32_t widthBits = DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::WorldWidthPixels);

		if (point[0] >= DisplayDescription_SignedDword(widthBits))
			point[0] = DisplayDescription_SignedDword(widthBits - 1u);

		if (point[1] < 0)
			point[1] = 0;

		const std::uint32_t heightBits = DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::WorldHeightPixels);

		if (point[1] >= DisplayDescription_SignedDword(heightBits))
			point[1] = DisplayDescription_SignedDword(heightBits - 1u);
	}

	// Original function: FUN_00482049
	bool DisplayDescription_WorldPixelToMapMigCoordinates(const void* description, const std::int32_t* worldPoint, std::uint32_t* packedMapPoint)
	{
		const std::uint32_t adjustedX = static_cast<std::uint32_t>(worldPoint[0]) - DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::MicroHalfHorizontalPixels);
		const std::uint32_t adjustedY = static_cast<std::uint32_t>(worldPoint[1]) - DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::MicroVerticalPixels);

		std::int32_t projectedPoint[2] =
		{
			DisplayDescription_SignedDword(adjustedX),
			DisplayDescription_SignedDword(adjustedY)
		};

		DisplayDescription_MakeValidWorldPixelPosition(description, projectedPoint);

		const std::int32_t estimatedX = DisplayDescription_DivideSigned(static_cast<std::uint32_t>(projectedPoint[0]) << 8, DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::MicroHorizontalFixed));
		const std::int32_t estimatedY = DisplayDescription_DivideSigned(static_cast<std::uint32_t>(projectedPoint[1]) << 8, DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::MicroVerticalFixed));

		std::int32_t bestDistance = 100000000;

		for (std::int32_t offsetY = -2; offsetY <= 15; ++offsetY)
		{
			const std::uint32_t candidateY = static_cast<std::uint32_t>(estimatedY) + static_cast<std::uint32_t>(offsetY);

			for (std::int32_t offsetX = -2; offsetX <= 4; ++offsetX)
			{
				const std::uint32_t candidateX = static_cast<std::uint32_t>(estimatedX) + static_cast<std::uint32_t>(offsetX);
				const std::uint32_t candidate = (candidateX & NXBasics::IntegerRange::UInt16Maximum) | ((candidateY & NXBasics::IntegerRange::UInt16Maximum) << 16);

				if (!NC2Logic::MapMigPoint_IsInsideMap(&candidate))
					continue;

				DisplayDescription_MapMigToWorldPixelCoordinates(description, candidate, projectedPoint, true);

				const std::uint32_t deltaX = static_cast<std::uint32_t>(worldPoint[0]) - static_cast<std::uint32_t>(projectedPoint[0]);
				const std::uint32_t deltaY = static_cast<std::uint32_t>(worldPoint[1]) - static_cast<std::uint32_t>(projectedPoint[1]);
				const std::uint32_t distanceBits = deltaY * deltaY + deltaX * deltaX;
				const std::int32_t distance = DisplayDescription_SignedDword(distanceBits);

				if (distance < bestDistance)
				{
					bestDistance = distance;
					*packedMapPoint = candidate;
				}
			}
		}

		return bestDistance != 100000000;
	}

	// Original function: FUN_0048213c
	void DisplayDescription_MapMigToWorldPixelCoordinates(const void* description, std::uint32_t packedMapPoint, std::int32_t* worldPoint, bool validatePosition)
	{
		// Only the local by-value copy is modified.
		NC2Logic::MapMigPoint_ValidatePosition(&packedMapPoint);

		std::int16_t coordinates[2];
		std::memcpy(coordinates, &packedMapPoint, sizeof(coordinates));

		const std::uint32_t mapX = static_cast<std::uint32_t>(static_cast<std::int32_t>(coordinates[0]));
		const std::uint32_t mapY = static_cast<std::uint32_t>(static_cast<std::int32_t>(coordinates[1]));

		const std::uint32_t horizontalFixed = DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::MicroHorizontalFixed) * mapX;
		const std::uint32_t halfHorizontalPixels = DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::MicroHalfHorizontalPixels);

		std::uint32_t worldX = DisplayDescription_FixedToPixels(horizontalFixed) + halfHorizontalPixels;
		worldPoint[0] = DisplayDescription_SignedDword(worldX);

		if ((static_cast<std::uint16_t>(coordinates[1]) & 1u) != 0)
		{
			worldX += halfHorizontalPixels;
			worldPoint[0] = DisplayDescription_SignedDword(worldX);
		}

		const std::uint32_t verticalFixed = DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::MicroVerticalFixed) * mapY;
		std::uint32_t worldY = DisplayDescription_FixedToPixels(verticalFixed);

		worldPoint[1] = DisplayDescription_SignedDword(worldY);

		const std::uint32_t mapHeight = NC2Logic::GameMap_GetHeightAtMapMigPoint(&packedMapPoint);
		const std::uint32_t heightFixed = DisplayDescription_ReadDword(description, NC2Layout::DisplayDescriptionOffset::MicroHeightFixed) * mapHeight;

		worldY -= DisplayDescription_FixedToPixels(heightFixed);
		worldPoint[1] = DisplayDescription_SignedDword(worldY);

		if (validatePosition)
			DisplayDescription_MakeValidWorldPixelPosition(description, worldPoint);
	}

	template<typename T>
	static T Display_ReadField(const void* display, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(display) + offset, sizeof(value));
		return value;
	}

	template<typename T>
	static void Display_WriteField(void* display, std::size_t offset, T value)
	{
		std::memcpy(static_cast<std::byte*>(display) + offset, &value, sizeof(value));
	}

	// Original function: FUN_0047c65d
	void* Display_Construct(void* display, NXBasics::CBitmap* bitmap, std::uint8_t active, NXBasics::CFont* font)
	{
		auto* displayBytes = static_cast<std::byte*>(display);

		DisplayDescription_Construct(display);

		DisplaySecondaryObjectReferences_Construct(displayBytes + NC2Layout::EngineDisplayOffset::PrimaryReferenceEnabled);

		// Original function: FUN_0042ae54, applied to the rectangle at +0x97A.
		Display_WriteField<std::uint16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleHeight, 0);
		Display_WriteField<std::uint16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleWidth, 0);
		Display_WriteField<std::uint16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleY, 0);
		Display_WriteField<std::uint16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleX, 0);

		std::memset(displayBytes + NC2Layout::EngineDisplayOffset::DestinationBitmap, 0, NC2Layout::RecordSize::DisplayInitializedTail);

		Display_WriteField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DestinationBitmap, bitmap);
		Display_WriteField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag, active);
		Display_WriteField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags, 7);

		void* buffer = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NToolGouraudShader::CGouraudShader), 1);

		if (buffer != nullptr)
		{
			::new (buffer) NToolGouraudShader::CGouraudShader;
		}

		Display_WriteField<void*>(display, NC2Layout::EngineDisplayOffset::GouraudShader, buffer);
		Display_WriteField<NXBasics::CFont*>(display, NC2Layout::EngineDisplayOffset::Font, font);

		Display_InitScrollVariables(display);

		C2DEngineDisplayManager::sTheObjectPtr->RegisterDisplayObject(display);

		Display_WriteField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::RainEnabled, 1);
		Display_WriteField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::SnowEnabled, 1);
		Display_WriteField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::SandEnabled, 1);

		return display;
	}

	// Original function: FUN_0047c713
	void Display_Destruct(void* display)
	{
		C2DEngineDisplayManager::sTheObjectPtr->UnregisterDisplayObject(display);

		Display_ExitScrollVariables(display);

		void* buffer;
		std::memcpy(&buffer, static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::GouraudShader, sizeof(buffer));

		NXBasics::Runtime_Free(buffer);
	}

	static bool Display_FloatComparisonBranchesOnDifference(float requested, float current)
	{
		// FCOMP; FNSTSW; SAHF; JNE skips both equal and unordered operands.
		return requested < current || requested > current;
	}

	// Original function: FUN_0047c73c
	bool Display_AssignNewDisplayDefinitionValues(void* display, float verticalSpacing, float horizontalSpacing, float heightScale, bool preservePosition)
	{
		auto* bytes = static_cast<std::byte*>(display);

		float currentValues[3];
		std::memcpy(currentValues, bytes, sizeof(currentValues));

		if (!Display_FloatComparisonBranchesOnDifference(verticalSpacing, currentValues[0]) && !Display_FloatComparisonBranchesOnDifference(horizontalSpacing, currentValues[1]) && !Display_FloatComparisonBranchesOnDifference(heightScale, currentValues[2]))
			return false;

		if (preservePosition)
		{
			const auto readDword = [bytes] (std::size_t offset) -> std::uint32_t
			{
				std::uint32_t value;
				std::memcpy(&value, bytes + offset, sizeof(value));
				return value;
			};

			const std::uint32_t centerX = DisplayDescription_HalveSigned(readDword(NC2Layout::EngineDisplayOffset::BufferWidth)) + readDword(NC2Layout::EngineDisplayOffset::WorldPixelX);
			const std::uint32_t centerY = DisplayDescription_HalveSigned(readDword(NC2Layout::EngineDisplayOffset::BufferHeight)) + readDword(NC2Layout::EngineDisplayOffset::WorldPixelY);

			std::int32_t oldWorldPoint[2];
			std::memcpy(&oldWorldPoint[0], &centerX, sizeof(centerX));
			std::memcpy(&oldWorldPoint[1], &centerY, sizeof(centerY));

			std::uint32_t packedMapPoint = 0;

			// Original return value is intentionally ignored.
			DisplayDescription_WorldPixelToMapMigCoordinates(display, oldWorldPoint, &packedMapPoint);

			DisplayDescription_AssignNewTriangleValues(display, verticalSpacing, horizontalSpacing, heightScale);
			Display_ReInit(display);

			std::int32_t newWorldPoint[2];

			// The packed map point is passed by value; the final argument enables position validation.
			DisplayDescription_MapMigToWorldPixelCoordinates(display, packedMapPoint, newWorldPoint, true);

			const std::uint32_t deltaX = static_cast<std::uint32_t>(newWorldPoint[0]) - static_cast<std::uint32_t>(oldWorldPoint[0]);
			const std::uint32_t wantedX = readDword(NC2Layout::EngineDisplayOffset::WantedWorldPixelX) + deltaX;
			std::memcpy(bytes + NC2Layout::EngineDisplayOffset::WantedWorldPixelX, &wantedX, sizeof(wantedX));

			const std::uint32_t deltaY = static_cast<std::uint32_t>(newWorldPoint[1]) - static_cast<std::uint32_t>(oldWorldPoint[1]);
			const std::uint32_t wantedY = readDword(NC2Layout::EngineDisplayOffset::WantedWorldPixelY) + deltaY;
			std::memcpy(bytes + NC2Layout::EngineDisplayOffset::WantedWorldPixelY, &wantedY, sizeof(wantedY));
		}
		else
		{
			DisplayDescription_AssignNewTriangleValues(display, verticalSpacing, horizontalSpacing, heightScale);
			Display_ReInit(display);
		}

		return true;
	}

	// Original function: FUN_0047c876
	void DE_SetWantedPositionToMapPoint(void* display, const std::uint32_t* packedMapPoint)
	{
		std::uint32_t mapPoint;
		std::memcpy(&mapPoint, packedMapPoint, sizeof(mapPoint));
		auto* wantedPoint = reinterpret_cast<std::int32_t*>(static_cast<std::byte*>(display) + NC2Layout::EngineDisplayOffset::WantedWorldPixelX);
		DisplayDescription_MapMigToWorldPixelCoordinates(display, mapPoint, wantedPoint, true);
		DE_SetWantedPositionToPixelPoint(display, wantedPoint);
	}

	// Original function: FUN_0047c8c6
	void DE_SetWantedPositionToPixelPoint(void* display, const std::int32_t* point)
	{
		const std::uint32_t y = static_cast<std::uint32_t>(point[1]) - DisplayDescription_HalveSigned(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight));
		const std::uint32_t x = static_cast<std::uint32_t>(point[0]) - DisplayDescription_HalveSigned(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth));
		DE_SetWantedPositionToPixelPosition(display, DisplayDescription_SignedDword(x), DisplayDescription_SignedDword(y));
	}

	// Original function: FUN_0047c8f5
	void DE_SetWantedPositionAddDeltaPixelPoint(void* display, std::int32_t deltaX, std::int32_t deltaY)
	{
		const std::uint32_t y = Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY) + static_cast<std::uint32_t>(deltaY);
		const std::uint32_t x = Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX) + static_cast<std::uint32_t>(deltaX);
		DE_SetWantedPositionToPixelPosition(display, DisplayDescription_SignedDword(x), DisplayDescription_SignedDword(y));
	}

	// Original function: FUN_0047c913
	void DE_SetWantedPositionToPixelPosition(void* display, std::int32_t pixelX, std::int32_t pixelY)
	{
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX, pixelX);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY, pixelY);
		if (pixelX < NC2Runtime::DisplayScrolling::MinimumWorldX)
			Display_WriteField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX, NC2Runtime::DisplayScrolling::MinimumWorldX);
		if (pixelY < NC2Runtime::DisplayScrolling::MinimumWorldY)
			Display_WriteField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY, NC2Runtime::DisplayScrolling::MinimumWorldY);

		const std::int32_t mapWidth = NC2Logic::GameMap_GetWidth();
		const std::uint32_t worldWidth = DisplayDescription_ConvertScaledFloat(Display_ReadField<float>(display, NC2Layout::DisplayDescriptionOffset::HorizontalSpacing), static_cast<float>(mapWidth));
		const float verticalSpacing = Display_ReadField<float>(display, NC2Layout::DisplayDescriptionOffset::VerticalSpacing);
		const std::int32_t mapHeight = NC2Logic::GameMap_GetHeight();
		const std::uint32_t maxX = worldWidth - Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth) + NC2Runtime::DisplayScrolling::MaximumWorldMargin;
		const std::uint32_t worldHeight = DisplayDescription_ConvertScaledFloat(verticalSpacing, static_cast<float>(mapHeight));
		const std::uint32_t maxY = worldHeight - Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight) + NC2Runtime::DisplayScrolling::MaximumWorldMargin;

		// The upper bounds are applied after both lower bounds, even for a small map.
		if (Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX) > DisplayDescription_SignedDword(maxX))
			Display_WriteField(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX, maxX);
		if (Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY) > DisplayDescription_SignedDword(maxY))
			Display_WriteField(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY, maxY);
	}

	// Original function: FUN_0047cc2b.
	void DE_ToDraw_SetState(void* display, std::uint32_t flags, bool enabled)
	{
		const std::uint32_t current = Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::DrawFlags, enabled ? current | flags : current & ~flags);
		Display_WriteField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag, 1);
	}

	// Original function: FUN_0047cc4a.
	void DE_ToDraw_ToggleFlagset(void* display, std::uint32_t flags)
	{
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::DrawFlags, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) ^ flags);
		Display_WriteField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag, 1);
	}

	// Original function: FUN_0047cebf; rebuild scrolling only across zero.
	void DE_Zoom_SetCounter(void* display, std::int32_t counter)
	{
		if (counter < -1) counter = -1;
		if (counter > 1) counter = 1;
		const std::int32_t previous = Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::ZoomCounter, counter);
		if ((previous < 0) != (counter < 0))
		{
			std::uint32_t center = 0;
			DE_Misc_GetDisplayedCenterMapPosition(display, &center);
			Display_ExitScrollVariables(display);
			Display_InitScrollVariables(display);
			DE_SetWantedPositionToMapPoint(display, &center);
		}
	}

	// Original function: FUN_0047cf2b.
	void DE_Zoom_ToggleLevel(void* display, bool allowZoomIn)
	{
		std::int32_t counter = DisplayDescription_SignedDword(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter) + 1u);
		if (counter == 2) counter = -1;
		if (!allowZoomIn && counter > 0) counter = -1;
		DE_Zoom_SetCounter(display, counter);
	}

	// Original function: FUN_0047cf4e.
	void DE_Zoom_In(void* display, bool allowZoomIn)
	{
		std::int32_t counter = DisplayDescription_SignedDword(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter) + 1u);
		if (!allowZoomIn && counter > 0) counter = 0;
		DE_Zoom_SetCounter(display, counter);
	}

	// Original function: FUN_0047cf68.
	void DE_Zoom_Out(void* display)
	{
		DE_Zoom_SetCounter(display, DisplayDescription_SignedDword(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter) - 1u));
	}

	// Original function: FUN_0047cf73.
	void DE_ControllingPlayer_SetId(void* display, std::int32_t playerId)
	{
		if (Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId) != playerId)
		{
			Display_WriteField(display, NC2Layout::EngineDisplayOffset::PlayerId, playerId);
			Display_WriteField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag, 1);
		}
	}

	// Original function: FUN_0047cf89; retains the converter's AL result.
	bool DE_Misc_GetDisplayedCenterMapPosition(const void* display, std::uint32_t* packedMapPoint)
	{
		const std::uint32_t x = DisplayDescription_HalveSigned(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth)) + Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelX);
		const std::uint32_t y = DisplayDescription_HalveSigned(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight)) + Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelY);
		const std::int32_t center[2] = { DisplayDescription_SignedDword(x), DisplayDescription_SignedDword(y) };
		return DisplayDescription_WorldPixelToMapMigCoordinates(display, center, packedMapPoint);
	}

	// Original function: FUN_0047c836
	void Display_ReInit(void* display)
	{
		DisplayDescription_CalculateResultingVars(display);

		auto* bytes = static_cast<std::byte*>(display);

		std::int32_t horizontalSpacingPixels;
		std::memcpy(&horizontalSpacingPixels, bytes + NC2Layout::DisplayDescriptionOffset::MicroHorizontalPixels, sizeof(horizontalSpacingPixels));

		std::uint32_t drawFlags;
		std::memcpy(&drawFlags, bytes + 0x70, sizeof(drawFlags));

		bytes[NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag] = std::byte{ 1 };

		if (horizontalSpacingPixels > 10)
			drawFlags |= 1u;
		else
			drawFlags &= ~1u;

		std::memcpy(bytes + 0x70, &drawFlags, sizeof(drawFlags));

		bytes[NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag] = std::byte{ 1 };

		if (horizontalSpacingPixels > 5)
			drawFlags |= NXBasics::BitMask::Bit1;
		else
			drawFlags &= ~0x02u;

		std::memcpy(bytes + 0x70, &drawFlags, sizeof(drawFlags));

		bytes[NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag] = std::byte{ 1 };
	}

	// Original function: FUN_0047cb4d
	void Display_GlobalWorldPixelToLocalDisplayPixelCoordinates(const void* display, std::int32_t* point, bool applyZoom)
	{
		const auto readWord = [] (const void* object, std::size_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
			return value;
		};

		const auto signedValue = [] (std::uint32_t bits) -> std::int32_t
		{
			std::int32_t value;
			std::memcpy(&value, &bits, sizeof(value));
			return value;
		};

		const auto storeWord = [] (std::int32_t* destination, std::uint32_t bits)
		{
			std::memcpy(destination, &bits, sizeof(bits));
		};

		const auto shiftRightSigned = [] (std::uint32_t bits, std::uint32_t count) -> std::uint32_t
		{
			count &= 31u;

			if (count == 0)
				return bits;

			const std::uint32_t shifted = bits >> count;

			if ((bits & NXBasics::IntegerRange::Bit31) != 0)
				return shifted | (NXBasics::IntegerRange::UInt32Maximum << (32u - count));

			return shifted;
		};

		storeWord(point, readWord(point, 0) - readWord(display, NC2Layout::EngineDisplayOffset::WorldPixelX));
		storeWord(point + 1, readWord(point, 4) - readWord(display, NC2Layout::EngineDisplayOffset::WorldPixelY));

		if (!applyZoom)
			return;

		const std::int32_t zoom = signedValue(readWord(display, NC2Layout::EngineDisplayOffset::ZoomCounter));

		if (zoom > 0)
		{
			const std::uint32_t centerX = static_cast<std::uint32_t>(signedValue(readWord(display, NC2Layout::EngineDisplayOffset::BufferWidth)) / 2);
			const std::uint32_t centerY = static_cast<std::uint32_t>(signedValue(readWord(display, NC2Layout::EngineDisplayOffset::BufferHeight)) / 2);
			const std::uint32_t shiftX = static_cast<std::uint32_t>(zoom) & 31u;

			storeWord(point, ((readWord(point, 0) - centerX) << shiftX) + centerX);

			const std::uint32_t shiftY = readWord(display, NC2Layout::EngineDisplayOffset::ZoomCounter) & 31u;
			storeWord(point + 1, ((readWord(point, 4) - centerY) << shiftY) + centerY);
		}
		else if (zoom < 0)
		{
			const std::uint32_t shiftX = 0u - static_cast<std::uint32_t>(zoom);
			storeWord(point, shiftRightSigned(readWord(point, 0), shiftX));

			const std::uint32_t shiftY = 0u - readWord(display, NC2Layout::EngineDisplayOffset::ZoomCounter);
			storeWord(point + 1, shiftRightSigned(readWord(point, 4), shiftY));
		}
	}

	// Original function: FUN_0047cbbc; inverse zoom precedes the world offset.
	void Display_LocalDisplayPixelToGlobalWorldPixelCoordinates(const void* display, std::int32_t* point, bool applyZoom)
	{
		std::uint32_t x = static_cast<std::uint32_t>(point[0]), y = static_cast<std::uint32_t>(point[1]);
		if (applyZoom)
		{
			const std::int32_t zoom = Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter);
			if (zoom > 0)
			{
				const std::uint32_t centerX = DisplayDescription_HalveSigned(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth));
				const std::uint32_t centerY = DisplayDescription_HalveSigned(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight));
				x = DisplayDescription_ShiftRightSigned(x - centerX, static_cast<std::uint32_t>(zoom)) + centerX;
				y = DisplayDescription_ShiftRightSigned(y - centerY, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter)) + centerY;
			}
			else if (zoom < 0)
			{
				x <<= (0u - static_cast<std::uint32_t>(zoom)) & 31u;
				y <<= (0u - Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter)) & 31u;
			}
		}
		point[0] = DisplayDescription_SignedDword(x + Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelX));
		point[1] = DisplayDescription_SignedDword(y + Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelY));
	}

	// Original function: FUN_0048143c
	void Display_InitScrollVariables(void* display)
	{
		auto* displayBytes = static_cast<std::byte*>(display);

		std::memset(displayBytes + NC2Layout::EngineDisplayOffset::WantedWorldPixelX, 0, NC2Layout::RecordSize::DisplayScrollState);

		const std::int32_t zoom = Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter);
		NXBasics::CBitmap* displayBitmap = Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DestinationBitmap);

		NXBasics::SRectangle rectangle = displayBitmap->GetRectangle();

		if (zoom < 0)
		{
			const std::uint32_t shift = (0u - static_cast<std::uint32_t>(zoom)) & 31u;
			const std::uint32_t factor = 1u << shift;

			rectangle.width = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(rectangle.width) * factor);
			rectangle.height = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(rectangle.height) * factor);
		}

		rectangle.x = 0;
		rectangle.y = 0;

		std::memcpy(displayBytes + NC2Layout::EngineDisplayOffset::BufferRectangle, &rectangle, sizeof(rectangle));

		if (zoom < 0)
		{
			const std::int32_t depth = Display_ReadField<std::int32_t>(displayBitmap, NXBasics::BitmapOffset::Depth);
			NXBasics::CBitmap* scrollBitmap = new (std::nothrow) NXBasics::CBitmap(rectangle, depth);

			Display_WriteField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap, scrollBitmap);
		}
		else
		{
			Display_WriteField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap, displayBitmap);
		}

		// Reload the assigned display bitmap at the original access point.
		displayBitmap = Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DestinationBitmap);

		const std::int32_t depth = Display_ReadField<std::int32_t>(displayBitmap, NXBasics::BitmapOffset::Depth);
		NXBasics::CBitmap* auxiliaryBitmap = new (std::nothrow) NXBasics::CBitmap(rectangle, depth);

		Display_WriteField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::StaticBitmap, auxiliaryBitmap);
		Display_WriteField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag, 1);
	}

	// Original function: FUN_0048150e
	void Display_ExitScrollVariables(void* display)
	{
		auto* displayBytes = static_cast<std::byte*>(display);

		NXBasics::CBitmap* displayBitmap;
		NXBasics::CBitmap* scrollBitmap;

		std::memcpy(&displayBitmap, displayBytes + NC2Layout::EngineDisplayOffset::DestinationBitmap, sizeof(displayBitmap));
		std::memcpy(&scrollBitmap, displayBytes + NC2Layout::EngineDisplayOffset::DrawingBitmap, sizeof(scrollBitmap));

		if (scrollBitmap != displayBitmap && scrollBitmap != nullptr)
		{
			delete scrollBitmap;
		}

		scrollBitmap = nullptr;
		std::memcpy(displayBytes + NC2Layout::EngineDisplayOffset::DrawingBitmap, &scrollBitmap, sizeof(scrollBitmap));

		NXBasics::CBitmap* auxiliaryBitmap;
		std::memcpy(&auxiliaryBitmap, displayBytes + NC2Layout::EngineDisplayOffset::StaticBitmap, sizeof(auxiliaryBitmap));

		if (auxiliaryBitmap != nullptr)
		{
			delete auxiliaryBitmap;
		}

		auxiliaryBitmap = nullptr;
		std::memcpy(displayBytes + NC2Layout::EngineDisplayOffset::StaticBitmap, &auxiliaryBitmap, sizeof(auxiliaryBitmap));
	}

	// Original function: FUN_004821c7
	void DisplayDescription_GetMapRectangle(const void* display, std::uint32_t pixelX, std::uint32_t pixelY, std::uint32_t pixelWidth, std::uint32_t pixelHeight, std::int16_t* rectangle)
	{
		const auto readWord = [] (const void* base, std::uint32_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(value));
			return value;
		};

		const auto signedDivide = [] (std::uint32_t numerator, std::uint32_t divisor) -> std::int32_t
		{
			std::int32_t signedNumerator;
			std::int32_t signedDivisor;
			std::memcpy(&signedNumerator, &numerator, sizeof(signedNumerator));
			std::memcpy(&signedDivisor, &divisor, sizeof(signedDivisor));

			return signedNumerator / signedDivisor;
		};

		const auto storeShort = [] (std::int16_t* destination, std::int32_t value)
		{
			const std::uint16_t bits = static_cast<std::uint16_t>(value);
			std::memcpy(destination, &bits, sizeof(bits));
		};

		const std::int32_t mapY = signedDivide(pixelY << 8, readWord(display, NC2Layout::DisplayDescriptionOffset::MicroVerticalFixed));
		storeShort(rectangle + 1, mapY);

		const std::uint32_t rowOffset = (static_cast<std::uint32_t>(mapY) & 1u) * readWord(display, NC2Layout::DisplayDescriptionOffset::MicroHalfHorizontalPixels);
		const std::uint32_t adjustedX = (pixelX + rowOffset) << 8;
		const std::int32_t mapX = signedDivide(adjustedX, readWord(display, NC2Layout::DisplayDescriptionOffset::MicroHorizontalFixed));
		storeShort(rectangle, mapX);

		const std::int32_t mapWidth = signedDivide(pixelWidth, readWord(display, NC2Layout::DisplayDescriptionOffset::MicroHorizontalPixels));
		storeShort(rectangle + 2, mapWidth);

		const std::int32_t mapHeight = signedDivide(pixelHeight, readWord(display, NC2Layout::DisplayDescriptionOffset::MicroVerticalPixels));
		storeShort(rectangle + 3, mapHeight);

		NC2Logic::MapMigRectangle_CutInsideMap(rectangle);
	}

	// Original function: FUN_00483e74
	void DisplayObjectReference_Reset(void* reference)
	{
		auto* data = static_cast<std::byte*>(reference);
		std::memset(data, 0, NC2Layout::RecordSize::DisplayObjectReference);

		const std::uint32_t invalidIndex = NXBasics::IntegerRange::UInt32Maximum;
		constexpr std::size_t indexOffsets[] = { NC2Layout::DisplayObjectReferenceOffset::HumanId, NC2Layout::DisplayObjectReferenceOffset::AnimalId, NC2Layout::DisplayObjectReferenceOffset::VehicleId, NC2Layout::DisplayObjectReferenceOffset::MapMoveableId, NC2Layout::DisplayObjectReferenceOffset::HouseId, NC2Layout::DisplayObjectReferenceOffset::GuideId, NC2Layout::DisplayObjectReferenceOffset::PlayerId };

		for (std::size_t offset : indexOffsets)
			std::memcpy(data + offset, &invalidIndex, sizeof(invalidIndex));
	}

	// Original function: FUN_00483ea1
	bool DisplayObjectReference_Validate(void* reference)
	{
		const auto readWord = [] (const void* object, std::size_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
			return value;
		};

		const std::uint32_t type = readWord(reference, NC2Layout::DisplayObjectReferenceOffset::Type);

		void* table;
		std::uint32_t stride;
		std::size_t indexOffset;
		std::size_t identityOffset;

		switch (type)
		{
			case SwitchIds::LogicObjectType::Human:
				table = NC2Logic::CHumanArray::mStaticVars;
				stride = NC2Logic::CHumanArray::ElementSize;
				indexOffset = NC2Layout::DisplayObjectReferenceOffset::HumanId;
				identityOffset = 0x08;
				break;

			case SwitchIds::LogicObjectType::Animal:
				table = NC2Logic::CAnimalArray::mStaticVars;
				stride = NC2Logic::CAnimalArray::ElementSize;
				indexOffset = NC2Layout::DisplayObjectReferenceOffset::AnimalId;
				identityOffset = 0x08;
				break;

			case SwitchIds::LogicObjectType::Vehicle:
				table = NC2Logic::CVehicleArray::mStaticVars;
				stride = NC2Logic::CVehicleArray::ElementSize;
				indexOffset = NC2Layout::DisplayObjectReferenceOffset::VehicleId;
				identityOffset = 0x08;
				break;

			case SwitchIds::LogicObjectType::House:
				table = NC2Logic::CHouseArray::mStaticVars;
				stride = NC2Logic::CHouseArray::ElementSize;
				indexOffset = NC2Layout::DisplayObjectReferenceOffset::HouseId;
				identityOffset = 0x08;
				break;

			case SwitchIds::LogicObjectType::Guide:
				table = NC2Logic::CGuideArray::mStaticVars;
				stride = NC2Logic::CGuideArray::ElementSize;
				indexOffset = NC2Layout::DisplayObjectReferenceOffset::GuideId;
				identityOffset = 0x10;
				break;

			default:
				return false;
		}

		const std::uint32_t index = readWord(reference, indexOffset);

		if (index == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const std::uintptr_t baseAddress = reinterpret_cast<std::uintptr_t>(table);
		const std::uintptr_t address = baseAddress + static_cast<std::size_t>(index) * stride;
		const auto* object = reinterpret_cast<const std::byte*>(static_cast<std::uintptr_t>(address));

		std::uint8_t active;
		std::memcpy(&active, object, sizeof(active));

		if (active != 0)
		{
			const std::uint32_t currentIdentity = readWord(object, identityOffset);
			const std::uint32_t storedIdentity = readWord(reference, indexOffset + 4);

			if (currentIdentity == storedIdentity)
				return false;
		}

		// True reports that an invalid reference was cleared.
		DisplayObjectReference_Reset(reference);
		return true;
	}

	// Original function: FUN_0047dbcf.
	void DE_SpecialFX_Earthquake_Start(void* display, std::uint32_t durationMilliseconds)
	{
		const auto endTime = ::timeGetTime() + durationMilliseconds;
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::EarthquakeOffsetX, 0u);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::EarthquakeOffsetY, 0u);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::EarthquakeEndTime, endTime);
	}

	// Original function: FUN_0047dc9d; the original destructor is an empty RET.
	static void __cdecl EarthquakeRandom_AtExit()
	{}
	// Original function: FUN_0047dbf3; l_DE_SpecialFX_Earthquake_Update.
	static void Display_UpdateEarthquake(void* display)
	{
		const std::uint32_t endTime = Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::EarthquakeEndTime);
		if (endTime == 0)
			return;
		if (::timeGetTime() > endTime)
		{
			Display_WriteField(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX) - Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::EarthquakeOffsetX));
			Display_WriteField(display, NC2Layout::EngineDisplayOffset::EarthquakeEndTime, 0u);
			Display_WriteField(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY) - Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::EarthquakeOffsetY));
			return;
		}

		// Original lazy CRandom at 00554d90, guarded by 00554d88.
		// 00410c3f leaves this fixed seed unchanged (its bit count is in [8,24]).
		static NC2Logic::SGameRandomState random{ NC2Runtime::RandomSeed::DisplayEffects, NC2Runtime::RandomSeed::DisplayEffects };
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(EarthquakeRandom_AtExit));
		}
		const std::uint32_t dx = NC2Logic::Random_Get(random) % NC2Runtime::EarthquakeDisplacement::HorizontalRange - NC2Runtime::EarthquakeDisplacement::HorizontalCenter;
		const std::uint32_t dy = NC2Logic::Random_Get(random) % NC2Runtime::EarthquakeDisplacement::VerticalRange - NC2Runtime::EarthquakeDisplacement::VerticalCenter;
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX) + dx);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::EarthquakeOffsetX, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::EarthquakeOffsetX) + dx);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY) + dy);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::EarthquakeOffsetY, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::EarthquakeOffsetY) + dy);
	}

	// Original function: FUN_0047cc71; l_DE_Hightlighting_CheckUpdate.
	static void Display_Highlighting_CheckUpdate(void* display)
	{
		if (Display_ReadField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::HighlightingEnabled) == 0)
			return;
		std::uint16_t bits[4];
		std::memcpy(bits, static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::MapRectangle, sizeof(bits));
		bits[0] = static_cast<std::uint16_t>(bits[0] - 8u);
		bits[1] = static_cast<std::uint16_t>(bits[1] - 16u);
		bits[2] = static_cast<std::uint16_t>(bits[2] + 16u);
		bits[3] = static_cast<std::uint16_t>(bits[3] + 40u);
		std::int16_t rectangle[4];
		std::memcpy(rectangle, bits, sizeof(rectangle));
		NC2Logic::MapMigRectangle_CutInsideMap(rectangle);
		if (C2IGG_EntryPoint_InputManager_Highlighting_Update(rectangle))
			Display_WriteField(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag, std::uint8_t{ 1 });
	}

	// Original function: FUN_0047c9ab.
	void Display_MapMigToLocalDisplayPixelCoordinates(const void* display, const void* packedMapPoint, std::int32_t* point, bool applyZoom)
	{
		const std::uint32_t position = Display_ReadField<std::uint32_t>(packedMapPoint, NC2Layout::MapCoordinatePairOffset::Coordinates);
		DisplayDescription_MapMigToWorldPixelCoordinates(display, position, point, true);
		Display_GlobalWorldPixelToLocalDisplayPixelCoordinates(display, point, applyZoom);
	}

	// Original function: FUN_00481544; l_DE_SetMapRectangle.
	static void Display_SetMapRectangle(void* display, std::uint32_t pixelX, std::uint32_t pixelY)
	{
		auto* bitmap = Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap);
		std::int16_t rectangle[4];
		DisplayDescription_GetMapRectangle(display, pixelX, pixelY, static_cast<std::uint32_t>(bitmap->GetRectangle().width), static_cast<std::uint32_t>(bitmap->GetRectangle().height), rectangle);

		const auto addShort = [] (std::int16_t& field, std::int32_t increment)
		{
			const std::uint16_t bits = static_cast<std::uint16_t>(static_cast<std::int32_t>(field) + increment);
			std::memcpy(&field, &bits, sizeof(field));
		};
		addShort(rectangle[0], -14);
		addShort(rectangle[2], 28);
		addShort(rectangle[1], -10);
		addShort(rectangle[3], 46);
		NC2Logic::MapMigRectangle_CutInsideMap(rectangle);
		std::memcpy(static_cast<std::byte*>(display) + NC2Layout::EngineDisplayOffset::MapRectangle, rectangle, sizeof(rectangle));

		const std::int32_t right = static_cast<std::int32_t>(rectangle[0]) + rectangle[2];
		const std::int32_t bottom = static_cast<std::int32_t>(rectangle[1]) + rectangle[3];
		const std::int32_t widthLimit = static_cast<std::uint16_t>(static_cast<std::uint32_t>(NC2Logic::GameMap_GetWidth()) - 1u);
		const std::int32_t heightLimit = static_cast<std::uint16_t>(static_cast<std::uint32_t>(NC2Logic::GameMap_GetHeight()) - 1u);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::MapMinimumX, static_cast<std::uint16_t>(rectangle[0] > 0 ? rectangle[0] : 0));
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::MapMaximumX, static_cast<std::uint16_t>(right < widthLimit ? right : widthLimit));
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::MapMinimumY, static_cast<std::uint16_t>(rectangle[1] > 0 ? rectangle[1] : 0));
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::MapMaximumY, static_cast<std::uint16_t>(bottom < heightLimit ? bottom : heightLimit));

		const std::uint32_t x = DisplayDescription_FixedToPixels(Display_ReadField<std::uint32_t>(display, NC2Layout::DisplayDescriptionOffset::MicroHorizontalFixed) * static_cast<std::uint32_t>(rectangle[0])) - pixelX;
		const std::uint32_t y = DisplayDescription_FixedToPixels(Display_ReadField<std::uint32_t>(display, NC2Layout::DisplayDescriptionOffset::MicroVerticalFixed) * static_cast<std::uint32_t>(rectangle[1])) - pixelY;
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::MapRectanglePixelX, x);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::MapRectanglePixelY, y);
	}

	// Original function: FUN_0048168c.
	static void Display_CopyStaticBitmap(void* display)
	{
		auto* source = Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::StaticBitmap);
		const std::int32_t x = Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapX);
		const std::int32_t y = Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapY);
		const std::int32_t remainingWidth = NXBasics::Runtime_TimeSigned32(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth) - static_cast<std::uint32_t>(x));
		const std::int32_t remainingHeight = NXBasics::Runtime_TimeSigned32(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight) - static_cast<std::uint32_t>(y));

		NXBasics::CBitmap bottomRight(source, NXBasics::SRectangle(x, y, remainingWidth, remainingHeight));
		bottomRight.CopyIntoBitmap(Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), 0, 0);
		if (x != 0)
		{
			NXBasics::CBitmap bottomLeft(source, NXBasics::SRectangle(0, y, x, remainingHeight));
			bottomLeft.CopyIntoBitmap(Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), remainingWidth, 0);
		}
		if (y != 0)
		{
			NXBasics::CBitmap topRight(source, NXBasics::SRectangle(x, 0, remainingWidth, y));
			topRight.CopyIntoBitmap(Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), 0, remainingHeight);
			if (x != 0)
			{
				NXBasics::CBitmap topLeft(source, NXBasics::SRectangle(0, 0, x, y));
				topLeft.CopyIntoBitmap(Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), remainingWidth, remainingHeight);
			}
		}
	}

	// The original drawing stage takes a copied 24-byte rectangle on the stack.
	extern void __fastcall Display_l_DE_DoUpdateStaticPart1(void* display, void* unusedEdx, S2DEngineDisplayRectangle rectangle);

	static std::uint32_t Display_WrapBitmapCoordinate(std::uint32_t coordinate, std::uint32_t size)
	{
		if (coordinate == 0)
			return 0;
		const std::int32_t divisor = NXBasics::Runtime_TimeSigned32(size);
		if ((coordinate & NXBasics::IntegerRange::Bit31) != 0)
		{
			const std::int32_t remainder = NXBasics::Runtime_TimeSigned32(0u - coordinate) % divisor;
			coordinate = size - static_cast<std::uint32_t>(remainder);
		}
		return static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(coordinate) % divisor);
	}

	// Original function: FUN_00481dd1; l_DE_WrapStaticBitmapPosition.
	static void Display_WrapStaticBitmapPosition(const void* display, S2DEngineDisplayRectangle& rectangle)
	{
		rectangle.bitmapX = Display_WrapBitmapCoordinate(rectangle.bitmapX, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth));
		rectangle.bitmapY = Display_WrapBitmapCoordinate(rectangle.bitmapY, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight));
	}

	// Original function: FUN_00481a74; l_DE_RebuildStaticCompletly.
	static void Display_RebuildStaticCompletly(void* display, std::uint32_t pixelX, std::uint32_t pixelY)
	{
		const S2DEngineDisplayRectangle rectangle{ pixelX, pixelY, 0, 0, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight) };
		Display_l_DE_DoUpdateStaticPart1(display, nullptr, rectangle);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::StaticBitmapX, 0u);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::StaticBitmapY, 0u);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag, std::uint8_t{ 0 });
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::WorldPixelX, pixelX);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::WorldPixelY, pixelY);
	}

	// Original function: FUN_004817dc; l_DE_DoUpdateStatic.
	static void Display_DoUpdateStatic(void* display, std::uint32_t pixelX, std::uint32_t pixelY)
	{
		const std::uint32_t oldX = Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelX);
		const std::uint32_t oldY = Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelY);
		const std::uint32_t dx = pixelX - oldX;
		const std::uint32_t dy = pixelY - oldY;
		const std::uint32_t absoluteX = (dx & NXBasics::IntegerRange::Bit31) != 0 ? 0u - dx : dx;
		const std::uint32_t absoluteY = (dy & NXBasics::IntegerRange::Bit31) != 0 ? 0u - dy : dy;
		const std::uint32_t width = Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth);
		const std::uint32_t height = Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight);
		if (NXBasics::Runtime_TimeSigned32(absoluteX) >= NXBasics::Runtime_TimeSigned32(width) || NXBasics::Runtime_TimeSigned32(absoluteY) >= NXBasics::Runtime_TimeSigned32(height) || Display_ReadField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag) != 0)
		{
			Display_RebuildStaticCompletly(display, pixelX, pixelY);
			return;
		}
		if (dx == 0 && dy == 0)
			return;

		const std::uint32_t product = dx * dy;
		const std::uint32_t area = (product & NXBasics::IntegerRange::Bit31) != 0 ? 0u - product : product;
		if ((area * 100u) / (height * width) >= NC2Runtime::DisplayScrolling::FullRebuildAreaPercent)
		{
			Display_RebuildStaticCompletly(display, pixelX, pixelY);
			return;
		}

		if (dx != 0)
		{
			const bool movesRight = NXBasics::Runtime_TimeSigned32(dx) > 0;
			S2DEngineDisplayRectangle rectangle{ oldX + (movesRight ? width : dx), oldY + dy, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapX) + (movesRight ? width : dx), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapY) + dy, absoluteX, height };
			Display_WrapStaticBitmapPosition(display, rectangle);
			Display_l_DE_DoUpdateStaticPart1(display, nullptr, rectangle);
		}
		if (dy != 0)
		{
			const bool movesRight = NXBasics::Runtime_TimeSigned32(dx) > 0;
			const bool movesDown = NXBasics::Runtime_TimeSigned32(dy) > 0;
			const std::uint32_t updatedHeight = Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight);
			S2DEngineDisplayRectangle rectangle{ Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelX) + (movesRight ? dx : 0u), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelY) + (movesDown ? updatedHeight : dy), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapX) + (movesRight ? dx : 0u), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapY) + (movesDown ? updatedHeight : dy), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth) - absoluteX, absoluteY };
			Display_WrapStaticBitmapPosition(display, rectangle);
			Display_l_DE_DoUpdateStaticPart1(display, nullptr, rectangle);
		}
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::StaticBitmapX, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapX) + dx);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::StaticBitmapY, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapY) + dy);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::StaticBitmapX, Display_WrapBitmapCoordinate(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapX), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth)));
		const std::uint32_t bitmapY = Display_WrapBitmapCoordinate(Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::StaticBitmapY), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight));
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::WorldPixelX, pixelX);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::StaticBitmapY, bitmapY);
		Display_WriteField(display, NC2Layout::EngineDisplayOffset::WorldPixelY, pixelY);
	}

	// Original function: FUN_004910ba; Mac name: NC2E2::Tool_Sound_IsCDAUsable.
	bool Tool_Sound_IsCDAUsable()
	{
		return NCDAudioManager::CCDAudioManager::sTheObjectPtr != nullptr;
	}

	// Original function: FUN_004910c5; l_DE_DrawForeground.
	static void Display_DrawForeground(void* display)
	{
		DrawAtom_FishManager_DoDraw(display);
		if ((Display_ReadField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) & 2u) != 0)
			E2ParticelManager_SortParticelAndDrawBottom(gE2ParticelManager, nullptr, display);
		const auto* rectangle = reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::MapRectangle);
		const auto* point = reinterpret_cast<const std::int32_t*>(static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::MapRectanglePixelX);
		if ((Display_ReadField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) & 1u) != 0)
		{
			Display_l_DE_DrawLandscapeAndCreaturesField(display, rectangle, Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), point, false, true);
			Display_l_DE_DrawLandscapeAndCreaturesField(display, rectangle, Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), point, false, false);
		}
		if (CLevelOfDetailManager::sLevelOfDetail == 2 && Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter) >= 0)
		{
			Display_l_DE_SpecialOverlay_Rain_Draw(display);
			Display_l_DE_SpecialOverlay_Snow_Draw(display);
			Display_l_DE_SpecialOverlay_Sand_Draw(display);
		}
		if (Display_ReadField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::TooltipOverlaysEnabled) != 0)
			Display_l_DE_DrawTooltipOverlays(display, nullptr, rectangle, Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), point);
		if ((Display_ReadField<std::uint8_t>(display, NC2Layout::WorldDisplayOffset::DrawFlagsByte1) & NXBasics::BitMask::Bit5) != 0)
		{
			Display_l_DE_Debug_DrawMapMoveablesDebugInfos(display);
			Display_l_DE_Debug_DrawHousesDebugInfos(display);
		}
		if ((Display_ReadField<std::uint8_t>(display, NC2Layout::WorldDisplayOffset::DrawFlagsByte1) & NXBasics::BitMask::ByteExceptBits0Through5) != 0)
			Display_l_DE_Debug_DrawGridLines(display);
		Display_InformGuiRedraw(display);

		if (Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter) > 0)
		{
			auto* bitmap = Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DestinationBitmap);
			NXBasics::SRectangle area = bitmap->GetRectangle();
			// Original 00410654 masks each dimension with 0000fffch.
			area.width = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.width) & NXBasics::BitMask::WordExceptBits0Through1);
			area.height = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.height) & NXBasics::BitMask::WordExceptBits0Through1);
			NXBasics::CBitmap window(bitmap, area);
			for (std::int32_t step = 0; step < Display_ReadField<std::int32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter); ++step)
				window.ZoomIn();
		}
	}

	// Original function: FUN_0048162f; C2DEngineDisplay::DE_UpdateDisplay.
	void Display_UpdateDisplay(void* display)
	{
		Display_UpdateEarthquake(display);
		Display_SetMapRectangle(display, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY));
		Display_Highlighting_CheckUpdate(display);
		Display_DoUpdateStatic(display, Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelX), Display_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WantedWorldPixelY));
		Display_CopyStaticBitmap(display);
		Display_DrawForeground(display);
		auto* workBitmap = Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap);
		auto* outputBitmap = Display_ReadField<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DestinationBitmap);
		if (workBitmap != outputBitmap)
			workBitmap->CopyIntoBitmap_HalfSize_Optimized(outputBitmap);
	}

	// Original function: FUN_00485bf0; semantic name derived from Windows xrefs.
	void WorldMusic_UpdateVisiblePlayerSelection()
	{
		if (gDirectMusicManager == nullptr)
			return;

		std::uint32_t selected = NXBasics::IntegerRange::UInt32Maximum;
		gWorldMusicState.selectedPlayer = selected;
		for (std::uint32_t player = 0; player < 20u; ++player)
		{
			if (player == gWorldMusicState.controllingPlayer || gWorldMusicState.visiblePlayerCounts[player] == 0)
				continue;
			if (selected == NXBasics::IntegerRange::UInt32Maximum || gWorldMusicState.visiblePlayerCounts[selected] < gWorldMusicState.visiblePlayerCounts[player])
			{
				selected = player;
				gWorldMusicState.selectedPlayer = selected;
			}
		}
	}

	// Original function: FUN_00484b6c
	// Original method: S2DEngineDisplaySearchObject::Tool_Validate.
	bool DisplaySearchObject_Validate(void* searchObject)
	{
		if (!DisplayObjectReference_Validate(searchObject))
			return false;

		std::memset(searchObject, 0, NC2Layout::RecordSize::DisplaySearchObject);
		DisplayObjectReference_Reset(searchObject);
		return true;
	}
}

// Original function: FUN_0047c9da; camera following uses the carrier and movement progress.
void __fastcall Display_DE_MapMoveableToGlobalWorldPixelCoordinates(void* display, void* unusedEdx, std::uint32_t moveableId, std::int32_t* point)
{
	(void) unusedEdx;
	using namespace NC2Lifetime;
	const auto moveableAt = [] (std::uint32_t id) -> const void*
	{
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(NC2Logic::gMapMoveableArrayStorage.entries) + id * NC2Layout::RecordSize::MapMoveable);
	};
	const auto vehicleAt = [] (std::uint32_t id) -> const void*
	{
		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(NC2Logic::CVehicleArray::mStaticVars) + id * NC2Logic::CVehicleArray::ElementSize);
	};
	const void* moveable = moveableAt(moveableId);
	if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 1)
	{
		const void* human = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(NC2Logic::CHumanArray::mStaticVars) + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * NC2Logic::CHumanArray::ElementSize);
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
			moveable = moveableAt(Read<std::uint32_t>(vehicleAt(Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId)), NC2Layout::VehicleOffset::MapMoveableId));
	}
	if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 3)
	{
		const void* vehicle = vehicleAt(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId));
		// Original 0044DA5D only returns the attached byte; no generation validation.
		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::InsideVehicleFlag) != 0)
			moveable = moveableAt(Read<std::uint32_t>(vehicleAt(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CarrierVehicleId)), NC2Layout::VehicleOffset::MapMoveableId));
	}
	const void* position = NC2Logic::MapMoveable_GetMapPositionRef(moveable);
	NC2E2::Display_MapMigToLocalDisplayPixelCoordinates(display, position, point, false);
	if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0 && Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) == 0)
	{
		std::uint32_t direction = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingDirection), progress = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress);
		if (direction < 8)
		{
			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag) != 0)
			{
				progress = NC2Runtime::WalkingProgress::FullStep - progress;
				direction = NC2Logic::VE_HexagonDirection_OppositeDirection(direction);
			}
			std::int32_t start[2], end[2];
			NC2E2::DisplayDescription_MapMigToWorldPixelCoordinates(display, Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates), start, true);
			std::uint32_t target = Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates);
			NC2Logic::MapMigPoint_MoveCursorUnwrapped(target, direction);
			NC2E2::DisplayDescription_MapMigToWorldPixelCoordinates(display, target, end, true);
			const std::int32_t dx = NXBasics::Runtime_TimeSigned32((static_cast<std::uint32_t>(end[0]) - static_cast<std::uint32_t>(start[0])) * progress) / NC2Runtime::WalkingProgress::FullStepSigned;
			const std::int32_t dy = NXBasics::Runtime_TimeSigned32((static_cast<std::uint32_t>(end[1]) - static_cast<std::uint32_t>(start[1])) * progress) / NC2Runtime::WalkingProgress::FullStepSigned;
			point[0] = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(point[0]) + static_cast<std::uint32_t>(dx));
			point[1] = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(point[1]) + static_cast<std::uint32_t>(dy));
		}
	}
	NC2E2::Display_LocalDisplayPixelToGlobalWorldPixelCoordinates(display, point, false);
}

namespace NC2E2
{
	// Original function: FUN_00491099; DirectMusic is disabled for a network game or active session.
	bool Tool_Sound_IsDMUsable()
	{
		return gDirectMusicManager != nullptr && NC2Logic::gGameStaticVars.networkGame == 0 && !NNetwork::Network_IsInSession();
	}
}

namespace NC2E2
{
	// Original function: FUN_00499019.
	void DE_Debug_DrawMicroGridMarker(const void* display, const void* point, const NXBasics::SColorRGB* outerColor, const NXBasics::SColorRGB* innerColor, std::uint32_t radius, bool filled)
	{
		std::int32_t pixel[2]; Display_MapMigToLocalDisplayPixelCoordinates(display, point, pixel, false);
		auto* bitmap = NC2Lifetime::Read<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap);
		if (filled)
		{
			bitmap->Draw_FilledCircle(pixel[0], pixel[1], radius + 1u, *outerColor); bitmap->Draw_FilledCircle(pixel[0], pixel[1], radius, *innerColor);
		}
		else
		{
			bitmap->Draw_Circle(pixel[0], pixel[1], radius - 1u, *outerColor); bitmap->Draw_Circle(pixel[0], pixel[1], radius + 1u, *outerColor); bitmap->Draw_Circle(pixel[0], pixel[1], radius, *innerColor);
		}
	}
	// Original function: FUN_004990a1; the native line starts at the second point.
	void DE_Debug_DrawMicroGridLine(const void* display, const void* first, const void* second, const NXBasics::SColorRGB* color)
	{
		std::int32_t firstPixel[2], secondPixel[2];
		Display_MapMigToLocalDisplayPixelCoordinates(display, first, firstPixel, false); Display_MapMigToLocalDisplayPixelCoordinates(display, second, secondPixel, false);
		NC2Lifetime::Read<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap)->Draw_Line(secondPixel[0], secondPixel[1], firstPixel[0], firstPixel[1], *color);
	}
}

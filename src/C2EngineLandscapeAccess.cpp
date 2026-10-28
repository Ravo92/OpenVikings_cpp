#include "C2E2StaticDataTerrain.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameMapAccess.h"
#include "C2HumanInitialization.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"
#include <cstring>
#include <new>

namespace NC2E2
{
	// Original function: FUN_0048a7fb; the original destructor is an empty RET.
	static void __cdecl MiscLandscapeTokenList_AtExit()
	{}
	// Original function: FUN_0048a7fa; the original destructor is an empty RET.
	static void __cdecl MiscHouseLandscapeTokenList_AtExit()
	{}
	template<void(__cdecl* AtExit)()> static NXBasics::CTokenList& GetLazyTokenList(const NXBasics::SToken* tokens)
	{
		static bool initialized = false;
		alignas(NXBasics::CTokenList) static std::byte storage[sizeof(NXBasics::CTokenList)];
		if (!initialized)
		{
			initialized = true;
			::new (storage) NXBasics::CTokenList(tokens);
			NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(AtExit));
		}
		return *reinterpret_cast<NXBasics::CTokenList*>(storage);
	}

	// Original function: FUN_00481002.
	std::int32_t Engine2d_EntryPoint_GetLandscapeIdByNamePtr(const char* name)
	{
		return E2StaticDataLandscapesManager_GetLandscapeId(name);
	}
	// Original function: FUN_004825be; Windows argument order and border rejection.
	bool EngineMap_MapMicroPoint_Landscape_SetType(const void* point, std::int32_t engineId, std::uint32_t valency, bool preserveWalkingBlock, std::int32_t player)
	{
		if (engineId == -1 || EngineMap_GetConstMapMicroPointPtr(point) == nullptr || NC2Logic::MapMigPoint_IsBorderPoint(point)) return false;
		const auto type = NC2Lifetime::Read<std::uint32_t>(gStaticLandscapeEntries, static_cast<std::size_t>(static_cast<std::uint32_t>(engineId)) * NC2Layout::RecordSize::StaticLandscapeUnsigned + NC2Layout::StaticLandscapeOffset::LogicTypeUnsigned);
		return NC2Logic::Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(point, type, valency, engineId, preserveWalkingBlock, player);
	}

	// Original function: FUN_0048100d; edits require the original modify scope.
	bool Engine2d_EntryPoint_Edit_SetMapMicroPointLandscape(const void* point, std::int32_t engineId, std::uint32_t valency, bool preserveWalkingBlock, std::int32_t player)
	{
		return NC2Logic::gGameMapModifyNestingDepth != 0 && EngineMap_MapMicroPoint_Landscape_SetType(point, engineId, valency, preserveWalkingBlock, player);
	}
	// Original function: FUN_0048a4ba; the complete 00568548..00568567 region contains
	// two owned count/pointer pairs, lazy token lists/flags, and the manager pointer.
	// Windows token tables: 004F4760..004F477F and 00506270..0050628F after lazy init.
	void* __fastcall E2StaticDataMiscLandscapeManager_Construct(void* manager, void*)
	{
		using namespace NC2Lifetime;
		static constexpr NXBasics::SToken landscapeTokens[] = { { "logictribetype", SwitchIds::MiscHouseLandscapeToken::LogicTribeType },{ "logiclandscapetype", SwitchIds::MiscLandscapeToken::LogicLandscapeType },{ "landscapename", SwitchIds::MiscHouseLandscapeToken::LandscapeName },{ nullptr, 0 } };
		static constexpr NXBasics::SToken groupTokens[] = { { "logictribetype", SwitchIds::MiscHouseLandscapeToken::LogicTribeType },{ "logichousetype", SwitchIds::MiscHouseLandscapeToken::LogicHouseType },{ "landscapename", SwitchIds::MiscHouseLandscapeToken::LandscapeName },{ nullptr, 0 } };
		gStaticMiscLandscapeEntryCount = 0;
		gStaticMiscLandscapeEntries = nullptr;
		gStaticMiscLandscapeGroupCount = 0;
		gStaticMiscLandscapeGroups = nullptr;
		gE2StaticDataMiscLandscapeManager = manager;
		NXBasics::CIniFileReader reader("data\\engine2d\\inis\\misc\\misc.ini", nullptr, false, nullptr, nullptr);
		if (!reader.IsLoaded()) return manager;
		const auto allocate = [] (std::uint32_t bytes)
		{
			auto* entries = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(bytes, 1));
			std::memset(entries, 0, bytes);
			return entries;
		};
		gStaticMiscLandscapeEntryCount = reader.GetNumberOfGroups("tribelandscapelinkdata");
		if (gStaticMiscLandscapeEntryCount != 0)
		{
			auto* entry = allocate(gStaticMiscLandscapeEntryCount * 12u);
			gStaticMiscLandscapeEntries = entry;
			for (auto* group = reader.GetFirstGroup("tribelandscapelinkdata"); group != nullptr; group = reader.GetNextGroup("tribelandscapelinkdata"), entry += 12)
				while (reader.GetNextCommandLineInGroup() != nullptr)
					switch (GetLazyTokenList<MiscLandscapeTokenList_AtExit>(landscapeTokens).FindToken(reader.GetCurrentName()))
					{
						case SwitchIds::MiscLandscapeToken::LogicTribeType: Write(entry, NC2Layout::MiscLandscapeLinkOffset::TribeType, reader.GetInt()); break;
						case SwitchIds::MiscLandscapeToken::LogicLandscapeType: Write(entry, NC2Layout::MiscLandscapeLinkOffset::LogicLandscapeType, reader.GetInt()); break;
						case SwitchIds::MiscLandscapeToken::LandscapeName: Write(entry, NC2Layout::MiscLandscapeLinkOffset::EngineLandscapeId, E2StaticDataLandscapesManager_GetLandscapeId(reader.GetString())); break;
					}
		}
		gStaticMiscLandscapeGroupCount = reader.GetNumberOfGroups("tribehouseLinkData");
		if (gStaticMiscLandscapeGroupCount != 0)
		{
			auto* entry = allocate(gStaticMiscLandscapeGroupCount * 20u);
			gStaticMiscLandscapeGroups = entry;
			for (auto* group = reader.GetFirstGroup("tribehouseLinkData"); group != nullptr; group = reader.GetNextGroup("tribehouseLinkData"), entry += 20)
			{
				auto* choices = allocate(reader.GetNumberOfCommandsInGroup("landscapename") * 8u);
				Write(entry, NC2Layout::MiscHouseLandscapeLinkOffset::Choices, choices);
				reader.SetCurrentLineToGroupStart();
				while (reader.GetNextCommandLineInGroup() != nullptr)
					switch (GetLazyTokenList<MiscHouseLandscapeTokenList_AtExit>(groupTokens).FindToken(reader.GetCurrentName()))
					{
						case SwitchIds::MiscHouseLandscapeToken::LogicTribeType: Write(entry, NC2Layout::MiscHouseLandscapeLinkOffset::TribeType, reader.GetInt()); break;
						case SwitchIds::MiscHouseLandscapeToken::LogicHouseType: Write(entry, NC2Layout::MiscHouseLandscapeLinkOffset::LogicHouseType, reader.GetInt()); break;
						case SwitchIds::MiscHouseLandscapeToken::LandscapeName:
							{
								const auto index = Read<std::uint32_t>(entry, NC2Layout::MiscHouseLandscapeLinkOffset::ChoiceCount);
								Write(choices, index * NC2Layout::RecordSize::WeightedLandscapeChoiceUnsigned, E2StaticDataLandscapesManager_GetLandscapeId(reader.GetString()));
								const auto weight = reader.GetTypeOfNextParameter() == 3 ? static_cast<std::uint32_t>(reader.GetInt()) : 1u;
								Write(choices, index * NC2Layout::RecordSize::WeightedLandscapeChoiceUnsigned + NC2Layout::WeightedLandscapeChoiceOffset::Weight, weight);
								Write(entry, NC2Layout::MiscHouseLandscapeLinkOffset::TotalWeight, Read<std::uint32_t>(entry, NC2Layout::MiscHouseLandscapeLinkOffset::TotalWeight) + weight);
								Write(entry, NC2Layout::MiscHouseLandscapeLinkOffset::ChoiceCount, index + 1u);
								break;
							}
					}
			}
		}
		return manager;
	}
	// Original function: FUN_0048a86c; exact tribe/type lookup.
	std::int32_t E2StaticDataMiscLandscapeManager_GetEngineLandscapeId(std::uint32_t tribe, std::uint32_t type)
	{
		using namespace NC2Lifetime;
		for (std::uint32_t index = 0; index < NC2E2::gStaticMiscLandscapeEntryCount; ++index)
		{
			const auto offset = index * NC2Layout::RecordSize::MiscLandscapeLinkUnsigned;
			if (Read<std::uint32_t>(NC2E2::gStaticMiscLandscapeEntries, offset) == tribe && Read<std::uint32_t>(NC2E2::gStaticMiscLandscapeEntries, offset + NC2Layout::MiscLandscapeLinkOffset::LogicLandscapeType) == type) return Read<std::int32_t>(NC2E2::gStaticMiscLandscapeEntries, offset + NC2Layout::MiscLandscapeLinkOffset::EngineLandscapeId);
		}
		return -1;
	}

	// Original function: FUN_0048a8a1. Exact type precedes type zero.
	// The constructor/destructor own 00568548..00568557; groups have a 20-byte stride,
	// nested eight-byte engine-id/weight entries, and unsigned total weight at +12.
	std::int32_t E2StaticDataMiscLandscapeManager_GetRandomEngineLandscapeId(std::uint32_t tribe, std::uint32_t type)
	{
		using namespace NC2Lifetime;
		for (std::uint32_t pass = 0; pass < 2; ++pass)
			for (std::uint32_t index = 0; index < NC2E2::gStaticMiscLandscapeGroupCount; ++index)
			{
				const auto offset = index * NC2Layout::RecordSize::MiscHouseLandscapeLinkUnsigned;
				if (Read<std::uint32_t>(NC2E2::gStaticMiscLandscapeGroups, offset) != tribe || Read<std::uint32_t>(NC2E2::gStaticMiscLandscapeGroups, offset + NC2Layout::MiscHouseLandscapeLinkOffset::LogicHouseType) != (pass == 0 ? type : 0u)) continue;
				auto choice = static_cast<std::uint32_t>(NXBasics::Runtime_Rand()) % Read<std::uint32_t>(NC2E2::gStaticMiscLandscapeGroups, offset + NC2Layout::MiscHouseLandscapeLinkOffset::TotalWeight);
				const auto count = Read<std::uint32_t>(NC2E2::gStaticMiscLandscapeGroups, offset + NC2Layout::MiscHouseLandscapeLinkOffset::ChoiceCount);
				const auto* entries = Read<const std::byte*>(NC2E2::gStaticMiscLandscapeGroups, offset + NC2Layout::MiscHouseLandscapeLinkOffset::Choices);
				for (std::uint32_t item = 0; item < count; ++item)
				{
					choice -= Read<std::uint32_t>(entries, item * NC2Layout::RecordSize::WeightedLandscapeChoiceUnsigned + NC2Layout::WeightedLandscapeChoiceOffset::Weight);
					if (NXBasics::Runtime_TimeSigned32(choice) <= 0) return Read<std::int32_t>(entries, item * NC2Layout::RecordSize::WeightedLandscapeChoiceUnsigned);
				}
			}
		return -1;
	}

}

namespace NC2LogicExternals
{
	// Original function: FUN_00480727; engine lookup precedes the logic lookup.
	const NC2Logic::CMapArea* External_Landscape_GetAreaPtr(const void* point, std::uint32_t areaKind)
	{
		const auto engine = NC2E2::EngineMap_GetConstMapMicroPointPtr(point);
		const auto logic = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
		const auto valency = NC2Lifetime::Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? NC2Lifetime::Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0;
		return External_Landscape_GetAreaPtr(NC2Lifetime::Read<std::uint16_t>(engine, NC2Layout::EngineMapMicroPointOffset::LandscapeId), valency, areaKind);
	}
	// Original function: FUN_00480893.
	std::int32_t External_Landscape_GetLandscapeIdByNamePtr(const char* name)
	{
		return NC2E2::Engine2d_EntryPoint_GetLandscapeIdByNamePtr(name);
	}
	// Original function: FUN_00480877.
	bool External_Landscape_SetMapMicroPointLandscape(const void* point, std::int32_t engineId, std::uint32_t valency, bool preserveWalkingBlock, std::int32_t player)
	{
		return NC2E2::Engine2d_EntryPoint_Edit_SetMapMicroPointLandscape(point, engineId, valency, preserveWalkingBlock, player);
	}
	// Original function: FUN_004808f4; distinct native house ruin adapter.
	std::int32_t External_HouseType_GetEngineRuineLandscapeId(std::uint32_t tribe, std::uint32_t type)
	{
		return External_Misc_GetEngineCadaverLandscapeId(tribe, type);
	}
	// Original function: FUN_00480904.
	std::int32_t External_Misc_GetEngineCadaverLandscapeId(std::uint32_t tribe, std::uint32_t type)
	{
		return NC2E2::E2StaticDataMiscLandscapeManager_GetEngineLandscapeId(tribe, type);
	}
	// Original function: FUN_004808f4; the Windows wrapper's original name is unresolved.
	std::int32_t External_Misc_GetEngineDepotLandscapeId(std::uint32_t tribe, std::uint32_t type)
	{
		return NC2E2::E2StaticDataMiscLandscapeManager_GetRandomEngineLandscapeId(tribe, type);
	}

	// Original function: FUN_0045343e.
	std::uint32_t External_Landscape_GetMaximumValency(std::int32_t engineId)
	{
		return engineId == -1 ? 0u : NC2Lifetime::Read<std::uint32_t>(External_Landscape_GetLogicDataPtr(engineId), NC2Layout::LandscapeLogicViewOffset::MaximumValency);
	}

	// Original function: FUN_00453456.
	std::int32_t External_Landscape_GetTransitionTarget(std::int32_t engineId, std::uint32_t transition)
	{
		if (engineId == -1) return -1;
		const auto* data = External_Landscape_GetLogicDataPtr(engineId);
		const auto count = NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::LandscapeLogicViewOffset::TransitionCount);
		const auto* entries = NC2Lifetime::Read<const std::byte*>(data, NC2Layout::LandscapeLogicViewOffset::Transitions);
		for (std::uint32_t i = 0; i < count; ++i) if (NC2Lifetime::Read<std::uint32_t>(entries, i * NC2Layout::RecordSize::LandscapeTransitionUnsigned) == transition) return NC2Lifetime::Read<std::int32_t>(entries, i * NC2Layout::RecordSize::LandscapeTransitionUnsigned + NC2Layout::LandscapeTransitionOffset::TargetLandscapeId);
		return -1;
	}

	// Original function: FUN_00480803.
	void External_Landscape_Clear(const void* point)
	{
		auto* micro = const_cast<void*>(NC2E2::EngineMap_GetConstMapMicroPointPtr(point));
		const auto id = NC2Lifetime::Read<std::uint16_t>(micro, NC2Layout::EngineMapMicroPointOffset::LandscapeId);
		if (id == NXBasics::IntegerRange::UInt16Maximum) return;
		const auto parameter = NC2Lifetime::Read<std::uint8_t>(NC2E2::gStaticLandscapeEntries, static_cast<std::size_t>(static_cast<std::uint32_t>(id)) * NC2Layout::RecordSize::StaticLandscapeUnsigned + NC2Layout::StaticLandscapeOffset::IsStaticUnsigned);
		NC2Lifetime::Write<std::uint16_t>(micro, NC2Layout::EngineMapMicroPointOffset::LandscapeId, NXBasics::IntegerRange::UInt16Maximum);
		NC2Logic::GameMapModifyManager_GameMapModified(0x32, point, parameter);
	}

	// Original function: FUN_0048083f.
	void External_Landscape_InternallySetData(const void* point, std::int32_t engineId)
	{
		auto* micro = const_cast<void*>(NC2E2::EngineMap_GetConstMapMicroPointPtr(point));
		if (NC2Lifetime::Read<std::uint16_t>(micro, NC2Layout::EngineMapMicroPointOffset::LandscapeId) == static_cast<std::uint32_t>(engineId)) return;
		NC2Lifetime::Write<std::uint16_t>(micro, NC2Layout::EngineMapMicroPointOffset::LandscapeId, static_cast<std::uint16_t>(engineId));
		NC2Logic::GameMapModifyManager_GameMapModified(0x32, point, NC2Lifetime::Read<std::uint8_t>(NC2E2::gStaticLandscapeEntries, static_cast<std::size_t>(static_cast<std::uint32_t>(engineId)) * NC2Layout::RecordSize::StaticLandscapeUnsigned + NC2Layout::StaticLandscapeOffset::IsStaticUnsigned));
	}

	// Original function: FUN_00480712.
	const void* External_Landscape_GetLogicDataPtr(std::int32_t engineId)
	{
		return static_cast<const std::byte*>(NC2E2::gStaticLandscapeEntries) + static_cast<std::size_t>(static_cast<std::uint32_t>(engineId)) * sizeof(NC2E2::SE2StaticDataLandscape) + offsetof(NC2E2::SE2StaticDataLandscape, logicType);
	}
	// Original function: FUN_00480914.
	std::int32_t External_Misc_GetEngineWallElementLandscapeId()
	{
		return NC2E2::gStaticLandscapeStorage.wallIds[0];
	}
	// Original function: FUN_0048091a.
	std::int32_t External_Misc_GetEngineWallGateLandscapeId(std::uint32_t gate, bool open)
	{
		return NC2E2::gStaticLandscapeStorage.gateIds[(gate % 3u) * 2u + (open ? 1u : 0u)];
	}

	// Original function: FUN_0048075e.
	const NC2Logic::CMapArea* External_Landscape_GetAreaPtr(std::int32_t engineId, std::uint32_t valency, std::uint32_t areaKind)
	{
		if (engineId == -1) return nullptr;
		const auto* landscape = static_cast<const NC2E2::SE2StaticDataLandscape*>(NC2E2::gStaticLandscapeEntries) + engineId;
		const auto& list = landscape->areas[areaKind];
		if (list.count == 0) return nullptr;
		for (auto index = NXBasics::Runtime_TimeSigned32(list.count - 1u); index >= 0; --index)
		{
			if (list.entries[index].valency <= valency) return list.entries[index].area;
		}
		return nullptr;
	}

	// Original function: FUN_004807a9.
	std::int32_t External_Landscape_GetEngineIdOfMapLandscape(const void* point)
	{
		const auto* micro = NC2E2::EngineMap_GetConstMapMicroPointPtr(point);
		if (micro == nullptr) return -1;
		const auto id = NC2Lifetime::Read<std::uint16_t>(micro, NC2Layout::EngineMapMicroPointOffset::LandscapeId);
		return id == NXBasics::IntegerRange::UInt16Maximum ? -1 : static_cast<std::int32_t>(id);
	}

	// Original function: FUN_004807c8.
	std::int32_t External_Landscape_GetEngineIdOfMapLandscape(std::int32_t landscapeType, bool requireMapLandscape)
	{
		const std::int32_t count = NC2E2::gStaticLandscapeCount;
		if (count <= 0) return -1;
		for (std::int32_t index = 0; index < count; ++index)
		{
			// Runtime stride follows the native landscape record.
			const auto* record = static_cast<const std::byte*>(NC2E2::gStaticLandscapeEntries) + static_cast<std::size_t>(static_cast<std::uint32_t>(index)) * sizeof(NC2E2::SE2StaticDataLandscape);
			if (NC2Lifetime::Read<std::int32_t>(record, NC2Layout::StaticLandscapeOffset::LogicType) != landscapeType) continue;
			if (!requireMapLandscape || NC2Lifetime::Read<std::uint8_t>(record, NC2Layout::StaticLandscapeOffset::PileableOnMap) != 0) return index;
		}
		return -1;
	}
}

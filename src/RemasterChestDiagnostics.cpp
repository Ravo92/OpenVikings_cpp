#include "RemasterChestDiagnostics.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineDisplayManager.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "RemasterLog.h"

#include <array>
#include <vector>

namespace NRemaster::ChestDiagnostics
{
	namespace
	{
		using NC2Lifetime::Read;
		using DrawState = std::array<std::uint32_t, 11>;

		struct SDisplaySample
		{
			const void* display;
			std::uint32_t pass;
			DrawState state;
		};

		struct SChestTrace
		{
			bool active = false;
			std::uint32_t point = 0;
			std::int32_t expectedGraphic = -1;
			std::vector<SDisplaySample> displays;
		};

		SChestTrace& Trace() noexcept
		{
			static SChestTrace trace;

			return trace;
		}

		const NC2E2::SE2StaticDataLandscape* Graphic(std::int32_t id) noexcept
		{
			const auto& storage = NC2E2::gStaticLandscapeStorage;

			if (id < 0 || id >= storage.count || storage.entries == nullptr)
				return nullptr;

			return static_cast<const NC2E2::SE2StaticDataLandscape*>(storage.entries) + id;
		}

		std::int32_t ConfiguredFrame(const NC2E2::SE2StaticDataLandscape* graphic, std::uint32_t valency) noexcept
		{
			if (graphic == nullptr || graphic->frameGroups == nullptr)
				return -1;

			for (auto index = graphic->frameGroupCount; index != 0; --index)
			{
				const auto& group = graphic->frameGroups[index - 1];

				if (group.valency <= valency)
					return group.count != 0 && group.frames != nullptr ? group.frames[0] : -1;
			}

			return -1;
		}

		std::int32_t ActiveCacheFlag() noexcept
		{
			const auto* manager = NC2E2::C2DEngineDisplayManager::sTheObjectPtr;
			const auto* display = manager != nullptr ? manager->GetActiveDisplay() : nullptr;

			return display != nullptr ? Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag) : -1;
		}

		void LogMapState(const char* event, const void* point, std::int32_t result, bool requestedPointMatches) noexcept
		{
			const auto& trace = Trace();
			const auto* logic = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
			const auto id = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(point);
			const auto* graphic = Graphic(id);
			const auto* expected = Graphic(trace.expectedGraphic);
			const auto valency = Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::LandscapeValency);
			Logging::Event(Logging::ECategory::Renderer, event, "tick=%u map=(%d,%d) kind=%u type=%u valency=%u graphic=%d name=\"%.40s\" expected_graphic=%d expected_name=\"%.40s\" expected_match=%u transition_result=%d requested_point_matches=%u static=%u foreground=%u background=%u loop=%u frame_groups=%u configured_first_frame=%d active_cache_dirty=%d", NC2Logic::gGameTime.tick, Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), static_cast<unsigned>(Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::ObjectType)), static_cast<unsigned>(Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::ObjectData)), static_cast<unsigned>(valency), id, graphic != nullptr ? graphic->name : "<invalid>", trace.expectedGraphic, expected != nullptr ? expected->name : "<invalid>", static_cast<unsigned>(id == trace.expectedGraphic), result, static_cast<unsigned>(requestedPointMatches), graphic != nullptr ? static_cast<unsigned>(graphic->isStatic) : 0u, graphic != nullptr ? static_cast<unsigned>(graphic->dynamicForeground) : 0u, graphic != nullptr ? static_cast<unsigned>(graphic->dynamicBackground) : 0u, graphic != nullptr ? static_cast<unsigned>(graphic->loopAnimation) : 0u, graphic != nullptr ? graphic->frameGroupCount : 0u, ConfiguredFrame(graphic, valency), ActiveCacheFlag());
		}
	}

	void BeginOpen(std::uint32_t humanId, std::uint32_t playerId, const void* point) noexcept
	{
		if (!Logging::Enabled(Logging::ECategory::Renderer))
		{

			return;
		}

		auto& trace = Trace();
		trace.active = true;
		trace.point = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		trace.displays.clear();
		const auto id = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(point);
		trace.expectedGraphic = Graphic(id) != nullptr ? NC2LogicExternals::External_Landscape_GetTransitionTarget(id, 11) : -1;
		Logging::Event(Logging::ECategory::Renderer, "chest_open_actor", "tick=%u human=%u player=%u map=(%d,%d) tracking=latest_opened_chest", NC2Logic::gGameTime.tick, humanId, playerId, Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y));
		LogMapState("chest_open_begin", point, -1, true);
	}

	void EndOpen(const void* point, std::int32_t transitionResult) noexcept
	{
		if (!Logging::Enabled(Logging::ECategory::Renderer) || !Trace().active)
		{

			return;
		}

		const auto& trace = Trace();
		LogMapState("chest_open_end", &trace.point, transitionResult, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates) == trace.point);
	}

	void ObserveDraw(const void* display, const void* point, bool staticPart, bool backgroundPart, bool submitted) noexcept
	{
		auto& trace = Trace();

		if (!trace.active || Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates) != trace.point)
			return;

		const auto* logic = NC2Logic::GameMap_GetConstMapMicroPointPtr(point);
		const auto id = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(point);
		const auto* graphic = Graphic(id);
		const auto kind = Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::ObjectType);
		const auto type = Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::ObjectData);
		const auto valency = Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::LandscapeValency);
		const auto pass = staticPart ? 0u : backgroundPart ? 1u
		                                                   : 2u;
		const DrawState state{ static_cast<std::uint32_t>(id), static_cast<std::uint32_t>(kind) | (static_cast<std::uint32_t>(type) << 8) | (static_cast<std::uint32_t>(valency) << 16), Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags), Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter), Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag), Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag), static_cast<std::uint32_t>(submitted), Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId), graphic != nullptr ? static_cast<std::uint32_t>(graphic->isStatic) | (static_cast<std::uint32_t>(graphic->dynamicForeground) << 8) | (static_cast<std::uint32_t>(graphic->dynamicBackground) << 16) : 0u, graphic != nullptr ? graphic->frameGroupCount : 0u, static_cast<std::uint32_t>(ConfiguredFrame(graphic, kind == 1 ? valency : 0u)) };

		try
		{
			bool found = false;

			for (auto& sample : trace.displays)
			{
				if (sample.display == display && sample.pass == pass)
				{
					if (sample.state == state)
						return;

					sample.state = state;
					found = true;
					break;
				}
			}

			if (!found)
				trace.displays.push_back({ display, pass, state });
		}
		catch (...)
		{
			trace.active = false;
			Logging::Event(Logging::ECategory::Renderer, "chest_trace_stopped", "reason=diagnostic_allocation_failed");

			return;
		}

		Logging::Event(Logging::ECategory::Renderer, "chest_draw", "tick=%u map=(%d,%d) display=%p active=%u player=%u pass=%s submitted=%u graphic=%d name=\"%.40s\" expected_graphic=%d expected_match=%u kind=%u type=%u valency=%u flags=0x%08X zoom=%d cache_dirty=%u static=%u foreground=%u background=%u draw_void_ever=%u frame_groups=%u configured_first_frame=%d", NC2Logic::gGameTime.tick, Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), display, state[5], state[7], staticPart ? "static" : backgroundPart ? "background" : "foreground", state[6], id, graphic != nullptr ? graphic->name : "<invalid>", trace.expectedGraphic, static_cast<unsigned>(id == trace.expectedGraphic), static_cast<unsigned>(kind), static_cast<unsigned>(type), static_cast<unsigned>(valency), state[2], Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter), state[4], graphic != nullptr ? static_cast<unsigned>(graphic->isStatic) : 0u, graphic != nullptr ? static_cast<unsigned>(graphic->dynamicForeground) : 0u, graphic != nullptr ? static_cast<unsigned>(graphic->dynamicBackground) : 0u, graphic != nullptr ? static_cast<unsigned>(graphic->drawVoidEver) : 0u, state[9], ConfiguredFrame(graphic, kind == 1 ? valency : 0u));
	}
}
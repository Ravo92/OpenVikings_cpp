#include "C2E2StaticDataParticles.h"
#include "C2EngineParticleDrawing.h"
#include "C2EngineParticleSimulation.h"
#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cmath>
#include <cstring>

namespace NC2E2
{
	using namespace NC2Lifetime;

	namespace
	{
		std::byte* At(void* base, std::uint32_t offset)
		{

			return static_cast<std::byte*>(base) + offset;
		}

		const std::byte* At(const void* base, std::uint32_t offset)
		{

			return static_cast<const std::byte*>(base) + offset;
		}

		void* StaticParticle(std::uint32_t id)
		{

			return At(gStaticParticelStorage.entries, id * NC2Layout::RecordSize::StaticParticleUnsigned);
		}

		std::uint32_t RandUnsigned()
		{

			return static_cast<std::uint32_t>(NXBasics::Runtime_Rand());
		}

		// Original function: FUN_00497f86; helper name unresolved.
		void ParticelPosition_SetFromMapWithHeight(void* position, const void* point)
		{
			Write(position, NC2Layout::ParticlePositionOffset::MapX, static_cast<double>(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X)) / NC2Logic::GameMap_GetWidth());
			Write(position, NC2Layout::ParticlePositionOffset::MapY, static_cast<double>(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y)) / NC2Logic::GameMap_GetHeight());

			if ((Read<std::uint8_t>(point, NC2Layout::MapCoordinatePairOffset::Y) & 1u) != 0)
				Write(position, NC2Layout::ParticlePositionOffset::MapX, Read<double>(position, NC2Layout::ParticlePositionOffset::MapX) + 1.0 / (2.0 * NC2Logic::GameMap_GetWidth()));

			Write(position, NC2Layout::ParticlePositionOffset::Height, NC2Logic::GameMap_GetHeightAtMapMigPoint(point) * 0.004);
		}

		// Original function: FUN_00497ffc; helper name unresolved.
		void ParticelPosition_SetFromMapWithoutHeight(void* position, const void* point)
		{
			Write(position, NC2Layout::ParticlePositionOffset::MapX, static_cast<double>(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X)) / NC2Logic::GameMap_GetWidth());
			Write(position, NC2Layout::ParticlePositionOffset::MapY, static_cast<double>(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y)) / NC2Logic::GameMap_GetHeight());

			if ((Read<std::uint8_t>(point, NC2Layout::MapCoordinatePairOffset::Y) & 1u) != 0)
				Write(position, NC2Layout::ParticlePositionOffset::MapX, Read<double>(position, NC2Layout::ParticlePositionOffset::MapX) + 1.0 / (2.0 * NC2Logic::GameMap_GetWidth()));

			Write(position, NC2Layout::ParticlePositionOffset::Height, 0.0);
		}

		// Original function: FUN_00487298; unsigned speed parameters.
		void ParticelObject_SetSpeed(void* particle, std::uint32_t x, std::uint32_t height, std::uint32_t y)
		{
			Write(particle, NC2Layout::ParticleOffset::SpeedMapX, (1.0 / NC2Logic::GameMap_GetWidth()) * x * 0.01);
			Write(particle, NC2Layout::ParticleOffset::SpeedHeight, static_cast<double>(height) * 0.00004);
			Write(particle, NC2Layout::ParticleOffset::SpeedMapY, (1.0 / NC2Logic::GameMap_GetHeight()) * y * 0.01);
		}

		// Original function: FUN_004873a8; helper name unresolved.
		void ParticelObject_UseWind(void* particle)
		{
			if (Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode) == SwitchIds::ParticleMotionMode::Fall)
				Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode, SwitchIds::ParticleMotionMode::FallWithGroundRefresh);

			if (Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode) == SwitchIds::ParticleMotionMode::Rise)
				Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode, SwitchIds::ParticleMotionMode::RiseAlternate);

			if (Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode) == SwitchIds::ParticleMotionMode::Stationary)
				Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode, SwitchIds::ParticleMotionMode::FollowGround);

			Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseWind, 1);
		}

		// Original function: FUN_00487315; frame/lifetime tick precedes deferred movement.
		void ParticelObject_TickLifetimeAndFrame(void* particle)
		{
			const auto age = Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks);

			if (age == 0)
			{
				Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active, 0);
			}
			else
			{
				Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, age - 1u);
			}

			Write(particle, NC2Layout::ParticleOffset::PendingMovementTicks, Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::PendingMovementTicks) + 1u);

			if (Read<std::uint8_t>(particle, NC2Layout::ParticleOffset::ValencyIsDirection) == 0)
			{
				void* data = StaticParticle(Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::StaticParticleId));
				Write(particle, NC2Layout::ParticleOffset::FrameIndex, Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::FrameIndex) + 1u);
				void* frames = ParticelGraphics_GetValency(At(data, NC2Layout::StaticParticleOffset::Graphics), Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::Valency));

				if (Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::FrameIndex) >= Read<std::uint32_t>(frames, NC2Layout::ParticleFramesOffset::FrameCount))
				{
					if (Read<std::uint8_t>(data, NC2Layout::StaticParticleOffset::AnimationLoops) == 0)
					{
						Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active, 0);

						return;
					}

					Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::FrameIndex, 0);
				}
			}

			if (Read<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseWind) != 0)
			{
				Write(particle, NC2Layout::ParticleOffset::Position, Read<double>(particle, NC2Layout::ParticleOffset::Position) + Read<double>(gE2ParticelWind, NC2Layout::ParticleWindOffset::DeltaMapX));
				Write(particle, NC2Layout::ParticleOffset::PositionMapY, Read<double>(particle, NC2Layout::ParticleOffset::PositionMapY) + Read<double>(gE2ParticelWind, NC2Layout::ParticleWindOffset::DeltaMapY));
			}
		}

		// Original function: FUN_00487172; helper name unresolved.
		void ParticelWind_Tick(void* wind)
		{
			Write(wind, NC2Layout::ParticleWindOffset::TicksSinceChange, Read<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::TicksSinceChange) + 1u);

			if (Read<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::TicksSinceChange) > 600)
			{
				E2ParticelWind_ChangeWind(wind);
				Write<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::TicksSinceChange, 0);
			}
		}

		// Original function: FUN_004874d6; signed random map-area offsets.
		void ParticelGenerator_OffsetMapPoint(const void* generator, void* point)
		{
			const auto width = Read<std::int32_t>(generator, NC2Layout::ParticleGeneratorOffset::AreaWidth);
			const auto dx = NXBasics::Runtime_Rand() % width - (width >> 1);
			Write<std::uint16_t>(point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(point, NC2Layout::MapCoordinatePairOffset::X) + dx));
			const auto height = Read<std::int32_t>(generator, NC2Layout::ParticleGeneratorOffset::AreaHeight);
			const auto dy = NXBasics::Runtime_Rand() % height - (height >> 1);
			Write<std::uint16_t>(point, NC2Layout::MapCoordinatePairOffset::Y, static_cast<std::uint16_t>(Read<std::uint16_t>(point, NC2Layout::MapCoordinatePairOffset::Y) + dy));
		}
	}

	// Original function: FUN_0049761d.
	void LogicParticelObject_SetStartPosition(void* particle, const void* mapPoint, const void* pixelPoint, std::uint32_t height)
	{
		void* position = At(particle, NC2Layout::ParticleOffset::Position);

		if (Read<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseMapHeights) != 0)
		{
			ParticelPosition_SetFromMapWithHeight(position, mapPoint);
			Write(particle, NC2Layout::ParticleOffset::CachedGroundHeight, Read<double>(particle, NC2Layout::ParticleOffset::PositionHeight));
			const double raised = Read<double>(particle, NC2Layout::ParticleOffset::PositionHeight) + static_cast<double>(height) * 0.004;
			Write(particle, NC2Layout::ParticleOffset::PositionHeight, raised < 1.0 || std::isnan(raised) ? raised : 1.0);
		}
		else
		{
			ParticelPosition_SetFromMapWithoutHeight(position, mapPoint);
		}

		Write(particle, NC2Layout::ParticleOffset::PositionPixelX, static_cast<double>(Read<std::int32_t>(pixelPoint, NC2Layout::PixelPointOffset::X)));
		Write(particle, NC2Layout::ParticleOffset::PositionPixelY, static_cast<double>(Read<std::int32_t>(pixelPoint, NC2Layout::PixelPointOffset::Y)));
		ParticelPosition_ToMapPoint(position, At(particle, NC2Layout::ParticleOffset::MapPoint));
	}

	// Original function: FUN_00497691.
	void LogicParticelObject_SetTargetPosition(void* particle, const void* mapPoint, const void* pixelPoint, std::uint32_t height)
	{
		void* position = At(particle, NC2Layout::ParticleOffset::TargetPosition);

		if (Read<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseMapHeights) != 0)
		{
			ParticelPosition_SetFromMapWithHeight(position, mapPoint);
			const double raised = Read<double>(particle, NC2Layout::ParticleOffset::TargetHeight) + static_cast<double>(height) * 0.004;
			Write(particle, NC2Layout::ParticleOffset::TargetHeight, raised < 1.0 || std::isnan(raised) ? raised : 1.0);
		}
		else
		{
			ParticelPosition_SetFromMapWithoutHeight(position, mapPoint);
		}

		Write(particle, NC2Layout::ParticleOffset::TargetPixelX, static_cast<double>(Read<std::int32_t>(pixelPoint, NC2Layout::PixelPointOffset::X)));
		Write(particle, NC2Layout::ParticleOffset::TargetPixelY, static_cast<double>(Read<std::int32_t>(pixelPoint, NC2Layout::PixelPointOffset::Y)));
		const auto mode = Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode);

		if (mode == SwitchIds::ParticleMotionMode::Linear || mode == SwitchIds::ParticleMotionMode::Parabolic)
			LogicParticelObject_CalculateMovingVector(particle);
	}

	// Original function: FUN_00497708.
	void LogicParticelObject_CalculateMovingVector(void* particle)
	{
		const double dx = Read<double>(particle, NC2Layout::ParticleOffset::Position) - Read<double>(particle, NC2Layout::ParticleOffset::TargetPosition);
		const double dh = Read<double>(particle, NC2Layout::ParticleOffset::PositionHeight) - Read<double>(particle, NC2Layout::ParticleOffset::TargetHeight);
		const double dy = Read<double>(particle, NC2Layout::ParticleOffset::PositionMapY) - Read<double>(particle, NC2Layout::ParticleOffset::TargetMapY);
		Write(particle, NC2Layout::ParticleOffset::VectorMapX, dx);
		Write(particle, NC2Layout::ParticleOffset::VectorHeight, dh);
		Write(particle, NC2Layout::ParticleOffset::VectorMapY, dy);
		const double ax = dx >= 0.0 ? dx : -dx;
		const double ay = dy >= 0.0 ? dy : -dy;
		double magnitude, duration;

		if (!(ax >= ay))
		{
			magnitude = ay;
			duration = ay / Read<double>(particle, NC2Layout::ParticleOffset::SpeedMapY);
		}
		else
		{
			magnitude = ax;
			duration = ax / Read<double>(particle, NC2Layout::ParticleOffset::SpeedMapX);

			if (!(duration < 1000000.0) && !std::isnan(duration))
				duration = 1000000.0;
		}

		if (magnitude != 0.0 && !std::isnan(magnitude))
		{
			Write(particle, NC2Layout::ParticleOffset::VectorMapX, dx / magnitude);
			Write(particle, NC2Layout::ParticleOffset::VectorMapY, dy / magnitude);
			Write(particle, NC2Layout::ParticleOffset::VectorHeight, dh / magnitude);
		}

		const auto mode = Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode);

		if (mode == SwitchIds::ParticleMotionMode::Linear || mode == SwitchIds::ParticleMotionMode::Parabolic)
		{
			if (mode == SwitchIds::ParticleMotionMode::Parabolic)
				Write(particle, NC2Layout::ParticleOffset::ParabolaBaselineHeight, Read<double>(particle, NC2Layout::ParticleOffset::PositionHeight));

			const auto ticks = static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(duration));
			Write(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks, ticks);
			Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, ticks);
			Write(particle, NC2Layout::ParticleOffset::SpeedHeight, magnitude / duration);
		}
		else
		{
			Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, 1);
		}

		Write(particle, NC2Layout::ParticleOffset::StepMapX, Read<double>(particle, NC2Layout::ParticleOffset::SpeedMapX) * Read<double>(particle, NC2Layout::ParticleOffset::VectorMapX));
		Write(particle, NC2Layout::ParticleOffset::StepHeight, Read<double>(particle, NC2Layout::ParticleOffset::SpeedHeight) * Read<double>(particle, NC2Layout::ParticleOffset::VectorHeight));
		Write(particle, NC2Layout::ParticleOffset::StepMapY, Read<double>(particle, NC2Layout::ParticleOffset::SpeedMapY) * Read<double>(particle, NC2Layout::ParticleOffset::VectorMapY));
		const double ticks = Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks);
		Write(particle, NC2Layout::ParticleOffset::StepPixelX, (Read<double>(particle, NC2Layout::ParticleOffset::PositionPixelX) - Read<double>(particle, NC2Layout::ParticleOffset::TargetPixelX)) / ticks);
		Write(particle, NC2Layout::ParticleOffset::StepPixelY, (Read<double>(particle, NC2Layout::ParticleOffset::PositionPixelY) - Read<double>(particle, NC2Layout::ParticleOffset::TargetPixelY)) / ticks);
	}

	// Original function: FUN_0049786c.
	void LogicParticelObject_CalculateMovingVector(void* particle, std::uint32_t duration)
	{
		const double dx = Read<double>(particle, NC2Layout::ParticleOffset::Position) - Read<double>(particle, NC2Layout::ParticleOffset::TargetPosition);
		const double dh = Read<double>(particle, NC2Layout::ParticleOffset::PositionHeight) - Read<double>(particle, NC2Layout::ParticleOffset::TargetHeight);
		const double dy = Read<double>(particle, NC2Layout::ParticleOffset::PositionMapY) - Read<double>(particle, NC2Layout::ParticleOffset::TargetMapY);
		Write(particle, NC2Layout::ParticleOffset::VectorMapX, dx);
		Write(particle, NC2Layout::ParticleOffset::VectorHeight, dh);
		Write(particle, NC2Layout::ParticleOffset::VectorMapY, dy);
		const double xSpeed = dx / duration, ySpeed = dy / duration;
		Write(particle, NC2Layout::ParticleOffset::SpeedMapX, xSpeed);
		Write(particle, NC2Layout::ParticleOffset::SpeedMapY, ySpeed);
		const double ax = dx >= 0.0 ? dx : -dx, ay = dy >= 0.0 ? dy : -dy;
		const double magnitude = ax >= ay ? ax : ay;

		if (magnitude != 0.0 && !std::isnan(magnitude))
		{
			Write(particle, NC2Layout::ParticleOffset::VectorMapX, dx / magnitude);
			Write(particle, NC2Layout::ParticleOffset::VectorMapY, dy / magnitude);
			Write(particle, NC2Layout::ParticleOffset::VectorHeight, dh / magnitude);
		}

		const auto mode = Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode);

		if (mode == SwitchIds::ParticleMotionMode::Linear || mode == SwitchIds::ParticleMotionMode::Parabolic)
		{
			if (mode == SwitchIds::ParticleMotionMode::Parabolic)
				Write(particle, NC2Layout::ParticleOffset::ParabolaBaselineHeight, Read<double>(particle, NC2Layout::ParticleOffset::PositionHeight));

			Write(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks, duration);
			Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, duration);
			Write(particle, NC2Layout::ParticleOffset::SpeedHeight, magnitude / duration);
		}
		else
		{
			Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, 1);
		}

		Write(particle, NC2Layout::ParticleOffset::StepMapX, xSpeed);
		Write(particle, NC2Layout::ParticleOffset::StepHeight, Read<double>(particle, NC2Layout::ParticleOffset::SpeedHeight) * Read<double>(particle, NC2Layout::ParticleOffset::VectorHeight));
		Write(particle, NC2Layout::ParticleOffset::StepMapY, ySpeed);
		Write(particle, NC2Layout::ParticleOffset::StepPixelX, (Read<double>(particle, NC2Layout::ParticleOffset::PositionPixelX) - Read<double>(particle, NC2Layout::ParticleOffset::TargetPixelX)) / duration);
		Write(particle, NC2Layout::ParticleOffset::StepPixelY, (Read<double>(particle, NC2Layout::ParticleOffset::PositionPixelY) - Read<double>(particle, NC2Layout::ParticleOffset::TargetPixelY)) / duration);
	}

	// Original function: FUN_0049798e; resets only the native fields, retaining pool-slot state.
	void LogicParticelObject_SetParticelId(void* particle, std::uint32_t id)
	{
		Write(particle, NC2Layout::ParticleOffset::StaticParticleId, id);
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::ValencyIsDirection, 0);
		void* data = StaticParticle(id);
		const auto maximum = Read<std::uint32_t>(data, NC2Layout::StaticParticleOffset::MaximumValency);
		Write(particle, NC2Layout::ParticleOffset::MaximumValency, maximum);
		Write(particle, NC2Layout::ParticleOffset::Valency, maximum - 1u);
		Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::FrameIndex, 0);
		Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode, SwitchIds::ParticleMotionMode::Stationary);
		Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::ValencyMode, SwitchIds::ParticleValencyMode::Fixed);
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseMapHeights, 0);
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::KillOnGroundCollision, 0);
		Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::PendingMovementTicks, 0);
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseWind, 0);
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::FadingSpawn, 0);
		void* frames = ParticelGraphics_GetValency(At(data, NC2Layout::StaticParticleOffset::Graphics), maximum - 1u);
		const auto age = frames != nullptr ? Read<std::uint32_t>(ParticelGraphics_GetValency(At(data, NC2Layout::StaticParticleOffset::Graphics), Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::Valency)), NC2Layout::ParticleFramesOffset::FrameCount) : 100u;
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::DrawBottom, 0);
		Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, age);
	}

	// Original function: FUN_00497c9b; sampled trajectory preserves the final ground-height cache.
	void LogicParticelObject_CalculateParrableHeight(void* particle)
	{
		const double x = Read<double>(particle, NC2Layout::ParticleOffset::Position), y = Read<double>(particle, NC2Layout::ParticleOffset::PositionMapY), height = Read<double>(particle, NC2Layout::ParticleOffset::PositionHeight);
		const auto point = Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::MapPoint);
		double maximum = height > Read<double>(particle, NC2Layout::ParticleOffset::TargetHeight) ? height : height + 0.1;

		if (maximum > 1.0)
			maximum = 1.0;

		Write(particle, NC2Layout::ParticleOffset::ParabolaHeight, maximum);

		for (auto remaining = Read<std::int32_t>(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks); remaining > 0; remaining -= 5)
		{
			Write(particle, NC2Layout::ParticleOffset::Position, Read<double>(particle, NC2Layout::ParticleOffset::Position) - Read<double>(particle, NC2Layout::ParticleOffset::StepMapX) * 5.0);
			Write(particle, NC2Layout::ParticleOffset::PositionMapY, Read<double>(particle, NC2Layout::ParticleOffset::PositionMapY) - Read<double>(particle, NC2Layout::ParticleOffset::StepMapY) * 5.0);
			Write(particle, NC2Layout::ParticleOffset::PositionHeight, Read<double>(particle, NC2Layout::ParticleOffset::PositionHeight) - Read<double>(particle, NC2Layout::ParticleOffset::StepHeight) * 5.0);
			const double progress = ((1.0 - static_cast<double>(remaining) / Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks)) - 0.5) * 2.0;
			const double correction = (progress * progress - 1.0) * Read<double>(particle, NC2Layout::ParticleOffset::ParabolaHeight);
			ParticelObject_RefreshGroundHeight(particle);

			if (!(Read<double>(particle, NC2Layout::ParticleOffset::PositionHeight) - correction >= Read<double>(particle, NC2Layout::ParticleOffset::CachedGroundHeight)))
				Write(particle, NC2Layout::ParticleOffset::ParabolaHeight, Read<double>(particle, NC2Layout::ParticleOffset::CachedGroundHeight) + 0.1);
		}

		if (Read<double>(particle, NC2Layout::ParticleOffset::ParabolaHeight) > 1.0)
			Write(particle, NC2Layout::ParticleOffset::ParabolaHeight, 1.0);

		Write(particle, NC2Layout::ParticleOffset::Position, x);
		Write(particle, NC2Layout::ParticleOffset::MapPoint, point);
		Write(particle, NC2Layout::ParticleOffset::PositionMapY, y);
		Write(particle, NC2Layout::ParticleOffset::PositionHeight, height);
	}

	// Original function: FUN_0048684b; a full pool returns null without modifying the high-water index.
	void* E2ParticelManager_GetCleanParticel(void* manager)
	{
		std::uint32_t index = 0;

		while (index < 10000 && Read<std::uint8_t>(manager, NC2Layout::ParticleManagerOffset::FirstParticleActiveUnsigned + index * NC2Layout::RecordSize::ParticleUnsigned) != 0)
			++index;

		if (index == 10000)
			return nullptr;

		if (index > Read<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastParticleIndex))
			Write(manager, NC2Layout::ParticleManagerOffset::LastParticleIndex, index);

		return At(manager, NC2Layout::ParticleManagerOffset::ParticlesUnsigned + index * NC2Layout::RecordSize::ParticleUnsigned);
	}

	// Original function: FUN_004864d1.
	void E2ParticelManager_CreateFallingSmokeParticel(void* manager, const void* mapPoint, std::uint32_t height, std::uint32_t lifetime)
	{
		void* particle = E2ParticelManager_GetCleanParticel(manager);
		static const auto smokeId = E2StaticDataParticelManager_GetParticelId("Smoke.org"); // Original lazy cache: 00568420/00568424.

		if (particle == nullptr)
		{

			return;
		}

		LogicParticelObject_SetParticelId(particle, static_cast<std::uint32_t>(smokeId));
		Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode, SwitchIds::ParticleMotionMode::Fall);
		ParticelObject_UseWind(particle);
		Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::ValencyMode, SwitchIds::ParticleValencyMode::Fixed);
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseMapHeights, 1);
		Write(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks, lifetime);
		Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, lifetime);
		LogicParticelObject_CalculateValency(particle);
		Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, lifetime - RandUnsigned() % (lifetime >> 1));
		LogicParticelObject_CalculateValency(particle);
		ParticelObject_SetSpeed(particle, 50, 100, 50);
		const std::int32_t pixel[2]{ };
		LogicParticelObject_SetStartPosition(particle, mapPoint, pixel, height);
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active, 1);
	}

	// Original function: FUN_00486585.
	void E2ParticelManager_CreateFallingSpawnParticel(void* manager, const void* source)
	{
		void* particle = E2ParticelManager_GetCleanParticel(manager);

		if (particle == nullptr)
		{

			return;
		}

		const auto id = Read<std::uint32_t>(source, NC2Layout::ParticleOffset::StaticParticleId), spawnId = Read<std::uint32_t>(StaticParticle(id), NC2Layout::StaticParticleOffset::SpawnParticleId);

		if (id == spawnId)
		{
			std::memcpy(particle, source, NC2Layout::RecordSize::Particle);
			Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::ValencyIsDirection, 0);
			Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks, 4);
			Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, 4);
			LogicParticelObject_CalculateValency(particle);
			Write(particle, NC2Layout::ParticleOffset::Valency, Read<std::uint32_t>(source, NC2Layout::ParticleOffset::Valency));
			Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode, SwitchIds::ParticleMotionMode::Stationary);
			Write(particle, NC2Layout::ParticleOffset::FrameIndex, Read<std::uint32_t>(source, NC2Layout::ParticleOffset::FrameIndex));
			Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::FadingSpawn, 1);
		}
		else
		{
			LogicParticelObject_SetParticelId(particle, spawnId);
			Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode, SwitchIds::ParticleMotionMode::Fall);
			ParticelObject_UseWind(particle);
			Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::ValencyMode, SwitchIds::ParticleValencyMode::Fixed);
			Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseMapHeights, 1);
			Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks, 4);
			Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, 4);
			LogicParticelObject_CalculateValency(particle);
			ParticelObject_SetSpeed(particle, 50, 100, 50);
			std::memcpy(At(particle, NC2Layout::ParticleOffset::UseMapHeights), At(source, NC2Layout::ParticleOffset::UseMapHeights), 112);
			Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active, 1);
		}
	}

	// Original function: FUN_0048664c.
	void E2ParticelManager_CreateMunitionParticel(void* manager, const void* sourcePoint, std::uint32_t height, const void* targetPoint, const void* sourcePixel, const void* targetPixel, std::uint32_t munitionType, std::uint32_t duration)
	{
		const auto id = gStaticParticelStorage.munitionIds[munitionType];

		if (id == -1)
			return;

		void* particle = E2ParticelManager_GetCleanParticel(manager);

		if (particle == nullptr)
		{

			return;
		}

		LogicParticelObject_SetParticelId(particle, static_cast<std::uint32_t>(id));
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseMapHeights, 1);
		LogicParticelObject_SetStartPosition(particle, sourcePoint, sourcePixel, height);
		LogicParticelObject_SetTargetPosition(particle, targetPoint, targetPixel, 0);
		Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode, SwitchIds::ParticleMotionMode::Parabolic);
		LogicParticelObject_CalculateMovingVector(particle, duration);

		if (Read<std::uint8_t>(StaticParticle(static_cast<std::uint32_t>(id)), NC2Layout::StaticParticleOffset::ValencyIsDirection) != 0)
		{
			Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::ValencyIsDirection, 1);
			Write(particle, NC2Layout::ParticleOffset::InitialDirectionValency, Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::Valency));
		}
		else
		{
			LogicParticelObject_CalculateValency(particle);
		}

		LogicParticelObject_CalculateParrableHeight(particle);
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active, 1);
	}

	// Original function: FUN_004866e1.
	void E2ParticelManager_StartParticelEffect(void* manager, std::uint32_t effectId, const void* mapPoint, std::uint32_t player)
	{
		E2ParticelManager_l_CreateEffectParticel(manager, effectId, Read<std::uint32_t>(mapPoint, NC2Layout::MapCoordinatePairOffset::Coordinates), 0, player);
	}

	// Original function: FUN_00486705.
	void E2ParticelManager_StartParticelEffectIntoDirection(void* manager, std::uint32_t effectId, const void* sourcePoint, const void* targetPoint, std::uint32_t player)
	{
		const auto id = Read<std::uint32_t>(gStaticParticelEffects, effectId * NC2Layout::RecordSize::ParticleEffectUnsigned);
		const auto maximum = Read<std::uint32_t>(StaticParticle(id), NC2Layout::StaticParticleOffset::MaximumValency);
		const auto angle = static_cast<std::uint32_t>(NC2Logic::VE_HexagonDirection_GetDirectionFromPositions360Degrees(Read<std::int16_t>(sourcePoint, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(sourcePoint, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(targetPoint, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(targetPoint, NC2Layout::MapCoordinatePairOffset::Y)));
		E2ParticelManager_l_CreateEffectParticel(manager, effectId, Read<std::uint32_t>(sourcePoint, NC2Layout::MapCoordinatePairOffset::Coordinates), angle * maximum / 360u, player);
	}

	// Original function: FUN_00486779; the native wind-before-movement assignment is deliberate.
	void E2ParticelManager_l_CreateEffectParticel(void* manager, std::uint32_t effectId, std::uint32_t mapPoint, std::uint32_t valency, std::uint32_t player)
	{
		void* particle = E2ParticelManager_GetCleanParticel(manager);

		if (particle == nullptr)
		{

			return;
		}

		void* effect = At(gStaticParticelEffects, effectId * NC2Layout::RecordSize::ParticleEffectUnsigned);
		LogicParticelObject_SetParticelId(particle, Read<std::uint32_t>(effect, NC2Layout::ParticleEffectOffset::ParticleId));

		if (Read<std::uint8_t>(effect, NC2Layout::ParticleEffectOffset::UseMapHeights) != 0)
			Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseMapHeights, 1);

		if (Read<std::uint8_t>(effect, NC2Layout::ParticleEffectOffset::UseWind) != 0)
			ParticelObject_UseWind(particle);

		Write(particle, NC2Layout::ParticleOffset::MovementMode, Read<std::uint32_t>(effect, NC2Layout::ParticleEffectOffset::MovementMode));
		const auto lifetime = Read<std::uint32_t>(effect, NC2Layout::ParticleEffectOffset::LifetimeTicks), startLife = Read<std::uint32_t>(effect, NC2Layout::ParticleEffectOffset::StartLifetimeTicks);

		if (lifetime != 0)
		{
			Write(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks, lifetime);
			Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, lifetime);
			LogicParticelObject_CalculateValency(particle);
		}

		if (startLife != 0)
		{
			Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, startLife);
			LogicParticelObject_CalculateValency(particle);
		}

		ParticelObject_SetSpeed(particle, Read<std::uint32_t>(effect, NC2Layout::ParticleEffectOffset::SpeedMapX), Read<std::uint32_t>(effect, NC2Layout::ParticleEffectOffset::SpeedHeight), Read<std::uint32_t>(effect, NC2Layout::ParticleEffectOffset::SpeedMapY));
		Write<std::uint16_t>(&mapPoint, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&mapPoint, NC2Layout::MapCoordinatePairOffset::X) + Read<std::uint16_t>(effect, NC2Layout::ParticleEffectOffset::MapOffsetX)));
		Write<std::uint16_t>(&mapPoint, NC2Layout::MapCoordinatePairOffset::Y, static_cast<std::uint16_t>(Read<std::uint16_t>(&mapPoint, NC2Layout::MapCoordinatePairOffset::Y) + Read<std::uint16_t>(effect, NC2Layout::ParticleEffectOffset::MapOffsetY)));
		LogicParticelObject_SetStartPosition(particle, &mapPoint, At(effect, NC2Layout::ParticleEffectOffset::PixelPoint), Read<std::uint32_t>(effect, NC2Layout::ParticleEffectOffset::HeightOffset));
		LogicParticelObject_SetTargetPosition(particle, &mapPoint, At(effect, NC2Layout::ParticleEffectOffset::PixelPoint), Read<std::uint32_t>(effect, NC2Layout::ParticleEffectOffset::HeightOffset));
		Write(particle, NC2Layout::ParticleOffset::Valency, valency);
		Write(particle, NC2Layout::ParticleOffset::PlayerId, player);
		Write(particle, NC2Layout::ParticleOffset::DrawBottom, Read<std::uint8_t>(effect, NC2Layout::ParticleEffectOffset::DrawBottom));
		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active, 1);
	}

	// Original function: FUN_00486bf1; generator capacity is one thousand.
	void E2ParticelManager_AddParticelGenerator(void* manager, std::uint32_t generatorId, const void* mapPoint, const void* pixelPoint, std::uint32_t chance, std::uint32_t identifier)
	{
		std::uint32_t index = 0;

		while (index < 1000 && Read<std::uint8_t>(manager, NC2Layout::ParticleManagerOffset::FirstGeneratorActiveUnsigned + index * NC2Layout::RecordSize::ParticleGeneratorSlotUnsigned) != 0)
			++index;

		if (index == 1000)
		{

			return;
		}

		void* record = At(manager, NC2Layout::ParticleManagerOffset::GeneratorsUnsigned + index * NC2Layout::RecordSize::ParticleGeneratorSlotUnsigned);
		Write(record, NC2Layout::ParticleGeneratorSlotOffset::ChancePercent, chance);
		Write(record, NC2Layout::ParticleGeneratorSlotOffset::PixelPoint, Read<std::uint32_t>(pixelPoint, NC2Layout::PixelPointOffset::X));
		Write(record, NC2Layout::ParticleGeneratorSlotOffset::PixelY, Read<std::uint32_t>(pixelPoint, NC2Layout::PixelPointOffset::Y));
		Write<std::uint8_t>(record, NC2Layout::ParticleGeneratorSlotOffset::Enabled, 0);
		Write(record, NC2Layout::ParticleGeneratorSlotOffset::GeneratorId, generatorId);
		Write(record, NC2Layout::ParticleGeneratorSlotOffset::Identifier, identifier);
		Write(record, NC2Layout::ParticleGeneratorSlotOffset::MapPoint, Read<std::uint32_t>(mapPoint, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write<std::uint8_t>(record, NC2Layout::ParticleGeneratorSlotOffset::Active, 1);

		if (index > Read<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastGeneratorIndex))
			Write(manager, NC2Layout::ParticleManagerOffset::LastGeneratorIndex, index);
	}

	// Original function: FUN_00486c7c; native scan also visits inactive records.
	void E2ParticelManager_EnableGenerator(void* manager, std::uint32_t identifier)
	{
		for (std::uint32_t index = 0; index <= Read<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastGeneratorIndex); ++index)
		{
			void* record = At(manager, NC2Layout::ParticleManagerOffset::GeneratorsUnsigned + index * NC2Layout::RecordSize::ParticleGeneratorSlotUnsigned);

			if (Read<std::uint32_t>(record, NC2Layout::ParticleGeneratorSlotOffset::Identifier) == identifier)
				Write<std::uint8_t>(record, NC2Layout::ParticleGeneratorSlotOffset::Enabled, 1);
		}
	}

	// Original function: FUN_00486ca1.
	void E2ParticelManager_DestroyGenerator(void* manager, std::uint32_t identifier)
	{
		std::uint32_t previous = 0;

		for (std::uint32_t index = 0; index <= Read<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastGeneratorIndex); ++index)
		{
			void* record = At(manager, NC2Layout::ParticleManagerOffset::GeneratorsUnsigned + index * NC2Layout::RecordSize::ParticleGeneratorSlotUnsigned);

			if (Read<std::uint32_t>(record, NC2Layout::ParticleGeneratorSlotOffset::Identifier) == identifier)
			{
				Write<std::uint8_t>(record, NC2Layout::ParticleGeneratorSlotOffset::Active, 0);
				Write<std::uint32_t>(record, NC2Layout::ParticleGeneratorSlotOffset::Identifier, NXBasics::IntegerRange::UInt32Maximum);

				if (index == Read<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastGeneratorIndex))
					Write(manager, NC2Layout::ParticleManagerOffset::LastGeneratorIndex, previous);
			}

			previous = index;
		}
	}

	// Original function: FUN_00486cdd.
	void E2ParticelManager_l_GeneratorTick(void* manager)
	{
		for (std::uint32_t index = 0; index <= Read<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastGeneratorIndex); ++index)
		{
			void* record = At(manager, NC2Layout::ParticleManagerOffset::GeneratorsUnsigned + index * NC2Layout::RecordSize::ParticleGeneratorSlotUnsigned);

			if (Read<std::uint8_t>(record, NC2Layout::ParticleGeneratorSlotOffset::Active) == 0 || Read<std::uint8_t>(record, NC2Layout::ParticleGeneratorSlotOffset::Enabled) == 0)
				continue;

			auto point = Read<std::uint32_t>(record, NC2Layout::ParticleGeneratorSlotOffset::MapPoint);

			if (RandUnsigned() % 100u >= Read<std::uint32_t>(record, NC2Layout::ParticleGeneratorSlotOffset::ChancePercent))
				continue;

			void* generator = At(Read<void*>(gE2ParticelGeneratorManager, NC2Layout::ParticleGeneratorManagerOffset::Entries), Read<std::uint32_t>(record, NC2Layout::ParticleGeneratorSlotOffset::GeneratorId) * NC2Layout::RecordSize::ParticleGeneratorUnsigned);

			if (Read<std::uint8_t>(generator, NC2Layout::ParticleGeneratorOffset::UseRandomArea) != 0)
				ParticelGenerator_OffsetMapPoint(generator, &point);

			E2ParticelGenerator_GenerateParticel(generator, &point, At(record, NC2Layout::ParticleGeneratorSlotOffset::PixelPoint));
		}
	}

	// Original function: FUN_0048741a.
	void E2ParticelGenerator_GenerateParticel(void* generator, const void* mapPoint, const void* pixelPoint)
	{
		void* particle = E2ParticelManager_GetCleanParticel(gE2ParticelManager);

		if (particle == nullptr)
		{

			return;
		}

		LogicParticelObject_SetParticelId(particle, Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorOffset::ParticleId));
		Write(particle, NC2Layout::ParticleOffset::MovementMode, Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorOffset::MovementMode));
		ParticelObject_SetSpeed(particle, Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorOffset::SpeedMapX), Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorOffset::SpeedHeight), Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorOffset::SpeedMapY));

		if (Read<std::uint8_t>(generator, NC2Layout::ParticleGeneratorOffset::UseMapHeights) != 0)
			Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::UseMapHeights, 1);

		if (Read<std::uint8_t>(generator, NC2Layout::ParticleGeneratorOffset::UseWind) != 0)
		{
			ParticelObject_UseWind(particle);
			Write(particle, NC2Layout::ParticleOffset::StoredWindParameter, Read<double>(generator, NC2Layout::ParticleGeneratorOffset::StoredWindParameter));
		}

		const auto lifetime = Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorOffset::LifetimeTicks);
		Write(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks, lifetime);
		Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, lifetime);
		LogicParticelObject_CalculateValency(particle);
		const auto goneLife = Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorOffset::MaximumGoneLifetime);

		if (goneLife != 0)
		{
			Write(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, lifetime - RandUnsigned() % goneLife);
			LogicParticelObject_CalculateValency(particle);
		}

		Write(particle, NC2Layout::ParticleOffset::ValencyMode, Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorOffset::ValencyMode));
		LogicParticelObject_SetStartPosition(particle, mapPoint, pixelPoint, Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorOffset::HeightOffset));

		if (Read<std::uint8_t>(generator, NC2Layout::ParticleGeneratorOffset::KillOnGroundCollision) != 0)
			Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::KillOnGroundCollision, 1);

		Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active, 1);
	}

	// Original function: FUN_0048718c.
	void E2ParticelWind_ChangeWind(void* wind)
	{
		auto angle = Read<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::AngleDegrees) + static_cast<std::uint32_t>(NXBasics::Runtime_Rand() % 91 - 40);

		if (angle > 360)
			angle = 0;

		Write(wind, NC2Layout::ParticleWindOffset::AngleDegrees, angle);
		const double radians = static_cast<double>(angle) * 0.005555555555555556 * 3.1415927410125732;
		Write(wind, NC2Layout::ParticleWindOffset::Cosine, std::cos(radians));
		Write(wind, NC2Layout::ParticleWindOffset::Sine, std::sin(radians));
		Write(wind, NC2Layout::ParticleWindOffset::HorizontalSpeed, RandUnsigned() % 5u);
		Write(wind, NC2Layout::ParticleWindOffset::VerticalSpeed, RandUnsigned() % 5u);

		if (Read<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::HorizontalSpeed) >= 5)
			Write<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::HorizontalSpeed, 4);

		if (Read<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::VerticalSpeed) >= 5)
			Write<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::VerticalSpeed, 4);

		Write(wind, NC2Layout::ParticleWindOffset::HorizontalScale, (static_cast<double>(Read<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::HorizontalSpeed)) * 0.01) * (1.0 / NC2Logic::GameMap_GetWidth()));
		Write(wind, NC2Layout::ParticleWindOffset::VerticalScale, (static_cast<double>(Read<std::uint32_t>(wind, NC2Layout::ParticleWindOffset::VerticalSpeed)) * 0.01) * (1.0 / NC2Logic::GameMap_GetHeight()));
		Write(wind, NC2Layout::ParticleWindOffset::DeltaMapX, Read<double>(wind, NC2Layout::ParticleWindOffset::HorizontalScale) * Read<double>(wind, NC2Layout::ParticleWindOffset::Cosine));
		Write(wind, NC2Layout::ParticleWindOffset::DeltaMapY, Read<double>(wind, NC2Layout::ParticleWindOffset::VerticalScale) * Read<double>(wind, NC2Layout::ParticleWindOffset::Sine));
	}

	// Original function: FUN_00486d61.
	void __fastcall E2ParticelManager_DoOneGameTick(void* manager, void* /*unusedEdx*/)
	{
		E2ParticelManager_l_GeneratorTick(manager);
		ParticelWind_Tick(Read<void*>(manager, NC2Layout::ParticleManagerOffset::Wind));
		std::uint32_t previous = 0;

		for (std::uint32_t index = 0; index <= Read<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastParticleIndex); ++index)
		{
			void* particle = At(manager, NC2Layout::ParticleManagerOffset::ParticlesUnsigned + index * NC2Layout::RecordSize::ParticleUnsigned);

			if (Read<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active) == 0)
				continue;

			ParticelObject_TickLifetimeAndFrame(particle);

			if (Read<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active) == 0)
			{
				if (Read<std::uint8_t>(particle, NC2Layout::ParticleOffset::FadingSpawn) == 0 && Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::StaticParticleId) == static_cast<std::uint32_t>(gStaticParticelStorage.munitionIds[1]))
				{
					// An expired projectile may have crossed the map boundary on its last movement tick.
					const void* micro = NC2Logic::GameMap_GetMapMicroPointPtr(At(particle, NC2Layout::ParticleOffset::MapPoint));

					if (micro != nullptr && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 && Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId) == NXBasics::IntegerRange::UInt16Maximum && !(Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0 && Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) != -1))
					{
						// EBP is particle + 0x90 in Windows 00486E03..00486E15.
						Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::MovementMode, SwitchIds::ParticleMotionMode::Stationary);
						Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::FadingSpawn, 0);
						Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::PendingMovementTicks, 0);
						Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::ValencyIsDirection, 0);
						Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::StaticParticleId, 10);
						std::memcpy(At(particle, NC2Layout::ParticleOffset::Position), At(particle, NC2Layout::ParticleOffset::TargetPosition), NC2Layout::RecordSize::ParticlePosition);
						Write<std::uint8_t>(particle, NC2Layout::ParticleOffset::Active, 1);
						Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks, 120);
						Write<std::uint32_t>(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks, 119);
					}
				}
				else if (index == Read<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastParticleIndex))
				{
					Write(manager, NC2Layout::ParticleManagerOffset::LastParticleIndex, previous);
				}
			}
			else if (Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::RemainingLifetimeTicks) + 1u < Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::TotalLifetimeTicks) && Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::ValencyMode) == SwitchIds::ParticleValencyMode::RemainingLifetime && Read<std::uint8_t>(StaticParticle(Read<std::uint32_t>(particle, NC2Layout::ParticleOffset::StaticParticleId)), NC2Layout::StaticParticleOffset::HasSpawnParticle) != 0 && Read<std::uint8_t>(particle, NC2Layout::ParticleOffset::FadingSpawn) == 0)
			{
				E2ParticelManager_CreateFallingSpawnParticel(manager, particle);
			}

			previous = index;
		}
	}
}
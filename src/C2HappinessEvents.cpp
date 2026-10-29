#include "C2GameState.h"
#include "C2HappinessEvents.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Complete Windows event-value table 00500B68..00500B7B; the next region starts with a string.
	static constexpr std::int32_t happinessEventValues[5] = { 0, -10, -25, -10, -20 };

	// Original function: FUN_004195d3; a full list only replaces the oldest matching event type.
	void Happiness_AddEventType(void* happiness, std::uint32_t type)
	{
		Happiness_UpdateEvents(happiness);
		std::int32_t oldest = -1;
		std::uint32_t expiration = NXBasics::IntegerRange::UInt32Maximum;
		std::uint32_t index = 0;

		for (; index < 10; ++index)
		{
			const auto offset = NC2Layout::HappinessPlayerOffset::EventsUnsigned + index * NC2Layout::RecordSize::HappinessEventUnsigned;
			const auto currentType = Read<std::uint32_t>(happiness, offset);

			if (currentType == 0)
				break;

			if (currentType == type && Read<std::uint32_t>(happiness, offset + NC2Layout::HappinessEventOffset::ExpiresAtTickUnsigned) < expiration)
			{
				oldest = static_cast<std::int32_t>(index);
				expiration = Read<std::uint32_t>(happiness, offset + NC2Layout::HappinessEventOffset::ExpiresAtTickUnsigned);
			}
		}

		if (index == 10)
		{
			if (oldest == -1)
			{

				return;
			}

			index = static_cast<std::uint32_t>(oldest);
		}

		const auto offset = NC2Layout::HappinessPlayerOffset::EventsUnsigned + index * NC2Layout::RecordSize::HappinessEventUnsigned;
		Write(happiness, offset, type);
		Write(happiness, offset + NC2Layout::HappinessEventOffset::ExpiresAtTickUnsigned, gGameTime.tick + 720u);
		Write(happiness, offset + NC2Layout::HappinessEventOffset::InitialValueUnsigned, happinessEventValues[type]);
	}

	// Original function: FUN_00419648; the native signed sum uses 32-bit wrap.
	std::int32_t Happiness_GetEventValue(const void* happiness)
	{
		std::uint32_t value = 0;

		for (std::uint32_t index = 0; index < 10; ++index)
		{
			const auto offset = NC2Layout::HappinessPlayerOffset::EventsUnsigned + index * NC2Layout::RecordSize::HappinessEventUnsigned;

			if (Read<std::uint32_t>(happiness, offset) != 0)
				value += static_cast<std::uint32_t>(Happiness_Events_GetValue(static_cast<const std::byte*>(happiness) + offset));
		}

		return NXBasics::Runtime_TimeSigned32(value);
	}

	// Original function: FUN_0041966d; expired zero-valued events only clear their type field.
	void Happiness_UpdateEvents(void* happiness)
	{
		for (std::uint32_t index = 0; index < 10; ++index)
		{
			const auto offset = NC2Layout::HappinessPlayerOffset::EventsUnsigned + index * NC2Layout::RecordSize::HappinessEventUnsigned;

			if (Read<std::uint32_t>(happiness, offset) != 0 && Happiness_Events_GetValue(static_cast<const std::byte*>(happiness) + offset) == 0)
				Write<std::uint32_t>(happiness, offset, 0);
		}
	}

	// Original function: FUN_00419690; values decay one step per 360 ticks after their initial expiration.
	std::int32_t Happiness_Events_GetValue(const void* event)
	{
		const auto tick = gGameTime.tick;
		const auto expiration = Read<std::uint32_t>(event, NC2Layout::HappinessEventOffset::ExpiresAtTick);
		const auto value = Read<std::int32_t>(event, NC2Layout::HappinessEventOffset::InitialValue);

		if (tick <= expiration)
			return value;

		const auto decay = (tick - expiration) / 360u;

		if (value > 0)
		{
			if (NXBasics::Runtime_TimeSigned32(decay) > value)
				return 0;

			return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(value) - decay);
		}

		if (value < 0)
		{
			if (NXBasics::Runtime_TimeSigned32(decay) > NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(value)))
				return 0;

			return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(value) + decay);
		}

		return 0;
	}

	// Original function: FUN_00419aed; neutral player IDs collapse before both native cooldown checks.
	void HappinessManager_Messages_PlayerAttackedByPlayer(void* manager, std::uint32_t player, std::uint32_t attacker)
	{
		if (player == 20)
			player = attacker;

		if (attacker == 20)
			attacker = player;

		const auto playerOffset = player * NC2Layout::RecordSize::HappinessPlayerUnsigned;

		if (Read<std::uint32_t>(manager, playerOffset + NC2Layout::HappinessPlayerOffset::AttackExpiresAtTickUnsigned) <= gGameTime.tick)
		{
			Write<std::uint8_t>(manager, playerOffset + NC2Layout::HappinessPlayerOffset::UnderAttackUnsigned, 1);
			Write(manager, playerOffset + NC2Layout::HappinessPlayerOffset::AttackerPlayerIdUnsigned, attacker);
			Write(manager, playerOffset + NC2Layout::HappinessPlayerOffset::AttackExpiresAtTickUnsigned, gGameTime.tick + 720u);
		}

		const auto attackerOffset = attacker * NC2Layout::RecordSize::HappinessPlayerUnsigned;

		if (Read<std::uint32_t>(manager, attackerOffset + NC2Layout::HappinessPlayerOffset::AttackExpiresAtTickUnsigned) <= gGameTime.tick)
		{
			Write<std::uint8_t>(manager, attackerOffset + NC2Layout::HappinessPlayerOffset::UnderAttackUnsigned, 1);
			Write(manager, attackerOffset + NC2Layout::HappinessPlayerOffset::AttackerPlayerIdUnsigned, player);
			Write(manager, attackerOffset + NC2Layout::HappinessPlayerOffset::AttackExpiresAtTickUnsigned, gGameTime.tick + 720u);
		}
	}

	// Original function: FUN_00419b74.
	void HappinessManager_Messages_CivilistDied(void* manager, const void* human)
	{
		Happiness_AddEventType(static_cast<std::byte*>(manager) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::HappinessPlayerUnsigned, 2);
	}

	// Original function: FUN_00419b8f.
	void HappinessManager_Messages_HouseAttacked(void* manager, const void* house)
	{
		Happiness_AddEventType(static_cast<std::byte*>(manager) + Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) * NC2Layout::RecordSize::HappinessPlayerUnsigned, 3);
	}

	// Original function: FUN_00419baa.
	void HappinessManager_Messages_HouseDestroyedTroughAttack(void* manager, const void* house)
	{
		Happiness_AddEventType(static_cast<std::byte*>(manager) + Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) * NC2Layout::RecordSize::HappinessPlayerUnsigned, 4);
	}
}
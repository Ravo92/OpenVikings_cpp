#include "NetworkSynchronization.h"
#include "C2GameTime.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2UserNetworkCommandAccess.h"
#include "NetworkShutdown.h"
#include "SwitchIdentifiers.h"
#include <algorithm>

namespace NNetwork::Synchronization
{
	using namespace NC2Lifetime;
	using namespace NC2Logic;

	namespace
	{
		struct SAutomaticSynchronization
		{
			std::uint32_t lastPoll = 0, budget = 0, pendingFrequency = 0, candidateFrequency = 0, candidateSamples = 0;
		};

		SAutomaticSynchronization state;
	}

	void Reset(std::uint32_t now)
	{
		state = { };
		state.lastPoll = now;
	}

	std::uint32_t GetBudget()
	{

		return state.budget;
	}

	void Update(std::uint32_t now)
	{
		gGameTime.autoAdjustNetworkFrequency = 1;

		if (!NNetwork::Network_IsHost() || now - state.lastPoll < 1000u)
			return;

		state.lastPoll = now;

		if (state.pendingFrequency != 0)
		{
			if (gGameTime.networkFrequency != state.pendingFrequency)
				return;

			state.pendingFrequency = 0;
		}

		std::uint32_t first = 0, second = 0, highestLatency = 0;
		const auto* players = static_cast<const NNetwork::SPlayerInfo*>(gDynamicNetworkData.playerInfoRecords);
		auto* peer = GetNetworkInstance()->peer;
		bool missing = false;

		for (std::uint32_t index = 0; index < gDynamicNetworkData.playerCount; ++index)
		{
			if (players[index].playerId == 0 || players[index].isLocal)
				continue;

			NNetwork::ModernConnectionQuality quality;

			if (peer == nullptr || !peer->ConnectionQuality(players[index].playerId, quality) || !quality.measured || quality.age >= 3000u)
			{
				missing = true;
				continue;
			}

			highestLatency = (std::max)(highestLatency, quality.latency);
			std::uint32_t peak = quality.latency;

			for (auto sample = quality.count > 8 ? quality.count - 8 : 0; sample < quality.count; ++sample)
			{
				if (quality.samples[sample] != UINT32_MAX)
					peak = (std::max)(peak, quality.samples[sample]);
			}

			const auto budget = peak + quality.jitter * 2u;

			if (budget >= first)
			{
				second = first;
				first = budget;
			}
			else
			{
				second = (std::max)(second, budget);
			}
		}
		// Client-to-client commands travel through the host: reserve both slowest legs.
		state.budget = missing ? 1000u : first == 0 && second == 0 ? 0u
		                                                           : first + second + 20u;
		const auto sampleIndex = gGameTime.latencySampleIndex % 10u;
		gGameTime.latencySamples[sampleIndex] = highestLatency;
		gGameTime.latencySampleIndex = (sampleIndex + 1u) % 10u;
		gGameTime.latencySampleTime = now;
		auto frequency = GameTime_Network_LatencyToFrequency(state.budget);

		if (gGameTime.speed >= 0.01 && gGameTime.speed < static_cast<double>(frequency))
			frequency = 1;

		if (frequency != state.candidateFrequency)
		{
			state.candidateFrequency = frequency;
			state.candidateSamples = 1;
		}
		else if (state.candidateSamples < 3)
		{
			++state.candidateSamples;
		}
		// Slow down promptly; require three stable observations before speeding up.

		if (frequency == gGameTime.networkFrequency || (frequency > gGameTime.networkFrequency && state.candidateSamples < 3))
		{

			return;
		}

		std::byte command[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
		command[0] = std::byte{ SwitchIds::UserCommand::SetNetworkFrequency };
		Write(command, NC2Layout::NetworkFrequencyCommandOffset::Frequency, frequency);
		auto* queue = reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(gUserNetworkCommandManager) + NC2Layout::UserNetworkCommandManagerOffset::SystemQueue);
		const auto queued = queue->GetCount();
		UserNetworkCommandManager_AddCommand(gUserNetworkCommandManager, nullptr, command, true, false);

		if (queue->GetCount() > queued)
			state.pendingFrequency = frequency;
	}

}
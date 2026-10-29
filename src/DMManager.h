#pragma once

#include <cstddef>
#include <cstdint>
#include "DirectMusicDriver.h"
#include "XCrtFileAccess.h"

// Original global: DAT_0050F698; the runtime manager owns no serialized game state.
inline void* gDirectMusicManager = nullptr;

namespace NDMManager
{
	// Runtime-only state: save files contain event IDs, never driver pointers or COM objects.
	struct SSegmentEntry
	{
		std::uint8_t active = 0;
		NDirectMusicDriver::Gedx8LoadedObject* object = nullptr;
		std::int32_t objectKind = 0;
		const char* fileName = nullptr;
		std::int32_t eventId = 0;
		std::uint32_t eventParameter = 0;
	};

	struct SManagerStorage
	{
		const std::uintptr_t* virtualTable = nullptr;
		std::uint8_t initialized = 0;
		std::uint8_t ducking = 0;
		std::uint32_t recoveryTick = 0;
		std::int32_t lastMissionEvent = 0;
		std::int32_t quality = 0;
		std::int32_t currentEvent = 0;
		std::uint32_t currentVariant = 0;
		std::int32_t volume = 70;
		NDirectMusicDriver::Gedx8DriverInstance* driver = nullptr;
		std::uint32_t voiceCount = 0;
		NDirectMusicDriver::Gedx8Audiopath* audioPath = nullptr;
		SSegmentEntry segments[20]{ };
		NXBasics::RuntimeBuffer<char> directory;
		NXBasics::RuntimeBuffer<char> error;
	};

	static_assert(sizeof(SSegmentEntry) == (sizeof(void*) == 8 ? 40 : 24));
	inline constexpr std::size_t ManagerSize = sizeof(SManagerStorage);
	void* Construct(void* manager);
	void Destruct(void* manager);
	// Remaster: game-thread initialization/command results mean request accepted;
	// driver failures are reported asynchronously to the debugger output.
	std::uint8_t Init(void* manager, std::int32_t mode, const char* directory);
	std::uint8_t Exit(void* manager);
	std::uint8_t Update(void* manager);
	// Remaster: includes queued playback requests while the music thread starts.
	bool HasPendingOrActiveEvent(const void* manager);
	std::int32_t MainVolume_Get(const void* manager);
	std::uint8_t MainVolume_Set(void* manager, std::int32_t volume);
	std::uint8_t ProcessEvent(void* manager, std::int32_t eventId, std::uint32_t eventParameter, std::int32_t mode);
	void ResetAllSegments(void* manager);
	std::uint8_t PrepareEvent(void* manager, std::int32_t eventId, std::uint32_t eventParameter);
	std::uint8_t StartEvent(void* manager, std::int32_t eventId, std::uint32_t eventParameter);
	const char* GetEventFileName(std::int32_t eventId, std::uint32_t variant);
}
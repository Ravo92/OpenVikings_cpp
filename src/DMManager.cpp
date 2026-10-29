#include "DMManager.h"
#include "XCrtFileAccess.h"
#include <cmath>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <new>
#include <process.h>
#include <Windows.h>

namespace NDMManager
{

	// Shared strings preserve identical pointers in the original tables.
	static constexpr char kMissionMidgard1[] = "Mission_Midgard1_Standard.sgt";
	static constexpr char kMissionMidgard2[] = "Mission_Midgard2_Standard.sgt";
	static constexpr char kMissionAddonFranken1[] = "Mission_AddOn_Franken1_Standard.sgt";
	static constexpr char kMissionAddonNordland[] = "Mission_AddOn_Nordland_Standard.sgt";
	static constexpr char kMissionAddonUnderworld[] = "Mission_AddOn_Underworld_Standard.sgt";
	static constexpr char kMissionAddonAsgard[] = "Mission_AddOn_Asgard_Standard.sgt";

	// Original data: 004fe4a8–004fe4d7.
	// Events 2–5; variants 1–3: Friendly, Neutral, Hostile.
	static constexpr const char* kThemeFileNames[] = {
		// Event 2
		"Theme_Viking_Friendly.sgt",
		"Theme_Viking_Neutral.sgt",
		"Theme_Viking_Hostile.sgt",

		// Event 3
		"Theme_Franken_Friendly.sgt",
		"Theme_Franken_Neutral.sgt",
		"Theme_Franken_Hostile.sgt",

		// Event 4
		"Theme_Byzanz_Friendly.sgt",
		"Theme_Byzanz_Neutral.sgt",
		"Theme_Byzanz_Hostile.sgt",

		// Event 5
		"Theme_Arabs_Friendly.sgt",
		"Theme_Arabs_Neutral.sgt",
		"Theme_Arabs_Hostile.sgt"
	};

	// Original data: 004fe4d8–004fe4e7.
	// Events 6–9; the variant is ignored.
	static constexpr const char* kAttackFileNames[] = {
		"Attack_Viking.sgt",
		"Attack_Franken.sgt",
		"Attack_Byzanz.sgt",
		"Attack_Arabs.sgt"
	};

	// Original data: 004fe4e8–004fe577.
	// Events 10–21; variants 4–6: Standard, Wealthy, Danger.
	static constexpr const char* kMissionFileNames[] = {
		// Event 10
		"Mission_Viking1_Standard.sgt",
		"Mission_Viking1_Wealthy.sgt",
		"Mission_Viking1_Danger.sgt",

		// Event 11
		"Mission_Franken1_Standard.sgt",
		"Mission_Franken1_Wealthy.sgt",
		"Mission_Franken1_Danger.sgt",

		// Event 12
		"Mission_Franken2_Standard.sgt",
		"Mission_Franken2_Wealthy.sgt",
		"Mission_Franken2_Danger.sgt",

		// Event 13
		"Mission_Byzanz1_Standard.sgt",
		"Mission_Byzanz1_Wealthy.sgt",
		"Mission_Byzanz1_Danger.sgt",

		// Event 14
		"Mission_Byzanz2_Standard.sgt",
		"Mission_Byzanz2_Wealthy.sgt",
		"Mission_Byzanz2_Danger.sgt",

		// Event 15
		"Mission_Byzanz3_Standard.sgt",
		"Mission_Byzanz3_Wealthy.sgt",
		"Mission_Byzanz3_Danger.sgt",

		// Event 16
		"Mission_Byzanz4_Standard.sgt",
		"Mission_Byzanz4_Wealthy.sgt",
		"Mission_Byzanz4_Danger.sgt",

		// Event 17
		"Mission_Arabs1_Standard.sgt",
		"Mission_Arabs1_Wealthy.sgt",
		"Mission_Arabs1_Danger.sgt",

		// Event 18
		"Mission_Arabs2_Standard.sgt",
		"Mission_Arabs2_Wealthy.sgt",
		"Mission_Arabs2_Danger.sgt",

		// Event 19
		"Mission_Arabs3_Standard.sgt",
		"Mission_Arabs3_Wealthy.sgt",
		"Mission_Arabs3_Danger.sgt",

		// Event 20
		kMissionMidgard1,
		kMissionMidgard1,
		kMissionMidgard1,

		// Event 21
		kMissionMidgard2,
		kMissionMidgard2,
		kMissionMidgard2
	};

	// Original data: 004fe578–004fe5d7.
	// Events 31–38; variants 4–6, including the original shared entries.
	static constexpr const char* kAddonMissionFileNames[] = {
		// Event 31
		"Mission_AddOn_Arabs1_Standard.sgt",
		"Mission_AddOn_Arabs1_Wealthy.sgt",
		"Mission_AddOn_Arabs1_Danger.sgt",

		// Event 32
		"Mission_AddOn_Arabs2_Standard.sgt",
		"Mission_AddOn_Arabs2_Wealthy.sgt",
		"Mission_AddOn_Arabs2_Danger.sgt",

		// Event 33
		kMissionAddonFranken1,
		kMissionAddonFranken1,
		kMissionAddonFranken1,

		// Event 34
		"Mission_AddOn_Franken2_Standard.sgt",
		"Mission_AddOn_Franken2_Wealthy.sgt",
		"Mission_AddOn_Franken2_Danger.sgt",

		// Event 35
		"Mission_AddOn_Franken3_Standard.sgt",
		"Mission_AddOn_Franken3_Wealthy.sgt",
		"Mission_AddOn_Franken3_Danger.sgt",

		// Event 36
		kMissionAddonNordland,
		kMissionAddonNordland,
		kMissionAddonNordland,

		// Event 37
		kMissionAddonUnderworld,
		kMissionAddonUnderworld,
		kMissionAddonUnderworld,

		// Event 38
		kMissionAddonAsgard,
		kMissionAddonAsgard,
		kMissionAddonAsgard
	};

	static_assert(sizeof(kThemeFileNames) / sizeof(kThemeFileNames[0]) == 12);
	static_assert(sizeof(kAttackFileNames) / sizeof(kAttackFileNames[0]) == 4);
	static_assert(sizeof(kMissionFileNames) / sizeof(kMissionFileNames[0]) == 36);
	static_assert(sizeof(kAddonMissionFileNames) / sizeof(kAddonMissionFileNames[0]) == 24);

	// Original data: 004fe484–004fe4a7; volume-recovery delays for events 22–30.
	static constexpr std::uint32_t kVolumeRecoveryDelayMs[9] = {
		2800, // Event 22
		3700, // Event 23
		5000, // Event 24
		3200, // Event 25
		3300, // Event 26
		8200, // Event 27
		8000, // Event 28
		2800, // Event 29
		3000  // Event 30
	};

	static_assert(sizeof(kVolumeRecoveryDelayMs) == 36);

	static SManagerStorage& State(void* manager)
	{

		return *static_cast<SManagerStorage*>(manager);
	}

	static const SManagerStorage& State(const void* manager)
	{

		return *static_cast<const SManagerStorage*>(manager);
	}

	static SSegmentEntry* FindSegmentEntry(void* manager, std::int32_t eventId, std::uint32_t eventParameter);

	static char* DuplicateString(const char* source)
	{
		// FUN_00410b59 uses FUN_004e5ddd, including its original new-handler mode.

		if (source == nullptr)
			return nullptr;

		const std::size_t bytes = std::strlen(source) + 1;
		auto* copy = static_cast<char*>(NXBasics::Runtime_AllocateWithNewHandler(bytes, 1));

		if (copy == nullptr)
			return nullptr;

		std::memcpy(copy, source, bytes);

		return copy;
	}

	static void SetErrorMessage(void* manager, const char* message)
	{
		State(manager).error.reset(DuplicateString(message));
	}

	// Native runtime state owns its strings and uses process-width pointers. Driver state belongs
	// exclusively to this thread; the game only submits commands and reads volume.
	enum class MusicCommandType
	{
		Process,
		Prepare,
		Start,
		Reset
	};

	struct MusicCommand
	{
		MusicCommandType type{ };
		std::int32_t eventId = 0;
		std::uint32_t parameter = 0;
		std::int32_t mode = 0;
	};

	struct RemasterMusicWorker
	{
		void* manager = nullptr;
		char* directory = nullptr;
		std::int32_t mode = 0;
		HANDLE thread = nullptr;
		HANDLE wake = nullptr;
		std::mutex mutex;
		MusicCommand commands[128]{ };
		std::size_t first = 0;
		std::size_t count = 0;
		bool stopping = false;
		bool accepting = true;
		bool musicRequested = false;
		bool overflowReported = false;
		std::atomic<std::int32_t> volume{ 70 };
	};

	// The game already owns one DirectMusic singleton. Only the game thread
	// publishes/retires its sidecar, and retirement joins before freeing anything.
	static RemasterMusicWorker* gMusicWorker = nullptr;
	static thread_local bool gOnMusicThread = false;

	static void ReportMusicError(void* manager)
	{
		const auto* message = State(manager).error.get();

		if (message == nullptr)
		{

			return;
		}

		if (*message != '\0')
		{
			OutputDebugStringA("DMManager reports error: ");
			OutputDebugStringA(message);
			OutputDebugStringA("\r\n");
		}

		State(manager).error.reset();
	}

	static std::uint8_t EnqueueMusicCommand(void* manager, MusicCommand command)
	{
		auto* worker = gMusicWorker;

		if (worker == nullptr || worker->manager != manager)
			return 0;

		const std::lock_guard<std::mutex> lock(worker->mutex);

		if (!worker->accepting || worker->stopping)
			return 0;
		// Repeated world-music updates must not accumulate during a slow driver init.

		if (worker->count != 0)
		{
			const auto& previous = worker->commands[(worker->first + worker->count - 1) % 128];

			if (previous.type == command.type && previous.eventId == command.eventId && previous.parameter == command.parameter && previous.mode == command.mode)
				return 1;
		}

		if (worker->count == 128)
		{
			if (!worker->overflowReported)
				OutputDebugStringA("DMManager: music command queue full; request rejected.\r\n");

			worker->overflowReported = true;
			return 0;
		}

		worker->commands[(worker->first + worker->count) % 128] = command;
		++worker->count;

		if (command.type == MusicCommandType::Reset || (command.type == MusicCommandType::Process && command.mode == 1))
		{
			worker->musicRequested = false;
		}
		else if ((command.type == MusicCommandType::Start || (command.type == MusicCommandType::Process && command.mode == 2)) && command.eventId > 0 && command.eventId < 39 && !(command.eventId >= 22 && command.eventId < 31) && worker->volume.load() > 0)
		{
			worker->musicRequested = true;
		}

		SetEvent(worker->wake);

		return 1;
	}

	static unsigned __stdcall RemasterMusicThread(void* context)
	{
		auto* worker = static_cast<RemasterMusicWorker*>(context);
		void* manager = worker->manager;
		gOnMusicThread = true;
		const DWORD started = timeGetTime();
		// DirectMusic uses STA. Keep creation, playback,
		// message dispatch and destruction in one apartment, with no COM handoff.
		const HRESULT apartment = CoInitialize(nullptr);
		const bool ready = SUCCEEDED(apartment) && Init(manager, worker->mode, worker->directory) != 0;

		if (FAILED(apartment))
			SetErrorMessage(manager, "DMManager: music thread COM initialization failed.");

		char timing[160];
		std::snprintf(timing, sizeof(timing), "DMManager: asynchronous initialization %s after %lu ms.\r\n", ready ? "ready" : "failed", static_cast<unsigned long>(timeGetTime() - started));
		OutputDebugStringA(timing);
		ReportMusicError(manager);

		if (ready)
			MainVolume_Set(manager, worker->volume.load());

		for (; ready;)
		{
			MSG message;

			while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				if (message.message == WM_QUIT)
				{
					const std::lock_guard<std::mutex> lock(worker->mutex);
					worker->stopping = true;
					break;
				}

				TranslateMessage(&message);
				DispatchMessageW(&message);
			}

			MusicCommand command;
			bool haveCommand = false;
			{
				const std::lock_guard<std::mutex> lock(worker->mutex);

				if (worker->stopping)
					break;

				if (worker->count != 0)
				{
					command = worker->commands[worker->first];
					worker->first = (worker->first + 1) % 128;
					--worker->count;
					haveCommand = true;
				}
			}
			const auto wantedVolume = worker->volume.load();

			if (State(manager).volume != wantedVolume)
				MainVolume_Set(manager, wantedVolume);

			if (haveCommand)
			{
				switch (command.type)
				{
					case MusicCommandType::Process:
						ProcessEvent(manager, command.eventId, command.parameter, command.mode);
						break;
					case MusicCommandType::Prepare:
						PrepareEvent(manager, command.eventId, command.parameter);
						break;
					case MusicCommandType::Start:
						StartEvent(manager, command.eventId, command.parameter);
						break;
					case MusicCommandType::Reset:
						ResetAllSegments(manager);
						break;
				}
			}

			Update(manager);
			ReportMusicError(manager);

			if (!haveCommand && MsgWaitForMultipleObjectsEx(1, &worker->wake, 20, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_FAILED)
			{
				SetErrorMessage(manager, "DMManager: music thread wait failed.");
				break;
			}
		}

		{
			const std::lock_guard<std::mutex> lock(worker->mutex);
			worker->accepting = false;
			worker->musicRequested = false;
			worker->count = 0;
		}
		Exit(manager);
		ReportMusicError(manager);

		if (SUCCEEDED(apartment))
			CoUninitialize();

		return 0;
	}

	static std::uint8_t StartMusicWorker(void* manager, std::int32_t mode, const char* directory)
	{
		if (manager == nullptr)
			return 0;

		if (gMusicWorker != nullptr)
			return gMusicWorker->manager == manager ? 1 : 0;

		auto* worker = new (std::nothrow) RemasterMusicWorker;

		if (worker == nullptr)
		{
			SetErrorMessage(manager, "DMManager: music worker allocation failed.");
			return 0;
		}

		worker->manager = manager;
		worker->mode = mode;
		worker->volume.store(State(manager).volume);
		worker->directory = DuplicateString(directory);
		worker->wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);

		if ((directory == nullptr || worker->directory != nullptr) && worker->wake != nullptr)
		{
			gMusicWorker = worker;
			worker->thread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, RemasterMusicThread, worker, 0, nullptr));

			if (worker->thread != nullptr)
				return 1;

			gMusicWorker = nullptr;
		}

		if (worker->wake != nullptr)
			CloseHandle(worker->wake);

		NXBasics::Runtime_Free(worker->directory);
		delete worker;
		SetErrorMessage(manager, "DMManager: could not start the music thread.");

		return 0;
	}

	static std::uint8_t StopMusicWorker(void* manager)
	{
		auto* worker = gMusicWorker;

		if (worker == nullptr || worker->manager != manager)
			return 0;

		{
			const std::lock_guard<std::mutex> lock(worker->mutex);
			worker->stopping = true;
			worker->accepting = false;
			worker->musicRequested = false;
			worker->count = 0;
		}
		SetEvent(worker->wake);
		// Shutdown joins before releasing driver state or its manager.
		DWORD signaled = 0;

		if (FAILED(CoWaitForMultipleHandles(0, INFINITE, 1, &worker->thread, &signaled)))
			WaitForSingleObject(worker->thread, INFINITE);

		State(manager).volume = worker->volume.load();
		CloseHandle(worker->thread);
		CloseHandle(worker->wake);
		NXBasics::Runtime_Free(worker->directory);
		gMusicWorker = nullptr;
		delete worker;

		return 1;
	}

	bool HasPendingOrActiveEvent(const void* manager)
	{
		if (manager == nullptr)
			return false;

		if (gMusicWorker != nullptr && gMusicWorker->manager == manager)
		{
			const std::lock_guard<std::mutex> lock(gMusicWorker->mutex);
			return gMusicWorker->musicRequested;
		}

		return State(manager).currentEvent != 0;
	}

	// Original deleting destructor: FUN_00403561; native slot at 004F3330.
#if defined(_WIN64)
	static void* DeletingDestructor(void* manager, std::uint32_t flags)
#else
	static void* __fastcall DeletingDestructor(void* manager, void*, std::uint32_t flags)
#endif
	{
		Destruct(manager);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(manager);

		return manager;
	}
	static const std::uintptr_t gManagerVirtualTable[] = { reinterpret_cast<std::uintptr_t>(&DeletingDestructor) };

	// Original function: FUN_00403524.
	void* Construct(void* manager)
	{
		auto* state = ::new (manager) SManagerStorage{ };
		state->virtualTable = gManagerVirtualTable;
		gDirectMusicManager = manager;

		return state;
	}

	// Original function: FUN_0040357d; remaster Init unwinds partial initialization.
	void Destruct(void* manager)
	{
		State(manager).virtualTable = gManagerVirtualTable;
		Exit(manager);
		State(manager).~SManagerStorage();
		gDirectMusicManager = nullptr;
	}

	// Original function: FUN_004035b6; remaster releases each acquired driver resource on failure.
	std::uint8_t Init(void* manager, std::int32_t mode, const char* directory)
	{
		if (!gOnMusicThread)
			return StartMusicWorker(manager, mode, directory);

		auto& state = State(manager);
		NXBasics::RuntimeBuffer<char> replacement(DuplicateString(directory));

		if (directory != nullptr && replacement == nullptr)
			return 0;

		state.directory = std::move(replacement);
		state.quality = mode;

		if (state.initialized != 0)
			return state.initialized;

		const auto fail = [&state, manager](const char* message) -> std::uint8_t
		{
			SetErrorMessage(manager, message);

			if (state.driver != nullptr)
				NDirectMusicDriver::geDestroyInstance(state.driver);

			NDirectMusicDriver::geGlobalExit();
			state.driver = nullptr;
			state.audioPath = nullptr;
			state.voiceCount = 0;
			state.initialized = state.ducking = 0;

			return 0;
		};
		NDirectMusicDriver::geGlobalInit();
		state.driver = NDirectMusicDriver::geCreateInstance();

		if (state.driver == nullptr)
			return fail("DMManager: geCreateInstance failed.");

		const NDirectMusicDriver::Gedx8SynthInitConfig configuration{ nullptr, mode == 1 ? 16u : mode == 2 ? 8u
			                                                                                               : 64u,
			                                                          mode == 1 ? 22050u : mode == 2 ? 11025u
			                                                                                         : 44100u };

		if (NDirectMusicDriver::geInitSynthesizer(state.driver, &configuration) == 0)
			return fail("DMManager: geInitSynthesizer failed.");

		state.voiceCount = configuration.voiceCount;
		state.currentEvent = 0;
		state.currentVariant = 0;
		state.initialized = 1;

		return 1;
	}

	// Original function: FUN_0040372f; error messages are replaced without stopping cleanup.
	std::uint8_t Exit(void* manager)
	{
		if (!gOnMusicThread)
			return StopMusicWorker(manager);

		auto& state = State(manager);

		if (state.initialized == 0)
			return 0;

		ResetAllSegments(manager);
		state.initialized = 0;

		for (auto& entry : state.segments)
		{
			if (entry.active != 0 && NDirectMusicDriver::geDestroySegment(state.driver, entry.object) == 0)
				SetErrorMessage(manager, "DMManager: geDestroySegment failed.");

			entry = { };
		}

		if (state.audioPath != nullptr && NDirectMusicDriver::geActivateAudiopath(state.driver, state.audioPath, 0) == 0)
			SetErrorMessage(manager, "DMManager: geActivateAudiopath failed.");

		if (state.audioPath != nullptr && NDirectMusicDriver::geDestroyAudiopath(state.driver, state.audioPath) == 0)
			SetErrorMessage(manager, "DMManager: geDestroyAudiopath failed.");

		NDirectMusicDriver::geReleaseCachedLoader(state.driver);
		NDirectMusicDriver::geExitSynthesizer(state.driver);
		NDirectMusicDriver::geDestroyInstance(state.driver);
		NDirectMusicDriver::geGlobalExit();
		state.driver = nullptr;
		state.audioPath = nullptr;
		state.ducking = 0;
		state.recoveryTick = 0;
		state.lastMissionEvent = state.quality = state.currentEvent = state.volume = 0;
		state.currentVariant = state.voiceCount = 0;

		return 1;
	}

	// Original function: FUN_00403858; the timer is read even for an inactive manager.
	std::uint8_t Update(void* manager)
	{
		if (!gOnMusicThread)
		{
			if (manager != nullptr && gMusicWorker == nullptr)
				ReportMusicError(manager);

			return 1;
		}

		const std::uint32_t now = timeGetTime();

		if (State(manager).initialized == 0 || State(manager).ducking == 0 || NXBasics::Runtime_TimeSigned32(now - State(manager).recoveryTick) < 0)
			return 1;

		auto* audiopath = State(manager).audioPath;

		if (audiopath != nullptr && NDirectMusicDriver::geSetVolumeOfAudiopath(State(manager).driver, audiopath, 0, 300) == 0)
		{
			SetErrorMessage(manager, "DMManager: geSetVolumeOfAudiopath failed.");
			return 0;
		}

		State(manager).ducking = 0;

		return 1;
	}

	// Original function: FUN_0040392e.
	std::int32_t MainVolume_Get(const void* manager)
	{
		if (manager == nullptr)
			return 0;

		if (!gOnMusicThread && gMusicWorker != nullptr && gMusicWorker->manager == manager)
			return gMusicWorker->volume.load();

		return State(manager).volume;
	}

	// Original function: FUN_00403932; exact PE constants at 004F3338/004F3340.
	std::uint8_t MainVolume_Set(void* manager, std::int32_t volume)
	{
		if (manager == nullptr)
			return 0;

		if (!gOnMusicThread)
		{
			const auto bounded = volume < 0 ? 0 : volume > 100 ? 100
			                                                   : volume;

			if (gMusicWorker == nullptr)
			{
				State(manager).volume = bounded;
				return 0;
			}

			if (gMusicWorker->manager != manager)
				return 0;

			bool accepting;
			{
				const std::lock_guard<std::mutex> lock(gMusicWorker->mutex);
				gMusicWorker->volume.store(bounded);

				if (bounded == 0)
					gMusicWorker->musicRequested = false;

				accepting = gMusicWorker->accepting && !gMusicWorker->stopping;
			}
			// Volume is a latest-value mailbox, so slider changes cannot fill the queue.
			SetEvent(gMusicWorker->wake);
			return accepting ? 1 : 0;
		}

		State(manager).volume = volume;

		if (volume > 100)
		{
			State(manager).volume = 100;
		}
		else if (volume < 0)
		{
			State(manager).volume = 0;
		}

		if (State(manager).initialized == 0)
			return 0;

		if (State(manager).volume == 0)
			ResetAllSegments(manager);

		const std::int32_t currentVolume = State(manager).volume;
		std::int32_t attenuation = -10000;

		if (currentVolume != 0)
		{
			const double logarithm = std::log10(static_cast<double>(currentVolume) * static_cast<double>(0.01f));
			// Bounded finite result; the original converts toward zero with FUN_004e6f4c.
			attenuation = -500 - static_cast<std::int32_t>(logarithm * -2000.0);
		}

		if (NDirectMusicDriver::geSetMasterVolume(State(manager).driver, attenuation) == 0)
		{
			SetErrorMessage(manager, "DMManager: geSetMasterVolume failed.");
			return 0;
		}

		return 1;
	}

	// Original function: FUN_004038bd
	std::uint8_t ProcessEvent(void* manager, std::int32_t eventId, std::uint32_t eventParameter, std::int32_t mode)
	{
		if (!gOnMusicThread)
			return EnqueueMusicCommand(manager, { MusicCommandType::Process, eventId, eventParameter, mode });

		std::uint8_t stateFlag;
		stateFlag = State(manager).initialized;

		if (stateFlag == 0)
		{
			return 1;
		}

		std::int32_t resolvedEventId = eventId;

		if ((eventId >= 10 && eventId < 22) || (eventId >= 31 && eventId < 39))
		{
			State(manager).lastMissionEvent = eventId;
		}
		else if (eventId == 1)
		{
			resolvedEventId = State(manager).lastMissionEvent;
		}

		std::uint8_t result = PrepareEvent(manager, resolvedEventId, eventParameter);

		if (mode == 1)
		{
			ResetAllSegments(manager);
		}
		else if (mode == 2)
		{
			// Read the current volume after the first helper call.
			std::int32_t volume;
			volume = State(manager).volume;

			if (volume > 0)
			{
				result = StartEvent(manager, resolvedEventId, eventParameter);
			}
		}

		return result;
	}

	// Original function: FUN_004039d8
	void ResetAllSegments(void* manager)
	{
		if (!gOnMusicThread)
		{
			EnqueueMusicCommand(manager, { MusicCommandType::Reset });

			return;
		}

		auto* entries = State(manager).segments;

		for (std::uint32_t index = 0; index < 20; ++index)
		{
			SSegmentEntry* entry = entries + index;

			if (entry->active != 0)
			{
				NDirectMusicDriver::geResetSegment(State(manager).driver, entry->object, 0);
			}
		}

		State(manager).currentEvent = 0;
		State(manager).currentVariant = 0;
	}

	// Original function: FUN_00403a08
	std::uint8_t PrepareEvent(void* manager, std::int32_t eventId, std::uint32_t eventParameter)
	{
		if (!gOnMusicThread)
			return EnqueueMusicCommand(manager, { MusicCommandType::Prepare, eventId, eventParameter });

		if (eventId <= 1 || eventId >= 39)
		{
			return 0;
		}

		if (eventId >= 22 && eventId < 31)
		{
			return 1;
		}

		if (FindSegmentEntry(manager, eventId, eventParameter) != nullptr)
		{
			return 1;
		}

		const char* fileName = GetEventFileName(eventId, eventParameter);

		if (fileName == nullptr)
		{
			return 0;
		}

		auto* entries = State(manager).segments;
		SSegmentEntry* entry = nullptr;

		for (std::uint32_t index = 0; index < 20; ++index)
		{
			SSegmentEntry* candidate = entries + index;

			if (candidate->active == 0)
			{
				entry = candidate;
				break;
			}
		}

		if (entry == nullptr)
		{
			SetErrorMessage(manager, "DMManager: No free segment entry found!");
			return 0;
		}

		*entry = { };

		entry->eventId = eventId;
		entry->eventParameter = eventParameter;
		entry->objectKind = 0;
		entry->fileName = fileName;

		const NDirectMusicDriver::Gedx8LoadDescriptor descriptor{ entry->objectKind, entry->fileName, entry->eventId, static_cast<std::int32_t>(entry->eventParameter), 0 };

		if (NDirectMusicDriver::geLoadCachedObject(State(manager).driver, 0, &descriptor, &entry->object, State(manager).directory.get()) == 0)
		{
			SetErrorMessage(manager, "DMManager: geLoadCachedObject failed.");
			return 0;
		}

		entry->active = 1;

		const std::int32_t mode = State(manager).quality;
		const NDirectMusicDriver::Gedx8AudiopathConfig configuration{
			mode == 2 ? 1 : 3,
			State(manager).voiceCount
		};

		if (State(manager).audioPath != nullptr)
		{
			return 1;
		}

		auto* audiopathOutput = &State(manager).audioPath;
		std::uint8_t created = 0;

		if (mode != 2)
		{
			created = NDirectMusicDriver::geCreateAudiopath(State(manager).driver, &configuration, audiopathOutput, entry->object);
		}

		if (created == 0)
		{
			created = NDirectMusicDriver::geCreateAudiopath(State(manager).driver, &configuration, audiopathOutput, nullptr);
		}

		if (created == 0)
		{
			SetErrorMessage(manager, "DMManager: geCreateAudiopath failed.");
			NDirectMusicDriver::geDestroySegment(State(manager).driver, entry->object);
			*entry = { };
			return 0;
		}

		if (NDirectMusicDriver::geActivateAudiopath(State(manager).driver, State(manager).audioPath, 1) == 0)
		{
			SetErrorMessage(manager, "DMManager: geActivateAudiopath failed.");
			NDirectMusicDriver::geDestroySegment(State(manager).driver, entry->object);
			*entry = { };
			NDirectMusicDriver::geDestroyAudiopath(State(manager).driver, State(manager).audioPath);
			State(manager).audioPath = nullptr;
			return 0;
		}

		// Preserve the original start-then-reset sequence and ignored result.
		StartEvent(manager, eventId, eventParameter);
		ResetAllSegments(manager);

		return 1;
	}

	// Original function: FUN_00403c0e
	std::uint8_t StartEvent(void* manager, std::int32_t eventId, std::uint32_t eventParameter)
	{
		if (!gOnMusicThread)
			return EnqueueMusicCommand(manager, { MusicCommandType::Start, eventId, eventParameter });

		if (eventId <= 1 || eventId >= 39)
		{
			return 0;
		}

		if (State(manager).currentEvent == eventId && State(manager).currentVariant == eventParameter)
		{
			return 1;
		}

		const bool isVolumeEvent = eventId >= 22 && eventId < 31;
		SSegmentEntry* entry = nullptr;

		if (!isVolumeEvent)
		{
			entry = FindSegmentEntry(manager, eventId, eventParameter);

			if (entry == nullptr)
			{
				return 0;
			}

			std::uint8_t playing = 0;

			if (NDirectMusicDriver::geGetPlaybackStateOfSegment(State(manager).driver, entry->object, &playing) == 0)
			{
				SetErrorMessage(manager, "DMManager: geGetPlaybackStateOfSegment failed.");
			}

			// The original examines the output even when the query failed.

			if (playing != 0)
			{
				return 1;
			}
		}

		if (isVolumeEvent)
		{
			auto* audiopath = State(manager).audioPath;

			if (audiopath != nullptr && State(manager).ducking == 0)
			{
				if (NDirectMusicDriver::geSetVolumeOfAudiopath(State(manager).driver, audiopath, -2000, 300) == 0)
				{
					SetErrorMessage(manager, "DMManager: geSetVolumeOfAudiopath failed.");
					return 0;
				}

				State(manager).recoveryTick = 0;
				State(manager).ducking = 1;
			}

			const std::uint32_t delay = kVolumeRecoveryDelayMs[eventId - 22];
			const std::uint32_t now = static_cast<std::uint32_t>(::timeGetTime());

			State(manager).recoveryTick = now + delay;
			return 1;
		}

		auto* audiopath = State(manager).audioPath;

		if (audiopath == nullptr)
		{
			return 1;
		}

		if (NDirectMusicDriver::geStartSegment(State(manager).driver, audiopath, entry->object, 0x2000, -1, 1) == 0)
		{
			SetErrorMessage(manager, "DMManager: geStartSegmentPlayback failed.");
			return 0;
		}

		State(manager).currentEvent = eventId;
		State(manager).currentVariant = eventParameter;

		return 1;
	}

	// Original function: FUN_00403d6c
	const char* GetEventFileName(std::int32_t eventId, std::uint32_t variant)
	{
		if (eventId >= 2 && eventId <= 5)
		{
			const std::uint32_t variantIndex = variant >= 1u && variant <= 3u ? variant - 1u : 0u;
			const std::uint32_t eventIndex = static_cast<std::uint32_t>(eventId - 2);

			return kThemeFileNames[eventIndex * 3u + variantIndex];
		}

		if (eventId >= 6 && eventId <= 9)
		{
			return kAttackFileNames[static_cast<std::uint32_t>(eventId - 6)];
		}

		if (eventId >= 10 && eventId <= 21)
		{
			const std::uint32_t variantIndex = variant >= 4u && variant <= 6u ? variant - 4u : 0u;
			const std::uint32_t eventIndex = static_cast<std::uint32_t>(eventId - 10);

			return kMissionFileNames[eventIndex * 3u + variantIndex];
		}

		if (eventId >= 31 && eventId <= 38)
		{
			const std::uint32_t variantIndex = variant >= 4u && variant <= 6u ? variant - 4u : 0u;
			const std::uint32_t eventIndex = static_cast<std::uint32_t>(eventId - 31);

			return kAddonMissionFileNames[eventIndex * 3u + variantIndex];
		}

		return nullptr;
	}

	// Original function: FUN_00403e10
	static SSegmentEntry* FindSegmentEntry(void* manager, std::int32_t eventId, std::uint32_t eventParameter)
	{
		auto* entries = State(manager).segments;

		for (std::uint32_t index = 0; index < 20; ++index)
		{
			SSegmentEntry* entry = entries + index;

			if (entry->active == 0)
			{
				continue;
			}

			if (entry->eventId == eventId && entry->eventParameter == eventParameter)
			{
				return entry;
			}
		}

		return nullptr;
	}
}
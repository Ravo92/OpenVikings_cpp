#include "DirectMusicDriver.h"

#if defined(_WIN64)
#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>
#define DM_STATIC 1
#include <dmusic.h>
#include "RemasterLog.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <vector>

// Native x64 replacement for the game-reachable gedx8 API. Original event selection,
// repeats, measure-boundary requests and voice ducking remain in DMManager.
namespace NDirectMusicDriver
{
	using Clock = std::chrono::steady_clock;

	struct Gedx8LoadedObject
	{
		DmSegment* segment = nullptr;
		std::filesystem::path filename;
		std::string label;

		~Gedx8LoadedObject()
		{
			if (segment != nullptr)
				DmSegment_release(segment);
		}
	};

	struct Gedx8Audiopath
	{
		bool active = false;
		float initialAttenuation = 0;
		float targetAttenuation = 0;
		Clock::time_point fadeStart{ };
		std::chrono::milliseconds fadeLength{ 0 };
	};

	struct Gedx8DriverInstance
	{
		static constexpr std::size_t FramesPerBuffer = 1024;
		static constexpr std::size_t BufferCount = 4;
		DmLoader* loader = nullptr;
		DmPerformance* performance = nullptr;
		std::filesystem::path directory;
		std::vector<std::unique_ptr<Gedx8LoadedObject>> objects;
		std::vector<std::unique_ptr<Gedx8Audiopath>> paths;
		HWAVEOUT output = nullptr;
		HANDLE stop = nullptr;
		HANDLE completed = nullptr;
		std::thread renderThread;
		std::mutex playbackMutex;
		std::array<WAVEHDR, BufferCount> headers{ };
		std::array<std::array<std::int16_t, FramesPerBuffer * 2>, BufferCount> samples{ };
		Gedx8Audiopath* currentPath = nullptr;
		Gedx8LoadedObject* requestedObject = nullptr;
		Gedx8LoadedObject* reportedObject = nullptr;
		float masterAttenuation = 0;
		std::atomic<bool> failed{ false };
	};

	namespace
	{
		std::vector<Gedx8DriverInstance*>& Instances()
		{
			static std::vector<Gedx8DriverInstance*> instances;

			return instances;
		}

		void Report(const char* event, const char* detail, std::uint32_t code = 0) noexcept
		{
			NRemaster::Logging::Event(NRemaster::Logging::ECategory::Renderer, event, "%s code=%u backend=GothicKit", detail, code);
			char text[512];
			std::snprintf(text, sizeof(text), "OpenVikings music: %s: %s (code=%u)\r\n", event, detail, code);
			OutputDebugStringA(text);
		}

		void LibraryLog(void*, DmLogLevel level, const char* message)
		{
			Report(level <= DmLogLevel_ERROR ? "music_decode_error" : "music_decode_warning", message);
		}

		std::uint32_t MusicRandom(void*) noexcept
		{
			// Music variations never consume the simulation's or CRT's random sequence.
			static std::atomic<std::uint32_t> state{ 0x6D757369u };
			auto previous = state.load(std::memory_order_relaxed);
			std::uint32_t next;

			do
			{
				next = previous;
				next ^= next << 13;
				next ^= next >> 17;
				next ^= next << 5;
			} while (!state.compare_exchange_weak(previous, next, std::memory_order_relaxed));

			return next;
		}

		void* ResolveFile(void* context, const char* filename, std::size_t* length) noexcept
		{
			if (filename == nullptr || length == nullptr)
				return nullptr;

			*length = 0;

			try
			{
				const auto& directory = static_cast<Gedx8DriverInstance*>(context)->directory;
				std::string relative(filename);
				std::replace(relative.begin(), relative.end(), '\\', '/');
				// Producer references can contain the composer's obsolete absolute directory.
				auto path = directory / std::filesystem::u8path(relative).relative_path();
				FILE* file = nullptr;

				if (_wfopen_s(&file, path.c_str(), L"rb") != 0)
				{
					path = directory / std::filesystem::u8path(relative).filename();

					if (_wfopen_s(&file, path.c_str(), L"rb") != 0)
					{
						if (_stricmp(relative.c_str(), "gm.dls") == 0)
						{
							wchar_t systemDirectory[MAX_PATH];
							const auto size = GetSystemDirectoryW(systemDirectory, MAX_PATH);

							if (size != 0 && size < MAX_PATH)
							{
								path = std::filesystem::path(systemDirectory) / L"drivers" / L"gm.dls";
								_wfopen_s(&file, path.c_str(), L"rb");
							}
						}

						if (file == nullptr)
						{
							Report("music_file_missing", filename);

							return nullptr;
						}
					}
				}

				std::unique_ptr<FILE, decltype(&std::fclose)> owned(file, &std::fclose);

				if (_fseeki64(file, 0, SEEK_END) != 0)
					return nullptr;

				const auto size = _ftelli64(file);

				if (size <= 0 || static_cast<std::uint64_t>(size) > SIZE_MAX || _fseeki64(file, 0, SEEK_SET) != 0)
					return nullptr;

				// The loader's documented allocator is malloc/free; do not use Runtime_Allocate.
				std::unique_ptr<void, decltype(&std::free)> data(std::malloc(static_cast<std::size_t>(size)), &std::free);

				if (data == nullptr || std::fread(data.get(), 1, static_cast<std::size_t>(size), file) != static_cast<std::size_t>(size))
					return nullptr;

				*length = static_cast<std::size_t>(size);

				return data.release();
			}
			catch (...)
			{
				Report("music_file_error", filename);

				return nullptr;
			}
		}

		float Attenuation(const Gedx8Audiopath& path) noexcept
		{
			if (path.fadeLength.count() == 0)
				return path.targetAttenuation;

			const auto elapsed = std::chrono::duration<float, std::milli>(Clock::now() - path.fadeStart).count();
			const auto phase = std::clamp(elapsed / static_cast<float>(path.fadeLength.count()), 0.0f, 1.0f);

			return path.initialAttenuation + (path.targetAttenuation - path.initialAttenuation) * phase;
		}

		void Render(Gedx8DriverInstance& instance) noexcept
		{
			const HANDLE events[]{ instance.stop, instance.completed };

			for (;;)
			{
				const auto wait = WaitForMultipleObjects(2, events, FALSE, 20);

				if (wait == WAIT_OBJECT_0)
					return;

				if (wait == WAIT_FAILED)
				{
					instance.failed.store(true);
					Report("music_output_failed", "buffer wait", GetLastError());

					return;
				}

				for (std::size_t index = 0; index < instance.headers.size(); ++index)
				{
					auto& header = instance.headers[index];

					if ((header.dwFlags & WHDR_DONE) == 0)
						continue;

					DmResult result;
					{
						const std::lock_guard<std::mutex> lock(instance.playbackMutex);
						const auto* path = instance.currentPath;
						const float attenuation = instance.masterAttenuation + (path != nullptr ? Attenuation(*path) : 0.0f);
						const float gain = path == nullptr || !path->active || attenuation <= -10000.0f ? 0.0f : std::pow(10.0f, attenuation / 2000.0f);
						DmPerformance_setVolume(instance.performance, gain);
						result = DmPerformance_renderPcm(instance.performance, instance.samples[index].data(), instance.samples[index].size(), static_cast<DmRenderOptions>(DmRender_SHORT | DmRender_STEREO));

						if (result == DmResult_SUCCESS && instance.requestedObject != nullptr && instance.reportedObject != instance.requestedObject && NRemaster::Logging::Enabled(NRemaster::Logging::ECategory::Renderer) && DmPerformance_isPlaying(instance.performance, instance.requestedObject->segment) != 0)
						{
							instance.reportedObject = instance.requestedObject;
							NRemaster::Logging::Event(NRemaster::Logging::ECategory::Renderer, "music_segment_rendering", "file=%s", instance.reportedObject->label.c_str());
						}
					}

					if (result != DmResult_SUCCESS)
					{
						instance.failed.store(true);
						Report("music_synthesis_failed", "PCM render", result);

						return;
					}

					const auto written = waveOutWrite(instance.output, &header, sizeof(header));

					if (written != MMSYSERR_NOERROR)
					{
						instance.failed.store(true);
						Report("music_output_failed", "waveOutWrite", written);

						return;
					}
				}
			}
		}

		void StopOutput(Gedx8DriverInstance& instance) noexcept
		{
			if (instance.stop != nullptr)
				SetEvent(instance.stop);

			if (instance.renderThread.joinable())
				instance.renderThread.join();

			if (instance.output != nullptr)
			{
				waveOutReset(instance.output);

				for (auto& header : instance.headers)
				{
					if ((header.dwFlags & WHDR_PREPARED) != 0)
						waveOutUnprepareHeader(instance.output, &header, sizeof(header));
				}

				waveOutClose(instance.output);
				instance.output = nullptr;
			}

			if (instance.completed != nullptr)
				CloseHandle(instance.completed);

			if (instance.stop != nullptr)
				CloseHandle(instance.stop);

			instance.completed = instance.stop = nullptr;
			instance.currentPath = nullptr;

			if (instance.performance != nullptr)
				DmPerformance_release(instance.performance);

			instance.performance = nullptr;
		}

		bool Owns(const Gedx8DriverInstance* instance, const Gedx8LoadedObject* object)
		{

			return instance != nullptr && object != nullptr && std::any_of(instance->objects.begin(), instance->objects.end(), [object](const auto& item)
			                                                               {

				                                                               return item.get() == object;
			                                                               });
		}

		bool Owns(const Gedx8DriverInstance* instance, const Gedx8Audiopath* path)
		{

			return instance != nullptr && path != nullptr && std::any_of(instance->paths.begin(), instance->paths.end(), [path](const auto& item)
			                                                             {

				                                                             return item.get() == path;
			                                                             });
		}
	}

	void __stdcall geGlobalInit()
	{
		Dm_setRandomNumberGenerator(&MusicRandom, nullptr);
		Dm_setLogger(DmLogLevel_WARN, &LibraryLog, nullptr);
	}

	void __stdcall geGlobalExit()
	{
		while (!Instances().empty())
		{
			geDestroyInstance(Instances().back());
		}
	}

	Gedx8DriverInstance* __stdcall geCreateInstance()
	{
		try
		{
			auto instance = std::make_unique<Gedx8DriverInstance>();
			Instances().push_back(instance.get());

			return instance.release();
		}
		catch (...)
		{

			return nullptr;
		}
	}

	u8 __cdecl geDestroyInstance(Gedx8DriverInstance* instance)
	{
		auto& instances = Instances();
		const auto found = std::find(instances.begin(), instances.end(), instance);

		if (found == instances.end())
			return 0;

		StopOutput(*instance);
		instance->objects.clear();

		if (instance->loader != nullptr)
			DmLoader_release(instance->loader);

		instances.erase(found);
		delete instance;

		return 1;
	}

	u8 __stdcall geInitSynthesizer(Gedx8DriverInstance* instance, const Gedx8SynthInitConfig* config)
	{
		if (instance == nullptr || config == nullptr || config->sampleRate == 0)
			return 0;

		if (instance->output != nullptr)
			return 1;

		try
		{
			const auto created = DmPerformance_create(&instance->performance, config->sampleRate);

			if (created != DmResult_SUCCESS)
			{
				Report("music_init_failed", "performance", created);
				StopOutput(*instance);

				return 0;
			}

			instance->failed.store(false);
			instance->stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
			instance->completed = CreateEventW(nullptr, FALSE, FALSE, nullptr);

			if (instance->stop == nullptr || instance->completed == nullptr)
			{
				Report("music_init_failed", "buffer events", GetLastError());
				StopOutput(*instance);

				return 0;
			}

			WAVEFORMATEX format{ WAVE_FORMAT_PCM, 2, config->sampleRate, config->sampleRate * 4, 4, 16, 0 };
			const auto opened = waveOutOpen(&instance->output, WAVE_MAPPER, &format, reinterpret_cast<DWORD_PTR>(instance->completed), 0, CALLBACK_EVENT);

			if (opened != MMSYSERR_NOERROR)
			{
				Report("music_init_failed", "waveOutOpen", opened);
				StopOutput(*instance);

				return 0;
			}

			waveOutPause(instance->output);

			for (std::size_t index = 0; index < instance->headers.size(); ++index)
			{
				auto& header = instance->headers[index];
				header = { };
				instance->samples[index].fill(0);
				header.lpData = reinterpret_cast<LPSTR>(instance->samples[index].data());
				header.dwBufferLength = static_cast<DWORD>(sizeof(instance->samples[index]));
				const auto prepared = waveOutPrepareHeader(instance->output, &header, sizeof(header));
				const auto written = prepared == MMSYSERR_NOERROR ? waveOutWrite(instance->output, &header, sizeof(header)) : prepared;

				if (written != MMSYSERR_NOERROR)
				{
					Report("music_init_failed", "PCM buffer", written);
					StopOutput(*instance);

					return 0;
				}
			}

			instance->renderThread = std::thread([instance]
			                                     {
				                                     Render(*instance);
			                                     });
			const auto restarted = waveOutRestart(instance->output);

			if (restarted != MMSYSERR_NOERROR)
			{
				Report("music_init_failed", "waveOutRestart", restarted);
				StopOutput(*instance);

				return 0;
			}

			NRemaster::Logging::Event(NRemaster::Logging::ECategory::Renderer, "music_ready", "backend=GothicKit rate=%u channels=2 bits=16", config->sampleRate);

			return 1;
		}
		catch (...)
		{
			StopOutput(*instance);

			return 0;
		}
	}

	u8 __stdcall geExitSynthesizer(Gedx8DriverInstance* instance)
	{
		if (instance == nullptr)
			return 0;

		StopOutput(*instance);

		return 1;
	}

	u8 __stdcall geSetMasterVolume(Gedx8DriverInstance* instance, s32 value)
	{
		if (instance == nullptr || instance->performance == nullptr)
			return 0;

		const std::lock_guard<std::mutex> lock(instance->playbackMutex);
		instance->masterAttenuation = static_cast<float>(std::clamp(value, -10000, 0));

		return 1;
	}

	u8 __stdcall geLoadCachedObject(Gedx8DriverInstance* instance, s32 loadMode, const Gedx8LoadDescriptor* descriptor, Gedx8LoadedObject** objectOut, const char* basePath)
	{
		if (objectOut == nullptr)
			return 0;

		*objectOut = nullptr;

		if (instance == nullptr || descriptor == nullptr || descriptor->objectKind != 0 || descriptor->fileName == nullptr || basePath == nullptr || loadMode != 0)
			return 0;

		try
		{
			const auto directory = std::filesystem::absolute(std::filesystem::u8path(basePath));
			const auto filename = directory / std::filesystem::u8path(descriptor->fileName);

			for (const auto& object : instance->objects)
			{
				if (object->filename == filename)
				{
					*objectOut = object.get();

					return 1;
				}
			}

			if (instance->loader == nullptr || instance->directory != directory)
			{
				geReleaseCachedLoader(instance);
				instance->directory = directory;
				const auto created = DmLoader_create(&instance->loader, DmLoader_DOWNLOAD);

				if (created != DmResult_SUCCESS || DmLoader_addResolver(instance->loader, &ResolveFile, instance) != DmResult_SUCCESS)
				{
					geReleaseCachedLoader(instance);

					return 0;
				}
			}

			auto object = std::make_unique<Gedx8LoadedObject>();
			object->filename = filename;
			object->label = descriptor->fileName;
			const auto loaded = DmLoader_getSegment(instance->loader, descriptor->fileName, &object->segment);

			if (loaded != DmResult_SUCCESS)
			{
				Report("music_segment_failed", descriptor->fileName, loaded);

				return 0;
			}

			instance->objects.push_back(std::move(object));
			*objectOut = instance->objects.back().get();
			NRemaster::Logging::Event(NRemaster::Logging::ECategory::Renderer, "music_segment_loaded", "file=%s", descriptor->fileName);

			return 1;
		}
		catch (...)
		{

			return 0;
		}
	}

	u8 __stdcall geReleaseCachedLoader(Gedx8DriverInstance* instance)
	{
		if (instance == nullptr)
			return 0;

		if (instance->loader != nullptr)
			DmLoader_release(instance->loader);

		instance->loader = nullptr;

		return 1;
	}

	u8 __stdcall geCreateAudiopath(Gedx8DriverInstance* instance, const Gedx8AudiopathConfig* config, Gedx8Audiopath** audiopathOut, Gedx8LoadedObject* object)
	{
		if (audiopathOut == nullptr)
			return 0;

		*audiopathOut = nullptr;

		if (instance == nullptr || config == nullptr || instance->performance == nullptr || (object != nullptr && !Owns(instance, object)))
			return 0;

		try
		{
			auto path = std::make_unique<Gedx8Audiopath>();
			const std::lock_guard<std::mutex> lock(instance->playbackMutex);
			instance->paths.push_back(std::move(path));
			*audiopathOut = instance->paths.back().get();

			return 1;
		}
		catch (...)
		{

			return 0;
		}
	}

	u8 __stdcall geActivateAudiopath(Gedx8DriverInstance* instance, Gedx8Audiopath* audiopath, s32 activeState)
	{
		if (!Owns(instance, audiopath))
			return 0;

		const std::lock_guard<std::mutex> lock(instance->playbackMutex);
		audiopath->active = activeState != 0;

		return 1;
	}

	u8 __stdcall geSetVolumeOfAudiopath(Gedx8DriverInstance* instance, Gedx8Audiopath* audiopath, s32 volume, s32 fadeMilliseconds)
	{
		if (!Owns(instance, audiopath))
			return 0;

		const std::lock_guard<std::mutex> lock(instance->playbackMutex);
		audiopath->initialAttenuation = Attenuation(*audiopath);
		audiopath->targetAttenuation = static_cast<float>(std::clamp(volume, -10000, 0));
		audiopath->fadeStart = Clock::now();
		audiopath->fadeLength = std::chrono::milliseconds(std::max(fadeMilliseconds, 0));

		return 1;
	}

	u8 __stdcall geDestroyAudiopath(Gedx8DriverInstance* instance, Gedx8Audiopath* audiopath)
	{
		if (!Owns(instance, audiopath))
			return 0;

		const std::lock_guard<std::mutex> lock(instance->playbackMutex);

		if (instance->currentPath == audiopath)
		{
			if (instance->performance != nullptr)
				DmPerformance_stopSegment(instance->performance, nullptr);

			instance->currentPath = nullptr;
			instance->requestedObject = nullptr;
			instance->reportedObject = nullptr;
		}

		instance->paths.erase(std::find_if(instance->paths.begin(), instance->paths.end(), [audiopath](const auto& item)
		                                   {

			                                   return item.get() == audiopath;
		                                   }));

		return 1;
	}

	u8 __stdcall geStartSegment(Gedx8DriverInstance* instance, Gedx8Audiopath* audiopath, Gedx8LoadedObject* object, u32 flags, s32 repeatCount, u8 downloadBeforePlay)
	{
		static_cast<void>(downloadBeforePlay); // References are downloaded before publishing the cache entry.

		if (!Owns(instance, object) || !Owns(instance, audiopath) || instance->performance == nullptr || instance->failed.load())
			return 0;

		const std::lock_guard<std::mutex> lock(instance->playbackMutex);
		DmSegment_setRepeats(object->segment, static_cast<std::uint32_t>(repeatCount));
		const auto result = DmPerformance_playSegment(instance->performance, object->segment, (flags & 0x2000u) != 0 ? DmTiming_MEASURE : DmTiming_INSTANT);

		if (result != DmResult_SUCCESS)
			return 0;

		instance->currentPath = audiopath;
		instance->requestedObject = object;
		NRemaster::Logging::Event(NRemaster::Logging::ECategory::Renderer, "music_segment_started", "file=%s repeats=%d flags=0x%08X state=scheduled", object->label.c_str(), repeatCount, flags);

		return 1;
	}

	u8 __stdcall geResetSegment(Gedx8DriverInstance* instance, Gedx8LoadedObject* object, s32 stopMode)
	{
		static_cast<void>(stopMode); // The game's only observed request is an immediate stop (0).

		if (!Owns(instance, object) || instance->performance == nullptr)
			return 0;

		const std::lock_guard<std::mutex> lock(instance->playbackMutex);

		const auto result = DmPerformance_stopSegment(instance->performance, object->segment);

		if (result != DmResult_SUCCESS)
			return 0;

		if (instance->requestedObject == object || instance->reportedObject == object)
		{
			NRemaster::Logging::Event(NRemaster::Logging::ECategory::Renderer, "music_segment_stopped", "file=%s", object->label.c_str());
		}

		if (instance->requestedObject == object)
			instance->requestedObject = nullptr;

		if (instance->reportedObject == object)
			instance->reportedObject = nullptr;

		return 1;
	}

	u8 __stdcall geGetPlaybackStateOfSegment(Gedx8DriverInstance* instance, Gedx8LoadedObject* object, u8* stateOut)
	{
		if (stateOut == nullptr)
			return 0;

		*stateOut = 0;

		if (!Owns(instance, object) || instance->performance == nullptr || instance->failed.load())
			return 0;

		const std::lock_guard<std::mutex> lock(instance->playbackMutex);
		*stateOut = DmPerformance_isPlaying(instance->performance, object->segment) != 0;

		return 1;
	}

	u8 __stdcall geDestroySegment(Gedx8DriverInstance* instance, Gedx8LoadedObject* object)
	{
		if (!Owns(instance, object))
			return 0;

		if (instance->performance != nullptr)
			geResetSegment(instance, object, 0);

		instance->objects.erase(std::find_if(instance->objects.begin(), instance->objects.end(), [object](const auto& item)
		                                     {

			                                     return item.get() == object;
		                                     }));

		return 1;
	}
}
#endif
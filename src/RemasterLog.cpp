#include "RemasterLog.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <io.h>
#include <mutex>
#include <string>
#include <thread>
#include <Windows.h>

namespace NRemaster::Logging
{
	namespace
	{
		constexpr std::size_t CategoryCount = static_cast<std::size_t>(ECategory::Count);
		constexpr std::array<const wchar_t*, CategoryCount> Filenames{ L"network.log", L"ai.jsonl", L"renderer.log" };
		struct SRecord { ECategory category; std::uint64_t sequence; SYSTEMTIME time; std::string message; };
		struct SLogger
		{
			std::atomic<std::uint32_t> categories{ 0 };
			std::mutex mutex;
			std::condition_variable ready;
			// Opt-in diagnostics retain every accepted entry; there is no overflow/drop policy.
			std::deque<SRecord> queue;
			std::filesystem::path directory;
			std::thread writer;
			std::uint64_t sequence = 0;
			bool accepting = false;
			~SLogger() { Stop(); }
			void Stop() noexcept
			{
				{ std::lock_guard<std::mutex> lock(mutex); accepting = false; categories.store(0); }
				ready.notify_one();
				if (writer.joinable()) writer.join();
			}
		};
		SLogger& Log() { static SLogger instance; return instance; }
		std::uint32_t Bit(ECategory category) { const auto index = static_cast<std::uint32_t>(category); return index < CategoryCount ? 1u << index : 0; }
		std::uint32_t Categories(const char* commandLine)
		{
			std::uint32_t result = 0;
			if (commandLine == nullptr) return result;
			const char* cursor = commandLine;
			while (*cursor != 0)
			{
				while (*cursor == ' ' || *cursor == '\t') ++cursor;
				std::string argument; bool quoted = false;
				while (*cursor != 0 && (quoted || (*cursor != ' ' && *cursor != '\t')))
				{
					if (*cursor == '"') quoted = !quoted;
					else argument += *cursor;
					++cursor;
				}
				if (_stricmp(argument.c_str(), "-log:network") == 0) result |= Bit(ECategory::Network);
				else if (_stricmp(argument.c_str(), "-log:ai") == 0) result |= Bit(ECategory::AI);
				else if (_stricmp(argument.c_str(), "-log:renderer") == 0) result |= Bit(ECategory::Renderer);
			}
			return result;
		}
		void WriteAll(std::FILE* file, const std::string& text)
		{
			std::size_t written = 0;
			while (written < text.size())
			{
				const auto count = std::fwrite(text.data() + written, 1, text.size() - written, file);
				if (count == 0) throw 0;
				written += count;
			}
		}
		void Run() noexcept
		{
			auto& log = Log();
			std::array<std::FILE*, CategoryCount> files{};
			try
			{
				for (;;)
				{
					std::deque<SRecord> batch;
					{ std::unique_lock<std::mutex> lock(log.mutex); log.ready.wait(lock, [&] { return !log.accepting || !log.queue.empty(); });
						if (log.queue.empty() && !log.accepting) break;
						batch.swap(log.queue);
					}
					for (const auto& record : batch)
					{
						const auto index = static_cast<std::size_t>(record.category);
						if (files[index] == nullptr)
						{
							std::filesystem::create_directories(log.directory);
							files[index] = _wfopen((log.directory / Filenames[index]).c_str(), L"ab");
							if (files[index] == nullptr) throw 0;
						}
						char prefix[160];
						if (record.category == ECategory::AI && !record.message.empty() && record.message.front() == '{')
						{
							std::snprintf(prefix, sizeof(prefix), "{\"sequence\":%llu,", static_cast<unsigned long long>(record.sequence));
							WriteAll(files[index], std::string(prefix) + record.message.substr(1));
						}
						else
						{
							std::snprintf(prefix, sizeof(prefix), "[%04u-%02u-%02u %02u:%02u:%02u.%03u] [%llu] ", record.time.wYear, record.time.wMonth, record.time.wDay, record.time.wHour, record.time.wMinute, record.time.wSecond, record.time.wMilliseconds, static_cast<unsigned long long>(record.sequence));
							WriteAll(files[index], std::string(prefix) + record.message + (record.message.empty() || record.message.back() != '\n' ? "\n" : ""));
						}
					}
					for (auto* file : files) if (file != nullptr && std::fflush(file) != 0) throw 0;
				}
			}
			catch (...)
			{
				{ std::lock_guard<std::mutex> lock(log.mutex); log.accepting = false; log.categories.store(0); }
				OutputDebugStringA("OpenVikings: logging failed; queued records could not all be saved (file system or memory error).\n");
			}
			for (auto* file : files)
			{
				if (file == nullptr) continue;
				if (std::fflush(file) != 0 || _commit(_fileno(file)) != 0) OutputDebugStringA("OpenVikings: final log flush failed.\n");
				if (std::fclose(file) != 0) OutputDebugStringA("OpenVikings: closing log file failed.\n");
			}
		}
	}
	void Initialize(const char* commandLine) noexcept
	{
		try
		{
			const auto categories = Categories(commandLine);
			if (categories == 0) return; // No directory, file or writer thread when logging is disabled.
			auto& log = Log();
			if (log.writer.joinable()) return;
			log.directory = std::filesystem::current_path() / L"logs";
			log.accepting = true; log.sequence = 0; log.categories.store(categories);
			try { log.writer = std::thread(Run); } catch (...) { log.accepting = false; log.categories.store(0); throw; }
			for (std::size_t i = 0; i < CategoryCount; ++i)
			{
				const auto category = static_cast<ECategory>(i);
				if (category != ECategory::AI) Event(category, "log_start", "process=%lu build=%s %s", GetCurrentProcessId(), __DATE__, __TIME__);
			}
		}
		catch (...) { OutputDebugStringA("OpenVikings: logging initialization failed.\n"); }
	}
	bool Enabled(ECategory category) noexcept { return (Log().categories.load() & Bit(category)) != 0; }
	bool Write(ECategory category, std::string message) noexcept
	{
		if (!Enabled(category)) return false;
		try
		{
			auto& log = Log();
			{ std::lock_guard<std::mutex> lock(log.mutex);
				if (!log.accepting || (log.categories.load() & Bit(category)) == 0) return false;
				SYSTEMTIME time; GetLocalTime(&time);
				log.queue.push_back(SRecord{ category, ++log.sequence, time, std::move(message) });
			}
			log.ready.notify_one();
			return true;
		}
		catch (...) { OutputDebugStringA("OpenVikings: could not queue a log record (memory allocation failed).\n"); return false; }
	}
	void Event(ECategory category, const char* event, const char* format, ...) noexcept
	{
		if (!Enabled(category)) return;
		try
		{
			va_list args; va_start(args, format); const auto count = _vscprintf(format, args); va_end(args);
			if (count < 0) return;
			std::string text(static_cast<std::size_t>(count) + 1, '\0');
			va_start(args, format); std::vsnprintf(text.data(), text.size(), format, args); va_end(args); text.resize(static_cast<std::size_t>(count));
			Write(category, std::string(event) + ": " + text);
		}
		catch (...) { OutputDebugStringA("OpenVikings: could not format a log record.\n"); }
	}
	void Shutdown() noexcept
	{
		for (std::size_t i = 0; i < CategoryCount; ++i)
		{
			const auto category = static_cast<ECategory>(i);
			if (category != ECategory::AI) Event(category, "log_end", "normal shutdown; draining all accepted records");
		}
		Log().Stop();
	}
}

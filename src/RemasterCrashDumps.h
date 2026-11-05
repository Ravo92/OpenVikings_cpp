#pragma once

namespace NRemaster::CrashDumps
{
#if defined(_DEBUG)
	bool RunHelperIfRequested(const char* commandLine, int& exitCode) noexcept;
	void Initialize() noexcept;
	void WriteFatalError(const char* message) noexcept;
#else
	inline bool RunHelperIfRequested(const char*, int&) noexcept
	{

		return false;
	}

	inline void Initialize() noexcept
	{
	}

	inline void WriteFatalError(const char*) noexcept
	{
	}
#endif
}
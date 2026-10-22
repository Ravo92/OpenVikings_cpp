#pragma once

#include <cstddef>
#include <cstdint>

// Native, statically linked subset of the supplied gedx8musicdrv reconstruction.
// The caller owns the COM apartment and uses all operations on its music thread.
namespace NDirectMusicDriver
{
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using s8 = std::int8_t;
	using s16 = std::int16_t;
	using s32 = std::int32_t;

	struct Gedx8DriverInstance;
	struct Gedx8LoadedObject;
	struct Gedx8Audiopath;

	struct Gedx8SynthInitConfig
	{
		void* windowHandle;
		u32 voiceCount;
		u32 sampleRate;
	};
	struct Gedx8LoadDescriptor
	{
		s32 objectKind;
		const char* fileName;
		s32 metadata08;
		s32 metadata0C;
		s32 reserved10;
	};
	struct Gedx8AudiopathConfig
	{
		s32 type;
		u32 pchannelCount;
	};

	static_assert(sizeof(Gedx8SynthInitConfig) == (sizeof(void*) == 8 ? 16 : 12));
	static_assert(sizeof(Gedx8LoadDescriptor) == (sizeof(void*) == 8 ? 32 : 20));
	static_assert(offsetof(Gedx8LoadDescriptor, fileName) == sizeof(void*));
	static_assert(sizeof(Gedx8AudiopathConfig) == 8);

	void __stdcall geGlobalInit();
	void __stdcall geGlobalExit();
	Gedx8DriverInstance* __stdcall geCreateInstance();
	u8 __cdecl geDestroyInstance(Gedx8DriverInstance* instance);
	u8 __stdcall geInitSynthesizer(Gedx8DriverInstance* instance, const Gedx8SynthInitConfig* config);
	u8 __stdcall geExitSynthesizer(Gedx8DriverInstance* instance);
	u8 __stdcall geSetMasterVolume(Gedx8DriverInstance* instance, s32 value);
	u8 __stdcall geLoadCachedObject(Gedx8DriverInstance* instance, s32 loadMode, const Gedx8LoadDescriptor* descriptor, Gedx8LoadedObject** objectOut, const char* basePath);
	u8 __stdcall geReleaseCachedLoader(Gedx8DriverInstance* instance);
	u8 __stdcall geCreateAudiopath(Gedx8DriverInstance* instance, const Gedx8AudiopathConfig* config, Gedx8Audiopath** audiopathOut, Gedx8LoadedObject* object);
	u8 __stdcall geActivateAudiopath(Gedx8DriverInstance* instance, Gedx8Audiopath* audiopath, s32 activeState);
	u8 __stdcall geSetVolumeOfAudiopath(Gedx8DriverInstance* instance, Gedx8Audiopath* audiopath, s32 volume, s32 fadeMilliseconds);
	u8 __stdcall geDestroyAudiopath(Gedx8DriverInstance* instance, Gedx8Audiopath* audiopath);
	u8 __stdcall geStartSegment(Gedx8DriverInstance* instance, Gedx8Audiopath* audiopath, Gedx8LoadedObject* object, u32 flags, s32 repeatCount, u8 downloadBeforePlay);
	u8 __stdcall geResetSegment(Gedx8DriverInstance* instance, Gedx8LoadedObject* object, s32 stopMode);
	u8 __stdcall geGetPlaybackStateOfSegment(Gedx8DriverInstance* instance, Gedx8LoadedObject* object, u8* stateOut);
	u8 __stdcall geDestroySegment(Gedx8DriverInstance* instance, Gedx8LoadedObject* object);
}

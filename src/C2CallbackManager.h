#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Numeric values are retained until the original enum names are mapped.
	using TCallbackTypes = std::uint32_t;
	// Runtime slots carry borrowed pointers or fixed-width game values, never serialized object layouts.
	using TCallbackFunction = bool(__cdecl*)(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);

	class CCallbackManager
	{
	public:
		inline static CCallbackManager* sTheObjectPtr = nullptr;

		CCallbackManager();
		~CCallbackManager();
		CCallbackManager(const CCallbackManager&) = delete;
		CCallbackManager& operator=(const CCallbackManager&) = delete;

		void RegisterCallback(TCallbackTypes type, TCallbackFunction callback, std::uintptr_t userData, std::int32_t priority);
		bool UnRegisterCallback(TCallbackTypes type, TCallbackFunction callback, std::uintptr_t userData);
		bool ExecuteCallback(TCallbackTypes type, bool forward, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);

	private:
		struct SCallbackElement
		{
			std::int32_t priority;
			TCallbackFunction callback;
			std::uintptr_t userData;
			SCallbackElement* previous;
			SCallbackElement* next;
		};

		static constexpr std::size_t CallbackTypeCount = 0x34;

		SCallbackElement* l_FindCallbackElement(TCallbackTypes type, TCallbackFunction callback, std::uintptr_t userData);

		SCallbackElement* mFirst[CallbackTypeCount];
		SCallbackElement* mLast[CallbackTypeCount];

		static_assert(sizeof(SCallbackElement) == (sizeof(void*) == 8 ? 40 : 20));
		static_assert(offsetof(SCallbackElement, priority) == 0);
		static_assert(offsetof(SCallbackElement, callback) == sizeof(void*));
		static_assert(offsetof(SCallbackElement, userData) == 2 * sizeof(void*));
		static_assert(offsetof(SCallbackElement, previous) == 3 * sizeof(void*));
		static_assert(offsetof(SCallbackElement, next) == 4 * sizeof(void*));
	};

	static_assert(sizeof(std::uintptr_t) == sizeof(void*));
	static_assert(sizeof(CCallbackManager) == 104 * sizeof(void*));
}

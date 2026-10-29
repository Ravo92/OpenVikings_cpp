#include "C2CallbackManager.h"
#include "XCrtFileAccess.h"

#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_00411ca8.
	CCallbackManager::CCallbackManager()
	{
		static_assert(offsetof(CCallbackManager, mFirst) == 0);
		static_assert(offsetof(CCallbackManager, mLast) == CallbackTypeCount * sizeof(void*));
		sTheObjectPtr = this;
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_00411cc5.
	CCallbackManager::~CCallbackManager()
	{
		for (std::size_t type = 0; type < CallbackTypeCount; ++type)
		{
			while (mFirst[type] != nullptr)
			{
				SCallbackElement* element = mFirst[type];

				UnRegisterCallback(static_cast<TCallbackTypes>(type), element->callback, element->userData);
			}
		}

		sTheObjectPtr = nullptr;
	}

	// Reference: Mac CCallbackManager::l_FindCallbackElement()
	CCallbackManager::SCallbackElement* CCallbackManager::l_FindCallbackElement(TCallbackTypes type, TCallbackFunction callback, std::uintptr_t userData)
	{
		SCallbackElement* element = mFirst[type];

		while (element != nullptr)
		{
			if (element->callback == callback && element->userData == userData)
			{
				return element;
			}

			element = element->next;
		}

		return nullptr;
	}

	// Original function: FUN_00411cf8.
	void CCallbackManager::RegisterCallback(TCallbackTypes type, TCallbackFunction callback, std::uintptr_t userData, std::int32_t priority)
	{
		if (l_FindCallbackElement(type, callback, userData) != nullptr)
		{
			UnRegisterCallback(type, callback, userData);
		}

		auto* element = static_cast<SCallbackElement*>(NXBasics::Runtime_Allocate(sizeof(SCallbackElement)));
		std::memset(element, 0, sizeof(SCallbackElement));

		element->priority = priority;
		element->callback = callback;
		element->userData = userData;

		SCallbackElement* first = mFirst[type];

		if (first == nullptr)
		{
			mFirst[type] = element;
			mLast[type] = element;
			return;
		}

		if (first->priority < priority)
		{
			element->next = first;
			first->previous = element;
			mFirst[type] = element;

			return;
		}

		SCallbackElement* previous = first;

		// Equal priorities retain registration order.

		while (previous->next != nullptr && previous->next->priority >= priority)
		{
			previous = previous->next;
		}

		element->previous = previous;
		element->next = previous->next;
		previous->next = element;

		if (element->next == nullptr)
		{
			mLast[type] = element;
		}
		else
		{
			element->next->previous = element;
		}
	}

	// Original function: FUN_00411e1d.
	bool CCallbackManager::ExecuteCallback(TCallbackTypes type, bool forward, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3)
	{
		SCallbackElement* element = forward ? mFirst[type] : mLast[type];

		while (element != nullptr)
		{
			if (!element->callback(type, element->userData, param1, param2, param3))
			{
				return false;
			}

			element = forward ? element->next : element->previous;
		}

		return true;
	}

	// Original function: FUN_00411db3
	bool CCallbackManager::UnRegisterCallback(TCallbackTypes type, TCallbackFunction callback, std::uintptr_t userData)
	{
		SCallbackElement* element = l_FindCallbackElement(type, callback, userData);

		if (element == nullptr)
		{
			return false;
		}

		if (element->previous == nullptr)
		{
			mFirst[type] = element->next;
		}
		else
		{
			element->previous->next = element->next;
		}

		if (element->next == nullptr)
		{
			mLast[type] = element->previous;
		}
		else
		{
			element->next->previous = element->previous;
		}

		NXBasics::Runtime_Free(element);

		return true;
	}
}
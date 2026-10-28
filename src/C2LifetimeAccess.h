#pragma once

#include "C2CallbackManager.h"
#include "C2ObjectLayout.h"
#include "XCrtFileAccess.h"
#include "XList.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>

// Mechanical access to original objects whose complete class layouts are still unresolved.
namespace NC2Lifetime
{
	template<class T> T Read(const void* object, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
		return value;
	}

	template<class T> void Write(void* object, std::size_t offset, T value)
	{
		std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
	}

	inline void FreeField(void* object, std::size_t offset)
	{
		NXBasics::Runtime_Free(Read<void*>(object, offset));
	}

	inline void FreeAndClearField(void* object, std::size_t offset)
	{
		FreeField(object, offset);
		Write<void*>(object, offset, nullptr);
	}

	// Original scalar CStorable deleting-destructor slot. Array deletion uses
	// separately reconstructed vector wrappers, not a native scalar vtable slot.
	inline void DeleteStorable(void* object)
	{
		if (object != nullptr)
		{
			using TDelete = void* (__thiscall*)(void*, std::uint32_t);
			Read<TDelete>(Read<void*>(object, NC2Layout::StorableOffset::VirtualTable), NC2Layout::StorableVirtualTableOffset::DeletingDestructor)(object, 1);
		}
	}

	template<class T, class F> void DestroyOwned(T* object, F destruct)
	{
		if (object != nullptr)
		{
			destruct(object);
			NXBasics::Runtime_Free(object);
		}
	}

	inline void Unregister(NC2Logic::TCallbackFunction callback, std::uintptr_t userData, std::initializer_list<std::uint32_t> types)
	{
		for (std::uint32_t type : types)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->UnRegisterCallback(type, callback, userData);
		}
	}

	// Each tag represents a distinct original CList specialization. The default +4
	// payload deleter is Runtime_Free; owning hyperlink payloads specialize it.
	template<class Tag> class CRuntimeOwnedList : public NXBasics::CListBase
	{
	public:
		explicit CRuntimeOwnedList(bool deleteObjects) : CListBase(deleteObjects)
		{}
		static void* Delete(CRuntimeOwnedList* list, std::uint32_t flags);
		~CRuntimeOwnedList() override
		{
			if (mDeleteObjects != 0)
			{
				DeleteAllElements();
			}
		}

	protected:
		void l_DeleteData(void* data) override
		{
			NXBasics::Runtime_Free(data);
		}
	};

	inline void DrainRuntimeList(void* object)
	{
		auto* list = static_cast<NXBasics::CListBase*>(object);
		while (void* data = list->l_Base_GetStartElement())
		{
			list->l_Base_RemoveElement(data);
			NXBasics::Runtime_Free(data);
		}
	}
}

#include "C2EngineDisplayAccess.h"
#include "C2EngineDisplayManager.h"
#include "C2GameMapAccess.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "XCrtFileAccess.h"
#include "XMKeyManager.h"
#include "XNumericConstants.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2E2
{
	// ABI adapter for the typed InformThatStaticMapChanged overload.
	void __fastcall DisplayManager_InformThatStaticMapChanged(void* manager, void* /*unusedEdx*/, const void* rectangle, std::int32_t playerId)
	{
		static_cast<C2DEngineDisplayManager*>(manager)->InformThatStaticMapChanged(static_cast<const std::int16_t*>(rectangle), static_cast<std::uint32_t>(playerId));
	}

	// Original function: FUN_004811ae
	C2DEngineDisplayManager::C2DEngineDisplayManager() : CDisplayObjectList(0), mActiveDisplay(nullptr)
	{
		static_assert(offsetof(C2DEngineDisplayManager, mActiveDisplay) == sizeof(CDisplayObjectList));
		sTheObjectPtr = this;

		// Preserve the initialized data fields without overwriting the C++ vtable pointer.
		mCount = 0;
		mFirst = nullptr;
		mLast = nullptr;
		mDestroyValues = 0;
		std::memset(mPadding11, 0, sizeof(mPadding11));
		mActiveDisplay = nullptr;
	}

	// Original function: FUN_004811e3.
	C2DEngineDisplayManager::~C2DEngineDisplayManager()
	{
		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_004813e8. The base destructor releases the list nodes.
	CDisplayObjectList::~CDisplayObjectList()
	{
		if (mDestroyValues != 0)
		{
			ClearOwnedEntries();
		}
	}

	// Original function: FUN_00481420.
	void* __fastcall CDisplayObjectList::Delete(CDisplayObjectList* list, void* /*unusedEdx*/, std::uint32_t flags)
	{
		list->CDisplayObjectList::~CDisplayObjectList();

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004811ef
	void C2DEngineDisplayManager::RegisterDisplayObject(void* display)
	{
		PushBack(display);

		std::uint8_t active;
		std::memcpy(&active, static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::ActiveDisplayFlag, sizeof(active));

		if (active != 0)
		{
			mActiveDisplay = display;
		}
	}

	// Original function: FUN_0048120b
	void C2DEngineDisplayManager::UnregisterDisplayObject(void* display)
	{
		Remove(display);

		std::uint8_t active;
		std::memcpy(&active, static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::ActiveDisplayFlag, sizeof(active));

		if (active != 0)
		{
			mActiveDisplay = nullptr;
		}
	}

	// Original function: FUN_00481228.
	void C2DEngineDisplayManager::InformThatStaticMapChanged(const std::int16_t* rectangle, std::uint32_t playerId)
	{
		Iterator iterator(*this);

		for (void* display = iterator.First(); display != nullptr; display = iterator.Next())
		{
			std::uint32_t player;
			std::memcpy(&player, static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::PlayerId, sizeof(player));
			const auto* visible = reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::MapRectangle);

			if ((playerId == NXBasics::IntegerRange::UInt32Maximum || playerId == player) && NC2Logic::MapMigRectangle_IsTouching(rectangle, visible))
				static_cast<std::byte*>(display)[NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag] = std::byte{ 1 };
		}
	}

	// Original function: FUN_00481284
	void C2DEngineDisplayManager::InformThatStaticMapChanged()
	{
		Iterator iterator(*this);

		for (void* display = iterator.First(); display != nullptr; display = iterator.Next())
		{
			static_cast<std::byte*>(display)[NC2Layout::EngineDisplayOffset::StaticRebuildRequiredFlag] = std::byte{ 1 };
		}
	}

	// Original function: FUN_004812b4.
	void C2DEngineDisplayManager::InformThatMapXtructed()
	{
		const std::uint32_t center = static_cast<std::uint16_t>(NC2Logic::GameMap_GetWidth() / 2) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(NC2Logic::GameMap_GetHeight() / 2)) << 16);
		Iterator iterator(*this);

		for (void* display = iterator.First(); display != nullptr; display = iterator.Next())
		{
			Display_ReInit(display);
			DE_SetWantedPositionToMapPoint(display, &center);
		}
	}

	// Original function: FUN_00481319.
	void C2DEngineDisplayManager::InformThatControllingPlayerChanged(std::int32_t playerId)
	{
		Iterator iterator(*this);

		for (void* display = iterator.First(); display != nullptr; display = iterator.Next())
		{
			DE_ControllingPlayer_SetId(display, playerId);
		}
	}

	// Original function: FUN_00481385.
	void C2DEngineDisplayManager::InformThatLoadDone()
	{
		Iterator iterator(*this);

		for (void* display = iterator.First(); display != nullptr; display = iterator.Next())
		{
			std::uint8_t enabled;
			void* target;
			std::memcpy(&enabled, static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::PrimaryReferenceEnabled, sizeof(enabled));
			std::memcpy(&target, static_cast<const std::byte*>(display) + NC2Layout::EngineDisplayOffset::PrimaryMarkedObject, sizeof(target));

			if (enabled != 0 && target != nullptr)
			{
				std::memset(target, 0, NC2Layout::RecordSize::DisplaySearchObject);
				DisplayObjectReference_Reset(target);
			}

			const std::uint32_t zero = 0;
			std::memcpy(static_cast<std::byte*>(display) + NC2Layout::EngineDisplayOffset::SecondaryMarkedObjectCount, &zero, sizeof(zero));
		}
	}

	// Original function: FUN_00481405
	void CDisplayObjectList::DestroyValue(void* value)
	{
		if (value == nullptr)
		{

			return;
		}

		Display_Destruct(value);
		NXBasics::Runtime_Free(value);
	}

	// Original function: FUN_0048134e.
	void C2DEngineDisplayManager::InformSetExplorationState(bool enabled)
	{
		Iterator iterator(*this);

		for (void* display = iterator.First(); display != nullptr; display = iterator.Next())
		{
			DE_ToDraw_SetState(display, 4, enabled);
		}
	}
}

// Original function: FUN_0047e669; CE2Manager::Inform_SetExplorationState.
void __cdecl E2Manager_Inform_SetExplorationState(bool enabled)
{
	NC2E2::C2DEngineDisplayManager::sTheObjectPtr->InformSetExplorationState(enabled);
}
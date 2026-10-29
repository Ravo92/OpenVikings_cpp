#include "InGameGuiStorage.h"
#include "C2CallbackManager.h"
#include "C2MapGeometry.h"
#include "InGameGuiOverviewHandler.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstring>
#include <Windows.h>

namespace NC2InGameGuiOverviewDisplay
{
	template <typename T>
	static T Overview_Read(const void* object, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));

		return value;
	}

	template <typename T>
	static void Overview_Write(void* object, std::size_t offset, T value)
	{
		std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
	}

	static std::int16_t Overview_LowSignedWord(std::uint32_t bits)
	{
		const std::uint16_t word = static_cast<std::uint16_t>(bits);
		std::int16_t value;
		std::memcpy(&value, &word, sizeof(value));

		return value;
	}

	// Original function: FUN_004b7f9c; startup initializes only the two points.
	void CWorldOverviewElementHandler::InitializeStaticChangedArea()
	{
		mStaticVars.changedAreaMinimum[1] = 0;
		mStaticVars.changedAreaMinimum[0] = 0;
		mStaticVars.changedAreaMaximum[1] = 0;
		mStaticVars.changedAreaMaximum[0] = 0;
	}

	// Original function: FUN_004b7fb7.
	CWorldOverviewElementHandler::CWorldOverviewElementHandler() : mElements(false)
	{
		sTheObjectPtr = this;
		// Windows clears the entire list object, including its vtable. Keep the
		// native C++ vptr while reproducing the zeroed data fields and non-ownership.
		std::memset(reinterpret_cast<std::byte*>(&mElements) + sizeof(void*), 0, sizeof(mElements) - sizeof(void*));
		std::memset(&mStaticVars, 0, sizeof(mStaticVars));
		constexpr std::uint32_t callbackTypes[] = { 0x31, 0x32, 0x33, 2, 0x0D, 1 };

		for (const std::uint32_t type : callbackTypes)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(type, l_MapOverview_Callback, 0, 0);
		}
	}

	// Original function: FUN_004b8054. The separately instantiated mElements
	// destructor implements FUN_004b84b4 / FUN_004b84d1 using the shared GUI list.
	CWorldOverviewElementHandler::~CWorldOverviewElementHandler()
	{
		constexpr std::uint32_t callbackTypes[] = { 0x31, 0x32, 0x33, 2, 0x0D, 1 };

		for (const std::uint32_t type : callbackTypes)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->UnRegisterCallback(type, l_MapOverview_Callback, 0);
		}

		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_004b80cb.
	bool __cdecl CWorldOverviewElementHandler::l_MapOverview_Callback(std::uint32_t type, std::uintptr_t callbackValueUserData, std::uintptr_t callbackValueParam1, std::uintptr_t callbackValueParam2, std::uintptr_t callbackValueParam3)
	{
		std::uint32_t userData = static_cast<std::uint32_t>(callbackValueUserData);
		std::uint32_t param1 = static_cast<std::uint32_t>(callbackValueParam1);
		std::uint32_t param2 = static_cast<std::uint32_t>(callbackValueParam2);
		std::uint32_t param3 = static_cast<std::uint32_t>(callbackValueParam3);
		(void)userData;
		auto& state = mStaticVars;

		switch (type)
		{
			case SwitchIds::CallbackType::MapPointChanged:
			{
				const std::int16_t x = Overview_LowSignedWord(param2);
				const std::int16_t y = Overview_LowSignedWord(param2 >> 16);

				if (x < state.changedAreaMinimum[0])
					state.changedAreaMinimum[0] = x;

				if (x > state.changedAreaMaximum[0])
					state.changedAreaMaximum[0] = x;

				if (y < state.changedAreaMinimum[1])
					state.changedAreaMinimum[1] = y;

				if (y > state.changedAreaMaximum[1])
					state.changedAreaMaximum[1] = y;

				break;
			}
			case SwitchIds::CallbackType::EndMapChanges:
				if (state.changedAreaMinimum[0] != 32000)
				{
					for (std::int16_t& coordinate : state.changedAreaMinimum)
					{
						coordinate = Overview_LowSignedWord(static_cast<std::uint32_t>(coordinate) - 4u);
					}

					NC2Logic::MapMigPoint_ValidatePosition(state.changedAreaMinimum);

					for (std::int16_t& coordinate : state.changedAreaMaximum)
					{
						coordinate = Overview_LowSignedWord(static_cast<std::uint32_t>(coordinate) + 4u);
					}

					NC2Logic::MapMigPoint_ValidatePosition(state.changedAreaMaximum);
					sTheObjectPtr->System_UpdateAllElements();
				}

				break;
			case SwitchIds::CallbackType::BeginMapChanges:
				state.changedAreaMaximum[1] = 0;
				state.changedAreaMaximum[0] = 0;
				state.changedAreaMinimum[1] = 32000;
				state.changedAreaMinimum[0] = 32000;
				break;
			case SwitchIds::CallbackType::MapAreaChanged:
				state.changedAreaMinimum[1] = Overview_LowSignedWord(param2 >> 16);
				state.changedAreaMaximum[0] = Overview_LowSignedWord(param2 + param3);
				state.changedAreaMaximum[1] = Overview_LowSignedWord((param3 >> 16) + static_cast<std::uint16_t>(state.changedAreaMinimum[1]));
				state.changedAreaMinimum[1] = Overview_LowSignedWord(static_cast<std::uint32_t>(state.changedAreaMinimum[1]) - 4u);
				state.changedAreaMinimum[0] = Overview_LowSignedWord(param2 - 4u);
				NC2Logic::MapMigPoint_ValidatePosition(state.changedAreaMinimum);

				for (std::int16_t& coordinate : state.changedAreaMaximum)
				{
					coordinate = Overview_LowSignedWord(static_cast<std::uint32_t>(coordinate) + 4u);
				}

				NC2Logic::MapMigPoint_ValidatePosition(state.changedAreaMaximum);
				sTheObjectPtr->System_UpdateAllElements();
				sTheObjectPtr->System_RedrawAllElements();
				break;
			case SwitchIds::CallbackType::MapLifetime:
				if (static_cast<std::uint16_t>(param1) != 0)
				{
					state.fullRebuildRequested = true;
					std::memset(state.zoomByView, 0, sizeof(state.zoomByView));
					sTheObjectPtr->System_ScanTextures();
					sTheObjectPtr->System_UpdateAllElements();
					sTheObjectPtr->System_RedrawAllElements();
				}

				break;
			case SwitchIds::CallbackType::GameTick:
				++state.redrawFrameCounter;

				if (NXBasics::Runtime_TimeSigned32(state.redrawFrameCounter) >= 12)
				{
					state.redrawFrameCounter -= 12u;
					sTheObjectPtr->System_RedrawAllElements();
				}

				break;
		}

		return true;
	}

	// Original function: FUN_004b82ac.
	void CWorldOverviewElementHandler::System_RegisterElement(CWorldOverviewBitmapGuiElement* element)
	{
		mElements.InsertAtEnd(element);
	}

	// Original function: FUN_004b82b8.
	void CWorldOverviewElementHandler::System_UnRegisterElement(CWorldOverviewBitmapGuiElement* element)
	{
		mElements.l_Base_RemoveElement(element);
	}

	// Original function: FUN_004b82c4.
	void CWorldOverviewElementHandler::System_UpdateAllElements()
	{
		NXBasics::CListBase::Iterator iterator(mElements);

		for (void* element = iterator.First(); element != nullptr; element = iterator.Next())
		{
			if (mStaticVars.fullRebuildRequested)
			{
				Overview_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 0, 1);
			}
			else
			{
				WorldOverviewElement_UpdateArea(element, mStaticVars.changedAreaMinimum, mStaticVars.changedAreaMaximum);
			}
		}

		mStaticVars.fullRebuildRequested = false;
	}

	// Original function: FUN_004b8317.
	void WorldOverviewElement_UpdateArea(void* element, const std::int16_t* minimum, const std::int16_t* maximum)
	{
		if (Overview_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 8) != 0)
		{
			if (minimum[0] < Overview_Read<std::int16_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 10))
				Overview_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 10, minimum[0]);

			if (maximum[0] > Overview_Read<std::int16_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 14))
				Overview_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 14, maximum[0]);

			if (minimum[1] < Overview_Read<std::int16_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 12))
				Overview_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 12, minimum[1]);

			if (maximum[1] > Overview_Read<std::int16_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 16))
				Overview_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 16, maximum[1]);
		}
		else
		{
			std::memcpy(static_cast<std::byte*>(element) + offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 10, minimum, 4);
			std::memcpy(static_cast<std::byte*>(element) + offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 14, maximum, 4);
		}

		Overview_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 8, 1);
	}

	// Original function: FUN_004b83a6.
	void CWorldOverviewElementHandler::System_ControllingPlayerChanged(std::int32_t playerId)
	{
		NXBasics::CListBase::Iterator iterator(mElements);

		for (void* element = iterator.First(); element != nullptr; element = iterator.Next())
		{
			Overview_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 4, playerId);
			Overview_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 0, 1);
		}
	}

	// Original function: FUN_004b83e1.
	void CWorldOverviewElementHandler::System_SetExplorationState(bool enabled)
	{
		NXBasics::CListBase::Iterator iterator(mElements);

		for (void* element = iterator.First(); element != nullptr; element = iterator.Next())
		{
			Overview_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 45, static_cast<std::uint8_t>(enabled));
			Overview_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 0, 1);
		}
	}

	// Original function: FUN_004b841c.
	void CWorldOverviewElementHandler::System_ScanTextures()
	{
		NXBasics::CListBase::Iterator iterator(mElements);

		for (void* element = iterator.First(); element != nullptr; element = iterator.Next())
		{
			Overview_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 9, 1);
		}
	}

	// Original function: FUN_004b844c.
	void CWorldOverviewElementHandler::System_RedrawAllElements()
	{
		NXBasics::CListBase::Iterator iterator(mElements);

		for (void* element = iterator.First(); element != nullptr; element = iterator.Next())
		{
			Overview_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 1, 1);
		}
	}

	// Original function: FUN_004b847c.
	void CWorldOverviewElementHandler::Marker_AddInfoMarker(std::int32_t playerId, const std::uint32_t* packedMapPoint)
	{
		NXBasics::CListBase::Iterator iterator(mElements);

		for (void* element = iterator.First(); element != nullptr; element = iterator.Next())
		{
			WorldOverviewElement_MarkerAddInfo(element, playerId, packedMapPoint);
		}
	}

	// Original function: FUN_004cfc94; l_Marker_Add.
	static void WorldOverviewElement_MarkerAdd(void* element, std::uint32_t markerType, const std::uint32_t* packedMapPoint)
	{
		std::uint32_t selected = 0;
		std::uint32_t oldest = NXBasics::IntegerRange::UInt32Maximum;

		for (std::uint32_t index = 0; index < 20; ++index)
		{
			const std::uint32_t timestamp = Overview_Read<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 56 + index * 12u);

			if (timestamp == 0)
			{
				selected = index;
				break;
			}

			if (timestamp < oldest)
			{
				oldest = timestamp;
				selected = index;
			}
		}

		Overview_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 60 + selected * 12u, markerType);
		Overview_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 64 + selected * 12u, *packedMapPoint);
		Overview_Write<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 56 + selected * 12u, timeGetTime());
	}

	// Original function: FUN_004cfc7a; Marker_AddInfoMarker.
	void WorldOverviewElement_MarkerAddInfo(void* element, std::int32_t playerId, const std::uint32_t* packedMapPoint)
	{
		if (playerId == Overview_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 4))
			WorldOverviewElement_MarkerAdd(element, 0, packedMapPoint);
	}

}
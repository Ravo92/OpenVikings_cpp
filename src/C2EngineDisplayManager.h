#pragma once

#include "XMKeyManager.h"

namespace NC2E2
{
	class CDisplayObjectList : public NXSysKeyManager::CKeyEntryListBase
	{
	public:
		explicit CDisplayObjectList(std::uint8_t destroyValues) : CKeyEntryListBase(destroyValues)
		{}
		~CDisplayObjectList() override;
		static void* __fastcall Delete(CDisplayObjectList* list, void* unusedEdx, std::uint32_t flags);

	protected:
		void DestroyValue(void* value) override;
	};

	static_assert(sizeof(CDisplayObjectList) == sizeof(NXSysKeyManager::CKeyEntryListBase));

	class C2DEngineDisplayManager : public CDisplayObjectList
	{
	public:
		C2DEngineDisplayManager();
		~C2DEngineDisplayManager() override;

		void RegisterDisplayObject(void* display);
		void UnregisterDisplayObject(void* display);
		void InformThatStaticMapChanged();
		void InformThatStaticMapChanged(const std::int16_t* rectangle, std::uint32_t playerId);
		void InformThatMapXtructed();
		void InformThatLoadDone();
		void InformThatControllingPlayerChanged(std::int32_t playerId);
		void InformSetExplorationState(bool enabled);
		void* GetActiveDisplay() const
		{
			return mActiveDisplay;
		}

		// Original global: 00555114.
		inline static C2DEngineDisplayManager* sTheObjectPtr = nullptr;

	private:
		void* mActiveDisplay;
	};

	static_assert(sizeof(C2DEngineDisplayManager) == (sizeof(void*) == 8 ? 48 : 24));
}

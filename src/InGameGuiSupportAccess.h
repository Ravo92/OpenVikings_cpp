#pragma once

#include "XGuiStorage.h"
#include "C2SavegameFormats.h"

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	class CIoHelper;
}

namespace NC2InGameGuiInputManager
{
	class CInputManager
	{
	  public:
		CInputManager();
		~CInputManager();

	  private:
		alignas(4) std::byte mStorage[3964];
	};

	static_assert(sizeof(CInputManager) == 3964);
}

namespace NC2InGameGuiMessages
{
	inline void* gMessageManager = nullptr; // 00569408

	// Physical Windows message envelopes. Displayed and history prefixes have different meanings.
	struct SGuiDisplayedMessageStorage
	{
		std::uint32_t createdTick;
		std::uint32_t uniqueId;
		std::byte message[772];
		void* miniButton;
	};

	struct SGuiMessageHistoryStorage
	{
		std::uint8_t allocated;
		std::byte unresolved01[3];
		std::uint32_t createdTick;
		std::byte message[772];
	};

	struct SGuiMessageManagerStorage
	{
		std::uint32_t displayedCount;
		std::uint32_t nextMessageId;
		std::uint32_t minimumPriority;
		void* presentationWindow;
		SGuiDisplayedMessageStorage* displayedMessages;
		SGuiMessageHistoryStorage* historyMessages;
	};

	class CMessageManager
	{
	  public:
		CMessageManager();
		~CMessageManager();
		CMessageManager(const CMessageManager&) = delete;
		CMessageManager& operator=(const CMessageManager&) = delete;
		bool Message_Add(const void* message);
		void Message_Remove(std::uint32_t uniqueId, bool addToHistory);
		void Message_RemoveHumanMessage(std::uint32_t humanId, std::uint32_t type);
		void Message_RemoveAll(bool addToHistory);
		void* Message_GetPtr(std::uint32_t uniqueId) const;
		void MiniButton_Removed(std::uint32_t uniqueId);
		void PresentationWindow_Open(std::uint32_t uniqueId);
		void PresentationWindow_Closed();
		void Message_RemoveAllForHuman(std::uint32_t humanId);
		void System_Update();
		void MessagePriority_Toggle();
		void System_HumanArrayDestructed();
		void IO_Savegame_Save(NC2Logic::CIoHelper* io);
		void IO_Savegame_Load(NC2Logic::CIoHelper* io);
		void MessagePriority_Set(std::uint32_t priority);
		static std::int32_t MessagePriority_Calculate(std::uint32_t type, std::uint32_t job);
		static bool __cdecl ls_Callback_Logic(std::uint32_t type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);

	  private:
		void l_GuiElements_UpdateMiniButtons();
		void l_MessageHistory_Add(const void* message);
		bool l_MessageHistory_IsSameInList(const void* message) const;
		bool l_MessageHistory_IsSimmilarInList(const void* message) const;
		alignas(SGuiMessageManagerStorage) std::byte mStorage[sizeof(SGuiMessageManagerStorage)];
	};

	static_assert(sizeof(CMessageManager) == (sizeof(void*) == 8 ? 40 : 24));
	static_assert(sizeof(SGuiMessageManagerStorage) == sizeof(CMessageManager));
	static_assert(offsetof(SGuiMessageManagerStorage, presentationWindow) == (sizeof(void*) == 8 ? 16 : 12));
	static_assert(offsetof(SGuiMessageManagerStorage, displayedMessages) == (sizeof(void*) == 8 ? 24 : 16));
	static_assert(offsetof(SGuiMessageManagerStorage, historyMessages) == (sizeof(void*) == 8 ? 32 : 20));
	static_assert(offsetof(SGuiDisplayedMessageStorage, miniButton) == (sizeof(void*) == 8 ? 784 : 780));
	static_assert(sizeof(SGuiDisplayedMessageStorage) == (sizeof(void*) == 8 ? 792 : 784));
	static_assert(sizeof(SGuiMessageHistoryStorage) == 780);
	static_assert(offsetof(SGuiDisplayedMessageStorage, message) + sizeof(SGuiDisplayedMessageStorage::message) == NC2SaveFormat::GuiMessageBytes);

	struct SMessageMiniButtonStorage
	{
		NC2InGameGuiBase::SBaseToolElementStorage base;
		std::uint32_t messageId;
		char tooltip[1000];
	};

	static_assert(sizeof(SMessageMiniButtonStorage) == (sizeof(void*) == 8 ? 1080 : 1052));
	std::uint32_t SingleMessageDisplayed_GetValidHumanId(const void* record);
	bool SingleMessageDisplayed_GetMapPosition(const void* record, std::uint32_t* point);
	void* MessageMiniButton_Construct(void* button, std::uint32_t uniqueId, std::int32_t x);
	void Tool_CreateMessageString(const void* message, char* output, std::uint32_t capacity);
}

namespace NC2InGameGuiGroups
{
	// Physical Windows group storage; the four-byte manager prefix remains unresolved.
	struct SObjectGroupObjectStorage
	{
		std::uint32_t type, objectId, uniqueId;
	};

	struct SObjectGroupStorage
	{
		std::uint32_t count;
		SObjectGroupObjectStorage objects[100];
	};

	struct SObjectGroupManagerStorage
	{
		std::byte unresolvedPrefix[4];
		SObjectGroupStorage groups[10];
	};

	inline void* gObjectGroupManager = nullptr; // 005690D8

	class CObjectGroupManager
	{
	  public:
		CObjectGroupManager();
		~CObjectGroupManager();
		std::int32_t Misc_GetGroupIdOfHuman(std::uint32_t humanId) const;
		std::int32_t Misc_GetGroupIdOfVehicle(std::uint32_t vehicleId) const;
		std::int32_t Misc_GetGroupIdOfHouse(std::uint32_t houseId) const;
		void Group_VerifyAllObjects();
		void Group_CreateNewGroup(std::uint32_t index);
		void Group_AddSelectionToGroup(std::uint32_t index);
		void Group_Choose(std::uint32_t index);
		void IO_Savegame_Save(NC2Logic::CIoHelper* io);
		void IO_Savegame_Load(NC2Logic::CIoHelper* io);
		static bool __cdecl ls_Callback_Logic(std::uint32_t type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);

	  private:
		void l_Group_VerifyAllObjects(void* group);
		bool l_Group_IsObjectValid(const void* object) const;
		void l_Group_AddHuman(void* group, std::uint32_t id);
		void l_Group_AddVehicle(void* group, std::uint32_t id);
		void l_Group_AddHouse(void* group, std::uint32_t id);
		bool l_Group_IsSelected(const void* group) const;
		bool l_Group_GetPosition(const void* group, std::uint32_t* point) const;
		void l_Group_RemoveObject(void* group, std::uint32_t index);
		void l_Group_RemoveHuman(void* group, std::uint32_t id);
		void l_Group_RemoveVehicle(void* group, std::uint32_t id);
		void l_Group_RemoveHouse(void* group, std::uint32_t id);
		alignas(4) std::byte mStorage[12044];
	};

	static_assert(sizeof(CObjectGroupManager) == 12044);
}

namespace NC2InGameGuiDeadHumanDataManager
{
	// Owned ring records; the original save format omits the manager's pointer.
	struct SDeadHumanRecordStorage
	{
		std::uint32_t humanUniqueId;
		char name[40];
		std::byte unresolved2C[4];
		std::uint32_t deathTick, jobType, mapPosition;
	};

	struct SDeadHumanManagerStorage
	{
		std::uint32_t count, nextRecordIndex;
		SDeadHumanRecordStorage* records;
	};

	inline void* gDeadHumanDataManager = nullptr; // 00569404

	class CDeadHumanDataManager
	{
	  public:
		CDeadHumanDataManager();
		~CDeadHumanDataManager();
		void DeadHuman_Add(std::uint32_t humanId);
		void* DeadHuman_GetDataPtrByHumanUniqueId(std::uint32_t uniqueId) const;
		void System_HumanArrayDestructed();
		void IO_Savegame_Save(NC2Logic::CIoHelper* io);
		void IO_Savegame_Load(NC2Logic::CIoHelper* io);
		static bool __cdecl ls_Callback_Logic(std::uint32_t type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);

	  private:
		alignas(SDeadHumanManagerStorage) std::byte mStorage[sizeof(SDeadHumanManagerStorage)];
	};

	static_assert(sizeof(CDeadHumanDataManager) == (sizeof(void*) == 8 ? 16 : 12));
	static_assert(offsetof(SDeadHumanManagerStorage, records) == 8);
	static_assert(sizeof(SDeadHumanRecordStorage) == 60);
}
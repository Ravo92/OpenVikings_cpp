#include "C2CallbackManager.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2UserMessageAccess.h"
#include "InGameGuiSupportAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include <cstring>
#include <initializer_list>

namespace
{
	using namespace NC2Lifetime;

	void RegisterCallbacks(NC2Logic::TCallbackFunction callback, std::initializer_list<std::uint32_t> types, std::int32_t priority)
	{
		for (auto type : types)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(type, callback, 0, priority);
		}
	}

	void UnregisterCallbacks(NC2Logic::TCallbackFunction callback, std::initializer_list<std::uint32_t> types)
	{
		for (auto type : types)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->UnRegisterCallback(type, callback, 0);
		}
	}
}

namespace NC2InGameGuiMessages
{
	// Original function: FUN_004b3997.
	CMessageManager::CMessageManager()
	{
		gMessageManager = this;
		std::memset(this, 0, NC2Layout::RecordSize::GuiMessageStatePrefix);
		std::memset(mStorage + NC2Layout::GuiMessageManagerOffset::PresentationWindow, 0, NC2Layout::RecordSize::GuiMessagePointerRegion);
		RegisterCallbacks(ls_Callback_Logic, { SwitchIds::CallbackType::GameTick, SwitchIds::CallbackType::LoadDone, SwitchIds::CallbackType::HumanLifetime, SwitchIds::CallbackType::HumanManagerLifetime, 0x2E }, NC2Runtime::GuiCallbackPriority::StateManagers);
		Write<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::NextMessageId, 1);
		Write<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::MinimumPriority, 0);
		auto* active = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::RecordSize::GuiDisplayedMessagePool, 1));

		if (active != nullptr)
			for (std::uint32_t index = 0; index < NC2Runtime::GuiMessageLimits::Capacity; ++index)
			{
				NC2Logic::ToUserMessage_Construct(active + NC2Layout::GuiMessageEnvelopeOffset::MessageUnsigned + index * NC2Layout::RecordSize::GuiDisplayedMessageUnsigned);
			}

		Write(this, NC2Layout::GuiMessageManagerOffset::DisplayedMessages, active);
		std::memset(active, 0, NC2Layout::RecordSize::GuiDisplayedMessagePool);
		auto* pending = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::RecordSize::GuiMessageHistoryPool, 1));

		if (pending != nullptr)
			for (std::uint32_t index = 0; index < NC2Runtime::GuiMessageLimits::Capacity; ++index)
			{
				NC2Logic::ToUserMessage_Construct(pending + NC2Layout::GuiMessageEnvelopeOffset::MessageUnsigned + index * NC2Layout::RecordSize::GuiMessageHistoryUnsigned);
			}

		Write(this, NC2Layout::GuiMessageManagerOffset::HistoryMessages, pending);
		std::memset(pending, 0, NC2Layout::RecordSize::GuiMessageHistoryPool);
	}

	// Original function: FUN_004b3abd.
	CMessageManager::~CMessageManager()
	{
		NXBasics::Runtime_Free(Read<void*>(this, NC2Layout::GuiMessageManagerOffset::HistoryMessages));
		NXBasics::Runtime_Free(Read<void*>(this, NC2Layout::GuiMessageManagerOffset::DisplayedMessages));
		UnregisterCallbacks(ls_Callback_Logic, { 0x2E, SwitchIds::CallbackType::HumanManagerLifetime, SwitchIds::CallbackType::HumanLifetime, SwitchIds::CallbackType::LoadDone, SwitchIds::CallbackType::GameTick });
		gMessageManager = nullptr;
	}
}

namespace NC2InGameGuiMessages
{
	using namespace NC2Lifetime;

	// Original function: FUN_004b3b2e; mini-button pointers are not serialized.
	void CMessageManager::IO_Savegame_Save(NC2Logic::CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::GuiSavegameChunk::Messages, 2);
		io->IO_Memory_WriteBinary(this, 8);

		for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount); ++index)
		{
			io->IO_Memory_WriteBinary(Read<std::byte*>(this, NC2Layout::GuiMessageManagerOffset::DisplayedMessages) + index * NC2Layout::RecordSize::GuiDisplayedMessageUnsigned, NC2Layout::RecordSize::GuiDisplayedMessageSerializedPrefix);
		}

		io->IO_Memory_WriteBinary(Read<void*>(this, NC2Layout::GuiMessageManagerOffset::HistoryMessages), NC2Layout::RecordSize::GuiMessageHistoryPool);
		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_004b3b92; discard current mini-buttons before replacing records.
	void CMessageManager::IO_Savegame_Load(NC2Logic::CIoHelper* io)
	{
		while (Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount) != 0)
			Message_Remove(Read<std::uint32_t>(Read<void*>(this, NC2Layout::GuiMessageManagerOffset::DisplayedMessages), NC2Layout::GuiDisplayedMessageOffset::UniqueId), false);

		if (io->mReadHeader.marker != SwitchIds::GuiSavegameChunk::Messages || io->mReadHeader.version != 2)
		{
			io->IO_File_Chunk_Skip();

			return;
		}

		io->IO_File_ReadBinary(this, 8);

		for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount); ++index)
		{
			auto* record = Read<std::byte*>(this, NC2Layout::GuiMessageManagerOffset::DisplayedMessages) + index * NC2Layout::RecordSize::GuiDisplayedMessageUnsigned;
			io->IO_File_ReadBinary(record, NC2Layout::RecordSize::GuiDisplayedMessageSerializedPrefix);
			Write<void*>(record, NC2Layout::GuiDisplayedMessageOffset::MiniButton, nullptr);
		}

		io->IO_File_ReadBinary(Read<void*>(this, NC2Layout::GuiMessageManagerOffset::HistoryMessages), NC2Layout::RecordSize::GuiMessageHistoryPool);
	}

	// Original function: FUN_004b4481.
	void CMessageManager::MessagePriority_Set(std::uint32_t priority)
	{
		Write(this, NC2Layout::GuiMessageManagerOffset::MinimumPriority, priority);
		System_Update();
	}
}
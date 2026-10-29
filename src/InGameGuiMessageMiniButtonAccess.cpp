#include "C2EngineDrawingAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "GuiClockToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiSupportAccess.h"
#include "SwitchIdentifiers.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XGuiElementAccess.h"
#include "XMKeyManager.h"
#include "XNumericConstants.h"

#include <cstring>

namespace NC2InGameGuiMessages
{

	namespace
	{
		using namespace NC2Lifetime;
		using namespace NXBaseGui;
		extern const std::uintptr_t gMessageMiniButtonVirtualTable[18];

		// Original function: FUN_004d10e4.
		void MessageMiniButton_Destruct(void* button)
		{
			Write(button, NC2Layout::GuiBaseElementOffset::VirtualTable, gMessageMiniButtonVirtualTable + 0);
			static_cast<CMessageManager*>(gMessageManager)->MiniButton_Removed(Read<std::uint32_t>(button, NC2Layout::MessageMiniButtonOffset::MessageId));
			Write(button, NC2Layout::GuiBaseElementOffset::VirtualTable, NC2InGameGuiBase::gBaseToolElementVirtualTable + 0);
			BaseElement_Destruct(button);
		}

		// Original function: FUN_004d10c8.
		void* __fastcall MessageMiniButton_Delete(void* button, void*, std::uint32_t flags)
		{
			MessageMiniButton_Destruct(button);
			if ((flags & 1u) != 0) NXBasics::Runtime_Free(button);
			return button;
		}

		// Original function: FUN_004d110a; Ctrl removes all messages, otherwise only this one.
		bool __fastcall MessageMiniButton_MessageHandle(void* button, void*, std::uint32_t message, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
		{
			if (message != 0x20) return false;
			auto* manager = static_cast<CMessageManager*>(gMessageManager);
			if ((Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags) & 2u) != 0) manager->Message_RemoveAll(true);
			else manager->Message_Remove(Read<std::uint32_t>(button, NC2Layout::MessageMiniButtonOffset::MessageId), true);
			return true;
		}

		// Original function: FUN_004d12de.
		void MessageMiniButton_IconPrint(void*, NXBasics::CBitmap* bitmap, std::uint32_t icon)
		{
			const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
			if (NXBasics::BobManager_GetBobType(resources.windowBobManager, icon) == 4) NXBasics::BobManager_PrintBob_UsingShadedAlpha(resources.windowBobManager, icon, bitmap, 0, 0, resources.iconsLeftPalette, 0x80);
			else NXBasics::BobManager_PrintBob(resources.windowBobManager, icon, bitmap, 0, 0, resources.iconsLeftPalette);
		}

		// Original function: FUN_004d1152; rectangle parameter is unused by Windows.
		void __fastcall MessageMiniButton_Draw(void* button, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle*)
		{
			const auto* record = static_cast<CMessageManager*>(gMessageManager)->Message_GetPtr(Read<std::uint32_t>(button, NC2Layout::MessageMiniButtonOffset::MessageId));
			std::uint32_t icon = 0x43;
			if (record != nullptr)
			{
				std::uint32_t job = 0;
				if (SingleMessageDisplayed_GetValidHumanId(record) != NXBasics::IntegerRange::UInt32Maximum) job = Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars, SingleMessageDisplayed_GetValidHumanId(record) * NC2Logic::CHumanArray::ElementSize + NC2Layout::HumanOffset::JobType);
				const auto priority = CMessageManager::MessagePriority_Calculate(Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::Type), job);
				if (priority == 1) icon = 0x44;
				else if (priority == 2) icon = 0x45;
			}
			MessageMiniButton_IconPrint(button, bitmap, icon);
			if (record == nullptr) return;
			const auto human = Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::HumanId);
			if (human != NXBasics::IntegerRange::UInt32Maximum)
			{
				if (Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::Type) == 0x2E) MessageMiniButton_IconPrint(button, bitmap, 0x5A);
				else NC2E2::DrawAtom_MapMoveable_DoDraw(Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars, human * NC2Logic::CHumanArray::ElementSize + NC2Layout::HumanOffset::MapMoveableId), bitmap, 0x1B, 0x33, nullptr, true, -1);
				return;
			}
			const auto vehicle = Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::VehicleId);
			if (vehicle != NXBasics::IntegerRange::UInt32Maximum)
			{
				switch (Read<std::int32_t>(NC2Logic::CVehicleArray::mStaticVars, vehicle * NC2Logic::CVehicleArray::ElementSize + NC2Layout::VehicleOffset::LogicType))
				{
					case SwitchIds::VehicleType::Handcart: MessageMiniButton_IconPrint(button, bitmap, 0x5D); return;
					case SwitchIds::VehicleType::Oxcart: case SwitchIds::VehicleType::OxcartFrame: MessageMiniButton_IconPrint(button, bitmap, 0x5E); return;
					case SwitchIds::VehicleType::SmallShip: case SwitchIds::VehicleType::LargeShip: MessageMiniButton_IconPrint(button, bitmap, 0x5C); return;
					case SwitchIds::VehicleType::Catapult: MessageMiniButton_IconPrint(button, bitmap, 0x5F); return;
				}
			}
			switch (Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::Type))
			{
				case SwitchIds::UserMessage::HouseFinished: case SwitchIds::UserMessage::HouseUpgradeFinished: MessageMiniButton_IconPrint(button, bitmap, 0x59); break;
				case SwitchIds::UserMessage::HouseAttacked: MessageMiniButton_IconPrint(button, bitmap, 0x5A); break;
				case SwitchIds::UserMessage::HeroDied: case SwitchIds::UserMessage::PlayerDied: MessageMiniButton_IconPrint(button, bitmap, 0x57); break;
				case SwitchIds::UserMessage::PlayerSeen: case SwitchIds::UserMessage::DiplomacyChanged: MessageMiniButton_IconPrint(button, bitmap, 0x58); break;
				case SwitchIds::UserMessage::SpecialItemFound: MessageMiniButton_IconPrint(button, bitmap, 0x5B); break;
			}
		}

		// Original function: FUN_004d1291.
		void __fastcall MessageMiniButton_DoPressedAction(void* button)
		{
			static_cast<CMessageManager*>(gMessageManager)->PresentationWindow_Open(Read<std::uint32_t>(button, NC2Layout::MessageMiniButtonOffset::MessageId));
			NC2GuiToolsBase::FXTool_ClickConfirm();
		}

		// Original function: FUN_004d12a4; failed lookup leaves the output reference untouched.
		bool __fastcall MessageMiniButton_ToolTipAvailable(void* button, void*, const char** output)
		{
			const auto* record = static_cast<CMessageManager*>(gMessageManager)->Message_GetPtr(Read<std::uint32_t>(button, NC2Layout::MessageMiniButtonOffset::MessageId));
			if (record == nullptr) return false;
			auto* text = static_cast<char*>(button) + offsetof(SMessageMiniButtonStorage, tooltip);
			Tool_CreateMessageString(static_cast<const std::byte*>(record) + 8, text, 1000);
			*output = text;
			return true;
		}

		// Original function: FUN_004bf2f4; shared unconditional-true GUI virtual entry.
		bool __fastcall GuiElement_ReturnTrue(void*)
		{
			return true;
		}

		// Complete Windows miniature-button vtable at 004f7cbc..004f7d03.
		const std::uintptr_t gMessageMiniButtonVirtualTable[18] = {
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&MessageMiniButton_Delete>()),
			reinterpret_cast<std::uintptr_t>(&BaseElement_HasSubElements),
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&MessageMiniButton_MessageHandle>()),
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&MessageMiniButton_Draw>()),
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_Activate>()),
			reinterpret_cast<std::uintptr_t>(&BaseElement_DeActivate),
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_ButtonPressed>()),
			reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonReleased),
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_IsHit>()),
			reinterpret_cast<std::uintptr_t>(&BaseElement_GetRealActiveElementPtr),
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_FindElementOnPosition>()),
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_ParentToLocalPosition>()),
			reinterpret_cast<std::uintptr_t>(&BaseElement_Notify),
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&MessageMiniButton_ToolTipAvailable>()),
			reinterpret_cast<std::uintptr_t>(&GuiElement_ReturnTrue),
			reinterpret_cast<std::uintptr_t>(&GuiElement_ReturnTrue),
			reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_UnresolvedVirtual40>()),
			reinterpret_cast<std::uintptr_t>(&MessageMiniButton_DoPressedAction)
		};
	}

	// Original function: FUN_004d107b; complete 041ch-byte CMessageMiniButton construction.
	void* MessageMiniButton_Construct(void* button, std::uint32_t uniqueId, std::int32_t x)
	{
		const NXBasics::SRectangle rectangle(x, 0, 0x36, 0x45);
		NC2InGameGuiBase::BaseToolElement_Construct(button, &rectangle, 2, 0x20);
		NC2Lifetime::Write(button, NC2Layout::GuiBaseElementOffset::VirtualTable, gMessageMiniButtonVirtualTable + 0);
		std::memset(static_cast<std::byte*>(button) + offsetof(SMessageMiniButtonStorage, messageId), 0, sizeof(std::uint32_t) + 1000);
		NC2Lifetime::Write(button, NC2Layout::MessageMiniButtonOffset::MessageId, uniqueId);
		return button;
	}
}

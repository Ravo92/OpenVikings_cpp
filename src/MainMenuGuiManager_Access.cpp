#include "C2EngineDisplayAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiToolsBase.h"
#include "MainMenuGuiManager.h"
#include "MainMenuGuiManager_Access.h"
#include "MainMenuWorldDisplayElement.h"
#include "SwitchIdentifiers.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XStringMemory.h"

#include <cstring>
#include <new>


// Genuine message dispatch, world drawing and tooltip drawing remain separate families.
extern bool __fastcall MainMenuGuiManagerDefaultMessageHandlerElement_XGui_BE_Message_Handle(void*, void* unusedEdx, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
extern bool __fastcall MainMenuGuiManagerPrimaryMessageHandlerElement_XGui_BE_Message_Handle(void*, void* unusedEdx, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
extern void __fastcall MainMenuGuiManagerMousePointer_DrawMouse(void*, void* unusedEdx, NXBasics::CBitmap*, const std::int32_t*);
extern void __cdecl E2Manager_Inform_SetExplorationState(bool);
extern void __fastcall Display_DE_MapMoveableToGlobalWorldPixelCoordinates(void*, void* unusedEdx, std::uint32_t, std::int32_t*);

namespace NC2MainMenuGuiManager
{
	using namespace NC2Lifetime;
	static void* __fastcall DefaultMessageHandler_Delete(void*, void* unusedEdx, std::uint32_t);
	static void* __fastcall PrimaryMessageHandler_Delete(void*, void* unusedEdx, std::uint32_t);
	static void* __fastcall MousePointer_Delete(void*, void* unusedEdx, std::uint32_t);
	static const std::uintptr_t gDefaultMessageHandlerVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(&DefaultMessageHandler_Delete),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&MainMenuGuiManagerDefaultMessageHandlerElement_XGui_BE_Message_Handle),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Draw),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Activate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_UnresolvedVirtual40),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify)
	};
	static const std::uintptr_t gPrimaryMessageHandlerVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(&PrimaryMessageHandler_Delete),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&MainMenuGuiManagerPrimaryMessageHandlerElement_XGui_BE_Message_Handle),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Draw),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Activate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_UnresolvedVirtual40),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify)
	};

	static const std::uintptr_t gMousePointerVirtualTable[2] = {
		reinterpret_cast<std::uintptr_t>(&MainMenuGuiManagerMousePointer_DrawMouse),
		reinterpret_cast<std::uintptr_t>(&MousePointer_Delete)
	};

	// Original function: FUN_004d6b8f.
	void* DefaultMessageHandler_Construct(void* element)
	{
		const NXBasics::SRectangle rectangle(0, 0, 0, 0);
		NXBaseGui::BaseElement_Construct(element, &rectangle, 0, 0);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gDefaultMessageHandlerVirtualTable + 0);
		return element;
	}

	// Shared default-message-handler teardown used by the manager and deleting destructor.
	void DefaultMessageHandler_Destruct(void* element)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gDefaultMessageHandlerVirtualTable + 0);
		NXBaseGui::BaseElement_Destruct(element);
	}

	// Original function: FUN_004d33a4.
	static void* __fastcall DefaultMessageHandler_Delete(void* element, void* unusedEdx, std::uint32_t flags)
	{
		(void) unusedEdx;
		DefaultMessageHandler_Destruct(element);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(element);
		return element;
	}

	// Original function: FUN_004db0a0.
	void* PrimaryMessageHandler_Construct(void* element)
	{
		const NXBasics::SRectangle rectangle(0, 0, 0, 0);
		NXBaseGui::BaseElement_Construct(element, &rectangle, 4, 0x200);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gPrimaryMessageHandlerVirtualTable + 0);
		return element;
	}

	// Original function: FUN_004db407.
	static void* __fastcall PrimaryMessageHandler_Delete(void* element, void* unusedEdx, std::uint32_t flags)
	{
		(void) unusedEdx;
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gPrimaryMessageHandlerVirtualTable + 0);
		NXBaseGui::BaseElement_Destruct(element);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(element);
		return element;
	}

	// Original function: FUN_004dae2f.
	void* MousePointer_Construct(void* pointer)
	{
		Write(pointer, NC2Layout::GuiMousePointerOffset::MethodTable, gMousePointerVirtualTable + 0);
		std::memset(static_cast<std::byte*>(pointer) + NC2Layout::GuiMousePointerOffset::TooltipDeadlineTick, 0, NC2Layout::RecordSize::GuiMousePointerVariables);
		return pointer;
	}

	// Original function: FUN_004dae4c; draw is slot zero, deletion is slot one.
	static void* __fastcall MousePointer_Delete(void* pointer, void* unusedEdx, std::uint32_t flags)
	{
		(void) unusedEdx;
		Write(pointer, NC2Layout::GuiMousePointerOffset::MethodTable, gMousePointerVirtualTable + 0);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(pointer);
		return pointer;
	}
}

namespace NC2MainMenuGuiWorldElement
{
	using namespace NC2Lifetime;
	static const std::uintptr_t gWorldDisplayElementVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(&MainMenuWorldDisplayElement_Delete),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&MainMenuWorldDisplayElement_XGui_BE_Message_Handle),
		reinterpret_cast<std::uintptr_t>(&MainMenuWorldDisplayElement_XGui_BE_Element_Draw),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Activate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_UnresolvedVirtual40),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify)
	};

	// Original function: FUN_004da53a.
	void* WorldDisplayElement_Construct(void* element, const void* rectangle, bool active)
	{
		auto* desktopBitmap = NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap();
		NC2InGameGuiBase::BaseBitmapToolElement_Construct(element, static_cast<const NXBasics::SRectangle*>(rectangle), 1, 0x1E3, Read<std::int32_t>(desktopBitmap, NC2Layout::BitmapStorageOffset::Depth));
		auto* presentation = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay();
		if (active && presentation->RegisterMainWorld(element))
		{
			delete Read<NXBasics::CBitmap*>(element, NC2Layout::MenuWorldElementOffset::Bitmap);
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CBitmap));
			auto* nativeBitmap = memory != nullptr ? ::new(memory) NXBasics::CBitmap(presentation->GetWidth(), presentation->GetHeight(), Read<std::int32_t>(desktopBitmap, NC2Layout::BitmapStorageOffset::Depth)) : nullptr;
			Write(element, NC2Layout::MenuWorldElementOffset::Bitmap, nativeBitmap);
			presentation->SetMainWorldBitmap(nativeBitmap);
		}
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldDisplayElementVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + 0x30, 0, 40);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::EngineDisplay, 1);
		void* display = nullptr;
		if (allocation != nullptr)
		{
			auto* font = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08;
			auto* bitmap = Read<NXBasics::CBitmap*>(element, NC2Layout::MenuWorldElementOffset::Bitmap);
			display = NC2E2::Display_Construct(allocation, bitmap, active, font);
		}
		Write(element, NC2Layout::MenuWorldElementOffset::Display, display);
		NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr->ScreenGuiElement_FrameCall_Register(element);
		NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr->Exploration_Set(false);
		MainMenuWorldDisplayElement_l_Tool_DarkenBitmap_Init(element);
		return element;
	}

	// Original function: FUN_004da5db; release resources before the bitmap base.
	void WorldDisplayElement_Destruct(void* element)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldDisplayElementVirtualTable + 0);
		WorldDisplayElement_l_Tool_DarkenBitmap_Exit(element);
		NC2MainMenuGuiManager::CGuiManager::sTheObjectPtr->ScreenGuiElement_FrameCall_UnRegister(element);
		void* display = Read<void*>(element, NC2Layout::MenuWorldElementOffset::Display);
		if (display != nullptr)
		{
			NC2E2::Display_Destruct(display);
			NXBasics::Runtime_Free(display);
		}
		NC2InGameGuiBase::BaseBitmapToolElement_Destruct(element);
	}

	// Original function: FUN_004dad5c.
	static void FollowMode_Validate(void* element)
	{
		if (Read<std::uint8_t>(element, NC2Layout::MenuWorldElementOffset::FollowActive) == 0) return;
		const void* object = nullptr;
		std::uint32_t generation = 0;
		bool hasFollowTarget = false;
		if (Read<std::uint8_t>(element, NC2Layout::MenuWorldElementOffset::FollowHuman) != 0)
		{
			object = reinterpret_cast<const std::byte*>(NC2Logic::CHumanArray::mStaticVars) + Read<std::uint32_t>(element, NC2Layout::MenuWorldElementOffset::FollowHumanId) * 0xC58u;
			generation = Read<std::uint32_t>(element, NC2Layout::MenuWorldElementOffset::FollowHumanRevision);
			hasFollowTarget = true;
		}
		else if (Read<std::uint8_t>(element, NC2Layout::MenuWorldElementOffset::FollowVehicle) != 0)
		{
			object = reinterpret_cast<const std::byte*>(NC2Logic::CVehicleArray::mStaticVars) + Read<std::uint32_t>(element, NC2Layout::MenuWorldElementOffset::FollowVehicleId) * 0x54Cu;
			generation = Read<std::uint32_t>(element, NC2Layout::MenuWorldElementOffset::FollowVehicleRevision);
			hasFollowTarget = true;
		}
		if (hasFollowTarget && Read<std::uint8_t>(object, NC2Layout::ActorIdentityOffset::Allocated) != 0 && Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::UniqueId) == generation) return;
		std::memset(static_cast<std::byte*>(element) + 0x3C, 0, 24);
	}

	// Original function: FUN_004dadb4.
	void WorldDisplayElement_l_FollowMode_Update(void* element)
	{
		FollowMode_Validate(element);
		if (Read<std::uint8_t>(element, NC2Layout::MenuWorldElementOffset::FollowActive) == 0) return;
		std::uint32_t moveableId;
		if (Read<std::uint8_t>(element, NC2Layout::MenuWorldElementOffset::FollowHuman) != 0)
			moveableId = Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars, Read<std::uint32_t>(element, NC2Layout::MenuWorldElementOffset::FollowHumanId) * NC2Layout::RecordSize::HumanUnsigned + NC2Layout::HumanOffset::MapMoveableId);
		else if (Read<std::uint8_t>(element, NC2Layout::MenuWorldElementOffset::FollowVehicle) != 0)
			moveableId = Read<std::uint32_t>(NC2Logic::CVehicleArray::mStaticVars, Read<std::uint32_t>(element, NC2Layout::MenuWorldElementOffset::FollowVehicleId) * NC2Layout::RecordSize::VehicleUnsigned + NC2Layout::VehicleOffset::MapMoveableId);
		else return;
		std::int32_t point[2];
		Display_DE_MapMoveableToGlobalWorldPixelCoordinates(Read<void*>(element, NC2Layout::MenuWorldElementOffset::Display), nullptr, moveableId, point);
		point[1] = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(point[1]) - 25u);
		NC2E2::DE_SetWantedPositionToPixelPoint(Read<void*>(element, NC2Layout::MenuWorldElementOffset::Display), point);
	}

	// Original function: FUN_004dad1b.
	void WorldDisplayElement_FollowHuman(void* element, std::int32_t humanId)
	{
		std::memset(static_cast<std::byte*>(element) + 0x3C, 0, 24);
		Write(element, NC2Layout::MenuWorldElementOffset::FollowHumanId, humanId);
		Write<std::uint8_t>(element, NC2Layout::MenuWorldElementOffset::FollowActive, 1);
		Write<std::uint8_t>(element, NC2Layout::MenuWorldElementOffset::FollowHuman, 1);
		const auto generation = Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars, static_cast<std::uint32_t>(humanId) * NC2Layout::RecordSize::HumanUnsigned + NC2Layout::HumanOffset::UniqueId);
		Write(element, NC2Layout::MenuWorldElementOffset::FollowHumanRevision, generation);
		WorldDisplayElement_l_FollowMode_Update(element);
	}
}

namespace NC2InGameGuiBase
{
	// Original function: FUN_004b8fd0; the separator is the exact CR/LF pair.
	void BaseToolTextButton_SetCaptionString(void* element, const char* text)
	{
		using namespace NC2Lifetime;
		NXBasics::Runtime_Free(Read<char*>(element, NC2Layout::GuiTextButtonOffset::Caption));
		Write<char*>(element, NC2Layout::GuiTextButtonOffset::Caption, nullptr);
		char* copy = NXBasics::StringDuplicate(text);
		Write(element, NC2Layout::GuiTextButtonOffset::Caption, copy);
		const char* separator = std::strstr(copy, "\r\n");
		Write<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::LineCount, 0);
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::MultilineFlag, separator != nullptr);
		if (separator != nullptr)
		{
			Write<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::LineCount, 1);
			separator = std::strstr(Read<char*>(element, NC2Layout::GuiTextButtonOffset::Caption), "\r\n");
			while (separator != nullptr)
			{
				Write(element, NC2Layout::GuiTextButtonOffset::LineCount, Read<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::LineCount) + 1u);
				separator = std::strstr(separator + 2, "\r\n");
			}
		}
	}

	// Original function: FUN_004b9042; copies all four SColorRGB bytes.
	void BaseToolTextButton_SetBackgroundColor(void* element, const void* color)
	{
		using namespace NC2Lifetime;
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::FillSolidColorFlag, 1);
		Write(element, NC2Layout::GuiTextButtonOffset::SolidColor, Read<std::uint32_t>(color, NC2Layout::ColorRGBOffset::PackedColor));
	}

	// Original function: FUN_004b9052.
	void BaseToolTextButton_SetToolTipString(void* element, const char* text)
	{
		using namespace NC2Lifetime;
		NXBasics::Runtime_Free(Read<char*>(element, NC2Layout::GuiTextButtonOffset::Tooltip));
		Write<char*>(element, NC2Layout::GuiTextButtonOffset::Tooltip, nullptr);
		if (text != nullptr) Write(element, NC2Layout::GuiTextButtonOffset::Tooltip, NXBasics::StringDuplicate(text));
	}
}

namespace NC2MainMenuGuiManager
{
	// Original function: FUN_004d31af.
	void CGuiManager::Exploration_Set(bool enabled)
	{
		E2Manager_Inform_SetExplorationState(enabled);
	}
}

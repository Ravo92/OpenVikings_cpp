#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "MainMenuGuiManager.h"
#include "MainMenuGuiManager_Access.h"
#include "XBitmap.h"
#include "XGuiDesktop.h"
#include "XGuiLists.h"

#include <cstring>
#include <new>

namespace NC2MainMenuGuiManager
{
	using TElementList = NXBasics::CList<NXBaseGui::CBaseElement>;

	static TElementList* ElementList(void* object, std::uint32_t offset)
	{

		return std::launder(reinterpret_cast<TElementList*>(static_cast<std::byte*>(object) + offset));
	}

	// Original function: FUN_004d2b7c; SObjectVars constructor at manager +4.
	void* GuiObjectVars_Construct(void* object)
	{
		new (static_cast<std::byte*>(object) + 4) TElementList(false);
		new (static_cast<std::byte*>(object) + 0x18) TElementList(false);

		return object;
	}

	// Original function: FUN_004d2bdd; SObjectVars destruction.
	void GuiObjectVars_Destruct(void* manager)
	{
		NXBasics::CList<NXBaseGui::CBaseElement>::Delete(ElementList(manager, 0x1C), 0);
		NXBasics::CList<NXBaseGui::CBaseElement>::Delete(ElementList(manager, 8), 0);
	}

	// Original function: FUN_004d32ff.
	NXBasics::CBitmap* CGuiManager::Debug_GetDesktopBitmapPtr() const
	{

		return NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap();
	}

	// Original function: FUN_004d6aa5. The PE supplies the two pushes which
	// overlap the instruction export's final jump-table data boundary.
	void CGuiManager::l_ScreenGuiElement_RemoveAll()
	{
		auto* list = ElementList(this, 8);

		for (void* element = list->l_Base_GetStartElement(); element != nullptr; element = list->l_Base_GetStartElement())
		{
			list->l_Base_RemoveElement(element);
			NXBaseGui::CDesktop::sTheObjectPtr->Element_Remove(static_cast<NXBaseGui::CBaseElement*>(element));
			using TDelete = void*(__thiscall*)(void*, std::uint32_t);
			NC2Lifetime::Read<TDelete*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[0](element, 1);
		}

		std::memset(reinterpret_cast<std::byte*>(this) + 0x30, 0, 10 * sizeof(void*));
	}

	// Shared Windows insertion sequence, inlined in Screen_UpdateGui.
	void CGuiManager::l_ScreenGuiElement_Add(void* element)
	{
		ElementList(this, 8)->InsertAtEnd(element);
		NXBaseGui::CDesktop::sTheObjectPtr->Element_Add(static_cast<NXBaseGui::CBaseElement*>(element), true);
	}

	// Original function: FUN_004d6aea.
	void CGuiManager::ScreenGuiElement_FrameCall_Register(void* element)
	{
		ElementList(this, 0x1C)->InsertAtEnd(element);
	}

	// Original function: FUN_004d6af9.
	void CGuiManager::ScreenGuiElement_FrameCall_UnRegister(void* element)
	{
		ElementList(this, 0x1C)->l_Base_RemoveElement(element);
	}

	// Original function: FUN_004d6b08.
	void CGuiManager::l_ScreenGuiElement_FrameCall_Do()
	{
		NXBasics::CListBase::Iterator iterator(*ElementList(this, 0x1C));
		using TMessage = std::uint32_t(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

		for (void* element = iterator.First(); element != nullptr; element = iterator.Next())
		{
			NC2Lifetime::Read<TMessage*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](element, 0x800, 0x7EE, 0, 0, 0);
		}
	}
}
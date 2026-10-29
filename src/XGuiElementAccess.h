#pragma once
#include "XGuiStorage.h"

namespace NXBasics
{
	class CBitmap;
}
namespace NXBaseGui
{

	// Adapt the reconstructed ECX/unused-EDX entry to the native x64 argument order.
	template<auto Function> struct SNativeGuiVirtualEntry;
#if defined(_WIN64)
	template<typename Result, typename... Args, Result(*Function)(void*, void*, Args...)>
	struct SNativeGuiVirtualEntry<Function>
	{
		static Result Invoke(void* object, Args... args) { return Function(object, nullptr, args...); }
		static constexpr auto Get() { return &Invoke; }
	};
#endif
	template<auto Function> constexpr auto NativeGuiVirtualEntry()
	{
#if defined(_WIN64)
		return SNativeGuiVirtualEntry<Function>::Get();
#else
		return Function;
#endif
	}

	extern const std::uintptr_t gBaseElementVirtualTable[18];
	inline NXBasics::SRectangle BaseElement_GetRectangle(const void* element)
	{
		NXBasics::SRectangle rectangle(0, 0, 0, 0);
		std::memcpy(&rectangle, static_cast<const std::byte*>(element) + offsetof(SBaseElementStorage, rectangle), sizeof(rectangle));
		return rectangle;
	}
	void* BaseElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t messageMask);
	void BaseElement_Destruct(void* element);
	bool __fastcall BaseElement_HasSubElements(void* element);
	bool __fastcall BaseElement_IsHit(void* element, void* unusedEdx, const std::int32_t* point);
	bool __fastcall BaseElement_ToolTip_AtOnce(void* element);
	std::uint32_t __fastcall BaseElement_UnresolvedVirtual40(void* element, void* unusedEdx, std::uint32_t param1, std::uint32_t param2);
	void __fastcall BaseElement_Notify(void* element);
	void __fastcall BaseWindow_DrawOverlays(void* window, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	void* __fastcall BaseWindow_GetRealActiveElementPtr(void* window);
	void __fastcall BaseElement_Draw(void* element, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	void __fastcall BaseElement_Activate(void* element, void* unusedEdx, const std::int32_t* point);
	void __fastcall BaseElement_DeActivate(void* element);
	bool __fastcall BaseElement_ButtonPressed(void* element, void* unusedEdx, const std::int32_t* point);
	bool __fastcall BaseElement_ButtonReleased(void* element);
	void BaseElement_MouseEnteredArea(void* element);
	bool BaseElement_CanBeActivated(const void* element);
	bool BaseElement_CanBePressed(const void* element);
	bool __fastcall BaseElement_Message_Handle(void* element, void* unusedEdx, std::uint32_t message, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3, std::uint32_t param4);
	void __fastcall BaseElement_ParentToLocalPosition(void* element, void* unusedEdx, std::int32_t* point);
	void* __fastcall BaseElement_FindElementOnPosition(void* element, void* unusedEdx, const std::int32_t* point, bool includeDisabled);
	void* __fastcall BaseElement_GetRealActiveElementPtr(void* element);
	bool __fastcall BaseElement_ToolTip_Available(void* element, void* unusedEdx, const char** output);
	bool __fastcall BaseElement_Hide_CanBeHidden(void* element);
}

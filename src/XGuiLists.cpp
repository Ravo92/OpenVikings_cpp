#include "XCrtFileAccess.h"
#include "XGuiLists.h"

namespace NXBasics
{
	// Original function: FUN_00478398; distinct native list specialization.
	template <>
	CList<NXBaseGui::CBaseElement>::~CList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004783e3; the scalar wrapper retains flag bit 0.
	template <>
	void* CList<NXBaseGui::CBaseElement>::Delete(CList* list, std::uint32_t flags)
	{
		list->CList::~CList();

		if (flags & 1u)
			Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004783c6; distinct native list specialization.
	template <>
	CList<NXBaseGui::CBaseWindow>::~CList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004783ff; the scalar wrapper retains flag bit 0.
	template <>
	void* CList<NXBaseGui::CBaseWindow>::Delete(CList* list, std::uint32_t flags)
	{
		list->CList::~CList();

		if (flags & 1u)
			Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004a2ee2; distinct native list specialization.
	template <>
	CList<NC2InGameGuiWorldWindow::CWorldObserveDisplayWindow>::~CList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004a2f1c; the scalar wrapper retains flag bit 0.
	template <>
	void* CList<NC2InGameGuiWorldWindow::CWorldObserveDisplayWindow>::Delete(CList* list, std::uint32_t flags)
	{
		list->CList::~CList();

		if (flags & 1u)
			Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004a2eff; distinct native list specialization.
	template <>
	CList<NC2InGameGuiWorldElement::CWorldDisplayElement>::~CList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004a2f38; the scalar wrapper retains flag bit 0.
	template <>
	void* CList<NC2InGameGuiWorldElement::CWorldDisplayElement>::Delete(CList* list, std::uint32_t flags)
	{
		list->CList::~CList();

		if (flags & 1u)
			Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004b84b4; distinct native list specialization.
	template <>
	CList<NC2InGameGuiOverviewDisplay::CWorldOverviewBitmapGuiElement>::~CList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004b84d1; the scalar wrapper retains flag bit 0.
	template <>
	void* CList<NC2InGameGuiOverviewDisplay::CWorldOverviewBitmapGuiElement>::Delete(CList* list, std::uint32_t flags)
	{
		list->CList::~CList();

		if (flags & 1u)
			Runtime_Free(list);

		return list;
	}
}
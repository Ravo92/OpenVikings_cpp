#include "HyperLinkTextSystem.h"

namespace NHyperLinkTextSystem
{
	// Original function: FUN_004cbc49; the native list payload callback pops its argument.
	void __stdcall LineHandlerList_DeleteData(void* data)
	{
		if (data != nullptr)
		{
			static_cast<SDisplayManagerLineHandler*>(data)->~SDisplayManagerLineHandler();
			NXBasics::Runtime_Free(data);
		}
	}

	// Original function: FUN_004d1c37; only text entities own their content pointer.
	void __stdcall MicroEntityList_DeleteData(void* data)
	{
		if (data == nullptr)
		{

			return;
		}

		auto* entity = static_cast<SMicroEntity*>(data);

		if (entity->type == 2)
			NXBasics::Runtime_Free(entity->content.text);

		entity->type = 0;
		NXBasics::Runtime_Free(entity);
	}

	// Original function: FUN_004d1caf; four distinct non-owning resource lists.
	CDataContainer::SVars::SVars() : fonts(false), bitmaps(false), palettes(false), textFiles(false)
	{
	}

	// Original function: FUN_004d1dd0; member destruction runs text, palette, bitmap, font.
	CDataContainer::SVars::~SVars()
	{
	}
}

namespace NC2Lifetime
{
	// Original function: FUN_004cbc80; distinct native specialization and base destruction.
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SLineListTag>::~CRuntimeOwnedList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004cbc64; native allocation is freed only for flag bit 0.
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SLineListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags)
	{
		list->CRuntimeOwnedList::~CRuntimeOwnedList();

		if (flags & 1u)
			NXBasics::Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004cbcb9; distinct native specialization and base destruction.
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SLineEntityListTag>::~CRuntimeOwnedList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004cbc9d; native allocation is freed only for flag bit 0.
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SLineEntityListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags)
	{
		list->CRuntimeOwnedList::~CRuntimeOwnedList();

		if (flags & 1u)
			NXBasics::Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004d1c79; distinct native specialization and base destruction.
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SMicroEntityListTag>::~CRuntimeOwnedList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004d1c5d; native allocation is freed only for flag bit 0.
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SMicroEntityListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags)
	{
		list->CRuntimeOwnedList::~CRuntimeOwnedList();

		if (flags & 1u)
			NXBasics::Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004d20fa; distinct native specialization and base destruction.
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SFontListTag>::~CRuntimeOwnedList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004d216e; native allocation is freed only for flag bit 0.
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SFontListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags)
	{
		list->CRuntimeOwnedList::~CRuntimeOwnedList();

		if (flags & 1u)
			NXBasics::Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004d2117; distinct native specialization and base destruction.
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SBitmapListTag>::~CRuntimeOwnedList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004d218a; native allocation is freed only for flag bit 0.
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SBitmapListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags)
	{
		list->CRuntimeOwnedList::~CRuntimeOwnedList();

		if (flags & 1u)
			NXBasics::Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004d2134; distinct native specialization and base destruction.
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SPaletteListTag>::~CRuntimeOwnedList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004d21a6; native allocation is freed only for flag bit 0.
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SPaletteListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags)
	{
		list->CRuntimeOwnedList::~CRuntimeOwnedList();

		if (flags & 1u)
			NXBasics::Runtime_Free(list);

		return list;
	}

	// Original function: FUN_004d2151; distinct native specialization and base destruction.
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::STextFileListTag>::~CRuntimeOwnedList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_004d21c2; native allocation is freed only for flag bit 0.
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::STextFileListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags)
	{
		list->CRuntimeOwnedList::~CRuntimeOwnedList();

		if (flags & 1u)
			NXBasics::Runtime_Free(list);

		return list;
	}

	template <>
	void CRuntimeOwnedList<NHyperLinkTextSystem::SLineListTag>::l_DeleteData(void* data)
	{
		NHyperLinkTextSystem::LineHandlerList_DeleteData(data);
	}

	template <>
	void CRuntimeOwnedList<NHyperLinkTextSystem::SMicroEntityListTag>::l_DeleteData(void* data)
	{
		NHyperLinkTextSystem::MicroEntityList_DeleteData(data);
	}
}
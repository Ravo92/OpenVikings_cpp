#pragma once

#include "XList.h"

#include <cstring>
#include <type_traits>

namespace NXBaseGui
{
	class CBaseElement;
	class CBaseWindow;
}

namespace NC2InGameGuiOverviewDisplay
{
	class CWorldOverviewBitmapGuiElement;
}

namespace NC2InGameGuiWorldElement
{
	class CWorldDisplayElement;
}
namespace NC2InGameGuiWorldWindow
{
	class CWorldObserveDisplayWindow;
}

namespace NXBasics
{
	template<typename T>
	class CList final : public CListBase
	{
		static_assert(std::is_same_v<T, NXBaseGui::CBaseElement> || std::is_same_v<T, NXBaseGui::CBaseWindow> || std::is_same_v<T, NC2InGameGuiOverviewDisplay::CWorldOverviewBitmapGuiElement> || std::is_same_v<T, NC2InGameGuiWorldElement::CWorldDisplayElement> || std::is_same_v<T, NC2InGameGuiWorldWindow::CWorldObserveDisplayWindow>, "Only the verified GUI list specializations are implemented");

	public:
		explicit CList(bool deleteObjects) : CListBase(deleteObjects)
		{}

		~CList() override;
		static void* Delete(CList* list, std::uint32_t flags);

	protected:
		// Original function shared by the verified GUI specializations: LAB_004783b5
		void l_DeleteData(void* data) override
		{
			if (data == nullptr)
				return;

			using TDeletingDestructor = void* (__thiscall*)(void*, unsigned int);

			TDeletingDestructor* methods;
			std::memcpy(&methods, data, sizeof(methods));
			methods[0](data, 1u);
		}
	};

	template<> CList<NXBaseGui::CBaseElement>::~CList();
	template<> void* CList<NXBaseGui::CBaseElement>::Delete(CList* list, std::uint32_t flags);
	template<> CList<NXBaseGui::CBaseWindow>::~CList();
	template<> void* CList<NXBaseGui::CBaseWindow>::Delete(CList* list, std::uint32_t flags);
	template<> CList<NC2InGameGuiWorldWindow::CWorldObserveDisplayWindow>::~CList();
	template<> void* CList<NC2InGameGuiWorldWindow::CWorldObserveDisplayWindow>::Delete(CList* list, std::uint32_t flags);
	template<> CList<NC2InGameGuiWorldElement::CWorldDisplayElement>::~CList();
	template<> void* CList<NC2InGameGuiWorldElement::CWorldDisplayElement>::Delete(CList* list, std::uint32_t flags);
	template<> CList<NC2InGameGuiOverviewDisplay::CWorldOverviewBitmapGuiElement>::~CList();
	template<> void* CList<NC2InGameGuiOverviewDisplay::CWorldOverviewBitmapGuiElement>::Delete(CList* list, std::uint32_t flags);

	static_assert(sizeof(CList<NXBaseGui::CBaseElement>) == sizeof(CListBase));
	static_assert(sizeof(CList<NXBaseGui::CBaseWindow>) == sizeof(CListBase));
	static_assert(sizeof(CList<NC2InGameGuiWorldElement::CWorldDisplayElement>) == sizeof(CListBase));
	static_assert(sizeof(CList<NC2InGameGuiWorldWindow::CWorldObserveDisplayWindow>) == sizeof(CListBase));
}

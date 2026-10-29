#pragma once

#include "XList.h"
#include "XCrtFileAccess.h"

namespace NC2Lifetime
{
	// Each tag represents a distinct original CList specialization. The default +4
	// payload deleter is Runtime_Free; owning hyperlink payloads specialize it.
	template<class Tag> class CRuntimeOwnedList : public NXBasics::CListBase
	{
	public:
		explicit CRuntimeOwnedList(bool deleteObjects) : CListBase(deleteObjects)
		{}
		static void* Delete(CRuntimeOwnedList* list, std::uint32_t flags);
		~CRuntimeOwnedList() override
		{
			if (mDeleteObjects != 0)
			{
				DeleteAllElements();
			}
		}

	protected:
		void l_DeleteData(void* data) override
		{
			NXBasics::Runtime_Free(data);
		}
	};

}

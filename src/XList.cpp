#include "XCrtFileAccess.h"
#include "XList.h"

#include <cstdint>

namespace NXBasics
{
	CListBase::Iterator::Iterator(CListBase& list) : mList(&list), mPosition(nullptr), mStarted(false), mPadding09{ }
	{}

	// Original function: FUN_0048d143.
	void* CListBase::Iterator::First()
	{
		if (mList->mCount == 0)
			return nullptr;
		mStarted = true;
		mPosition = mList->mFirst;
		return mPosition->data;
	}

	// Original function: FUN_004259f3.
	void* CListBase::Iterator::Next()
	{
		if (!mStarted)
			return First();
		if (mPosition != nullptr)
		{
			mPosition = mPosition->next;
			if (mPosition != nullptr)
				return mPosition->data;
		}
		mStarted = false;
		return nullptr;
	}

	// Original function: FUN_00439bed
	CListBase::CListBase(bool deleteObjects) : mCount(0), mFirst(nullptr), mLast(nullptr), mDeleteObjects(static_cast<std::uint8_t>(deleteObjects)), mPadding11{ }
	{}

	// Original function: FUN_00439c3f
	// Original deleting destructor: FUN_00439c13
	CListBase::~CListBase()
	{
		while (mFirst != nullptr)
			l_Base_RemoveFromStart();
	}

	// Original function: FUN_00439c59.
	void CListBase::InsertAtStart(void* data)
	{
		auto* node = static_cast<SNode*>(Runtime_AllocateWithNewHandler(sizeof(SNode), 1));
		if (node != nullptr)
		{
			node->next = mFirst;
			node->data = data;
		}
		if (mFirst == nullptr)
			mLast = node;
		++mCount;
		mFirst = node;
	}

	// Original function: FUN_00439c8b.
	void CListBase::InsertAtEnd(void* data)
	{
		auto* node = static_cast<SNode*>(Runtime_AllocateWithNewHandler(sizeof(SNode), 1));
		if (node != nullptr)
		{
			node->next = nullptr;
			node->data = data;
		}
		if (mCount == 0)
			mFirst = node;
		else
			mLast->next = node;
		++mCount;
		mLast = node;
	}

	// Original function: FUN_00439cc2.
	void CListBase::InsertBefore(void* position, void* data)
	{
		SNode* previous = nullptr;
		if (mCount != 0)
		{
			for (SNode* node = mFirst; node->next != nullptr; node = node->next)
			{
				if (node->next->data == position)
				{
					previous = node;
					break;
				}
			}
		}
		if (previous == nullptr)
		{
			InsertAtStart(data);
			return;
		}
		auto* node = static_cast<SNode*>(Runtime_AllocateWithNewHandler(sizeof(SNode), 1));
		if (node != nullptr)
		{
			node->next = previous->next;
			node->data = data;
		}
		previous->next = node;
		++mCount;
	}

	// Original function: FUN_00439d1e
	void* CListBase::l_Base_RemoveFromStart()
	{
		if (mCount == 0)
			return nullptr;

		SNode* node = mFirst;
		SNode* next = node->next;
		void* data = node->data;

		if (node == mLast)
		{
			mLast = nullptr;
			mFirst = nullptr;
		}
		else
		{
			mFirst = next;
		}

		Runtime_Free(node);
		--mCount;

		return data;
	}

	// Original function: FUN_00439d54
	void* CListBase::l_Base_RemoveFromEnd()
	{
		if (mCount == 0)
			return nullptr;

		void* data = mLast->data;
		SNode* previous = nullptr;
		SNode* current = mFirst;

		while (current->next != nullptr)
		{
			if (current->next->data == data)
			{
				previous = current;
				break;
			}

			current = current->next;
		}

		if (previous != nullptr)
			previous->next = nullptr;

		Runtime_Free(mLast);

		const std::uint32_t count = mCount;
		mLast = previous;

		// Preserve the original check before decrementing the count.
		if (count == 0)
			mFirst = nullptr;

		mCount = count - 1u;
		return data;
	}

	// Original function: FUN_00439da8
	void* CListBase::l_Base_RemoveElement(void* data)
	{
		if (mCount == 0)
			return data;

		if (mFirst->data == data)
			return l_Base_RemoveFromStart();

		if (mLast->data == data)
			return l_Base_RemoveFromEnd();

		SNode* previous = mFirst;

		while (previous->next != nullptr)
		{
			SNode* node = previous->next;

			if (node->data == data)
			{
				previous->next = node->next;
				Runtime_Free(node);
				--mCount;
				return data;
			}

			previous = node;
		}

		return data;
	}

	bool CListBase::ContainsBeforeNullData(const void* data) const
	{
		if (mCount == 0)
			return false;

		// Match the iterator termination used by CDesktop::Window_Close.
		for (const SNode* node = mFirst; node != nullptr; node = node->next)
		{
			if (node->data == nullptr)
				return false;

			if (node->data == data)
				return true;
		}

		return false;
	}

	// Original function: FUN_00439e0e
	void CListBase::DeleteStartElement()
	{
		void* data = l_Base_RemoveFromStart();

		if (data != nullptr)
			l_DeleteData(data);
	}

	// Original function: FUN_00439e24
	void CListBase::DeleteAllElements()
	{
		while (mCount != 0)
		{
			if (mFirst->data == nullptr)
				break;

			DeleteStartElement();
		}
	}

	// Original function: FUN_00439e41
	void* CListBase::l_Base_GetStartElement() const
	{
		if (mCount == 0)
			return nullptr;

		return mFirst->data;
	}

	// Original function: FUN_00439e51; count controls access to the last node.
	void* CListBase::l_Base_GetEndElement() const
	{
		if (mCount == 0)
			return nullptr;

		return mLast->data;
	}

	void CListBase::operator delete(void* allocation) noexcept
	{
		Runtime_Free(allocation);
	}
}

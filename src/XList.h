#pragma once

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CListBase
	{
	protected:
		struct SNode;

	public:
		class Iterator
		{
		public:
			explicit Iterator(CListBase& list);
			void* First();
			void* Next();

		private:
			CListBase* mList;
			SNode* mPosition;
			bool mStarted;
			std::byte mPadding09[3];
		};

		// Original function: FUN_00439bed
		explicit CListBase(bool deleteObjects);

		// Original function: FUN_00439c3f
		// Original deleting destructor: FUN_00439c13
		virtual ~CListBase();

		void InsertAtStart(void* data);
		void InsertAtEnd(void* data);
		void InsertBefore(void* position, void* data);
		std::uint32_t GetCount() const
		{
			return mCount;
		}

		CListBase(const CListBase&) = delete;
		CListBase& operator=(const CListBase&) = delete;

		// Original function: FUN_00439d1e
		void* l_Base_RemoveFromStart();

		void* l_Base_RemoveFromEnd();
		void* l_Base_RemoveElement(void* data);
		bool ContainsBeforeNullData(const void* data) const;

		// Original function: FUN_00439e0e
		void DeleteStartElement();

		// Original function: FUN_00439e24
		void DeleteAllElements();

		// Original function: FUN_00439e41
		void* l_Base_GetStartElement() const;

		void* l_Base_GetEndElement() const;

		static void operator delete(void* allocation) noexcept;

	protected:
		// Original virtual slot: +0x04.
		// Original base entry: FUN_004e7bef, pure virtual failure.
		virtual void l_DeleteData(void* data) = 0;

		struct SNode
		{
			SNode* next;
			void* data;
		};

		std::uint32_t mCount;       // +0x04
		SNode* mFirst;              // +0x08
		SNode* mLast;               // +0x0C
		std::uint8_t mDeleteObjects; // +0x10
		std::byte mPadding11[3];    // +0x11

		static_assert(sizeof(SNode) == 2 * sizeof(void*));
		static_assert(offsetof(SNode, data) == sizeof(void*));
	};

	static_assert(sizeof(CListBase) == (sizeof(void*) == 8 ? 40 : 20), "Unexpected native CListBase layout");
	static_assert(sizeof(CListBase::Iterator) == (sizeof(void*) == 8 ? 24 : 12));
}

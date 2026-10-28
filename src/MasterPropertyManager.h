#pragma once

#include "XList.h"

#include <cstddef>
#include <cstdint>
#include <new>

namespace NMasterPropertyManager
{
	struct SProperty
	{
		char* name;                  // +0x00
		std::uint8_t isInteger;       // +0x04
		std::byte padding05[3];       // +0x05
		std::int32_t integerValue;    // +0x08
		char* stringValue;            // +0x0C
	};

	static_assert(sizeof(SProperty) == (sizeof(void*) == 8 ? 24 : 16), "Unexpected native property layout");
	static_assert(offsetof(SProperty, name) == 0);
	static_assert(offsetof(SProperty, isInteger) == sizeof(void*));
	static_assert(offsetof(SProperty, integerValue) == sizeof(void*) + 4);
	static_assert(offsetof(SProperty, stringValue) == sizeof(void*) + 8);

	class CPropertyManager
	{
	public:
		// Original function: FUN_004e1408
		CPropertyManager();

		// Original function: FUN_004e1436
		~CPropertyManager();

		static void* operator new(std::size_t size);
		static void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
		static void operator delete(void* allocation) noexcept;
		static void operator delete(void* allocation, const std::nothrow_t&) noexcept;

		// Original function: FUN_004e1463
		void Property_WorkOnCommandLine(const char* commandLine);

		// Original function: FUN_004e1551
		void Property_WorkOnIniFile(const char* fileName);

		// Original function: FUN_004e163b
		std::int32_t Property_GetIntegerValue(const char* propertyName);

		// Original function: FUN_004e1658
		const char* Property_GetStringValuePtr(const char* propertyName);

		// Original function: FUN_004e166b
		bool Property_DoesExists(const char* propertyName);

		// Original function: FUN_004e167d
		void Property_Add(const char* propertyName, bool isInteger, std::int32_t integerValue, const char* stringValue);

		// Original function: FUN_004e16fb
		bool Property_Remove(const char* propertyName);

		void Options_Load();
		void Options_Save_Global();
		void Options_Save_InGame();
		void System_AddDefaultProperties();

		// Original global: DAT_005697ec
		inline static CPropertyManager* sTheObjectPtr = nullptr;

	private:
		// Internal specialization of the shared list implementation.
		class SObjectVars final : public NXBasics::CListBase
		{
		public:
			explicit SObjectVars(std::uint8_t destroyValues);

			// Original function: FUN_004e1d00
			~SObjectVars() override;
			void* Delete(std::uint32_t flags);

		protected:
			// Original function: LAB_00479e17
			void l_DeleteData(void* value) override;
		};

		static_assert(sizeof(SObjectVars) == sizeof(NXBasics::CListBase), "Property list adds no runtime fields");

		// Original function: FUN_004e171d
		SProperty* l_Property_GetPtr(const char* propertyName);

		// Original function: FUN_004e1765
		void l_Property_RemoveAndDelete(SProperty* property);

		std::uint8_t mState;       // +0x00; meaning not yet identified.
		std::byte mPadding01[3];   // +0x01; not initialized by the constructor.
		SObjectVars mProperties;   // +0x04
	};

	static_assert(sizeof(CPropertyManager) == (sizeof(void*) == 8 ? 48 : 24), "Unexpected native property manager layout");
}

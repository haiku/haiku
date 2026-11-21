/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MessageItemTest.h"

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


template<> struct ArrayTypeBase<bool> {
	class ArrayType {
	public:
		ArrayType()
			: array(NULL),
			  size(0)
		{
		}
		ArrayType(const ArrayType& rhs)
			: array(NULL),
			  size(0)
		{
			*this = rhs;
		}
		ArrayType(bool* data, uint32 len)
			: array(NULL),
			  size(0)
		{
			Assign(data, len);
		}
		~ArrayType()
		{
			if (array)
				delete[] array;
		}

		ArrayType& operator=(const ArrayType& rhs)
		{
			if (this != &rhs)
				Assign(rhs.array, rhs.size);
			return *this;
		}

		uint32 Size() { return size; }
		bool& operator[](int index)
		{
			// We're just gonna let a segfault happen
			return array[index];
		}

	private:
		void Assign(bool* data, uint32 len)
		{
			size = len;

			if (array)
				delete[] array;
			array = new bool[Size()];
			memcpy(array, data, Size());
		}
		bool* array;
		uint32 size;
	};

	typedef uint32 SizeType;
	static SizeType Size(ArrayType& array) { return array.Size(); }
};


struct BoolInitPolicy : public ArrayTypeBase<bool> {
	static bool Zero() { return false; }
	static bool Test1() { return true; }
	static bool Test2() { return false; }
	static size_t SizeOf(const bool&) { return sizeof(bool); }
	static ArrayType Array()
	{
		static bool array[] = {true, true, true};
		return ArrayType(array, 3);
	}
};


typedef MessageItemAssertPolicy<bool> BoolAssertPolicy;
typedef
MessageItemFuncPolicy<bool, &BMessage::AddBool, &BMessage::FindBool,
		&BMessage::FindBool, &BMessage::FindBool, &BMessage::HasBool,
		&BMessage::ReplaceBool>
	BoolFuncPolicy;
typedef
MessageItemTest<bool, B_BOOL_TYPE, BoolFuncPolicy, BoolInitPolicy,
		BoolAssertPolicy>
	MessageBoolItemTest;


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageBoolItemTest, getTestSuiteName());

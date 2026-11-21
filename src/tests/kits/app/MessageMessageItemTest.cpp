/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MessageItemTest.h"

#include <Message.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


struct MessageFuncPolicy : public TypePolicy<BMessage> {
	static status_t FindData(BMessage& msg, const char* name, type_code type, int32 index,
							 const void** data, ssize_t* size)
	{
		*data = NULL;
		char* ptr;
		status_t err = msg.FindData(name, type, index, (const void**)&ptr, size);
		if (!err) {
			err = sMsg.Unflatten(ptr);
			if (!err)
				*(BMessage**)data = &sMsg;
		}
		return err;
		//	return (msg.*FindDataFunc)(name, type, index, data, size);
	}

	static status_t Replace(BMessage& msg, const char* name, int32 index, BMessage& val)
	{
		return msg.ReplaceMessage(name, index, &val);
	}

	static bool Has(BMessage& msg, const char* name, int32 index)
	{
		return msg.HasMessage(name, index);
	}

	static BMessage QuickFind(BMessage& msg, const char* name, int32 index)
	{
		BMessage val;
		msg.FindMessage(name, index, &val);
		return val;
	}

	static status_t ShortFind(BMessage& msg, const char* name, BMessage* val)
	{
		return msg.FindMessage(name, val);
	}

	static status_t Find(BMessage& msg, const char* name, int32 index, BMessage* val)
	{
		return msg.FindMessage(name, index, val);
	}

	static status_t AddData(BMessage& msg, const char* name, type_code type, BMessage* data,
							ssize_t size, bool)
	{
		char* buf = new char[size];
		status_t err = data->Flatten(buf, size);
		if (!err)
			err = msg.AddData(name, type, buf, size, false);
		delete[] buf;
		return err;
	}

	static status_t Add(BMessage& msg, const char* name, BMessage& val)
	{
		return msg.AddMessage(name, &val);
	}

private:
	static BMessage sMsg;
};


BMessage MessageFuncPolicy::sMsg;


template<> struct ArrayTypeBase<BMessage> {
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
		ArrayType(BMessage* data, uint32 len)
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
		BMessage& operator[](int index)
		{
			// We're just gonna let a segfault happen
			return array[index];
		}

	private:
		void Assign(BMessage* data, uint32 len)
		{
			size = len;

			if (array)
				delete[] array;
			array = new BMessage[Size()];
			for (uint32 i = 0; i < Size(); ++i)
				array[i] = data[i];
		}
		BMessage* array;
		uint32 size;
	};

	typedef uint32 SizeType;
	static SizeType Size(ArrayType& array) { return array.Size(); }
};


struct MessageInitPolicy : public ArrayTypeBase<BMessage> {
	static BMessage Zero() { return BMessage(); }
	static BMessage Test1()
	{
		BMessage msg('1234');
		msg.AddInt32("int32", 1234);
		return msg;
	}
	static BMessage Test2()
	{
		BMessage msg('5678');
		msg.AddString("string", "5678");
		return msg;
	}
	static size_t SizeOf(const BMessage& data) { return data.FlattenedSize(); }
	static ArrayType Array()
	{
		BMessage array[3];
		array[0] = Zero();
		array[1] = Test1();
		array[2] = Test2();
		return ArrayType(array, 3);
	}
};


struct MessageAssertPolicy {
	static BMessage Zero() { return BMessage(); }
	static BMessage Invalid() { return BMessage(); }
	static bool Size(size_t size, BMessage& msg)
	{
		ssize_t msgSize = msg.FlattenedSize();
		return size == (size_t)msgSize;
	}
};


template<> struct MessageItemComparePolicy<BMessage> {
	static bool Compare(const BMessage& lhs, const BMessage& rhs)
	{
		return lhs.what == rhs.what;
	}
};


typedef
MessageItemTest<BMessage, B_MESSAGE_TYPE, MessageFuncPolicy, MessageInitPolicy,
		MessageAssertPolicy>
	MessageMessageItemTest;


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageMessageItemTest, getTestSuiteName());

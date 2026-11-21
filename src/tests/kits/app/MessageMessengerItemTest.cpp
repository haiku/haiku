/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MessageItemTest.h"

#include <Looper.h>
#include <Messenger.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


struct MessengerFuncPolicy : public TypePolicy<BMessenger> {
	static status_t Add(BMessage& msg, const char* name, BMessenger& val)
	{
		return msg.AddMessenger(name, val);
	}
	static status_t AddData(BMessage& msg, const char* name, type_code type, BMessenger* data,
							ssize_t size, bool)
	{
		return msg.AddData(name, type, data, size);
	}
	static status_t Find(BMessage& msg, const char* name, int32 index, BMessenger* val)
	{
		return msg.FindMessenger(name, index, val);
	}
	static status_t ShortFind(BMessage& msg, const char* name, BMessenger* val)
	{
		return msg.FindMessenger(name, val);
	}
	static BMessenger QuickFind(BMessage& msg, const char* name, int32 index)
	{
		BMessenger msngr;
		msg.FindMessenger(name, index, &msngr);
		return msngr;
	}
	static bool Has(BMessage& msg, const char* name, int32 index)
	{
		return msg.HasMessenger(name, index);
	}
	static status_t Replace(BMessage& msg, const char* name, int32 index, BMessenger& val)
	{
		return msg.ReplaceMessenger(name, index, val);
	}
	static status_t FindData(BMessage& msg, const char* name, type_code type, int32 index,
							 const void** data, ssize_t* size)
	{
		return msg.FindData(name, type, index, data, size);
	}

private:
	static BMessenger sMsg;
};


BMessenger MessengerFuncPolicy::sMsg;


struct MessengerInitPolicy : public ArrayTypeBase<BMessenger> {
	static BMessenger Zero() { return BMessenger(); }
	static BMessenger Test1() { return BMessenger("application/x-vnd.Be-NPOS"); }
	static BMessenger Test2() { return BMessenger(&sLooper); }
	static size_t SizeOf(const BMessenger&) { return sizeof(BMessenger); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(Zero());
		array.push_back(Test1());
		array.push_back(Test2());
		return array;
	}

private:
	static BLooper sLooper;
};


// bonefish: TODO: Sorry, but this sucks. Just loading the App Kit test add-on
// will already create a BLooper.
BLooper MessengerInitPolicy::sLooper;


struct MessengerAssertPolicy {
	static BMessenger Zero() { return BMessenger(); }
	static BMessenger Invalid() { return BMessenger(); }
	static bool Size(size_t size, BMessenger& t) { return size == sizeof(t); }
};


typedef
MessageItemTest<BMessenger, B_MESSENGER_TYPE, MessengerFuncPolicy,
		MessengerInitPolicy, MessengerAssertPolicy>
	MessageMessengerItemTest;


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageMessengerItemTest, getTestSuiteName());

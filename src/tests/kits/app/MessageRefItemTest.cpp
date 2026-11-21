/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MessageItemTest.h"

#include <Entry.h>
#include <Path.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


struct RefFuncPolicy : public TypePolicy<entry_ref> {
	static status_t Add(BMessage& msg, const char* name, entry_ref& val)
	{
		return msg.AddRef(name, &val);
	}
	static status_t Find(BMessage& msg, const char* name, int32 index, entry_ref* val)
	{
		return msg.FindRef(name, index, val);
	}
	static status_t ShortFind(BMessage& msg, const char* name, entry_ref* val)
	{
		return msg.FindRef(name, val);
	}
	static bool Has(BMessage& msg, const char* name, int32 index)
	{
		return msg.HasRef(name, index);
	}
	static status_t Replace(BMessage& msg, const char* name, int32 index, entry_ref& val)
	{
		return msg.ReplaceRef(name, index, &val);
	}

	static status_t FindData(BMessage& msg, const char* name, type_code type, int32 index,
							 const void** data, ssize_t* size)
	{
		*data = NULL;
		char* ptr;
		status_t err = msg.FindData(name, type, index, (const void**)&ptr, size);
		if (!err) {
			BPath Path;
			err = Path.Unflatten(type, ptr, *size);
			if (!err) {
				if (Path.Path()) {
					err = get_ref_for_path(Path.Path(), &sRef);
					if (!err)
						*(entry_ref**)data = &sRef;
				} else {
					sRef = entry_ref();
					*(entry_ref**)data = &sRef;
				}
			}
		}
		return err;
	}

	static entry_ref QuickFind(BMessage& msg, const char* name, int32 index)
	{
		//	TODO: make cooler
		//	There must be some 1337 template voodoo way of making the inclusion
		//	of this function conditional.  In the meantime, we offer this
		//	cheesy hack
		entry_ref ref;
		msg.FindRef(name, index, &ref);
		return ref;
	}

	static status_t AddData(BMessage& msg, const char* name, type_code type, entry_ref* data,
							ssize_t size, bool)
	{
		BPath Path(data);
		status_t err = Path.InitCheck();
		char* buf = NULL;
		//	if (!err)
		{
			buf = new char[size];
			err = Path.Flatten(buf, size);
		}

		if (!err)
			err = msg.AddData(name, type, buf, size, false);
		delete[] buf;
		return err;
	}


private:
	static entry_ref sRef;
};


entry_ref RefFuncPolicy::sRef;


struct RefInitPolicy : public ArrayTypeBase<entry_ref> {
	static entry_ref Zero() { return entry_ref(); }
	static entry_ref Test1()
	{
		entry_ref ref;
		get_ref_for_path("/system/apps/Terminal", &ref);
		return ref;
	}
	static entry_ref Test2()
	{
		entry_ref ref;
		get_ref_for_path("/system/lib/libroot.so", &ref);
		return ref;
	}
	static size_t SizeOf(const entry_ref& data)
	{
		BPath Path(&data);
		return Path.FlattenedSize();
	}
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(Zero());
		array.push_back(Test1());
		array.push_back(Test2());
		return array;
	}
};


struct RefAssertPolicy {
	static entry_ref Zero() { return RefInitPolicy::Zero(); }
	static entry_ref Invalid() { return RefInitPolicy::Zero(); }
	// TODO: fix
	static bool Size(size_t size, entry_ref& ref)
	{
		return size == RefInitPolicy::SizeOf(ref);
	}
};


typedef
MessageItemTest<entry_ref, B_REF_TYPE, RefFuncPolicy, RefInitPolicy, RefAssertPolicy>
	MessageRefItemTest;


// CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageRefItemTest, getTestSuiteName());
	// FIXME: fails

#include <cppunit/TestAssert.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <TestShell.h>

#include <os/support/ObjectList.h>
#include <shared/AutoDeleter.h>
#include <util/OpenHashTable.h>

#include "BOpenHashTableTest.h"


namespace {

class Entry {
public:
	Entry(uint32_t value)
		:
		fValue(value),
		fNext(NULL)
	{
	}

	const uint32_t Value() const
	{
		return fValue;
	}

	Entry* Next() const
	{
		return fNext;
	}

private:
	uint32_t	fValue;
	Entry		*fNext;

	friend class EntryDefinition;
};


class EntryDefinition {
public:
	typedef uint32_t KeyType;
	typedef Entry ValueType;

	size_t HashKey(const KeyType& key) const
	{
		return key;
	}

	size_t Hash(Entry* entry) const
	{
		return entry->fValue;
	}

	bool Compare(const KeyType& key, Entry* entry) const
	{
		return key == entry->fValue;
	}

	Entry*& GetLink(Entry* entry) const
	{
		return entry->fNext;
	}
};

}


CppUnit::Test* BOpenHashTableTest::Suite()
{
	CppUnit::TestSuite* suite = new CppUnit::TestSuite("BOpenHashTable");

	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Insert test",
					   &BOpenHashTableTest::InsertTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Insert unchecked test",
					   &BOpenHashTableTest::InsertUncheckedTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Insert unchecked uninitialized test",
					   &BOpenHashTableTest::InsertUncheckedUninitializedTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Iterate and count test",
					   &BOpenHashTableTest::IterateAndCountTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Lookup test",
					   &BOpenHashTableTest::LookupTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Resize test",
					   &BOpenHashTableTest::ResizeTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Remove test",
					   &BOpenHashTableTest::RemoveTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Remove unchecked test",
					   &BOpenHashTableTest::RemoveUncheckedTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Remove when not present test",
					   &BOpenHashTableTest::RemoveWhenNotPresentTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Duplicate insert test",
					   &BOpenHashTableTest::DuplicateInsertTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Disable auto expand",
					   &BOpenHashTableTest::DisableAutoExpandTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Init with zero size",
					   &BOpenHashTableTest::InitWithZeroSizeTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Clear test",
					   &BOpenHashTableTest::ClearTest));
	suite->addTest(new CppUnit::TestCaller<BOpenHashTableTest>(
					   "BOpenHashTable::Clear and return test",
					   &BOpenHashTableTest::ClearAndReturnTest));

	return suite;
}


BOpenHashTableTest::BOpenHashTableTest(std::string name)
	: BTestCase(name)
{
}


void BOpenHashTableTest::InsertTest()
{
	Entry entry(123);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(), B_OK);

	CPPUNIT_ASSERT_EQUAL(table.Insert(&entry), B_OK);
}


void BOpenHashTableTest::InsertUncheckedTest()
{
	Entry entry(123);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(), B_OK);

	table.InsertUnchecked(&entry);
}


void BOpenHashTableTest::InsertUncheckedUninitializedTest()
{
	Entry entry(123);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(), B_OK);

	table.InsertUnchecked(&entry);
}


void BOpenHashTableTest::IterateAndCountTest() {
	const size_t kEntryCount = 20;

	BObjectList<Entry> entries(20, true);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(kEntryCount * 2), B_OK);

	for (uint32_t i = 0; i < kEntryCount; ++i) {
		Entry* entry = new Entry(i);
		entries.AddItem(entry);
		CPPUNIT_ASSERT_EQUAL(table.Insert(entry), B_OK);
	}

	// Verify that the table contains the expected values.
	uint64_t map = 0;
	BOpenHashTable<EntryDefinition>::Iterator iterator = table.GetIterator();
	while (iterator.HasNext()) {
		Entry* entry = iterator.Next();
		CPPUNIT_ASSERT_EQUAL(0, map & (1 << entry->Value()));
		map |= (1 << entry->Value());
	}

	CPPUNIT_ASSERT_EQUAL(map, (1 << kEntryCount) - 1);
	CPPUNIT_ASSERT_EQUAL(kEntryCount, table.CountElements());
}


void BOpenHashTableTest::ResizeTest()
{
	// This is the same as IterateAndCountTest, except that the table will
	// be resized mid insertion.
	const size_t kEntryCount = 20;
	BObjectList<Entry> entries(20, true);
	BOpenHashTable<EntryDefinition> table;

	// Start off with capacity for 8 elements. This will mean that the table
	// will be resized in the loop below since we are inserting 20 elements.
	CPPUNIT_ASSERT_EQUAL(table.Init(8), B_OK);

	for (uint32_t i = 0; i < kEntryCount; ++i) {
		Entry* entry = new Entry(i);
		entries.AddItem(entry);
		CPPUNIT_ASSERT_EQUAL(table.Insert(entry), B_OK);
	}

	// Verify that after resize the expected elements are present within
	// the table.
	uint64_t map = 0;
	BOpenHashTable<EntryDefinition>::Iterator iterator = table.GetIterator();
	while (iterator.HasNext()) {
		Entry* entry = iterator.Next();
		CPPUNIT_ASSERT_EQUAL(0, map & (1 << entry->Value()));
		map |= (1 << entry->Value());
	}

	CPPUNIT_ASSERT_EQUAL(map, (1 << kEntryCount) - 1);
	CPPUNIT_ASSERT_EQUAL(kEntryCount, table.CountElements());
}


void BOpenHashTableTest::LookupTest() {
	Entry entry(123);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(0), B_OK);

	CPPUNIT_ASSERT_EQUAL(table.Insert(&entry), B_OK);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), &entry);
}


void BOpenHashTableTest::RemoveTest() {
	Entry entry(123);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(0), B_OK);

	CPPUNIT_ASSERT_EQUAL(table.Insert(&entry), B_OK);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), &entry);

	table.Remove(&entry);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), NULL);
}


void BOpenHashTableTest::RemoveUncheckedTest()
{
	Entry entry(123);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(0), B_OK);

	CPPUNIT_ASSERT_EQUAL(table.Insert(&entry), B_OK);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), &entry);

	table.RemoveUnchecked(&entry);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), NULL);
}


void BOpenHashTableTest::RemoveWhenNotPresentTest()
{
	Entry entry1(123);
	Entry entry2(456);
	Entry entry3(789);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(), B_OK);

	// Only add the first two entries.
	table.Insert(&entry1);
	table.Insert(&entry2);

	// entry3 is not in the table, but we'll remove it anyway.
	table.Remove(&entry3);
	table.RemoveUnchecked(&entry3);

	// The original two entries should still be there.
	CPPUNIT_ASSERT_EQUAL(table.CountElements(), 2);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), &entry1);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(456), &entry2);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(789), NULL);
}


void BOpenHashTableTest::DuplicateInsertTest()
{
	Entry entry(123);

	BOpenHashTable<EntryDefinition, true, true> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(0), B_OK);

	CPPUNIT_ASSERT_EQUAL(table.Insert(&entry), B_OK);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), &entry);

	CPPUNIT_ASSERT_DEBUGGER(table.Insert(&entry));

	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), &entry);

	// The item can basically never be removed now since there is a cycle,
	// but we'll break into the debugger on remove when that happens as well.
	CPPUNIT_ASSERT_DEBUGGER(table.Remove(&entry));
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), &entry);

	CPPUNIT_ASSERT_DEBUGGER(table.Remove(&entry));
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), &entry);
}


void BOpenHashTableTest::DisableAutoExpandTest()
{
	// Insert multiple items into a table with a fixed size of 1. This
	// essentially turns this BOpenHashTable into a linked list, since resize
	// will never occur.
	Entry entry1(123);
	Entry entry2(456);

	BOpenHashTable<EntryDefinition, false> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(1), B_OK);

	CPPUNIT_ASSERT_EQUAL(table.Insert(&entry1), B_OK);
	CPPUNIT_ASSERT_EQUAL(table.Insert(&entry2), B_OK);
	CPPUNIT_ASSERT_EQUAL(table.CountElements(), 2);
}


void BOpenHashTableTest::InitWithZeroSizeTest()
{
	Entry entry(123);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(0), B_OK);

	CPPUNIT_ASSERT_EQUAL(table.Insert(&entry), B_OK);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(123), &entry);
}


void BOpenHashTableTest::ClearTest()
{
	const size_t kEntryCount = 3;

	BObjectList<Entry> entries(20, true);

	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(), B_OK);

	for (uint32_t i = 0; i < kEntryCount; ++i) {
		Entry* entry = new Entry(i);
		entries.AddItem(entry);
		CPPUNIT_ASSERT_EQUAL(table.Insert(entry), B_OK);
	}

	CPPUNIT_ASSERT_EQUAL(table.CountElements(), kEntryCount);
	CPPUNIT_ASSERT(table.Lookup(2) != NULL);

	CPPUNIT_ASSERT_EQUAL(table.Clear(false), NULL);
	CPPUNIT_ASSERT_EQUAL(table.CountElements(), 0);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(2), NULL);
	CPPUNIT_ASSERT_EQUAL(table.GetIterator().HasNext(), false);
}


void BOpenHashTableTest::ClearAndReturnTest()
{
	// Same as ClearTest(), except that Clear(true) is called, which tells
	// the BOpenHashTable<T> to return a linked list of entries before clearing
	// the table.
	const size_t kEntryCount = 3;
	BOpenHashTable<EntryDefinition> table;
	CPPUNIT_ASSERT_EQUAL(table.Init(), B_OK);

	for (uint32_t i = 0; i < kEntryCount; ++i) {
		Entry* entry = new Entry(i);
		CPPUNIT_ASSERT_EQUAL(table.Insert(entry), B_OK);
	}

	CPPUNIT_ASSERT_EQUAL(table.CountElements(), kEntryCount);
	CPPUNIT_ASSERT(table.Lookup(2) != NULL);

	Entry* head = table.Clear(true);
	CPPUNIT_ASSERT(head != NULL);

	CPPUNIT_ASSERT_EQUAL(table.CountElements(), 0);
	CPPUNIT_ASSERT_EQUAL(table.Lookup(2), NULL);
	CPPUNIT_ASSERT_EQUAL(table.GetIterator().HasNext(), false);

	size_t items_returned = 0;
	while (head != NULL) {
		Entry* next = head->Next();
		delete head;
		head = next;

		++items_returned;
	}

	CPPUNIT_ASSERT_EQUAL(items_returned, kEntryCount);
}

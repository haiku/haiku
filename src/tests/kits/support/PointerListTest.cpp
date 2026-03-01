/*
 * Copyright 2004, Michael Pfeiffer (laplace@users.sourceforge.net).
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>

#include <ObjectList.h>
#include <StopWatch.h>
#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class Item {
public:
	Item() { Init(); }

	Item(const Item& item)
		:
		fValue(item.fValue)
	{
		Init();
	}

	Item(int value)
		:
		fValue(value)
	{
		Init();
	}

	virtual ~Item() { fInstances--; }

	int Value() const { return fValue; }

	bool Equals(const Item* item) const { return item != NULL && fValue == item->fValue; }

	static int GetNumberOfInstances() { return fInstances; }

	void Print() const { fprintf(stderr, "[%d] %d", fID, fValue); }

	static int Compare(const void* a, const void* b)
	{
		const Item* itemA = (const Item*)a;
		const Item* itemB = (const Item*)b;
		if (itemA == itemB)
			return 0;
		if (itemA == NULL)
			return -1;
		if (itemB == NULL)
			return 1;
		return itemA->Value() - itemB->Value();
	}

private:
	void Init()
	{
		fID = fNextID++;
		fInstances++;
	}

	int fID; // unique id for each created Item
	int fValue; // the value of the item

	static int fNextID;
	static int fInstances;
};


int Item::fNextID = 0;
int Item::fInstances = 0;

static void* gData = NULL;


static int
CompareWithData(const void* a, const void* b, void* data)
{
	CPPUNIT_ASSERT(gData == data);
	return Item::Compare(a, b);
}


static void*
CopyTo(void* item, void* data)
{
	_PointerList_* list = (_PointerList_*)data;
	list->AddItem(item);
	return NULL;
}


static void*
FirstItem(void* item, void* data)
{
	return item;
}


class PointerListTest : public CppUnit::TestFixture {
public:
	CPPUNIT_TEST_SUITE(PointerListTest);
	CPPUNIT_TEST(Lifecycle_AddAndRemove_InstancesMatch);
	CPPUNIT_TEST(Owning_ConstructorAndClone_InstancesMatch);
	CPPUNIT_TEST(SortItems_RandomValues_IsSorted);
	CPPUNIT_TEST(HSortItems_RandomValues_IsHSorted);
	CPPUNIT_TEST(SortItems_WithState_IsSorted);
	CPPUNIT_TEST(EachElement_IterateAll_ValidItems);
	CPPUNIT_TEST(BinarySearch_Search_ReturnsCorrectResult);
	CPPUNIT_TEST(BinarySearchIndex_Search_ReturnsCorrectIndex);
	CPPUNIT_TEST(ReplaceItem_InvalidIndex_ReturnsFalse);
	CPPUNIT_TEST(SortItems_RepeatedCalls_NoCrash);
	CPPUNIT_TEST_SUITE_END();

	void Lifecycle_AddAndRemove_InstancesMatch();
	void Owning_ConstructorAndClone_InstancesMatch();
	void SortItems_RandomValues_IsSorted();
	void HSortItems_RandomValues_IsHSorted();
	void SortItems_WithState_IsSorted();
	void EachElement_IterateAll_ValidItems();
	void BinarySearch_Search_ReturnsCorrectResult();
	void BinarySearchIndex_Search_ReturnsCorrectIndex();
	void ReplaceItem_InvalidIndex_ReturnsFalse();
	void SortItems_RepeatedCalls_NoCrash();

private:
	Item* CreateItem();
	void Initialize(_PointerList_& list, int size);
	void MakeEmpty(_PointerList_& list);
	bool Equals(const _PointerList_& list1, const _PointerList_& list2);
	bool IsSorted(const _PointerList_& list, int32 n);

	bool IsSorted(const _PointerList_& list) { return IsSorted(list, list.CountItems()); }

	bool IsHSorted(const _PointerList_& list) { return IsSorted(list, list.CountItems() - 1); }

	int IndexOf(const _PointerList_& list, int value);
};


#define MAX_ID 10000
#define NOT_USED_ID -1
#define NOT_USED_ID_HIGH (MAX_ID + 1)


Item*
PointerListTest::CreateItem()
{
	return new Item(rand() % (MAX_ID + 1));
}


void
PointerListTest::Initialize(_PointerList_& list, int size)
{
	for (int32 i = 0; i < size; i++)
		list.AddItem(CreateItem());
}


void
PointerListTest::MakeEmpty(_PointerList_& list)
{
	const int32 n = list.CountItems();
	for (int32 i = 0; i < n; i++) {
		Item* item = (Item*)list.ItemAt(i);
		delete item;
	}
	list.MakeEmpty();
}


bool
PointerListTest::Equals(const _PointerList_& list1, const _PointerList_& list2)
{
	const int32 n = list1.CountItems();
	if (n != list2.CountItems())
		return false;

	for (int32 i = 0; i < n; i++) {
		Item* item1 = (Item*)list1.ItemAt(i);
		Item* item2 = (Item*)list2.ItemAt(i);
		if (item1 == item2)
			continue;
		if (item1 == NULL || !item1->Equals(item2))
			return false;
	}

	return true;
}


bool
PointerListTest::IsSorted(const _PointerList_& list, int32 n)
{
	int prevValue = -1;
	for (int32 i = 0; i < n; i++) {
		Item* item = (Item*)list.ItemAt(i);
		CPPUNIT_ASSERT(item != NULL);
		int value = item->Value();
		if (value < prevValue)
			return false;
		prevValue = value;
	}
	return true;
}


int
PointerListTest::IndexOf(const _PointerList_& list, int value)
{
	int n = list.CountItems();
	for (int32 i = 0; i < n; i++) {
		Item* item = (Item*)list.ItemAt(i);
		if (item != NULL && item->Value() == value)
			return i;
	}
	return -1;
}


void
PointerListTest::Lifecycle_AddAndRemove_InstancesMatch()
{
	_PointerList_ list;
	int numberOfInstances = Item::GetNumberOfInstances();
	CPPUNIT_ASSERT(list.CountItems() == 0);

	Initialize(list, 10);
	CPPUNIT_ASSERT(list.CountItems() == 10);

	int newInstances = Item::GetNumberOfInstances() - numberOfInstances;
	CPPUNIT_ASSERT_EQUAL(10, newInstances);

	numberOfInstances = Item::GetNumberOfInstances();
	MakeEmpty(list);
	int deletedInstances = numberOfInstances - Item::GetNumberOfInstances();
	CPPUNIT_ASSERT_EQUAL(10, deletedInstances);
}


void
PointerListTest::Owning_ConstructorAndClone_InstancesMatch()
{
	_PointerList_ list(10, true);
	CPPUNIT_ASSERT(list.CountItems() == 0);

	int numberOfInstances = Item::GetNumberOfInstances();
	Initialize(list, 10);
	CPPUNIT_ASSERT(list.CountItems() == 10);
	CPPUNIT_ASSERT_EQUAL(10, Item::GetNumberOfInstances() - numberOfInstances);

	_PointerList_* clone = new _PointerList_(list);
	CPPUNIT_ASSERT_EQUAL(10, Item::GetNumberOfInstances() - numberOfInstances);

	MakeEmpty(list);
	CPPUNIT_ASSERT_EQUAL(0, Item::GetNumberOfInstances() - numberOfInstances);

	delete clone;
	CPPUNIT_ASSERT_EQUAL(0, Item::GetNumberOfInstances() - numberOfInstances);
}


void
PointerListTest::SortItems_RandomValues_IsSorted()
{
	for (int i = 0; i < 10; i++) {
		_PointerList_ list;
		Initialize(list, i);

		list.SortItems(Item::Compare);
		CPPUNIT_ASSERT(IsSorted(list));

		MakeEmpty(list);
	}
}


void
PointerListTest::HSortItems_RandomValues_IsHSorted()
{
	for (int i = 1; i < 10; i++) {
		_PointerList_ list;
		Initialize(list, i);

		_PointerList_ clone(list);
		int lastItem = clone.CountItems() - 1;
		Item* item = (Item*)clone.ItemAt(0);

		clone.HSortItems(Item::Compare);
		CPPUNIT_ASSERT(IsHSorted(clone));
		CPPUNIT_ASSERT_EQUAL(item, (Item*)clone.ItemAt(lastItem));

		MakeEmpty(list);
	}
}


void
PointerListTest::SortItems_WithState_IsSorted()
{
	gData = (void*)0x4711;

	// The original test used FROM=10000 TO=10000, which is just one iteration.
	for (int i = 10; i <= 20; i++) {
		_PointerList_ list;
		Initialize(list, i);

		_PointerList_ clone(list);
		CPPUNIT_ASSERT(Equals(list, clone));

		list.SortItems(CompareWithData, gData);
		CPPUNIT_ASSERT(IsSorted(list));

		list.SortItems(CompareWithData, gData);
		CPPUNIT_ASSERT(IsSorted(list));

		int lastItem = clone.CountItems() - 1;
		bool hasItems = clone.CountItems() > 0;
		Item* item = NULL;
		if (hasItems)
			item = (Item*)clone.ItemAt(0);

		clone.HSortItems(CompareWithData, gData);
		CPPUNIT_ASSERT(IsHSorted(clone));
		CPPUNIT_ASSERT(!hasItems || item == (Item*)clone.ItemAt(lastItem));

		MakeEmpty(list);
	}
}


void
PointerListTest::EachElement_IterateAll_ValidItems()
{
	_PointerList_ list;
	Initialize(list, 10);
	CPPUNIT_ASSERT_EQUAL((int32)10, list.CountItems());

	_PointerList_ clone;
	list.EachElement(CopyTo, &clone);
	CPPUNIT_ASSERT_EQUAL(list.CountItems(), clone.CountItems());

	void* item = list.EachElement(FirstItem, NULL);
	CPPUNIT_ASSERT(item == list.ItemAt(0));

	MakeEmpty(list);
}


void
PointerListTest::BinarySearch_Search_ReturnsCorrectResult()
{
	_PointerList_ list;
	Initialize(list, 10);
	list.SortItems(Item::Compare);
	CPPUNIT_ASSERT(IsSorted(list));

	gData = (void*)0x4711;

	Item notInListLow(NOT_USED_ID);
	Item notInListHigh(NOT_USED_ID_HIGH);

	for (int32 i = 0; i < 10; i++) {
		Item* item = (Item*)list.ItemAt(i);
		CPPUNIT_ASSERT(item != NULL);

		Item* found = (Item*)list.BinarySearch(item, Item::Compare);
		CPPUNIT_ASSERT(item->Equals(found));

		found = (Item*)list.BinarySearch(item, CompareWithData, gData);
		CPPUNIT_ASSERT(item->Equals(found));

		found = (Item*)list.BinarySearch(&notInListLow, Item::Compare);
		CPPUNIT_ASSERT(found == NULL);

		found = (Item*)list.BinarySearch(&notInListHigh, Item::Compare);
		CPPUNIT_ASSERT(found == NULL);
	}

	MakeEmpty(list);
}


class Value {
public:
	Value(int value) : value(value) {}

	int value;
};


static int
ValuePredicate(const void* _item, void* _value)
{
	const Item* item = (const Item*)_item;
	Value* value = (Value*)_value;
	return item->Value() - value->value;
}


void
PointerListTest::BinarySearchIndex_Search_ReturnsCorrectIndex()
{
	_PointerList_ list;
	Initialize(list, 10);
	list.SortItems(Item::Compare);
	CPPUNIT_ASSERT(IsSorted(list));

	Item notInListLow(NOT_USED_ID);
	Item notInListHigh(NOT_USED_ID_HIGH);
	gData = (void*)0x4711;

	for (int32 i = 0; i < 10; i++) {
		Item* item = (Item*)list.ItemAt(i);
		CPPUNIT_ASSERT(item != NULL);
		Value value(item->Value());

		int index = IndexOf(list, item->Value());
		int searchIndex;
		searchIndex = list.BinarySearchIndex(item, Item::Compare);
		CPPUNIT_ASSERT_EQUAL(index, searchIndex);

		searchIndex = list.BinarySearchIndex(item, CompareWithData, gData);
		CPPUNIT_ASSERT_EQUAL(index, searchIndex);

		searchIndex = list.BinarySearchIndexByPredicate(&value, ValuePredicate);
		CPPUNIT_ASSERT_EQUAL(index, searchIndex);

		// notInListLow
		searchIndex = list.BinarySearchIndex(&notInListLow, Item::Compare);
		CPPUNIT_ASSERT_EQUAL(-1, searchIndex);

		// notInListHigh
		searchIndex = list.BinarySearchIndex(&notInListHigh, Item::Compare);
		CPPUNIT_ASSERT_EQUAL(-(list.CountItems() + 1), searchIndex);
	}

	MakeEmpty(list);

	for (int i = 0; i < 3; i++)
		list.AddItem(new Item(2 * i));
	Item notInList(3);
	CPPUNIT_ASSERT_EQUAL(-1, IndexOf(list, 3));

	int index = list.BinarySearchIndex(&notInList, Item::Compare);
	CPPUNIT_ASSERT_EQUAL(-3, index);

	MakeEmpty(list);
}


void
PointerListTest::ReplaceItem_InvalidIndex_ReturnsFalse()
{
	_PointerList_ list;
	Initialize(list, 10);
	// R5 crashes on many NULL parameters, so we only test what's safe or expected
	// to fail in Haiku
	CPPUNIT_ASSERT(!list.ReplaceItem(-1, NULL));
	CPPUNIT_ASSERT(!list.ReplaceItem(100, NULL));
	MakeEmpty(list);
}


static int
SortItemTestPositive(const void* item1, const void* item2)
{
	return 1;
}


static int
SortItemTestNegative(const void* item1, const void* item2)
{
	return -1;
}


static int
SortItemTestEqual(const void* item1, const void* item2)
{
	return 0;
}


void
PointerListTest::SortItems_RepeatedCalls_NoCrash()
{
	_PointerList_ list;
	for (int i = 0; i < 16; i++) { // FIXME: crashes at i = 16???
		list.AddItem(new Item(i));
		list.SortItems(SortItemTestPositive);
		list.SortItems(SortItemTestNegative);
		list.SortItems(SortItemTestEqual);
	}
	MakeEmpty(list);
}


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(PointerListTest, getTestSuiteName());

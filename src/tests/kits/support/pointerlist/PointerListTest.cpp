/* 
** Copyright 2004, Michael Pfeiffer (laplace@users.sourceforge.net).
** Distributed under the terms of the MIT License.
**
*/

#include "ObjectList.h"
#include <StopWatch.h>
#include <stdio.h>
#include <stdlib.h>

#include "PointerListTest.h"

AssertStatistics *AssertStatistics::fStatistics = NULL;

AssertStatistics::AssertStatistics()
{
	fAssertions = 0;
	fPassed = 0;
	fFailed = 0;
}

AssertStatistics* AssertStatistics::GetInstance()
{
	if (fStatistics == NULL) {
		fStatistics = new AssertStatistics();
	}
	return fStatistics;
}

void AssertStatistics::Print()
{
	fprintf(stderr, "Assert Statistics:\n");
	fprintf(stderr, "Assertions: %d\n", fAssertions);
	fprintf(stderr, "Passed: %d\n", fPassed);
	fprintf(stderr, "Failed: %d\n", fFailed);
}

#undef assert
#define assert(expr) \
	if (!(expr)) {\
		fprintf(stderr, "FAILED [%d] ("#expr")\n", __LINE__); \
		AssertStatistics::GetInstance()->AssertFailed(); \
	}\
	else {\
		AssertStatistics::GetInstance()->AssertPassed(); \
	}
	

int Item::Compare(const void* a, const void* b)
{
	Item* itemA = (Item*)a;
	Item* itemB = (Item*)b;
	if (itemA == itemB) return 0;
	if (itemA == NULL) return -1;
	if (itemB == NULL) return 1;
	return itemA->Value() - itemB->Value();
}

int Item::fNextID = 0;
int Item::fInstances = 0;

class PointerListTest
{
public:
	Item* CreateItem();
	void Initialize(_PointerList_& list, int size);
	void Print(const _PointerList_& list);
	void MakeEmpty(_PointerList_& list);
	bool Equals(const _PointerList_& list1, const _PointerList_& list2);
	bool IsSorted(const _PointerList_& list, int32 n);
	bool IsSorted(const _PointerList_& list) { return IsSorted(list, list.CountItems()); }
	bool IsHSorted(const _PointerList_& list) { return IsSorted(list, list.CountItems()-1); }
	int  IndexOf(const _PointerList_& list, int value);
	Item* ItemFor(const _PointerList_& list, int value);

	void CreationTest();
	void OwningTest();
	void SortTest();
	void SortTestWithState();
	void EachElementTest();
	void BinarySearchTest();
	void BinarySearchIndexTest();
	void NullTest();
	void Run();	
};

#define MAX_ID 10000
#define NOT_USED_ID -1
#define NOT_USED_ID_HIGH (MAX_ID+1)

// Create an Item with a random value
Item* PointerListTest::CreateItem()
{
	return new Item(rand() % (MAX_ID+1));
}

// Add size number of new items to the list.
void PointerListTest::Initialize(_PointerList_& list, int size)
{
	for (int32 i = 0; i < size; i ++) {
		list.AddItem(CreateItem());
	}
}

// Print the list to stderr
void PointerListTest::Print(const _PointerList_& list)
{
	const int32 n = list.CountItems();
	for (int32 i = 0; i < n; i ++) {
		Item* item = (Item*)list.ItemAt(i);
		if (i > 0) {
			fprintf(stderr, ", ");
		}
		if (item != NULL) {
			item->Print();
		} else {
			fprintf(stderr, "NULL");
		}
	}
	fprintf(stderr, "\n");
}

// delete the Items in the list
void PointerListTest::MakeEmpty(_PointerList_& list)
{
	const int32 n = list.CountItems();
	for (int32 i = 0; i < n; i ++) {
		Item* item = (Item*)list.ItemAt(i);
		if (item != NULL) {
			delete item;
		}
	}
	list.MakeEmpty();
}

// contain the lists the same items or values
bool PointerListTest::Equals(const _PointerList_& list1, const _PointerList_& list2)
{
	const int32 n = list1.CountItems();

	if (n != list2.CountItems()) return false;
	
	for (int32 i = 0; i < n; i ++) {
		Item* item1 = (Item*)list1.ItemAt(i);
		Item* item2 = (Item*)list2.ItemAt(i);
		if (item1 == item2) continue;
		if (item1 == NULL || !item1->Equals(item2)) return false;
	}
	
	return true;
}

// is the list sorted
bool PointerListTest::IsSorted(const _PointerList_& list, int32 n)
{
	int prevValue = -1; // item values are >= 0
	for (int32 i = 0; i < n; i ++) {
		Item* item = (Item*)list.ItemAt(i);
		assert(item != NULL);
		int value = item->Value();
		if (value < prevValue) {
			return false;
		}
		prevValue = value;
	}
	return true;
}

int PointerListTest::IndexOf(const _PointerList_& list, int value)
{
	int n = list.CountItems();
	for (int32 i = 0; i < n; i ++) {
		Item* item = (Item*)list.ItemAt(i);
		if (item != NULL && item->Value() == value) {
			return i;
		}
	}
	return -1;
}

Item* PointerListTest::ItemFor(const _PointerList_& list, int value)
{
	int32 index = IndexOf(list, value);
	if (index >= 0) {
		list.ItemAt(index);
	}
	return NULL;
}



void PointerListTest::CreationTest()
{
	_PointerList_ list;
	int numberOfInstances = Item::GetNumberOfInstances();
	assert(list.Owning() == false);
		
	assert(list.CountItems() == 0);
	Initialize(list, 10);
	
	assert(list.CountItems() == 10);
	
	int newInstances = Item::GetNumberOfInstances() - numberOfInstances;
	assert(newInstances == 10);

	numberOfInstances = Item::GetNumberOfInstances();
	MakeEmpty(list);
	int deletedInstances = numberOfInstances - Item::GetNumberOfInstances();
	assert(deletedInstances == 10);
}

void PointerListTest::OwningTest()
{
	_PointerList_ list(10, true);
	assert(list.CountItems() == 0);
	assert(list.Owning() == true);
	
	int numberOfInstances = Item::GetNumberOfInstances();
	Initialize(list, 10);
	assert(list.CountItems() == 10);
	assert(Item::GetNumberOfInstances() - numberOfInstances == 10);
	
	_PointerList_* clone = new _PointerList_(list);
	assert(Item::GetNumberOfInstances() - numberOfInstances == 10);
	assert(clone->Owning() == true);
	
	MakeEmpty(list);
	assert(Item::GetNumberOfInstances() - numberOfInstances == 0);

	delete clone;	
	assert(Item::GetNumberOfInstances() - numberOfInstances == 0);
}

void PointerListTest::SortTest()
{
	for (int i = 0; i < 10; i ++) {
		_PointerList_ list;
		Initialize(list, i);
	
		_PointerList_ clone(list);
		assert(Equals(list, clone));
		assert(clone.Owning() == false);

		list.SortItems(Item::Compare);
		assert(IsSorted(list));

		int lastItem = clone.CountItems()-1;
		bool hasItems = clone.CountItems() > 0;
		Item* item = NULL;
		if (hasItems) {
			item = (Item*)clone.ItemAt(0);
		}
		
		// HSortItems seems to put the first item at the end of the list
		// and sort the rest.
		clone.HSortItems(Item::Compare);
		assert(IsHSorted(clone));
		assert(!hasItems || item == (Item*)clone.ItemAt(lastItem));
		
		MakeEmpty(list);
	}
}

static void* gData = NULL;

int Compare(const void* a, const void* b, void* data)
{
	// check data has the expected value
	assert(gData == data);
	return Item::Compare(a, b);
}
#define FROM 10000
#define TO   10000
void PointerListTest::SortTestWithState()
{
	gData = (void*)0x4711;
	
	for (int i = FROM; i < (TO+1); i ++) {
		BStopWatch* watch = new BStopWatch("Initialize");
		_PointerList_ list;
		Initialize(list, i);
		delete watch;
	
		watch = new BStopWatch("Clone");
		_PointerList_ clone(list);
		delete watch;
		assert(Equals(list, clone));
		assert(clone.Owning() == false);

		watch = new BStopWatch("SortItems");
		list.SortItems(::Compare, gData);
		delete watch;
		assert(IsSorted(list));
		
		watch = new BStopWatch("SortItems (sorted list)");
		list.SortItems(::Compare, gData);
		delete watch;
		assert(IsSorted(list));

		int lastItem = clone.CountItems()-1;
		bool hasItems = clone.CountItems() > 0;
		Item* item = NULL;
		if (hasItems) {
			item = (Item*)clone.ItemAt(0);
		}
		
		// HSortItems seems to put the first item at the end of the list
		// and sort the rest.
		watch = new BStopWatch("HSortItems");
		clone.HSortItems(Compare, gData);
		delete watch;
		assert(IsHSorted(clone));
		assert(!hasItems || item == (Item*)clone.ItemAt(lastItem));
		
		watch = new BStopWatch("MakeEmpty");
		MakeEmpty(list);
		delete watch;
	}
}

void* CopyTo(void* item, void* data)
{
	_PointerList_* list = (_PointerList_*)data;
	list->AddItem(item);
	return NULL;
}

void* FirstItem(void* item, void* data)
{
	return item;
}

void PointerListTest::EachElementTest()
{
	_PointerList_ list;
	Initialize(list, 10);
	assert(list.CountItems() == 10);
	
	_PointerList_ clone;
	list.EachElement(CopyTo, &clone);
	assert(clone.CountItems() == list.CountItems());

	void* item = list.EachElement(FirstItem, NULL);
	assert (item == list.ItemAt(0));
	
	MakeEmpty(list);
}

void PointerListTest::BinarySearchTest()
{
	_PointerList_ list;
	Initialize(list, 10);
	list.SortItems(Item::Compare);
	assert(IsSorted(list));
	
	gData = (void*)0x4711;
	
	Item notInListLow(NOT_USED_ID);
	Item notInListHigh(NOT_USED_ID_HIGH);
	
	for (int32 i = 0; i < 10; i ++) {
		Item* item = (Item*)list.ItemAt(i);
		assert(item != NULL);
		
		Item* found = (Item*)list.BinarySearch(item, Item::Compare);
		assert(item->Equals(found));

		found = (Item*)list.BinarySearch(item, ::Compare, gData);
		assert(item->Equals(found));

		found = (Item*)list.BinarySearch(&notInListLow, Item::Compare);
		assert(found == NULL);

		found = (Item*)list.BinarySearch(&notInListLow, ::Compare, gData);
		assert(found == NULL);

		found = (Item*)list.BinarySearch(&notInListHigh, Item::Compare);
		assert(found == NULL);

		found = (Item*)list.BinarySearch(&notInListHigh, ::Compare, gData);
		assert(found == NULL);
	}	
	
	MakeEmpty(list);
}

class Value
{
public:
	Value(int value) : value(value) {};
	int value;
};

static int ValuePredicate(const void* _item, void* _value)
{
	Item* item = (Item*)_item;
	Value* value = (Value*)_value;
	return item->Value() - value->value;
}

void PointerListTest::BinarySearchIndexTest()
{
	_PointerList_ list;
	Initialize(list, 10);
	list.SortItems(Item::Compare);
	assert(IsSorted(list));
	
	Item notInListLow(NOT_USED_ID);
	Item notInListHigh(NOT_USED_ID_HIGH);
	gData = (void*)0x4711;
	
	for (int32 i = 0; i < 10; i ++) {
		Item* item = (Item*)list.ItemAt(i);
		assert(item != NULL);
		Value value(item->Value());

		int index = IndexOf(list, item->Value());
		int searchIndex;
		searchIndex = list.BinarySearchIndex(item, Item::Compare);
		assert(index == searchIndex);

		searchIndex = list.BinarySearchIndex(item, ::Compare, gData);
		assert(index == searchIndex);
		
		searchIndex = list.BinarySearchIndexByPredicate(&value, ValuePredicate);
		assert(index == searchIndex);

		// notInListLow
		searchIndex = list.BinarySearchIndex(&notInListLow, Item::Compare);
		assert(searchIndex == -1);

		searchIndex = list.BinarySearchIndex(&notInListLow, ::Compare, gData);
		assert(searchIndex == -1);
		
		value.value = notInListLow.Value();
		searchIndex = list.BinarySearchIndexByPredicate(&value, ValuePredicate);
		assert(searchIndex == -1);

		// notInListHigh
		searchIndex = list.BinarySearchIndex(&notInListHigh, Item::Compare);
		assert(searchIndex == -(list.CountItems()+1));

		searchIndex = list.BinarySearchIndex(&notInListHigh, ::Compare, gData);
		assert(searchIndex == -(list.CountItems()+1));
		
		value.value = notInListHigh.Value();
		searchIndex = list.BinarySearchIndexByPredicate(&value, ValuePredicate);
		assert(searchIndex == -(list.CountItems()+1));
	}

	MakeEmpty(list);
	
	for (int i = 0; i < 3; i ++) {
		list.AddItem(new Item(2 * i));
	}
	Item notInList(3);
	assert(IndexOf(list, 3) == -1);
	
	int index = list.BinarySearchIndex(&notInList, Item::Compare);
	assert (index == -3);
	
	index = list.BinarySearchIndex(&notInList, ::Compare, gData);
	assert (index == -3);
	
	Value value(notInList.Value());
	index = list.BinarySearchIndexByPredicate(&value, ValuePredicate);
	assert (index == -3);
		
	MakeEmpty(list);
}

void PointerListTest::NullTest()
{
	_PointerList_ list;
	Initialize(list, 10);
	// R5 crashes
	// list.EachElement(NULL, NULL);
	// list.SortItems(NULL);
	// list.SortItems(NULL, NULL);
	// list.HSortItems(NULL);
	// list.HSortItems(NULL, NULL);
	// list.BinarySearch(NULL, NULL);
	// list.BinarySearch(NULL, NULL, NULL);
	// list.BinarySearchIndex(NULL, NULL);
	// list.BinarySearchIndex(NULL, NULL, NULL);
	// list.BinarySearchIndexByPredicate(NULL, NULL);
	assert(!list.ReplaceItem(-1, NULL));
	assert(!list.ReplaceItem(100, NULL));
}

void PointerListTest::Run() 
{
	CreationTest();	
	OwningTest();
	SortTest();
	SortTestWithState();
	EachElementTest();
	BinarySearchTest();
	BinarySearchIndexTest();
	NullTest();
}

int main(int argc, char* argv[]) 
{
	// initialize srand with constant to get reproducable results
	srand(0);
	PointerListTest test;
	test.Run();
	AssertStatistics::GetInstance()->Print();
}
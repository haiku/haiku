/*
 * Copyright 2018, Sean Healy
 * Distributed under the terms of the MIT License.
 */



#include "common.h"

#include <Application.h>
#include <Window.h>
#include <TestUtils.h>


#include <OutlineListView.h>

BListItem* gExpected[16];
int gIndex = 0;
int gCount = 0;

namespace CppUnit {

template<>
struct assertion_traits<BListItem*>
{
	static bool equal(const BListItem* x, const BListItem* y) {
		return x == y;
	}

	static string toString(const BListItem* x) {
		if (x == NULL)
			return "(null)";
		return ((BStringItem*)x)->Text();
	}
};

}


BListItem*
CheckExpected(BListItem* item, void* arg)
{
	BStringItem* str = (BStringItem*)item;
	fprintf(stderr, "Item @%d: %s\n", gIndex, str == NULL ? "(null)" : str->Text());

	CHK(gIndex < gCount);
	CPPUNIT_ASSERT_EQUAL(gExpected[gIndex], item);
	gIndex++;
	return NULL;
}


BListItem*
FillExpected(BListItem* item, void* arg)
{
	gExpected[gCount] = item;
	gCount++;
	return NULL;
}


void
CheckItemsUnder(BOutlineListView* view, BListItem* superitem, bool oneLevelOnly)
{
	for (int i = 0; i < gCount; i++)
		CPPUNIT_ASSERT_EQUAL(gExpected[i], view->ItemUnderAt(superitem, oneLevelOnly, i));

	// Check that we don't get more items
	CPPUNIT_ASSERT_EQUAL((BListItem*)NULL, view->ItemUnderAt(superitem, oneLevelOnly, gCount));
}


class OutlineListViewTest: public TestCase
{
	public:
		OutlineListViewTest() {}
		OutlineListViewTest(std::string name) : TestCase(name) {}

		void setUp() {
			fApplication = new BApplication("application/x-vnd.OutlineListView.test");
		}
		void tearDown() {
			 delete fApplication;
		}

		void EachItemUnder();
		void AddUnder();
		void ItemUnderAt();

		static Test* Suite();

	private:
		BApplication *fApplication;
		static BOutlineListView* _SetupTest(const char* name);
};


void
OutlineListViewTest::EachItemUnder()
{
	BOutlineListView* view = _SetupTest("OutlineListView_EachItemUnder");

	// First test is easy
	gExpected[0] = view->FullListItemAt(6);
	gExpected[1] = view->FullListItemAt(8);
	gExpected[2] = view->FullListItemAt(9);
	gCount = 3;
	gIndex = 0;

	fprintf(stderr, "Easy test\n");
	view->EachItemUnder(view->FullListItemAt(5), true, CheckExpected, NULL);
	CPPUNIT_ASSERT_EQUAL(gCount, view->CountItemsUnder(view->FullListItemAt(5), true));

	// Check that collapsing an item does not change the outcome
	gIndex = 0;
	view->Collapse(view->FullListItemAt(0));

	fprintf(stderr, "One collapsed\n");
	view->EachItemUnder(view->FullListItemAt(5), true, CheckExpected, NULL);

	gIndex = 0;
	view->Collapse(view->FullListItemAt(5));

	fprintf(stderr, "Two collapsed\n");
	view->EachItemUnder(view->FullListItemAt(5), true, CheckExpected, NULL);
	CPPUNIT_ASSERT_EQUAL(gCount, view->CountItemsUnder(view->FullListItemAt(5), true));

	// Also check deeper levels
	gExpected[1] = view->FullListItemAt(7);
	gExpected[2] = view->FullListItemAt(8);
	gExpected[3] = view->FullListItemAt(9);
	gCount = 4;
	gIndex = 0;

	fprintf(stderr, "All levels\n");
	view->EachItemUnder(view->FullListItemAt(5), false, CheckExpected, NULL);
	CPPUNIT_ASSERT_EQUAL(gCount, view->CountItemsUnder(view->FullListItemAt(5), false));

	view->Expand(view->FullListItemAt(5));
	view->Collapse(view->FullListItemAt(6));
	gIndex = 0;

	fprintf(stderr, "All levels with a collapsed sublevel\n");
	view->EachItemUnder(view->FullListItemAt(5), false, CheckExpected, NULL);
	CPPUNIT_ASSERT_EQUAL(gCount, view->CountItemsUnder(view->FullListItemAt(5), false));

	// NULL is the parent of level 0 items
	gExpected[0] = view->FullListItemAt(0);
	gExpected[1] = view->FullListItemAt(5);
	gExpected[2] = view->FullListItemAt(10);
	gCount = 3;
	gIndex = 0;

	fprintf(stderr, "Level 0\n");
	view->EachItemUnder(NULL, true, CheckExpected, NULL);
	CPPUNIT_ASSERT_EQUAL(gCount, view->CountItemsUnder(NULL, true));

	// No visits when the item is not in the list
	BListItem* notfound = new BStringItem("Not found");
	gCount = 0;
	gIndex = 0;

	fprintf(stderr, "Item not in the list\n");
	view->EachItemUnder(notfound, true, CheckExpected, NULL);
	CPPUNIT_ASSERT_EQUAL(gCount, view->CountItemsUnder(notfound, true));
	view->EachItemUnder(notfound, false, CheckExpected, NULL);
	CPPUNIT_ASSERT_EQUAL(gCount, view->CountItemsUnder(notfound, false));

	// Don't actually run anything
	delete view->Window();
}


void
OutlineListViewTest::AddUnder()
{
	BOutlineListView* view = _SetupTest("OutlineListView_AddUnder");

	BListItem* one = view->FullListItemAt(0);
	BListItem* oneA = view->FullListItemAt(1);
	BListItem* oneA0 = new BStringItem("One-A-0");
	BListItem* oneA1 = view->FullListItemAt(2);

	int32 count = view->FullListCountItems();

	BListItem* last = view->FullListItemAt(count - 1);
	BListItem* newLast = new BStringItem("NewLast");

	view->AddUnder(newLast, NULL);
	view->AddUnder(oneA0, oneA);

	fprintf(stderr, "Count\n");
	CPPUNIT_ASSERT_EQUAL(count + 2, view->FullListCountItems());

	fprintf(stderr, "Insertion order\n");
	CPPUNIT_ASSERT_EQUAL(one, view->FullListItemAt(0));
	CPPUNIT_ASSERT_EQUAL(oneA, view->FullListItemAt(1));
	CPPUNIT_ASSERT_EQUAL(oneA0, view->FullListItemAt(2));
	CPPUNIT_ASSERT_EQUAL(oneA1, view->FullListItemAt(3));
	CPPUNIT_ASSERT_EQUAL(last, view->FullListItemAt(count));
	CPPUNIT_ASSERT_EQUAL(newLast, view->FullListItemAt(count + 1));

	fprintf(stderr, "Levels\n");
	CPPUNIT_ASSERT_EQUAL(0, one->OutlineLevel());
	CPPUNIT_ASSERT_EQUAL(1, oneA->OutlineLevel());
	CPPUNIT_ASSERT_EQUAL(2, oneA0->OutlineLevel());
	CPPUNIT_ASSERT_EQUAL(2, oneA1->OutlineLevel());
	CPPUNIT_ASSERT_EQUAL(0, newLast->OutlineLevel());

	// Don't actually run anything
	delete view->Window();
}


void
OutlineListViewTest::ItemUnderAt()
{
	BOutlineListView* view = _SetupTest("OutlineListView_ItemUnderAt");

	// EachItemUnder has already been checked, we can use it to know what to expect
	gCount = 0;
	view->EachItemUnder(view->FullListItemAt(5), true, FillExpected, NULL);

	fprintf(stderr, "Easy test\n");
	CheckItemsUnder(view, view->FullListItemAt(5), true);

	// Check that collapsing an item does not change the outcome
	view->Collapse(view->FullListItemAt(0));

	fprintf(stderr, "One collapsed\n");
	CheckItemsUnder(view, view->FullListItemAt(5), true);

	view->Collapse(view->FullListItemAt(5));

	fprintf(stderr, "Two collapsed\n");
	CheckItemsUnder(view, view->FullListItemAt(5), true);

	// Also check deeper levels
	gCount = 0;
	view->EachItemUnder(view->FullListItemAt(5), false, FillExpected, NULL);

	fprintf(stderr, "All levels\n");
	CheckItemsUnder(view, view->FullListItemAt(5), false);

	view->Expand(view->FullListItemAt(5));
	view->Collapse(view->FullListItemAt(6));

	fprintf(stderr, "All levels with a collapsed sublevel\n");
	CheckItemsUnder(view, view->FullListItemAt(5), false);

	// NULL is the parent of level 0 items
	gCount = 0;
	view->EachItemUnder(NULL, true, FillExpected, NULL);

	fprintf(stderr, "Level 0\n");
	CheckItemsUnder(view, NULL, true);

	// Get NULL when the item is not in the list
	BListItem* notfound = new BStringItem("Not found");
	fprintf(stderr, "Item not in the list\n");
	CPPUNIT_ASSERT_EQUAL((BListItem*)NULL, view->ItemUnderAt(notfound, true, 0));
	CPPUNIT_ASSERT_EQUAL((BListItem*)NULL, view->ItemUnderAt(notfound, false, 0));

	// Don't actually run anything
	delete view->Window();
}


Test*
OutlineListViewTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BOutlineListView, SuiteOfTests, OutlineListViewTest, EachItemUnder);
	ADD_TEST4(BOutlineListView, SuiteOfTests, OutlineListViewTest, AddUnder);
	ADD_TEST4(BOutlineListView, SuiteOfTests, OutlineListViewTest, ItemUnderAt);

	return SuiteOfTests;
}


BOutlineListView*
OutlineListViewTest::_SetupTest(const char* name)
{
	BWindow* window = new BWindow(BRect(50, 50, 550, 550), name,
		B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE, 0);

	BOutlineListView* view = new BOutlineListView(BRect(5, 5, 495, 495), "View",
		B_MULTIPLE_SELECTION_LIST, B_FOLLOW_ALL);
	window->AddChild(view);

	view->AddItem(new BStringItem("One", 0));
	view->AddItem(new BStringItem("One-A", 1));
	view->AddItem(new BStringItem("One-A-1", 2));
	view->AddItem(new BStringItem("One-B", 1));
	view->AddItem(new BStringItem("One-C", 1));

	view->AddItem(new BStringItem("Two", 0));
	view->AddItem(new BStringItem("Two-A", 1));
	view->AddItem(new BStringItem("Two-A-1", 2));
	view->AddItem(new BStringItem("Two-B", 1));
	view->AddItem(new BStringItem("Two-C", 1));

	view->AddItem(new BStringItem("Three", 0));
	view->AddItem(new BStringItem("Three-A", 1));
	view->AddItem(new BStringItem("Three-A-1", 2));
	view->AddItem(new BStringItem("Three-B", 1));
	view->AddItem(new BStringItem("Three-C", 1));

	return view;
}


CppUnit::Test* OutlineListViewTestSuite()
{
	CppUnit::TestSuite* testSuite = new CppUnit::TestSuite();

	testSuite->addTest(OutlineListViewTest::Suite());

	return testSuite;
}

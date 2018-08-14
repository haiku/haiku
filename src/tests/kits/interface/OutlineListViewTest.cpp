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


BListItem* eachitemunder(BListItem* item, void* arg) {
	BStringItem* str = (BStringItem*)item;
	fprintf(stderr, "Item @%d: %s\n", gIndex, str->Text());

	CHK(gIndex < gCount);
	CPPUNIT_ASSERT_EQUAL(item, gExpected[gIndex]);
	gIndex++;
	return NULL;
}


class OutlineListViewTest: public TestCase
{
	public:
		OutlineListViewTest() {}
		OutlineListViewTest(std::string name) : TestCase(name) {}

		void EachItemUnder();

		static Test* Suite();
};


void OutlineListViewTest::EachItemUnder() {
	BApplication app(
		"application/x-vnd.OutlineListView_EachItemUnder.test");
	BWindow* window = new BWindow(BRect(50,50,550,550),
		"OutlineListView_EachItemUnder", B_TITLED_WINDOW,
		B_QUIT_ON_WINDOW_CLOSE, 0);
	BOutlineListView* view = new BOutlineListView(BRect(5,5,495,495), "View",
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

	// First test is easy
	gExpected[0] = view->FullListItemAt(6);
	gExpected[1] = view->FullListItemAt(8);
	gExpected[2] = view->FullListItemAt(9);
	gCount = 3;
	gIndex = 0;

	fprintf(stderr, "Easy test\n");
	view->EachItemUnder(view->FullListItemAt(5), true, eachitemunder, NULL);

	// Check that collapsing an item does not change the outcome
	gIndex = 0;
	view->Collapse(view->FullListItemAt(0));

	fprintf(stderr, "One collapsed\n");
	view->EachItemUnder(view->FullListItemAt(5), true, eachitemunder, NULL);

	gIndex = 0;
	view->Collapse(view->FullListItemAt(5));

	fprintf(stderr, "Two collapsed\n");
	view->EachItemUnder(view->FullListItemAt(5), true, eachitemunder, NULL);

	// Don't actually run anything
	delete window;
}


Test* OutlineListViewTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BOutlineListView, SuiteOfTests, OutlineListViewTest,
		EachItemUnder);

	return SuiteOfTests;
}


CppUnit::Test* OutlineListViewTestSuite()
{
	CppUnit::TestSuite* testSuite = new CppUnit::TestSuite();

	testSuite->addTest(OutlineListViewTest::Suite());

	return testSuite;
}

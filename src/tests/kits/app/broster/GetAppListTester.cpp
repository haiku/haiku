//------------------------------------------------------------------------------
//	GetAppListTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <OS.h>
#include <Handler.h>
#include <List.h>
#include <Looper.h>
#include <Roster.h>
#include <String.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "AppRunner.h"
#include "GetAppListTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// contains_list
static
bool
contains_list(const BList &a, const BList &b)
{
	int32 bCount = b.CountItems();
	bool contains = true;
	for (int32 i = 0; contains && i < bCount; i++)
		contains = a.HasItem(b.ItemAt(i));
	return contains;
}

// check_list
static
void
check_list(const BList &toCheck, const BList &base, const BList &extendedBase,
		   const BList &expected)
{
	// toCheck and extendedBase must have prefix base
	int32 baseCount = base.CountItems();
	for (int32 i = 0; i < baseCount; i++) {
		CHK(base.ItemAt(i) == toCheck.ItemAt(i));
		CHK(base.ItemAt(i) == extendedBase.ItemAt(i));
	}
	// toCheck must have correct size
	int32 toCheckCount = toCheck.CountItems();
	int32 extendedBaseCount = extendedBase.CountItems();
	int32 expectedCount = expected.CountItems();
	CHK(toCheckCount == extendedBaseCount + expectedCount);
	// toCheck must contain all elements of extendedBase and expected
	// (arbitrary order)
	BList list(extendedBase);
	list.AddList((BList*)&expected);
	CHK(contains_list(toCheck, list));
	CHK(contains_list(list, toCheck));
}

// check_list
static
void
check_list(const BList &toCheck, const BList &base, const BList &expected)
{
	check_list(toCheck, base, base, expected);
}

// check_list
static
void
check_list(const BList &toCheck, const BList &expected)
{
	BList base;
	check_list(toCheck, base, expected);
}


/*
	void GetAppList(BList *teamIDList) const
	@case 1			teamIDList is NULL
	@results		Should do nothing.
*/
void GetAppListTester::GetAppListTestA1()
{
// R5: crashes when passing a NULL BList
#ifndef TEST_R5
	BRoster roster;
	roster.GetAppList(NULL);
#endif
}

/*
	void GetAppList(BList *teamIDList) const
	@case 2			teamIDList is not NULL and not empty
	@results		Should append the team IDs of all running apps to
					teamIDList.
*/
void GetAppListTester::GetAppListTestA2()
{
	// create a list with some dummy entries
	BList list;
	list.AddItem((void*)-7);
	list.AddItem((void*)-42);
	// get a list of running applications for reference
	BRoster roster;
	BList list1(list);
	roster.GetAppList(&list1);
	// run some apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	AppRunner runner3(true);
	CHK(runner1.Run("AppRunTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp2") == B_OK);
	CHK(runner3.Run("BMessengerTestApp1") == B_OK);
	BList expectedApps;
	expectedApps.AddItem((void*)runner1.Team());
	expectedApps.AddItem((void*)runner2.Team());
	expectedApps.AddItem((void*)runner3.Team());
	// get a new app list and check it
	BList list2(list);
	roster.GetAppList(&list2);
	check_list(list2, list, list1, expectedApps);
	// quit app 1
	runner1.WaitFor(true);
	expectedApps.RemoveItem((void*)runner1.Team());
	BList list3(list);
	roster.GetAppList(&list3);
	check_list(list3, list, list1, expectedApps);
	// quit app 2
	runner2.WaitFor(true);
	expectedApps.RemoveItem((void*)runner2.Team());
	BList list4(list);
	roster.GetAppList(&list4);
	check_list(list4, list, list1, expectedApps);
	// quit app 3
	runner3.WaitFor(true);
	expectedApps.RemoveItem((void*)runner3.Team());
	BList list5(list);
	roster.GetAppList(&list5);
	check_list(list5, list, list1, expectedApps);
}

/*
	void GetAppList(const char *signature, BList *teamIDList) const
	@case 1			signature or teamIDList are NULL
	@results		Should do nothing/should not modify teamIDList.
*/
void GetAppListTester::GetAppListTestB1()
{
// R5: crashes when passing a NULL signature/BList
#ifndef TEST_R5
	const char *signature = "application/x-vnd.obos-app-run-testapp1";
	// create a list with some dummy entries
	BList emptyList;
	BList list;
	list.AddItem((void*)-7);
	list.AddItem((void*)-42);
	// NULL signature and list
	BRoster roster;
	roster.GetAppList(NULL, NULL);
	// NULL signature
	BList list1(list);
	roster.GetAppList(NULL, &list1);
	check_list(list1, list, list, emptyList);
	// NULL list
	AppRunner runner(true);
	CHK(runner.Run("AppRunTestApp1") == B_OK);
	roster.GetAppList(signature, NULL);
	runner.WaitFor(true);
#endif
}

/*
	void GetAppList(const char *signature, BList *teamIDList) const
	@case 2			teamIDList is not NULL and not empty, signature is not
					NULL, but no app with this signature is running
	@results		Should not modify teamIDList.
*/
void GetAppListTester::GetAppListTestB2()
{
	const char *signature = "application/x-vnd.obos-does-not-exist";
	// create a list with some dummy entries
	BList list;
	list.AddItem((void*)-7);
	list.AddItem((void*)-42);
	// get a list of running applications for reference
	BRoster roster;
	BList list1(list);
	roster.GetAppList(signature, &list1);
	// run some apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	AppRunner runner3(true);
	CHK(runner1.Run("AppRunTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp2") == B_OK);
	CHK(runner3.Run("BMessengerTestApp1") == B_OK);
	BList expectedApps;
	// get a new app list and check it
	BList list2(list);
	roster.GetAppList(signature, &list2);
	check_list(list2, list, list1, expectedApps);
	// quit app 1
	runner1.WaitFor(true);
	BList list3(list);
	roster.GetAppList(signature, &list3);
	check_list(list3, list, list1, expectedApps);
	// quit app 2
	runner2.WaitFor(true);
	BList list4(list);
	roster.GetAppList(signature, &list4);
	check_list(list4, list, list1, expectedApps);
	// quit app 3
	runner3.WaitFor(true);
	BList list5(list);
	roster.GetAppList(signature, &list5);
	check_list(list5, list, list1, expectedApps);
}

/*
	void GetAppList(const char *signature, BList *teamIDList) const
	@case 3			teamIDList is not NULL and not empty, signature is not
					NULL and app(s) with this signature is (are) running
	@results		Should append the team IDs of all running apps with the
					supplied signature to teamIDList.
*/
void GetAppListTester::GetAppListTestB3()
{
	const char *signature = "application/x-vnd.obos-app-run-testapp1";
	// create a list with some dummy entries
	BList list;
	list.AddItem((void*)-7);
	list.AddItem((void*)-42);
	// get a list of running applications for reference
	BRoster roster;
	BList list1(list);
	roster.GetAppList(signature, &list1);
	check_list(list1, list);
	// run some apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	AppRunner runner3(true);
	CHK(runner1.Run("AppRunTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp2") == B_OK);
	CHK(runner3.Run("BMessengerTestApp1") == B_OK);
	BList expectedApps;
	expectedApps.AddItem((void*)runner1.Team());
	expectedApps.AddItem((void*)runner2.Team());
	// get a new app list and check it
	BList list2(list);
	roster.GetAppList(signature, &list2);
	check_list(list2, list, expectedApps);
	// quit app 1
	runner1.WaitFor(true);
	expectedApps.RemoveItem((void*)runner1.Team());
	BList list3(list);
	roster.GetAppList(signature, &list3);
	check_list(list3, list, expectedApps);
	// quit app 2
	runner2.WaitFor(true);
	expectedApps.RemoveItem((void*)runner2.Team());
	BList list4(list);
	roster.GetAppList(signature, &list4);
	check_list(list4, list, expectedApps);
	// quit app 3
	runner3.WaitFor(true);
	BList list5(list);
	roster.GetAppList(signature, &list5);
	check_list(list5, list, expectedApps);
}


Test* GetAppListTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, GetAppListTester, GetAppListTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, GetAppListTester, GetAppListTestA2);

	ADD_TEST4(BRoster, SuiteOfTests, GetAppListTester, GetAppListTestB1);
	ADD_TEST4(BRoster, SuiteOfTests, GetAppListTester, GetAppListTestB2);
	ADD_TEST4(BRoster, SuiteOfTests, GetAppListTester, GetAppListTestB3);

	return SuiteOfTests;
}


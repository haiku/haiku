//------------------------------------------------------------------------------
//	GetRecentTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Entry.h>
#include <fs_attr.h>
#include <Message.h>
#include <Node.h>
#include <Roster.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "GetRecentTester.h"
#include "testapps/RecentAppsTestApp.h"

// Local Defines ---------------------------------------------------------------
status_t check_ref_count(BMessage *refMsg, int32 count);
status_t set_test_app_attributes(const entry_ref *app, const char *sig, const int32 *flags);
status_t launch_test_app(RecentAppsTestAppId id, const int32 *flags);
status_t get_test_app_ref(RecentAppsTestAppId id, entry_ref *ref);

const int32 kFlagsType = 'APPF';

const char *kSigAttr = "BEOS:APP_SIG";
const char *kFlagsAttr = "BEOS:APP_FLAGS";

const int32 kSingleLaunchFlags = B_SINGLE_LAUNCH;
const int32 kMultipleLaunchFlags = B_MULTIPLE_LAUNCH;
const int32 kExclusiveLaunchFlags = B_EXCLUSIVE_LAUNCH;

const int32 kSingleLaunchArgvOnlyFlags = B_SINGLE_LAUNCH | B_ARGV_ONLY;
const int32 kMultipleLaunchArgvOnlyFlags = B_MULTIPLE_LAUNCH | B_ARGV_ONLY;
const int32 kExclusiveLaunchArgvOnlyFlags = B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY;

const int32 kSingleLaunchBackgroundFlags = B_SINGLE_LAUNCH | B_BACKGROUND_APP;
const int32 kMultipleLaunchBackgroundFlags = B_MULTIPLE_LAUNCH | B_BACKGROUND_APP;
const int32 kExclusiveLaunchBackgroundFlags = B_EXCLUSIVE_LAUNCH | B_BACKGROUND_APP;

const int32 kSingleLaunchArgvOnlyBackgroundFlags = B_SINGLE_LAUNCH | B_ARGV_ONLY | B_BACKGROUND_APP;
const int32 kMultipleLaunchArgvOnlyBackgroundFlags = B_MULTIPLE_LAUNCH | B_ARGV_ONLY | B_BACKGROUND_APP;
const int32 kExclusiveLaunchArgvOnlyBackgroundFlags = B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY | B_BACKGROUND_APP;

const int32 kQualifyingFlags[] = {
	kSingleLaunchFlags,
	kMultipleLaunchFlags,
	kExclusiveLaunchFlags,
};

const int32 kNonQualifyingFlags[] = {
	kSingleLaunchArgvOnlyFlags,
	kMultipleLaunchArgvOnlyFlags,
	kExclusiveLaunchArgvOnlyFlags,
	kSingleLaunchBackgroundFlags,
	kMultipleLaunchBackgroundFlags,
	kExclusiveLaunchBackgroundFlags,
	kSingleLaunchArgvOnlyBackgroundFlags,
	kMultipleLaunchArgvOnlyBackgroundFlags,
	kExclusiveLaunchArgvOnlyBackgroundFlags,
};

const char *test_types[] = {
	"text/x-vnd.obos-recent-docs-test-type-1",
	"text/x-vnd.obos-recent-docs-test-type-2",
	"obos-test-supertype/x-vnd.obos-recent-docs-test-type-3",
	"obos-test-supertype/x-vnd.obos-recent-docs-test-type-4",
};

struct test_doc {
	const char *name;
	const char *type;
} test_docs[] = {
	{ "untyped", NULL },
	{ "type1a", test_types[0] },
	{ "type1b", test_types[0] },
	{ "type2a", test_types[1] },
	{ "type2b", test_types[1] },
	{ "type2c", test_types[1] },
	{ "type3", test_types[2] },
	{ "type4", test_types[3] },
};


// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*! \brief Verifies that the given BMessage has a \c "refs" field
	containing \a count B_REF_TYPE entries.
	
	\return
	- \c B_OK: The predicate is true
	- \c B_ERROR: The predicate is false
	- other error code: An error occured
*/
status_t
check_ref_count(BMessage *refMsg, int32 count)
{
	type_code type;
	int32 realCount;
	status_t err = refMsg ? B_OK : B_BAD_VALUE;
	
	if (!err)	
		err = refMsg->GetInfo("refs", &type, &realCount);
	if (!err) {
		err = realCount == count ? B_OK : B_ERROR;
		if (!err)
			err = type == B_REF_TYPE ? B_OK : B_ERROR;
	} else if (err == B_NAME_NOT_FOUND && count == 0) {
		err = B_OK;
	}

	return err;
}

/*! \brief Sets or unsets \c BEOS:APP_SIG and \c BEOS:APP_FLAGS attributes for
	the given entry (which is assumed to be an application).
	
	\param app The entry to modify
	\param sig If \c non-NULL, the given signature should be written to the entry's
	           \c BEOS:APP_SIG attribute. If \c NULL, said attribute should be
	           removed if it exists.
	\param sig If \c non-NULL, the given flags should be written to the entry's
	           \c BEOS:APP_FLAG attribute. If \c NULL, said attribute should be
	           removed if it exists.
*/
status_t
set_test_app_attributes(const entry_ref *app, const char *sig, const int32 *flags)
{
	BNode node;
	attr_info info;
	status_t err = app ? B_OK : B_BAD_VALUE;

	// Open a node on the given app
	if (!err) 
		err = node.SetTo(app);
	// Write/remove the appropriate attributes
	if (!err) {
		if (sig) {
			// Set the attribute
			ssize_t bytes = node.WriteAttr(kSigAttr, B_STRING_TYPE, 0, sig, strlen(sig)+1);
			if (bytes >= 0)
				err = bytes == (ssize_t)strlen(sig)+1 ? B_OK : B_FILE_ERROR;
			else
				err = bytes;
		} else {
			// See if the attribute exists and thus needs to be removed
			if (node.GetAttrInfo(kSigAttr, &info) == B_OK)
				err = node.RemoveAttr(kSigAttr);
		}
	}
	if (!err) {
		if (flags) {
			// Set the attribute
			ssize_t bytes = node.WriteAttr(kFlagsAttr, kFlagsType, 0, flags, sizeof(int32));	
			if (bytes >= 0)
				err = bytes == sizeof(int32) ? B_OK : B_FILE_ERROR;
			else
				err = bytes;
		} else {
			// See if the attribute exists and thus needs to be removed
			if (node.GetAttrInfo(kFlagsAttr, &info) == B_OK)
				err = node.RemoveAttr(kFlagsAttr);
		}
	}
	return err;
}

/*! \brief Launches the given test app after first setting the application entry's
	\c BEOS:APP_FLAGS attribute as specified.
	
	See RecentAppsTestApp.h for more info on RecentAppsTestAppId, and see
	set_test_app_attributes() for more info on the nature of the \a flags argument.
*/
status_t
launch_test_app(RecentAppsTestAppId id, const int32 *flags)
{
	entry_ref ref;
	status_t err = get_test_app_ref(id, &ref);
	
	// Set the attributes
	if (!err)
		err = set_test_app_attributes(&ref, kRecentAppsTestAppSigs[id], flags);	
	// Launch the app
	if (!err) {
		BRoster roster;
		err = roster.Launch(&ref);
	}
	// Give it time to do its thing
	if (!err)
		snooze(250000);
	return err;
}

/*! \brief Returns an entry_ref for the given test app in \a ref.
	
	\param id The id of the app of interest
	\param ref Pointer to a pre-allocated \c entry_ref structure
	           into which the result is set.
*/
status_t
get_test_app_ref(RecentAppsTestAppId id, entry_ref *ref)
{
	const char *testDir = BTestShell::GlobalTestDir();
	BEntry entry;

	status_t err = ref ? B_OK : B_BAD_VALUE;
	if (!err)
		err = testDir ? B_OK : B_NAME_NOT_FOUND;
	if (!err) {
		char path[B_PATH_NAME_LENGTH];
		sprintf(path, "%s/../kits/app/%s%s", testDir, kRecentAppsTestAppFilenames[id],
#if TEST_R5
		"_r5"
#else
		""
#endif		
		);
		err = entry.SetTo(path);
	}
	if (!err)
		err = entry.GetRef(ref);
	return err;
}

//------------------------------------------------------------------------------
// GetRecentApps()
//------------------------------------------------------------------------------

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case A1		refList is NULL; maxCount < 0
	@results		R5: crashes
	                OBOS: should quietly do nothing.
*/
void 
GetRecentTester::GetRecentAppsTestA1()
{
#if !TEST_R5
	BRoster roster;
	roster.GetRecentApps(NULL, -10);
#endif
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case A1		refList is NULL; maxCount == 0
	@results		R5: crashes
	                OBOS: should quietly do nothing.
*/
void 
GetRecentTester::GetRecentAppsTestA2()
{
#if !TEST_R5
	BRoster roster;
	roster.GetRecentApps(NULL, 0);
#endif
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case A1		refList is NULL; maxCount > 0
	@results		R5: crashes
	                OBOS: should quietly do nothing.
*/
void 
GetRecentTester::GetRecentAppsTestA3()
{
#if !TEST_R5
	BRoster roster;
	roster.GetRecentApps(NULL, 10);
#endif
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case A1		refList is valid; maxCount < 0
	@results		Should return zero recent apps
*/
void 
GetRecentTester::GetRecentAppsTestB1()
{
	BRoster roster;
	BMessage msg;
	CHK(launch_test_app(kControlApp, &kMultipleLaunchFlags) == B_OK);
	roster.GetRecentApps(&msg, -1);
	CHK(check_ref_count(&msg, 0) == B_OK);
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case A1		refList is valid; maxCount == 0
	@results		Should return zero recent apps
*/
void 
GetRecentTester::GetRecentAppsTestB2()
{
	BRoster roster;
	BMessage msg;
	CHK(launch_test_app(kControlApp, &kMultipleLaunchFlags) == B_OK);
	roster.GetRecentApps(&msg, 0);
	CHK(check_ref_count(&msg, 0) == B_OK);
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case A1		refList is valid; maxCount > 0
	@results		Should return maxCount apps
*/
void 
GetRecentTester::GetRecentAppsTestB3()
{
	BRoster roster;
	BMessage msg;
	CHK(launch_test_app(kControlApp, &kMultipleLaunchFlags) == B_OK);
	roster.GetRecentApps(&msg, 1);
	CHK(check_ref_count(&msg, 1) == B_OK);
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case C1		Our control app is launched, followed by an application
	                with no BEOS:APP_FLAGS attribute.
	@results		The latter application should *not* appear at the top
					of the recent apps list.
*/
void
GetRecentTester::GetRecentAppsTestC1()
{
	BRoster roster;
	BMessage msg;
	entry_ref ref;
	entry_ref appRef1;
	entry_ref appRef2;

	// Launch an app that *will* show up in the list
	NextSubTest();
	CHK(launch_test_app(kControlApp, &kMultipleLaunchFlags) == B_OK);
	roster.GetRecentApps(&msg, 1);
	CHK(msg.FindRef("refs", 0, &ref) == B_OK);
	CHK(get_test_app_ref(kControlApp, &appRef1) == B_OK);
	CHK(appRef1 == ref);

	// Now launch an app with no app flags attribute that
	// therefore shouldn't show up in the list
	NextSubTest();
	CHK(launch_test_app(kEmptyApp, NULL) == B_OK);
	msg.MakeEmpty();
	roster.GetRecentApps(&msg, 5);
	CHK(msg.FindRef("refs", 0, &ref) == B_OK);
	CHK(get_test_app_ref(kEmptyApp, &appRef2) == B_OK);
	CHK(appRef2 != ref);
	CHK(appRef1 == ref);
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case C1		Our control app is launched, followed by an application
	                with a non-qualifying BEOS:APP_FLAGS attribute.
	@results		The latter application should *not* appear at the top
					of the recent apps list.
*/
void
GetRecentTester::GetRecentAppsTestC2()
{
	uint count = sizeof(kNonQualifyingFlags) / sizeof(int32);
	for (uint i = 0; i < count; i++) {
		BRoster roster;
		BMessage msg;
		entry_ref ref1;
		entry_ref ref2;
		entry_ref appRef1;
		entry_ref appRef2;
	
		// Launch an app that *will* show up in the list
		NextSubTest();
		CHK(launch_test_app(kControlApp, &kMultipleLaunchFlags) == B_OK);
		roster.GetRecentApps(&msg, 1);
		CHK(msg.FindRef("refs", 0, &ref1) == B_OK);
		CHK(get_test_app_ref(kControlApp, &appRef1) == B_OK);
		CHK(appRef1 == ref1);
	
		// Now launch an app with a non-qualifying app flags attribute that
		// therefore shouldn't show up in the list
		NextSubTest();
		CHK(launch_test_app(kNonQualifyingApp, &kNonQualifyingFlags[i]) == B_OK);
		msg.MakeEmpty();
		roster.GetRecentApps(&msg, 10);	// Ask for 10, as R5 doesn't always return as many as requested
		CHK(msg.FindRef("refs", 0, &ref1) == B_OK);
		CHK(msg.FindRef("refs", 1, &ref2) == B_OK);
		CHK(get_test_app_ref(kNonQualifyingApp, &appRef2) == B_OK);
		CHK(appRef2 != ref1);
		CHK(appRef2 != ref2);
		CHK(appRef1 == ref1);
		CHK(appRef1 != ref2);
	}
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case C3		Application with a non-qualifying BEOS:APP_FLAGS
	                attribute is launched.
	@results		Said application should *not* appear at the top
					of the recent apps list.
*/
void
GetRecentTester::GetRecentAppsTestC3()
{
	uint count = sizeof(kQualifyingFlags) / sizeof(int32);
	for (uint i = 0; i < count; i++) {
		BRoster roster;
		BMessage msg;
		entry_ref ref1;
		entry_ref ref2;
		entry_ref appRef1;
		entry_ref appRef2;
	
		// Launch an app that *will* show up in the list
		NextSubTest();
		CHK(launch_test_app(kControlApp, &kMultipleLaunchFlags) == B_OK);
		roster.GetRecentApps(&msg, 1);
		CHK(msg.FindRef("refs", 0, &ref1) == B_OK);
		CHK(get_test_app_ref(kControlApp, &appRef1) == B_OK);
		CHK(appRef1 == ref1);
	
		// Now launch an app with a qualifying app flags attribute that
		// therefore *should* show up in the list
		NextSubTest();
		CHK(launch_test_app(kQualifyingApp, &kQualifyingFlags[i]) == B_OK);
		msg.MakeEmpty();
		roster.GetRecentApps(&msg, 5);
		CHK(msg.FindRef("refs", 0, &ref1) == B_OK);
		CHK(msg.FindRef("refs", 1, &ref2) == B_OK);
		CHK(get_test_app_ref(kQualifyingApp, &appRef2) == B_OK);
		CHK(appRef2 == ref1);
		CHK(appRef2 != ref2);
		CHK(appRef1 != ref1);
		CHK(appRef1 == ref2);
	}
}


/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case C4		Application with a valid BEOS:APP_SIG attribute and no
	                BEOS:APP_FLAGS attribute is launched.
	@results		Said application should *not* appear at the top
					of the recent apps list.
*/
void
GetRecentTester::GetRecentAppsTestC4()
{
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case C5		Application with a valid BEOS:APP_SIG attribute and a
	                non-qualifying BEOS:APP_FLAGS attribute is launched.
	@results		Said application should *not* appear at the top
					of the recent apps list.
*/
void
GetRecentTester::GetRecentAppsTestC5()
{
}

/*
	void GetRecentApps(BMessage *refList, int32 maxCount)
	@case C6		Application with a valid BEOS:APP_SIG attribute and a
	                qualifying BEOS:APP_FLAGS attribute is launched.
	@results		Said application should *not* appear at the top
					of the recent apps list.
*/
void
GetRecentTester::GetRecentAppsTestC6()
{
}



//------------------------------------------------------------------------------
// GetRecentDocuments()
//------------------------------------------------------------------------------


/*
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
	                        const char *fileType, const char *appSig)
	@case A1		refList is NULL; maxCount, fileType and appSig are NULL.
	@results		Should quietly do nothing.
*/
void GetRecentTester::GetRecentDocumentsTestA1()
{
// R5 crashes if refList is NULL
#if !TEST_R5
	BRoster roster;
	roster.GetRecentDocuments(NULL, 1);
#endif
}

/*
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
	                        const char *fileType, const char *appSig)
	@case A2		refList is non-NULL, maxCount is zero, fileType and
	                appSig are NULL.
	@results		R5: Returns one recent document.
	                OBOS: Returns an empty message
*/
void GetRecentTester::GetRecentDocumentsTestA2()
{
	BRoster roster;
	BMessage msg;
	roster.GetRecentDocuments(&msg, 0);
#if TEST_R5
	CHK(check_ref_count(&msg, 1) == B_OK);
#else
	CHK(check_ref_count(&msg, 0) == B_OK);
#endif
}

/*
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
	                        const char *fileType, const char *appSig)
	@case A3		refList is non-NULL, maxCount is negative, fileType and
	                appSig are NULL.
	@results		R5: Returns one recent document.
	                OBOS: Returns an empty message
*/
void GetRecentTester::GetRecentDocumentsTestA3()
{
}

/*
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
	                        const char *fileType, const char *appSig)
	@case B1		refList is valid, maxCount is negative, fileType and
	                appSig are NULL.
	@results		R5: Returns one recent document.
	                OBOS: Returns an empty message
*/
void GetRecentTester::GetRecentDocumentsTestB1()
{
}

void GetRecentTester::setUp()
{
	// Create our test documents and folders
//	create_test_doc(
	
}

void GetRecentTester::tearDown()
{
	// Remove all of our test docs and folders
//	for (uint i = 0; i < (sizeof(test_docs) / sizeof(test_doc)); i++) {
//		printf("test_doc[i] == { '%s', '%s' }\n", test_docs[i].name, test_docs[i].type);
//	}
}

Test* GetRecentTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestA2);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestA3);

	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestB1);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestB2);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestB3);

	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestC1);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestC2);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestC3);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestC4);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestC5);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentAppsTestC6);

	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentDocumentsTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentDocumentsTestA2);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentDocumentsTestA3);

	return SuiteOfTests;
}


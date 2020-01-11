//------------------------------------------------------------------------------
//	GetRecentTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_attr.h>
#include <Message.h>
#include <Node.h>
#include <Roster.h>

#ifdef TEST_OBOS
#include <RosterPrivate.h>
#endif

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "GetRecentTester.h"
#include "testapps/RecentAppsTestApp.h"

// Local Defines ---------------------------------------------------------------

//------------------------------------------------------------------------------
// Attribute names and types
//------------------------------------------------------------------------------

const int32 kFlagsType = 'APPF';

const char *kTypeAttr = "BEOS:TYPE";
const char *kSigAttr = "BEOS:APP_SIG";
const char *kFlagsAttr = "BEOS:APP_FLAGS";

#ifndef B_MIME_STRING_TYPE
#define B_MIME_STRING_TYPE 'MIMS'
#endif

//------------------------------------------------------------------------------
// BEOS:APP_FLAGS attributes
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// Temporary files
//------------------------------------------------------------------------------
const char *kTempDirRoot = "/tmp";
const char *kTempDir = "/tmp/obos-recent-tests";
const char *kTempSaveFile = "/tmp/obos-recent-tests/RosterSettingsTest";

const char *kTestType1 = "text/x-vnd.obos-recent-docs-test-type-1";
const char *kTestType2 = "text/x-vnd.obos-recent-docs-test-type-2";

const char *test_types[] = {
	kTestType1,
	kTestType2,
};

struct test_doc {
	const char *name;
	const char *type;
} test_docs[] = {
	{ "first-file-of-type-1", kTestType1 },
	{ "second-file-of-type-1", kTestType1 },
	{ "first-file-of-type-2", kTestType2 },
	{ "second-file-of-type-2", kTestType2 },
	{ "untyped", NULL },
};

enum test_doc_index {
	kTestDocUntyped,
	kTestDoc1Type1,
	kTestDoc2Type1,
	kTestDoc1Type2,
	kTestDoc2Type2,
};

const char *test_folders[] = {
	"test-folder-1",
	"test-folder-2",
	"test-folder-3",
	"test-folder-4",
};

const char *test_sigs[] = {
	"application/x-vnd.obos-recent-tests-1",
	"imposter/this-is-not-an-app-sig-now-is-it?",
	"application/x-vnd.obos-recent-tests-3",
	"application/x-vnd.obos-recent-tests-app-sig-4",
	"application/x-vnd.obos-recent-tests-5",
	"application/x-vnd.obos-recent-tests-6",
	"application/x-vnd.obos-recent-tests-7",
	"application/x-vnd.obos-recent-tests-8",
};

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

status_t check_ref_count(BMessage *refMsg, int32 count);
status_t set_test_app_attributes(const entry_ref *app, const char *sig, const int32 *flags);
status_t launch_test_app(RecentAppsTestAppId id, const int32 *flags);
status_t get_test_app_ref(RecentAppsTestAppId id, entry_ref *ref);
status_t get_test_ref(test_doc_index index, entry_ref *ref);


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
			ssize_t bytes = node.WriteAttr(kSigAttr, B_MIME_STRING_TYPE, 0, sig, strlen(sig)+1);
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
	std::string sig;
	// Set the attributes
	if (!err) {
		err = set_test_app_attributes(&ref, kRecentAppsTestAppSigs[id], flags);	
	}
	// Launch the app
	if (!err) {
		BRoster roster;
		err = roster.Launch(&ref);
//		err = roster.Launch(kRecentAppsTestAppSigs[id]);
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
		sprintf(path, "%s/%s%s", testDir, kRecentAppsTestAppFilenames[id],
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

/* \brief Fetches an entry_ref for the given test document

	The file is assumed to reside in the directory specified
	by \c kTempDir.

	\param leafname the name of the file with no path information
	\param ref pointer to a pre-allocated entry_ref into which the
	           result is stored
*/
status_t get_test_ref(const char *leafname, entry_ref *ref)
{
	BEntry entry;

	status_t err = leafname && ref ? B_OK : B_BAD_VALUE;
	if (!err) {
		char path[B_PATH_NAME_LENGTH];
		sprintf(path, "%s/%s", kTempDir, leafname);
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

//------------------------------------------------------------------------------
// GetRecentDocuments()
//------------------------------------------------------------------------------

/*
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
	                        const char *fileType, const char *appSig)
	@case 1			refList is NULL; all other params are valid
	@results		Should quietly do nothing.
*/
void
GetRecentTester::GetRecentDocumentsTest1()
{
// R5 crashes if refList is NULL
#if !TEST_R5
	BRoster roster;
	roster.GetRecentDocuments(NULL, 1);
	roster.GetRecentDocuments(NULL, 1, &test_types[0], 1, NULL);
#endif
}

/*
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
	                        const char *fileType, const char *appSig)
	@case 2			refList is non-NULL, maxCount is zero, fileType and
	                appSig are NULL.
	@results		R5: Returns one recent document.
	                OBOS: Returns an empty message
*/
void
GetRecentTester::GetRecentDocumentsTest2()
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
	@case 3			refList is non-NULL, maxCount is negative, fileType and
	                appSig are NULL.
	@results		R5: Returns one recent document.
	                OBOS: Returns an empty message
*/
void
GetRecentTester::GetRecentDocumentsTest3()
{
	BRoster roster;
	BMessage msg;
	roster.GetRecentDocuments(&msg, -1);
#if TEST_R5
	CHK(check_ref_count(&msg, 1) == B_OK);
#else
	CHK(check_ref_count(&msg, 0) == B_OK);
#endif
}

/*
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
	                        const char *fileType, const char *appSig)
	@case 4			Four recent docs are added, with each pair having matching
	                app sigs and each pair of docs with-non matching app sigs
	                having matching file types. Get it?
	@results		When no app sig and a count of 4 is specified, the four folders
					in reverse order are returned.
					When the first app sig and a count of 4 is specified, the two
					folders that match that sig are returned.
					When the second app sig and a count of 4 is specified, the two
					folders that match that sig are returned.
*/
void
GetRecentTester::GetRecentDocumentsTest4()
{
	entry_ref doc1;	// type1, sig1
	entry_ref doc2;	// type1, sig2
	entry_ref doc3;	// type2, sig1
	entry_ref doc4;	// type2, sig2
	entry_ref doc5;	// untyped, sig3
	entry_ref recent1;
	entry_ref recent2;
	entry_ref recent3;
	entry_ref recent4;
	entry_ref recent5;
	BRoster roster;
	BMessage msg;
	
//	ExecCommand("ls -l ", kTempDir);

	// Add four entries with two different app sigs (note that
	// docs 0 & 1 and 2 & 3 have matching file types). Then
	// add an untyped entry with a totally different app sig.
	CHK(get_test_ref(test_docs[0].name, &doc1) == B_OK);
	CHK(get_test_ref(test_docs[1].name, &doc2) == B_OK);
	CHK(get_test_ref(test_docs[2].name, &doc3) == B_OK);
	CHK(get_test_ref(test_docs[3].name, &doc4) == B_OK);
	CHK(get_test_ref(test_docs[4].name, &doc5) == B_OK);
	roster.AddToRecentDocuments(&doc1, test_sigs[0]);
	roster.AddToRecentDocuments(&doc2, test_sigs[1]);
	roster.AddToRecentDocuments(&doc3, test_sigs[0]);
	roster.AddToRecentDocuments(&doc4, test_sigs[1]);
	roster.AddToRecentDocuments(&doc5, test_sigs[2]);

	// NULL type, NULL sig
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 5) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(msg.FindRef("refs", 3, &recent4) == B_OK);
	CHK(msg.FindRef("refs", 4, &recent5) == B_OK);
	CHK(recent1 == doc5);
	CHK(recent2 == doc4);
	CHK(recent3 == doc3);
	CHK(recent4 == doc2);
	CHK(recent5 == doc1);

	// type1, NULL sig
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types[0]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 2) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == doc2);
	CHK(recent2 == doc1);

	// type2, NULL sig
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types[1]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 2) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);

	// [type1], NULL sig
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, &test_types[0], 1, NULL);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 2) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == doc2);
	CHK(recent2 == doc1);

	// [type2], NULL sig
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, &test_types[1], 1, NULL);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 2) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);

	// [type1, type2], NULL sig
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types, 2, NULL);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 4) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(msg.FindRef("refs", 3, &recent4) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);
	CHK(recent3 == doc2);
	CHK(recent4 == doc1);

//----------------------------------------------------------

	// NULL type, sig1
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, NULL, test_sigs[0]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 2) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == doc3);
	CHK(recent2 == doc1);

	// NULL type, sig2
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, NULL, test_sigs[1]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 2) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc2);

	// NULL type, sig3
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, NULL, test_sigs[2]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == doc5);

	// type1, sig1
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types[0], test_sigs[0]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc3);
	CHK(recent2 == doc2);
	CHK(recent3 == doc1);

	// type1, sig2
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types[0], test_sigs[1]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc2);
	CHK(recent3 == doc1);

	// type1, sig3
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types[0], test_sigs[2]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc5);
	CHK(recent2 == doc2);
	CHK(recent3 == doc1);

	// type2, sig1
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types[1], test_sigs[0]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);
	CHK(recent3 == doc1);

	// type2, sig2
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types[1], test_sigs[1]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);
	CHK(recent3 == doc2);

	// type2, sig3
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types[1], test_sigs[2]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc5);
	CHK(recent2 == doc4);
	CHK(recent3 == doc3);


//---------

	// [type1], sig1
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, &test_types[0], 1, test_sigs[0]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc3);
	CHK(recent2 == doc2);
	CHK(recent3 == doc1);

	// [type1], sig2
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, &test_types[0], 1, test_sigs[1]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc2);
	CHK(recent3 == doc1);

	// [type1], sig3
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, &test_types[0], 1, test_sigs[2]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc5);
	CHK(recent2 == doc2);
	CHK(recent3 == doc1);

	// [type2], sig1
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, &test_types[1], 1, test_sigs[0]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);
	CHK(recent3 == doc1);

	// [type2], sig2
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, &test_types[1], 1, test_sigs[1]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);
	CHK(recent3 == doc2);

	// [type2], sig3
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, &test_types[1], 1, test_sigs[2]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 3) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(recent1 == doc5);
	CHK(recent2 == doc4);
	CHK(recent3 == doc3);

	// [type1, type2], sig1
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types, 2, test_sigs[0]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 4) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(msg.FindRef("refs", 3, &recent4) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);
	CHK(recent3 == doc2);
	CHK(recent4 == doc1);

	// [type1, type2], sig2
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types, 2, test_sigs[1]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 4) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(msg.FindRef("refs", 3, &recent4) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);
	CHK(recent3 == doc2);
	CHK(recent4 == doc1);

	// [type1, type2], sig3
	NextSubTest();
	roster.GetRecentDocuments(&msg, 5, test_types, 2, test_sigs[2]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 5) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(msg.FindRef("refs", 3, &recent4) == B_OK);
	CHK(msg.FindRef("refs", 4, &recent5) == B_OK);
	CHK(recent1 == doc5);
	CHK(recent2 == doc4);
	CHK(recent3 == doc3);
	CHK(recent4 == doc2);
	CHK(recent5 == doc1);
}

/*
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
	                        const char *fileType, const char *appSig)
	@case 5			Six recent folders are added, two of which are duplicates
	                under different app sigs. 
	@results		A request for the four duplicates should return the
	                four non-duplicates.
*/
void
GetRecentTester::GetRecentDocumentsTest5()
{
	entry_ref doc1;
	entry_ref doc2;
	entry_ref doc3;
	entry_ref doc4;
	entry_ref recent1;
	entry_ref recent2;
	entry_ref recent3;
	entry_ref recent4;
	BRoster roster;
	BMessage msg;
	
	// Add two entries twice with different app sigs
	CHK(get_test_ref(test_docs[0].name, &doc1) == B_OK);
	CHK(get_test_ref(test_docs[1].name, &doc2) == B_OK);
	CHK(get_test_ref(test_docs[2].name, &doc3) == B_OK);
	CHK(get_test_ref(test_docs[3].name, &doc4) == B_OK);
	roster.AddToRecentDocuments(&doc1, test_sigs[4]);
	roster.AddToRecentDocuments(&doc2, test_sigs[5]);
	roster.AddToRecentDocuments(&doc3, test_sigs[4]);
	roster.AddToRecentDocuments(&doc4, test_sigs[5]);
	roster.AddToRecentDocuments(&doc3, test_sigs[6]);
	roster.AddToRecentDocuments(&doc4, test_sigs[7]);

	// Verify our duplicates exist
	NextSubTest();
	roster.GetRecentDocuments(&msg, 1, NULL, test_sigs[7]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == doc4);

	NextSubTest();
	roster.GetRecentDocuments(&msg, 1, NULL, test_sigs[6]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == doc3);

	NextSubTest();
	roster.GetRecentDocuments(&msg, 1, NULL, test_sigs[5]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == doc4);

	NextSubTest();
	roster.GetRecentDocuments(&msg, 1, NULL, test_sigs[4]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == doc3);

	// Verify that duplicates are not returned
	NextSubTest();
	roster.GetRecentDocuments(&msg, 4);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 4) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(msg.FindRef("refs", 3, &recent4) == B_OK);
	CHK(recent1 == doc4);
	CHK(recent2 == doc3);
	CHK(recent3 == doc2);
	CHK(recent4 == doc1);
}

//------------------------------------------------------------------------------
// GetRecentFolders()
//------------------------------------------------------------------------------

/*
	void GetRecentFolders(BMessage *refList, int32 maxCount, const char *appSig)
	@case 1			refList is NULL; maxCount is valid, appSig is NULL.
	@results		Should quietly do nothing.
*/
void
GetRecentTester::GetRecentFoldersTest1()
{
// R5 crashes if refList is NULL
#if !TEST_R5
	BRoster roster;
	roster.GetRecentFolders(NULL, 1);
#endif
}

/*
	void GetRecentFolders(BMessage *refList, int32 maxCount, const char *appSig)
	@case 2		refList is valid, maxCount is negative, appSig is NULL.
	@results		R5: Returns one recent document.
	                OBOS: Returns an empty message
*/
void
GetRecentTester::GetRecentFoldersTest2()
{
	entry_ref folder1;
	entry_ref folder2;
	entry_ref recent1;
	entry_ref recent2;
	BRoster roster;
	BMessage msg;
	
	CHK(get_test_ref(test_folders[0], &folder1) == B_OK);
	CHK(get_test_ref(test_folders[1], &folder2) == B_OK);
	roster.AddToRecentFolders(&folder1, NULL);
	roster.GetRecentFolders(&msg, -1);
//	msg.PrintToStream();
#ifdef TEST_R5
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == folder1);
#else 
	CHK(check_ref_count(&msg, 0) == B_OK);	
#endif
}

/*
	void GetRecentFolders(BMessage *refList, int32 maxCount, const char *appSig)
	@case 3			refList is valid, maxCount is zero, appSig is NULL.
	@results		R5: Returns one recent document.
	                OBOS: Returns an empty message
*/
void
GetRecentTester::GetRecentFoldersTest3()
{
	entry_ref folder1;
	entry_ref folder2;
	entry_ref recent1;
	entry_ref recent2;
	BRoster roster;
	BMessage msg;
	
	CHK(get_test_ref(test_folders[0], &folder1) == B_OK);
	CHK(get_test_ref(test_folders[1], &folder2) == B_OK);
	roster.AddToRecentFolders(&folder1, NULL);
	roster.GetRecentFolders(&msg, 0);
//	msg.PrintToStream();
#ifdef TEST_R5
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == folder1);
#else 
	CHK(check_ref_count(&msg, 0) == B_OK);	
#endif
}

/*
	void GetRecentFolders(BMessage *refList, int32 maxCount, const char *appSig)
	@case 4			Four recent folders are added, with each pair having matching
	                app sigs. 
	@results		When no app sig and a count of 4 is specified, the four folders
					in reverse order are returned.
					When the first app sig and a count of 4 is specified, the two
					folders that match that sig are returned.
					When the second app sig and a count of 4 is specified, the two
					folders that match that sig are returned.
*/
void
GetRecentTester::GetRecentFoldersTest4()
{
	entry_ref folder1;
	entry_ref folder2;
	entry_ref folder3;
	entry_ref folder4;
	entry_ref recent1;
	entry_ref recent2;
	entry_ref recent3;
	entry_ref recent4;
	BRoster roster;
	BMessage msg;
	
//	ExecCommand("ls -l ", kTempDir);

	// Add four entries with two different app sigs
	CHK(get_test_ref(test_folders[0], &folder1) == B_OK);
	CHK(get_test_ref(test_folders[1], &folder2) == B_OK);
	CHK(get_test_ref(test_folders[2], &folder3) == B_OK);
	CHK(get_test_ref(test_folders[3], &folder4) == B_OK);
	roster.AddToRecentFolders(&folder1, test_sigs[0]);
	roster.AddToRecentFolders(&folder2, test_sigs[1]);
	roster.AddToRecentFolders(&folder3, test_sigs[0]);
	roster.AddToRecentFolders(&folder4, test_sigs[1]);

	NextSubTest();
	roster.GetRecentFolders(&msg, 4, NULL);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 4) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(msg.FindRef("refs", 3, &recent4) == B_OK);
	CHK(recent1 == folder4);
	CHK(recent2 == folder3);
	CHK(recent3 == folder2);
	CHK(recent4 == folder1);

	NextSubTest();
	roster.GetRecentFolders(&msg, 4, test_sigs[0]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 2) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == folder3);
	CHK(recent2 == folder1);

	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentFolders(&msg, 4, test_sigs[1]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 2) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == folder4);
	CHK(recent2 == folder2);
}

/*
	void GetRecentFolders(BMessage *refList, int32 maxCount, const char *appSig)
	@case 5			Six recent folders are added, two of which are duplicates
	                under different app sigs. 
	@results		A request for the four duplicates should return the
	                four non-duplicates.
*/
void
GetRecentTester::GetRecentFoldersTest5()
{
	entry_ref folder1;
	entry_ref folder2;
	entry_ref folder3;
	entry_ref folder4;
	entry_ref recent1;
	entry_ref recent2;
	entry_ref recent3;
	entry_ref recent4;
	BRoster roster;
	BMessage msg;
	
	// Add two entries twice with different app sigs
	CHK(get_test_ref(test_folders[0], &folder1) == B_OK);
	CHK(get_test_ref(test_folders[1], &folder2) == B_OK);
	CHK(get_test_ref(test_folders[2], &folder3) == B_OK);
	CHK(get_test_ref(test_folders[3], &folder4) == B_OK);
	roster.AddToRecentFolders(&folder1, test_sigs[4]);
	roster.AddToRecentFolders(&folder2, test_sigs[5]);
	roster.AddToRecentFolders(&folder3, test_sigs[4]);
	roster.AddToRecentFolders(&folder4, test_sigs[5]);
	roster.AddToRecentFolders(&folder3, test_sigs[6]);
	roster.AddToRecentFolders(&folder4, test_sigs[7]);

	// Verify our duplicates exist
	NextSubTest();
	roster.GetRecentFolders(&msg, 1, test_sigs[7]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == folder4);

	NextSubTest();
	roster.GetRecentFolders(&msg, 1, test_sigs[6]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == folder3);

	NextSubTest();
	roster.GetRecentFolders(&msg, 1, test_sigs[5]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == folder4);

	NextSubTest();
	roster.GetRecentFolders(&msg, 1, test_sigs[4]);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 1) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(recent1 == folder3);

	// Verify that duplicates are not returned
	NextSubTest();
	roster.GetRecentFolders(&msg, 4, NULL);
//	msg.PrintToStream();
	CHK(check_ref_count(&msg, 4) == B_OK);	
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(msg.FindRef("refs", 2, &recent3) == B_OK);
	CHK(msg.FindRef("refs", 3, &recent4) == B_OK);
	CHK(recent1 == folder4);
	CHK(recent2 == folder3);
	CHK(recent3 == folder2);
	CHK(recent4 == folder1);
}

void
GetRecentTester::RecentListsLoadSaveClearTest()
{
#ifdef TEST_R5
	Outputf("(no tests actually performed for R5 version)\n");
#else
	entry_ref doc1;
	entry_ref doc2;
	entry_ref folder1;
	entry_ref folder2;
	entry_ref app1;
	entry_ref app2;
	const char *appSig1 = kRecentAppsTestAppSigs[kQualifyingApp];
	const char *appSig2 = kRecentAppsTestAppSigs[kControlApp];
	entry_ref recent1;
	entry_ref recent2;
	BRoster roster;
	BMessage msg;

	//--------------------------------------------------------------------
	// Add a few docs, folders, and apps. Check that they're there.
	// Save the to disk. Clear. Check that the lists are empty.
	// Load from disk. Check that things look like they used
	// to.
	//--------------------------------------------------------------------

	// Add
	CHK(get_test_ref(test_docs[0].name, &doc1) == B_OK);
	CHK(get_test_ref(test_docs[1].name, &doc2) == B_OK);
	CHK(get_test_ref(test_folders[0], &folder1) == B_OK);
	CHK(get_test_ref(test_folders[1], &folder2) == B_OK);
	CHK(get_test_app_ref(kQualifyingApp, &app1) == B_OK);
	CHK(get_test_app_ref(kControlApp, &app2) == B_OK);
	CHK(set_test_app_attributes(&app1, kRecentAppsTestAppSigs[kQualifyingApp],
	    &kMultipleLaunchFlags) == B_OK);
	CHK(set_test_app_attributes(&app2, kRecentAppsTestAppSigs[kControlApp],
	    &kMultipleLaunchFlags) == B_OK);
	roster.AddToRecentDocuments(&doc1, test_sigs[0]);
	roster.AddToRecentDocuments(&doc2, test_sigs[1]);
	roster.AddToRecentFolders(&folder1, test_sigs[0]);
	roster.AddToRecentFolders(&folder2, test_sigs[1]);
	BRoster::Private(roster).AddToRecentApps(appSig1);	
	BRoster::Private(roster).AddToRecentApps(appSig2);
	
	// Check #1
	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentDocuments(&msg, 2);
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == doc2);
	CHK(recent2 == doc1);

	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentFolders(&msg, 2);
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == folder2);
	CHK(recent2 == folder1);
	
	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentApps(&msg, 2);
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == app2);
	CHK(recent2 == app1);
	
	// Save to disk and clear
	NextSubTest();
	BRoster::Private(roster).SaveRecentLists(kTempSaveFile);
	BRoster::Private(roster).ClearRecentDocuments();
	BRoster::Private(roster).ClearRecentFolders();
	BRoster::Private(roster).ClearRecentApps();
	
	// Check #2
	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentDocuments(&msg, 2);
	CHK(check_ref_count(&msg, 0) == B_OK);
	
	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentFolders(&msg, 2);
	CHK(check_ref_count(&msg, 0) == B_OK);
	
	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentApps(&msg, 2);
	CHK(check_ref_count(&msg, 0) == B_OK);
	
	// Load back from disk
	NextSubTest();
	BRoster::Private(roster).LoadRecentLists(kTempSaveFile);
	
	// Check #3
	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentDocuments(&msg, 2);
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == doc2);
	CHK(recent2 == doc1);

	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentFolders(&msg, 2);
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == folder2);
	CHK(recent2 == folder1);
	
	NextSubTest();
	msg.MakeEmpty();
	roster.GetRecentApps(&msg, 2);
	CHK(msg.FindRef("refs", 0, &recent1) == B_OK);
	CHK(msg.FindRef("refs", 1, &recent2) == B_OK);
	CHK(recent1 == app2);
	CHK(recent2 == app1);
#endif	// ifdef TEST_R5 else
}


void
GetRecentTester::setUp()
{
	status_t err = B_OK;
	BDirectory tempDir;

	// Create our base folder for test entries
	if (!BEntry(kTempDir).Exists()) {
		BDirectory rootDir;
		err = rootDir.SetTo(kTempDirRoot);
		if (!err)
			err = rootDir.CreateDirectory(kTempDir, &tempDir);
		if (!err)
			err = tempDir.InitCheck();
		if (err) {
			printf("WARNING: Unable to create temp directory for BRoster::GetRecent*() tests, error "
			       "== 0x%lx. It's entirely possible that most of these tests will fail because "
			       "of this\n", err);
			return;
		}
	}
	
	// Create our test documents 
	if (!err) {
		int count = sizeof(test_docs) / sizeof(test_doc);
		for (int i = 0; !err && i < count; i++) {
			BFile file;

			char filename[B_PATH_NAME_LENGTH];
			sprintf(filename, "%s/%s", kTempDir, test_docs[i].name);
			ExecCommand("touch ", filename);
			err = file.SetTo(filename, B_READ_WRITE);

// For some reason, only the first CreateFile() call works with R5. None
// work with OBOS...
/*
			err = tempDir.CreateFile(test_docs[i].name, &file);
			if (!err)
				err = file.InitCheck();
*/

			// Write its type (if necessary)
			if (!err && test_docs[i].type) {
				int len = strlen(test_docs[i].type)+1;
				ssize_t bytes = file.WriteAttr(kTypeAttr, B_MIME_STRING_TYPE, 0,
					test_docs[i].type, len);
				if (bytes >= 0)
					err = bytes == len ? B_OK : B_FILE_ERROR;
				else
					err = bytes;
			}
		}
	}
	
	// Create our test folders
	if (!err) {
		int count = sizeof(test_folders) / sizeof(char*);
		for (int i = 0; !err && i < count; i++) {

			// For some reason, only the first CreateFile() call works with R5
			char dirname[B_PATH_NAME_LENGTH];
			sprintf(dirname, "%s/%s", kTempDir, test_folders[i]);
			if (!BEntry(dirname).Exists())
				ExecCommand("mkdir ", dirname);
/*
			err = tempDir.CreateDirectory(test_folders[i], NULL);
			if (err == B_FILE_EXISTS)
				err = B_OK;
*/
		}
	}
	
	// Let the user know if we hit an error
	if (err) {
		printf("WARNING: Error encountered while creating test files and/or folders, error "
		       "code == 0x%lx. It's entirely possible that most or all of these tests will "
		       "fail now because of this\n", err);
	}	
}

void
GetRecentTester::tearDown()
{
	// Remove the folder containing all of our test files
	if (BEntry(kTempDir).Exists()) 
		ExecCommand("rm -rf ", kTempDir);
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

	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentDocumentsTest1);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentDocumentsTest2);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentDocumentsTest3);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentDocumentsTest4);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentDocumentsTest5);

	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentFoldersTest1);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentFoldersTest2);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentFoldersTest3);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentFoldersTest4);
	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, GetRecentFoldersTest5);

	ADD_TEST4(BRoster, SuiteOfTests, GetRecentTester, RecentListsLoadSaveClearTest);

	return SuiteOfTests;
}


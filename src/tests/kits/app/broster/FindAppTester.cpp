//------------------------------------------------------------------------------
//	FindAppTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <utime.h>

// System Includes -------------------------------------------------------------
#include <be/app/Message.h>
#include <be/kernel/OS.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <File.h>
#include <Handler.h>
#include <Looper.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "AppRunner.h"
#include "FindAppTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

static const char *uninstalledType
	= "application/x-vnd.obos-roster-findapp-uninstalled";
static const char *appType1	= "application/x-vnd.obos-roster-findapp-app1";
static const char *appType2	= "application/x-vnd.obos-roster-findapp-app2";
static const char *fileType1 = "application/x-vnd.obos-roster-findapp-file1";
static const char *fileType2 = "application/x-vnd.obos-roster-findapp-file2";
static const char *textTestType = "text/x-vnd.obos-roster-findapp";

static const char *testDir	= "/tmp/testdir";
static const char *appFile1	= "/tmp/testdir/app1";
static const char *appFile2	= "/tmp/testdir/app2";


// install_type
static
void
install_type(const char *type, const char *preferredApp = NULL)
{
	BMimeType mimeType(type);
	CHK(mimeType.Install() == B_OK);
	if (preferredApp)
		CHK(mimeType.SetPreferredApp(preferredApp) == B_OK);
}

// ref_for_file
static
entry_ref
ref_for_file(const char *filename)
{
	entry_ref ref;
	BEntry entry;
	CHK(entry.SetTo(filename, true) == B_OK);
	CHK(entry.GetRef(&ref) == B_OK);
	return ref;
}

// create_app
static
void
create_app(const char *filename, const char *signature = NULL,
		   bool install = false, bool makeExecutable = true)
{
	system((string("touch ") + filename).c_str());
	if (makeExecutable)
		system((string("chmod a+x ") + filename).c_str());
	if (signature) {
		BFile file;
		CHK(file.SetTo(filename, B_READ_WRITE) == B_OK);
		BAppFileInfo appFileInfo;
		CHK(appFileInfo.SetTo(&file) == B_OK);
		CHK(appFileInfo.SetSignature(signature) == B_OK);
		if (install)
			CHK(BMimeType(signature).Install() == B_OK);
	}
}

// check_app_type
static
void
check_app_type(const char *signature, const char *filename)
{
	BMimeType type(signature);
	CHK(type.IsInstalled() == true);
	if (filename) {
		entry_ref appHint;
		CHK(type.GetAppHint(&appHint) == B_OK);
		CHK(ref_for_file(filename) == appHint);
	}
}

// set_file_time
static
void
set_file_time(const char *filename, time_t time)
{
	utimbuf buffer;
	buffer.actime = time;
	buffer.modtime = time;
	CHK(utime(filename, &buffer) == 0);
}

// set_version
static
void
set_version(const char *filename, uint32 version)
{
	version_info versionInfo = { 1, 1, 1, 1, version, "short1", "long1" };
	BFile file;
	CHK(file.SetTo(filename, B_READ_WRITE) == B_OK);
	BAppFileInfo appFileInfo;
	CHK(appFileInfo.SetTo(&file) == B_OK);
	CHK(appFileInfo.SetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK);
}

// set_type_app_hint
static
void
set_type_app_hint(const char *signature, const char *filename)
{
	BMimeType type(signature);
	if (!type.IsInstalled());
		CHK(type.Install() == B_OK);
	entry_ref fileRef(ref_for_file(filename));
	CHK(type.SetAppHint(&fileRef) == B_OK);
}

// setUp
void
FindAppTester::setUp()
{
	fApplication = new BApplication("application/x-vnd.obos-roster-findapp");
	system((string("mkdir ") + testDir).c_str());
}

// tearDown
void
FindAppTester::tearDown()
{
	BMimeType(uninstalledType).Delete();
	BMimeType(appType1).Delete();
	BMimeType(appType2).Delete();
	BMimeType(fileType1).Delete();
	BMimeType(fileType2).Delete();
	BMimeType(textTestType).Delete();
	delete fApplication;
	system((string("rm -rf ") + testDir).c_str());
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 1			mimeType or app are NULL
	@results		Should return B_BAD_VALUE.
*/
void FindAppTester::FindAppTestA1()
{
	BRoster roster;
	CHK(roster.FindApp((const char*)NULL, NULL) == B_BAD_VALUE);
// R5: crashes when passing a non-NULL type, but a NULL ref.
#ifndef TEST_R5
	CHK(roster.FindApp("image/gif", NULL) == B_BAD_VALUE);
#endif
	entry_ref ref;
	CHK(roster.FindApp((const char*)NULL, &ref) == B_BAD_VALUE);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 2			mimeType is invalid
	@results		Should return B_BAD_VALUE.
*/
void FindAppTester::FindAppTestA2()
{
	BRoster roster;
	entry_ref ref;
	CHK(roster.FindApp("invalid/mine/type", &ref) == B_BAD_VALUE);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 3			uninstalled type mimeType
	@results		Should return B_LAUNCH_FAILED_APP_NOT_FOUND.
*/
void FindAppTester::FindAppTestA3()
{
	BRoster roster;
	entry_ref ref;
	CHK(roster.FindApp(uninstalledType, &ref)
		== B_LAUNCH_FAILED_APP_NOT_FOUND);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 4			installed type mimeType, no preferred app
	@results		Should return B_LAUNCH_FAILED_NO_PREFERRED_APP.
*/
void FindAppTester::FindAppTestA4()
{
	BRoster roster;
	install_type(fileType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_LAUNCH_FAILED_NO_PREFERRED_APP);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 5			installed type mimeType, preferred app, app type not
					installed, app has no signature
	@results		Should return B_LAUNCH_FAILED_APP_NOT_FOUND.
*/
void FindAppTester::FindAppTestA5()
{
	BRoster roster;
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_LAUNCH_FAILED_APP_NOT_FOUND);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 6			installed type mimeType, preferred app, app type not
					installed, app has signature
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should install the app type and
					set the app hint on it.
*/
void FindAppTester::FindAppTestA6()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_OK);
	CHK(ref_for_file(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 7			installed type mimeType, preferred app, app type installed,
					app has signature
	@results		Should return B_OK and set the ref to refer to the
					application'sexecutable. Should set the app hint on the
					app type.
*/
void FindAppTester::FindAppTestA7()
{
	BRoster roster;
	create_app(appFile1, appType1, true);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_OK);
	CHK(ref_for_file(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 8			installed type mimeType, preferred app, app type installed,
					app has signature, app has no execute permission
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should set the app hint on the
					app type.
*/
void FindAppTester::FindAppTestA8()
{
	BRoster roster;
	create_app(appFile1, appType1, true, false);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_OK);
	CHK(ref_for_file(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 9			installed type mimeType, preferred app, app type installed,
					two apps have the signature
	@results		Should return B_OK and set the ref to refer to the
					application executable with the most recent modification
					time. Should set the app hint on the app type.
*/
void FindAppTester::FindAppTestA9()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType1, true);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_OK);
	CHK(ref_for_file(appFile2) == ref);
	check_app_type(appType1, appFile2);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 10		installed type mimeType, preferred app, app type installed,
					two apps have the signature, one has a version info, the
					other one is newer
	@results		Should return B_OK and set the ref to refer to the
					application executable with version info. Should set the
					app hint on the app type.
*/
void FindAppTester::FindAppTestA10()
{
	BRoster roster;
	create_app(appFile1, appType1);
	set_version(appFile1, 1);
	create_app(appFile2, appType1, true);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_OK);
	CHK(ref_for_file(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 11		installed type mimeType, preferred app, app type installed,
					two apps have the signature, both apps have a version info
	@results		Should return B_OK and set the ref to refer to the
					application executable with the greater version. Should
					set the app hint on the app type.
*/
void FindAppTester::FindAppTestA11()
{
	BRoster roster;
	create_app(appFile1, appType1);
	set_version(appFile1, 2);
	create_app(appFile2, appType1, true);
	set_version(appFile1, 1);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_OK);
	CHK(ref_for_file(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 12		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint that points to an app
					with a different signature
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should remove the incorrect app
					hint on the app type. (OBOS: Should set the correct app
					hint. Don't even return the wrong app?)
*/
void FindAppTester::FindAppTestA12()
{
	BRoster roster;
	create_app(appFile1, appType2);
	set_type_app_hint(appType1, appFile1);
	entry_ref appHint;
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_OK);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_OK);
	CHK(ref_for_file(appFile1) == ref);
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_ENTRY_NOT_FOUND);
	CHK(BMimeType(appType2).IsInstalled() == false);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 13		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint pointing to void,
					a differently named app with this signature exists
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should update the app
					hint on the app type.
*/
void FindAppTester::FindAppTestA13()
{
	BRoster roster;
	create_app(appFile1, appType1);
	set_type_app_hint(appType1, appFile2);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(fileType1, &ref) == B_OK);
	CHK(ref_for_file(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 14		mimeType is app signature, not installed
	@results		Should return B_OK and set the ref to refer to the
					application executable. Should set the app hint on the
					app type.
*/
void FindAppTester::FindAppTestA14()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref ref;
	CHK(roster.FindApp(appType1, &ref) == B_OK);
	CHK(ref_for_file(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 15		mimeType is installed, but has no preferred application,
					super type has preferred application
	@results		Should return B_OK and set the ref to refer to the
					application executable associated with the preferred app
					of the supertype. Should set the app hint on the app type.
*/
void FindAppTester::FindAppTestA15()
{
	BRoster roster;
	// make sure, the original preferred app for the "text" supertype is
	// re-installed
	struct TextTypeSaver {
		TextTypeSaver()
		{
			BMimeType textType("text");
			hasPreferredApp
				= (textType.GetPreferredApp(preferredApp) == B_OK);
		}

		~TextTypeSaver()
		{
			BMimeType textType("text");
			textType.SetPreferredApp(hasPreferredApp ? preferredApp : NULL);
		}

		bool	hasPreferredApp;
		char	preferredApp[B_MIME_TYPE_LENGTH];
	} _saver;

	create_app(appFile1, appType1);
	CHK(BMimeType("text").SetPreferredApp(appType1) == B_OK);
	install_type(textTestType);
	entry_ref ref;
	CHK(roster.FindApp(textTestType, &ref) == B_OK);
	CHK(ref_for_file(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	team_id FindApp(entry_ref *ref) const
	@case 1			ref is NULL
	@results		Should return B_BAD_VALUE.
*/
void FindAppTester::FindAppTestB1()
{
}


Test* FindAppTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA1);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA2);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA3);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA4);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA5);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA6);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA7);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA8);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA9);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA10);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA11);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA12);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA13);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA14);
	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestA15);

//	ADD_TEST4(BApplication, SuiteOfTests, FindAppTester, FindAppTestB1);

	return SuiteOfTests;
}


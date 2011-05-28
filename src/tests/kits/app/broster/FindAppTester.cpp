//------------------------------------------------------------------------------
//	FindAppTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <utime.h>

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <OS.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <File.h>
#include <FindDirectory.h>
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

static const char *testDir		= "/tmp/testdir";
static const char *appFile1		= "/tmp/testdir/app1";
static const char *appFile2		= "/tmp/testdir/app2";
static const char *testFile1	= "/tmp/testdir/testFile1";
static const char *testLink1	= "/tmp/testdir/testLink1";
static const char *trashAppName	= "roster-findapp-app";

// get_trash_app_file
static
const char*
get_trash_app_file()
{
	static char trashAppFile[B_PATH_NAME_LENGTH];
	static bool initialized = false;
	if (!initialized) {
		BPath path;
		CHK(find_directory(B_TRASH_DIRECTORY, &path) == B_OK);
		CHK(path.Append(trashAppName) == B_OK);
		strcpy(trashAppFile, path.Path());
		initialized = true;
	}
	return trashAppFile;
}

// install_type
static
void
install_type(const char *type, const char *preferredApp = NULL,
			 const char *snifferRule = NULL)
{
	BMimeType mimeType(type);
	if (!mimeType.IsInstalled())
		CHK(mimeType.Install() == B_OK);
	if (preferredApp)
		CHK(mimeType.SetPreferredApp(preferredApp) == B_OK);
	if (snifferRule)
		CHK(mimeType.SetSnifferRule(snifferRule) == B_OK);
}

// ref_for_path
static
entry_ref
ref_for_path(const char *filename, bool traverse = true)
{
	entry_ref ref;
	BEntry entry;
	CHK(entry.SetTo(filename, traverse) == B_OK);
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

// create_file
static
entry_ref
create_file(const char *filename, const char *type,
			const char *preferredApp = NULL, const char *appHintPath = NULL,
			const char *contents = NULL)
{
	if (contents)
		system((string("echo -n \"") + contents + "\" > " + filename).c_str());
	else
		system((string("touch ") + filename).c_str());
	if (type || preferredApp || appHintPath) {
		BFile file;
		CHK(file.SetTo(filename, B_READ_WRITE) == B_OK);
		BNodeInfo nodeInfo;
		CHK(nodeInfo.SetTo(&file) == B_OK);
		if (type)
			CHK(nodeInfo.SetType(type) == B_OK);
		if (preferredApp)
			CHK(nodeInfo.SetPreferredApp(preferredApp) == B_OK);
		if (appHintPath) {
			entry_ref appHint(ref_for_path(appHintPath));
			CHK(nodeInfo.SetAppHint(&appHint) == B_OK);
		}
	}
	return ref_for_path(filename);
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
		CHK(ref_for_path(filename) == appHint);
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
	entry_ref fileRef(ref_for_path(filename));
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
	system((string("rm -f ") + get_trash_app_file()).c_str());
}

// FindAppCaller
class FindAppCaller {
public:
	virtual status_t operator()(const char *type, entry_ref *ref) = 0;
};

/*
	@case 1			uninstalled type mimeType
	@results		Should return B_LAUNCH_FAILED_APP_NOT_FOUND.
*/
static
void
CommonFindAppTest1(FindAppCaller &caller)
{
	BRoster roster;
	entry_ref ref;
	CHK(caller(uninstalledType, &ref) == B_LAUNCH_FAILED_APP_NOT_FOUND);
}

/*
	@case 2			installed type mimeType, no preferred app
	@results		Should return B_LAUNCH_FAILED_NO_PREFERRED_APP.
*/
static
void
CommonFindAppTest2(FindAppCaller &caller)
{
	BRoster roster;
	install_type(fileType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_LAUNCH_FAILED_NO_PREFERRED_APP);
}

/*
	@case 3			installed type mimeType, preferred app, app type not
					installed, app has no signature
	@results		Should return B_LAUNCH_FAILED_APP_NOT_FOUND.
*/
static
void
CommonFindAppTest3(FindAppCaller &caller)
{
	BRoster roster;
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_LAUNCH_FAILED_APP_NOT_FOUND);
}

/*
	@case 4			installed type mimeType, preferred app, app type not
					installed, app has signature
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should install the app type and
					set the app hint on it.
*/
static
void
CommonFindAppTest4(FindAppCaller &caller)
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	@case 5			installed type mimeType, preferred app, app type installed,
					app has signature
	@results		Should return B_OK and set the ref to refer to the
					application'sexecutable. Should set the app hint on the
					app type.
*/
static
void
CommonFindAppTest5(FindAppCaller &caller)
{
	BRoster roster;
	create_app(appFile1, appType1, true);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	@case 6			installed type mimeType, preferred app, app type installed,
					app has signature, app has no execute permission
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should set the app hint on the
					app type.
*/
static
void
CommonFindAppTest6(FindAppCaller &caller)
{
	BRoster roster;
	create_app(appFile1, appType1, true, false);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	@case 7			installed type mimeType, preferred app, app type installed,
					two apps have the signature
	@results		Should return B_OK and set the ref to refer to the
					application executable with the most recent modification
					time. Should set the app hint on the app type.
*/
static
void
CommonFindAppTest7(FindAppCaller &caller)
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType1, true);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_OK);
	CHK(ref_for_path(appFile2) == ref);
	check_app_type(appType1, appFile2);
}

/*
	@case 8			installed type mimeType, preferred app, app type installed,
					two apps have the signature, one has a version info, the
					other one is newer
	@results		Should return B_OK and set the ref to refer to the
					application executable with version info. Should set the
					app hint on the app type.
*/
static
void
CommonFindAppTest8(FindAppCaller &caller)
{
	BRoster roster;
	create_app(appFile1, appType1);
	set_version(appFile1, 1);
	create_app(appFile2, appType1, true);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	@case 9			installed type mimeType, preferred app, app type installed,
					two apps have the signature, both apps have a version info
	@results		Should return B_OK and set the ref to refer to the
					application executable with the greater version. Should
					set the app hint on the app type.
*/
static
void
CommonFindAppTest9(FindAppCaller &caller)
{
	BRoster roster;
	create_app(appFile1, appType1);
	set_version(appFile1, 2);
	create_app(appFile2, appType1, true);
	set_version(appFile1, 1);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	@case 10		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint that points to an app
					with a different signature
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should remove the incorrect app
					hint on the app type. (OBOS: Should set the correct app
					hint. Don't even return the wrong app?)
*/
static
void
CommonFindAppTest10(FindAppCaller &caller)
{
	BRoster roster;
	create_app(appFile1, appType2);
	set_type_app_hint(appType1, appFile1);
	entry_ref appHint;
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_OK);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_ENTRY_NOT_FOUND);
// OBOS: We set the app hint for app type 2. There's no reason not to do it.
#ifdef TEST_R5
	CHK(BMimeType(appType2).IsInstalled() == false);
#else
	check_app_type(appType2, appFile1);
#endif
}

/*
	@case 11		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint pointing to void,
					a differently named app with this signature exists
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should update the app
					hint on the app type.
*/
static
void
CommonFindAppTest11(FindAppCaller &caller)
{
	BRoster roster;
	create_app(appFile1, appType1);
	set_type_app_hint(appType1, appFile2);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	@case 12		mimeType is app signature, not installed
	@results		Should return B_OK and set the ref to refer to the
					application executable. Should set the app hint on the
					app type.
*/
static
void
CommonFindAppTest12(FindAppCaller &caller)
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref ref;
	CHK(caller(appType1, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	@case 13		mimeType is installed, but has no preferred application,
					super type has preferred application
	@results		Should return B_OK and set the ref to refer to the
					application executable associated with the preferred app
					of the supertype. Should set the app hint on the app type.
*/
static
void
CommonFindAppTest13(FindAppCaller &caller)
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
	CHK(caller(textTestType, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	@case 14		installed type mimeType, preferred app, app type not installed,
					app has signature, app is trash
	@results		Should return B_LAUNCH_FAILED_APP_IN_TRASH.
*/
static
void
CommonFindAppTest14(FindAppCaller &caller)
{
	BRoster roster;
	create_app(get_trash_app_file(), appType1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_LAUNCH_FAILED_APP_IN_TRASH);
}

/*
	@case 15		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint pointing to void,
					no app with this signature exists
	@results		Should return B_LAUNCH_FAILED_APP_NOT_FOUND and unset the
					app type's app hint.
*/
static
void
CommonFindAppTest15(FindAppCaller &caller)
{
	BRoster roster;
	set_type_app_hint(appType1, appFile1);
	install_type(fileType1, appType1);
	entry_ref ref;
	CHK(caller(fileType1, &ref) == B_LAUNCH_FAILED_APP_NOT_FOUND);
	entry_ref appHint;
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_ENTRY_NOT_FOUND);
}

/*
	@case 16		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint pointing to a cyclic
					link, no app with this signature exists
	@results		R5: Should return B_OK and set the ref to refer to the
					link.
					OBOS: Should return B_LAUNCH_FAILED_APP_NOT_FOUND and
					unset the app type's app hint.
*/
static
void
CommonFindAppTest16(FindAppCaller &caller)
{
	BRoster roster;
	set_type_app_hint(appType1, appFile1);
	install_type(fileType1, appType1);
	system((string("ln -s ") + appFile1 + " " + appFile1).c_str());
	entry_ref ref;
	entry_ref appHint;
#ifdef TEST_R5
	CHK(caller(fileType1, &ref) == B_OK);
	CHK(ref_for_path(appFile1, false) == ref);
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_OK);
	CHK(appHint == ref);
#else
	CHK(caller(fileType1, &ref) == B_LAUNCH_FAILED_APP_NOT_FOUND);
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_ENTRY_NOT_FOUND);
#endif
}

typedef void commonTestFunction(FindAppCaller &caller);
static commonTestFunction *commonTestFunctions[] = {
	CommonFindAppTest1, CommonFindAppTest2, CommonFindAppTest3,
	CommonFindAppTest4, CommonFindAppTest5, CommonFindAppTest6,
	CommonFindAppTest7, CommonFindAppTest8, CommonFindAppTest9,
	CommonFindAppTest10, CommonFindAppTest11, CommonFindAppTest12,
	CommonFindAppTest13, CommonFindAppTest14, CommonFindAppTest15,
	CommonFindAppTest16
};
static int32 commonTestFunctionCount
	= sizeof(commonTestFunctions) / sizeof(commonTestFunction*);

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

// FindAppTypeCaller
class FindAppTypeCaller : public FindAppCaller {
public:
	virtual status_t operator()(const char *type, entry_ref *ref)
	{
		BRoster roster;
		return roster.FindApp(type, ref);
	}
};

/*
	status_t FindApp(const char *mimeType, entry_ref *app) const
	@case 3			FindApp(const char*, entry_ref*) cases 3-16
					(== common cases 1-14)
*/
void FindAppTester::FindAppTestA3()
{
	FindAppTypeCaller caller;
	for (int32 i = 0; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 1			ref or app are NULL
	@results		Should return B_BAD_VALUE.
*/
void FindAppTester::FindAppTestB1()
{
	BRoster roster;
	CHK(roster.FindApp((entry_ref*)NULL, NULL) == B_BAD_VALUE);
// R5: Crashes when passing a NULL (app) ref.
#ifndef TEST_R5
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, appType1));
	CHK(roster.FindApp(&fileRef, NULL) == B_BAD_VALUE);
#endif
	entry_ref ref;
	CHK(roster.FindApp((entry_ref*)NULL, &ref) == B_BAD_VALUE);
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 2			ref doesn't refer to an existing entry =>
	@results		Should return B_ENTRY_NOT_FOUND.
*/
void FindAppTester::FindAppTestB2()
{
	BRoster roster;
	entry_ref fileRef(ref_for_path(testFile1));
	entry_ref ref;
	CHK(roster.FindApp(&fileRef, &ref) == B_ENTRY_NOT_FOUND);
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 3			ref is valid, file has type and preferred app, preferred
					app is in trash
	@results		Should return B_LAUNCH_FAILED_APP_IN_TRASH.
*/
void FindAppTester::FindAppTestB3()
{
	BRoster roster;
	create_app(get_trash_app_file(), appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, appType1));
	entry_ref ref;
	CHK(roster.FindApp(&fileRef, &ref) == B_LAUNCH_FAILED_APP_IN_TRASH);
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 4			ref is valid, file has type and preferred app, app type is
					not installed, app exists and has signature
	@results		Should return B_OK and set the ref to refer to the file's
					(not the file type's) preferred application's executable.
					Should install the app type and set the app hint on it.
*/
void FindAppTester::FindAppTestB4()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, appType2));
	entry_ref ref;
	CHK(roster.FindApp(&fileRef, &ref) == B_OK);
	CHK(ref_for_path(appFile2) == ref);
	check_app_type(appType2, appFile2);
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 5			ref is valid, file has no type, but preferred app,
					app type is not installed, app exists and has signature
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should install the app type and
					set the app hint on it.
*/
void FindAppTester::FindAppTestB5()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	entry_ref ref;
	CHK(roster.FindApp(&fileRef, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 6			ref is valid, file has type and app hint, the type's
					preferred app type is not installed, app exists and has
					signature
	@results		Should return B_OK and set the ref to refer to the file
					type's preferred application's executable. Should install
					the app type and set the app hint on it.
*/
void FindAppTester::FindAppTestB6()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, NULL, appFile2));
	entry_ref ref;
	CHK(roster.FindApp(&fileRef, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 7			ref is valid, file has type, the type's preferred app
					type is not installed, app exists and has signature,
					file is executable
	@results		Should return B_OK and set the ref to refer to the file.
					Should not set the app hint on the app or file type (Why?).
*/
void FindAppTester::FindAppTestB7()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1));
	system((string("chmod a+x ") + testFile1).c_str());
	entry_ref ref;
	CHK(roster.FindApp(&fileRef, &ref) == B_OK);
	CHK(ref_for_path(testFile1) == ref);
	CHK(BMimeType(appType1).IsInstalled() == false);
	CHK(BMimeType(fileType1).GetAppHint(&ref) == B_ENTRY_NOT_FOUND);
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 8			ref is valid and refers to a link to a file, file has type,
					the type's preferred app type is not installed,
					app exists and has signature
	@results		Should return B_OK and set the ref to refer to the file
					type's preferred application's executable. Should install
					the app type and set the app hint on it.
*/
void FindAppTester::FindAppTestB8()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	create_file(testFile1, fileType1);
	system((string("ln -s ") + testFile1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1, false));
	entry_ref ref;
	CHK(roster.FindApp(&linkRef, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

// FileWithTypeCaller
class FileWithTypeCaller : public FindAppCaller {
public:
	virtual status_t operator()(const char *type, entry_ref *ref)
	{
		BRoster roster;
		entry_ref fileRef(create_file(testFile1, type));
		return roster.FindApp(&fileRef, ref);
	}
};

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 9			ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					FindApp(const char*, entry_ref*) cases 4-16
					(== common cases 2-14)
*/
void FindAppTester::FindAppTestB9()
{
	FileWithTypeCaller caller;
	for (int32 i = 0; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

// SniffFileTypeCaller
class SniffFileTypeCaller : public FindAppCaller {
public:
	virtual status_t operator()(const char *type, entry_ref *ref)
	{
		BRoster roster;
		entry_ref fileRef(create_file(testFile1, type, NULL, NULL,
						  "UnIQe pAtTeRn"));
		install_type(fileType1, NULL, "1.0 [0] ('UnIQe pAtTeRn')");
		return roster.FindApp(&fileRef, ref);
	}
};

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 10		ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					FindApp(const char*, entry_ref*) cases 3-16
					(== common cases 1-14)
*/
void FindAppTester::FindAppTestB10()
{
	SniffFileTypeCaller caller;
	for (int32 i = 1; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 11		ref is valid and refers to a cyclic link
	@results		Should return B_LAUNCH_FAILED_NO_RESOLVE_LINK.
*/
void FindAppTester::FindAppTestB11()
{
	BRoster roster;
	system((string("ln -s ") + testLink1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1, false));
	entry_ref ref;
	CHK(roster.FindApp(&linkRef, &ref) == B_LAUNCH_FAILED_NO_RESOLVE_LINK);
}

/*
	status_t FindApp(entry_ref *ref, entry_ref *app) const
	@case 12		ref is valid and refers to a link to void
	@results		Should return B_LAUNCH_FAILED_NO_RESOLVE_LINK.
*/
void FindAppTester::FindAppTestB12()
{
	BRoster roster;
	system((string("ln -s ") + testFile1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1, false));
	entry_ref ref;
	CHK(roster.FindApp(&linkRef, &ref) == B_LAUNCH_FAILED_NO_RESOLVE_LINK);
}


Test* FindAppTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestA2);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestA3);

	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB1);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB2);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB3);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB4);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB5);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB6);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB7);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB8);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB9);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB10);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB11);
	ADD_TEST4(BRoster, SuiteOfTests, FindAppTester, FindAppTestB12);

	return SuiteOfTests;
}


//------------------------------------------------------------------------------
//	LaunchTester.cpp
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
#include <FindDirectory.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "AppRunner.h"
#include "LaunchTester.h"
#include "LaunchTesterHelper.h"
#include "RosterTestAppDefs.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

static const char *testerSignature
	= "application/x-vnd.obos-roster-launch-test";
static const char *uninstalledType
	= "application/x-vnd.obos-roster-launch-uninstalled";
static const char *appType1	= "application/x-vnd.obos-roster-launch-app1";
static const char *appType2	= "application/x-vnd.obos-roster-launch-app2";
static const char *fileType1 = "application/x-vnd.obos-roster-launch-file1";
static const char *fileType2 = "application/x-vnd.obos-roster-launch-file2";
static const char *textTestType = "text/x-vnd.obos-roster-launch";

static const char *testDir		= "/tmp/testdir";
static const char *appFile1		= "/tmp/testdir/app1";
static const char *appFile2		= "/tmp/testdir/app2";
//static const char *testFile1	= "/tmp/testdir/testFile1";
//static const char *testLink1	= "/tmp/testdir/testLink1";
static const char *trashAppName	= "roster-launch-app";

// dump_messenger
/*static
void
dump_messenger(BMessenger messenger)
{
	struct fake_messenger {
		port_id	fPort;
		int32	fHandlerToken;
		team_id	fTeam;
		int32	extra0;
		int32	extra1;
		bool	fPreferredTarget;
		bool	extra2;
		bool	extra3;
		bool	extra4;
	} &fake = *(fake_messenger*)&messenger;
	printf("BMessenger: fPort:            %ld\n"
		   "            fHandlerToken:    %ld\n"
		   "            fTeam:            %ld\n"
		   "            fPreferredTarget: %d\n",
		   fake.fPort, fake.fHandlerToken, fake.fTeam, fake.fPreferredTarget);
}*/


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
ref_for_path(const char *filename)
{
	entry_ref ref;
	BEntry entry;
	CHK(entry.SetTo(filename, true) == B_OK);
	CHK(entry.GetRef(&ref) == B_OK);
	return ref;
}

// ref_for_team
static
entry_ref
ref_for_team(team_id team)
{
	BRoster roster;
	app_info info;
	CHK(roster.GetRunningAppInfo(team, &info) == B_OK);
	return info.ref;
}

// create_app
static
void
create_app(const char *filename, const char *signature,
		   bool install = false, bool makeExecutable = true)
{
	BString testApp;
	CHK(find_test_app("RosterLaunchTestApp1", &testApp) == B_OK);
	system((string("cp ") + testApp.String() + " " + filename).c_str());
	if (makeExecutable)
		system((string("chmod a+x ") + filename).c_str());
	BFile file;
	CHK(file.SetTo(filename, B_READ_WRITE) == B_OK);
	BAppFileInfo appFileInfo;
	CHK(appFileInfo.SetTo(&file) == B_OK);
	CHK(appFileInfo.SetSignature(signature) == B_OK);
	CHK(appFileInfo.SetAppFlags(B_SINGLE_LAUNCH) == B_OK);
	if (install)
		CHK(BMimeType(signature).Install() == B_OK);
	// We write the signature into a separate attribute, just in case we
	// decide to also test files without BEOS:APP_SIG attribute.
	BString signatureString(signature);
	file.WriteAttrString("signature", &signatureString);
}

// create_file
/*static
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
}*/

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
LaunchTester::setUp()
{
	fApplication = new RosterLaunchApp(testerSignature);
	system((string("mkdir ") + testDir).c_str());
}

// tearDown
void
LaunchTester::tearDown()
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

/*
	@case 1			uninstalled type mimeType
	@results		Should return B_LAUNCH_FAILED_APP_NOT_FOUND.
*/
static
void
CommonLaunchTest1(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	team_id team;
	CHK(context(uninstalledType, &team) == B_LAUNCH_FAILED_APP_NOT_FOUND);
}

/*
	@case 2			installed type mimeType, no preferred app
	@results		Should return B_LAUNCH_FAILED_NO_PREFERRED_APP.
*/
static
void
CommonLaunchTest2(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	install_type(fileType1);
	team_id team;
	CHK(context(fileType1, &team) == B_LAUNCH_FAILED_NO_PREFERRED_APP);
}

/*
	@case 3			installed type mimeType, preferred app, app type not
					installed, app has no signature
	@results		Should return B_LAUNCH_FAILED_APP_NOT_FOUND.
*/
static
void
CommonLaunchTest3(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_LAUNCH_FAILED_APP_NOT_FOUND);
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
CommonLaunchTest4(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	int32 cookie = 0;
	context.CheckNextMessage(team, cookie, MSG_STARTED);
	context.CheckNextMessage(team, cookie, MSG_READY_TO_RUN);
	context.CheckNextMessage(team, cookie, MSG_QUIT_REQUESTED);
	context.CheckNextMessage(team, cookie, MSG_TERMINATED);
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
CommonLaunchTest5(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(appFile1, appType1, true);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
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
CommonLaunchTest6(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(appFile1, appType1, true, false);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
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
CommonLaunchTest7(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType1, true);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
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
CommonLaunchTest8(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(appFile1, appType1);
	set_version(appFile1, 1);
	create_app(appFile2, appType1, true);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
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
CommonLaunchTest9(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(appFile1, appType1);
	set_version(appFile1, 2);
	create_app(appFile2, appType1, true);
	set_version(appFile1, 1);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
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
CommonLaunchTest10(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(appFile1, appType2);
	set_type_app_hint(appType1, appFile1);
	entry_ref appHint;
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_OK);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_ENTRY_NOT_FOUND);
	CHK(BMimeType(appType2).IsInstalled() == false);
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
CommonLaunchTest11(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(appFile1, appType1);
	set_type_app_hint(appType1, appFile2);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
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
CommonLaunchTest12(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(appFile1, appType1);
	team_id team;
	CHK(context(appType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
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
CommonLaunchTest13(LaunchCaller &caller)
{
	LaunchContext context(caller);
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
	team_id team;
	CHK(context(textTestType, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
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
CommonLaunchTest14(LaunchCaller &caller)
{
	LaunchContext context(caller);
	BRoster roster;
	create_app(get_trash_app_file(), appType1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(fileType1, &team) == B_LAUNCH_FAILED_APP_IN_TRASH);
}

typedef void commonTestFunction(LaunchCaller &caller);
static commonTestFunction *commonTestFunctions[] = {
	CommonLaunchTest1, CommonLaunchTest2, CommonLaunchTest3,
	CommonLaunchTest4, CommonLaunchTest5, CommonLaunchTest6,
	CommonLaunchTest7, CommonLaunchTest8, CommonLaunchTest9,
	CommonLaunchTest10, CommonLaunchTest11, CommonLaunchTest12,
	CommonLaunchTest13, CommonLaunchTest14
};
static int32 commonTestFunctionCount
	= sizeof(commonTestFunctions) / sizeof(commonTestFunction*);

/*
	status_t Launch(const char *mimeType, BMessage *initialMsgs,
					team_id *appTeam) const
	@case 1			mimeType is NULL
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestA1()
{
	BRoster roster;
	CHK(roster.Launch((const char*)NULL, (BMessage*)NULL, NULL)
		== B_BAD_VALUE);
}

/*
	status_t Launch(const char *mimeType, BMessage *initialMsgs,
					team_id *appTeam) const
	@case 2			mimeType is invalid
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestA2()
{
	BRoster roster;
	entry_ref ref;
	CHK(roster.Launch("invalid/mine/type", (BMessage*)NULL, NULL)
		== B_BAD_VALUE);
}

// LaunchTypeCaller
class LaunchTypeCaller : public LaunchCaller {
public:
	virtual status_t operator()(const char *type, team_id *team)
	{
		BRoster roster;
		return roster.Launch(type, (BMessage*)NULL, team);
	}
};

/*
	status_t Launch(const char *mimeType, BMessage *initialMsgs,
					team_id *appTeam) const
	@case 3			Launch(const char*, entry_ref*) cases 3-16
					(== common cases 1-14)
*/
void LaunchTester::LaunchTestA3()
{
	LaunchTypeCaller caller;
	for (int32 i = 0; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

#if 0
/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 1			ref or app are NULL
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestB1()
{
	BRoster roster;
	CHK(roster.Launch((entry_ref*)NULL, NULL) == B_BAD_VALUE);
// R5: Crashes when passing a NULL (app) ref.
#ifndef TEST_R5
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, appType1));
	CHK(roster.Launch(&fileRef, NULL) == B_BAD_VALUE);
#endif
	entry_ref ref;
	CHK(roster.Launch((entry_ref*)NULL, &ref) == B_BAD_VALUE);
}

/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 2			ref doesn't refer to an existing entry =>
	@results		Should return B_ENTRY_NOT_FOUND.
*/
void LaunchTester::LaunchTestB2()
{
	BRoster roster;
	entry_ref fileRef(ref_for_path(testFile1));
	entry_ref ref;
	CHK(roster.Launch(&fileRef, &ref) == B_ENTRY_NOT_FOUND);
}

/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 3			ref is valid, file has type and preferred app, preferred
					app is in trash
	@results		Should return B_LAUNCH_FAILED_APP_IN_TRASH.
*/
void LaunchTester::LaunchTestB3()
{
	BRoster roster;
	create_app(get_trash_app_file(), appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, appType1));
	entry_ref ref;
	CHK(roster.Launch(&fileRef, &ref) == B_LAUNCH_FAILED_APP_IN_TRASH);
}

/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 4			ref is valid, file has type and preferred app, app type is
					not installed, app exists and has signature
	@results		Should return B_OK and set the ref to refer to the file's
					(not the file type's) preferred application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestB4()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, appType2));
	entry_ref ref;
	CHK(roster.Launch(&fileRef, &ref) == B_OK);
	CHK(ref_for_path(appFile2) == ref);
	check_app_type(appType2, appFile2);
}

/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 5			ref is valid, file has no type, but preferred app,
					app type is not installed, app exists and has signature
	@results		Should return B_OK and set the ref to refer to the
					application's executable. Should install the app type and
					set the app hint on it.
*/
void LaunchTester::LaunchTestB5()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	entry_ref ref;
	CHK(roster.Launch(&fileRef, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 6			ref is valid, file has type and app hint, the type's
					preferred app type is not installed, app exists and has
					signature
	@results		Should return B_OK and set the ref to refer to the file
					type's preferred application's executable. Should install
					the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestB6()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, NULL, appFile2));
	entry_ref ref;
	CHK(roster.Launch(&fileRef, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 7			ref is valid, file has type, the type's preferred app
					type is not installed, app exists and has signature,
					file is executable
	@results		Should return B_OK and set the ref to refer to the file.
					Should not set the app hint on the app or file type (Why?).
*/
void LaunchTester::LaunchTestB7()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1));
	system((string("chmod a+x ") + testFile1).c_str());
	entry_ref ref;
	CHK(roster.Launch(&fileRef, &ref) == B_OK);
	CHK(ref_for_path(testFile1) == ref);
	CHK(BMimeType(appType1).IsInstalled() == false);
	CHK(BMimeType(fileType1).GetAppHint(&ref) == B_ENTRY_NOT_FOUND);
}

/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 8			ref is valid and refers to a link to a file, file has type,
					the type's preferred app type is not installed,
					app exists and has signature
	@results		Should return B_OK and set the ref to refer to the file
					type's preferred application's executable. Should install
					the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestB8()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	create_file(testFile1, fileType1);
	system((string("ln -s ") + testFile1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1));
	entry_ref ref;
	CHK(roster.Launch(&linkRef, &ref) == B_OK);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
}

// FileWithTypeCaller
class FileWithTypeCaller : public LaunchCaller {
public:
	virtual status_t operator()(const char *type, entry_ref *ref)
	{
		BRoster roster;
		entry_ref fileRef(create_file(testFile1, type));
		return roster.Launch(&fileRef, ref);
	}
};

/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 9			ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					Launch(const char*, entry_ref*) cases 4-16
					(== common cases 2-14)
*/
void LaunchTester::LaunchTestB9()
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
class SniffFileTypeCaller : public LaunchCaller {
public:
	virtual status_t operator()(const char *type, entry_ref *ref)
	{
		BRoster roster;
		entry_ref fileRef(create_file(testFile1, type, NULL, NULL,
						  "UnIQe pAtTeRn"));
		install_type(fileType1, NULL, "1.0 [0] ('UnIQe pAtTeRn')");
		return roster.Launch(&fileRef, ref);
	}
};

/*
	status_t Launch(entry_ref *ref, entry_ref *app) const
	@case 10		ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					Launch(const char*, entry_ref*) cases 3-16
					(== common cases 1-14)
*/
void LaunchTester::LaunchTestB10()
{
	SniffFileTypeCaller caller;
	for (int32 i = 1; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

#endif // 0


Test* LaunchTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestA2);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestA3);

//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB1);
//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB2);
//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB3);
//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB4);
//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB5);
//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB6);
//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB7);
//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB8);
//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB9);
//	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB10);

	return SuiteOfTests;
}


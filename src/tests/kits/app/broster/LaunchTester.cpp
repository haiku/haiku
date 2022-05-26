//------------------------------------------------------------------------------
//	LaunchTester.cpp
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
static const char *testFile1	= "/tmp/testdir/testFile1";
static const char *testLink1	= "/tmp/testdir/testLink1";
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
ref_for_path(const char *filename, bool traverse = true)
{
	entry_ref ref;
	BEntry entry;
	CHK(entry.SetTo(filename, traverse) == B_OK);
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
		   bool install = false, bool makeExecutable = true,
		   uint32 appFlags = B_SINGLE_LAUNCH)
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
	if (signature)
		CHK(appFileInfo.SetSignature(signature) == B_OK);
	CHK(appFileInfo.SetAppFlags(appFlags) == B_OK);
	if (install && signature)
		CHK(BMimeType(signature).Install() == B_OK);
	// We write the signature into a separate attribute, just in case we
	// decide to also test files without BEOS:APP_SIG attribute.
	BString signatureString(signature);
	file.WriteAttrString("signature", &signatureString);
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
	LaunchContext context;
	BRoster roster;
	team_id team;
	CHK(context(caller, uninstalledType, &team) == B_LAUNCH_FAILED_APP_NOT_FOUND);
}

/*
	@case 2			installed type mimeType, no preferred app
	@results		Should return B_LAUNCH_FAILED_NO_PREFERRED_APP.
*/
static
void
CommonLaunchTest2(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	install_type(fileType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_LAUNCH_FAILED_NO_PREFERRED_APP);
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
	LaunchContext context;
	BRoster roster;
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_LAUNCH_FAILED_APP_NOT_FOUND);
}

/*
	@case 4			installed type mimeType, preferred app, app type not
					installed, app has signature
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
static
void
CommonLaunchTest4(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 5			installed type mimeType, preferred app, app type installed,
					app has signature
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should set the app
					hint on the app type.
*/
static
void
CommonLaunchTest5(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1, true);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 6			installed type mimeType, preferred app, app type installed,
					app has signature, app has no execute permission
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should set the app
					hint on the app type.
*/
static
void
CommonLaunchTest6(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1, true, false);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 7			installed type mimeType, preferred app, app type installed,
					two apps have the signature
	@results		Should return B_OK and set team to the ID of the team
					running the application executable with the most recent
					modification time. Should set the app hint on the app type.
*/
static
void
CommonLaunchTest7(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType1, true);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile2) == ref);
	check_app_type(appType1, appFile2);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 8			installed type mimeType, preferred app, app type installed,
					two apps have the signature, one has a version info, the
					other one is newer
	@results		Should return B_OK and set team to the ID of the team
					running the application executable with version info.
					Should set the app hint on the app type.
*/
static
void
CommonLaunchTest8(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	set_version(appFile1, 1);
	create_app(appFile2, appType1, true);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 9			installed type mimeType, preferred app, app type installed,
					two apps have the signature, both apps have a version info
	@results		Should return B_OK and set team to the ID of the team
					running the application executable with the greater
					version. Should set the app hint on the app type.
*/
static
void
CommonLaunchTest9(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	set_version(appFile1, 2);
	create_app(appFile2, appType1, true);
	set_version(appFile1, 1);
	set_file_time(appFile2, time(NULL) + 1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 10		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint that points to an app
					with a different signature
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should remove the
					incorrect app hint on the app type. (Haiku: Should set the
					correct app hint. Don't even return the wrong app?)
*/
static
void
CommonLaunchTest10(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType2);
	set_type_app_hint(appType1, appFile1);
	entry_ref appHint;
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_OK);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_ENTRY_NOT_FOUND);
// Haiku: We set the app hint for app type 2. There's no reason not to do it.
#ifdef TEST_R5
	CHK(BMimeType(appType2).IsInstalled() == false);
#else
	check_app_type(appType2, appFile1);
#endif
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 11		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint pointing to void,
					a differently named app with this signature exists
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should update the
					app hint on the app type.
*/
static
void
CommonLaunchTest11(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	set_type_app_hint(appType1, appFile2);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 12		mimeType is app signature, not installed
	@results		Should return B_OK and set team to the ID of the team
					running the application executable. Should set the app
					hint on the app type.
*/
static
void
CommonLaunchTest12(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	team_id team;
	CHK(context(caller, appType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 13		mimeType is installed, but has no preferred application,
					super type has preferred application
	@results		Should return B_OK and set team to the ID of the team
					running the application executable associated with the
					preferred app of the supertype. Should set the app hint
					on the app type.
*/
static
void
CommonLaunchTest13(LaunchCaller &caller)
{
	LaunchContext context;
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
	CHK(context(caller, textTestType, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 14		installed type mimeType, preferred app, app type not
					installed, app has signature, app is trash
	@results		Should return B_LAUNCH_FAILED_APP_IN_TRASH.
*/
static
void
CommonLaunchTest14(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(get_trash_app_file(), appType1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_LAUNCH_FAILED_APP_IN_TRASH);
}

/*
	@case 15		installed type mimeType, preferred app, app type not
					installed, app has signature, team is NULL
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
static
void
CommonLaunchTest15(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	CHK(context(caller, fileType1, NULL) == B_OK);
	context.WaitForMessage(uint32(MSG_STARTED), true);
	team_id team = context.TeamAt(0);
	CHK(team >= 0);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	@case 16		launch the app two times: B_MULTIPLE_LAUNCH | B_ARGV_ONLY
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	ArgvReceived(), ReadyToRun(), QuitRequested()
*/
static
void
CommonLaunchTest16(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1, false, true,
			   B_MULTIPLE_LAUNCH | B_ARGV_ONLY);
	install_type(fileType1, appType1);
	// launch app 1
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_OK);
	entry_ref ref2 = ref_for_team(team2);
	CHK(ref_for_path(appFile1) == ref2);
	check_app_type(appType1, appFile1);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
//	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
//	if (caller.SupportsRefs() && !caller.SupportsArgv())
//		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
	// checks 2
	cookie = 0;
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller2, team2, cookie, &ref2));
//	CHK(context.CheckMessageMessages(caller2, team2, cookie));
	CHK(context.CheckArgvMessage(caller2, team2, cookie, &ref2));
//	if (caller.SupportsRefs() && !caller.SupportsArgv())
//		CHK(context.CheckRefsMessage(caller2, team2, cookie));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_TERMINATED));
}

/*
	@case 17		launch the app two times: B_MULTIPLE_LAUNCH | B_ARGV_ONLY
	@results		first app:	{Message,Argv,Refs}Received()*, ReadyToRun(),
								QuitRequested()
					second app:	{Message,Argv,Refs}Received()*, ReadyToRun(),
								QuitRequested()
*/
static
void
CommonLaunchTest17(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1, false, true,
			   B_MULTIPLE_LAUNCH);
	install_type(fileType1, appType1);
	// launch app 1
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_OK);
	entry_ref ref2 = ref_for_team(team2);
	CHK(ref_for_path(appFile1) == ref2);
	check_app_type(appType1, appFile1);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
	// checks 2
	cookie = 0;
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller2, team2, cookie, &ref2));
	CHK(context.CheckMessageMessages(caller2, team2, cookie));
	CHK(context.CheckArgvMessage(caller2, team2, cookie, &ref2));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller2, team2, cookie));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_TERMINATED));
}

/*
	@case 18		launch the app two times: B_SINGLE_LAUNCH | B_ARGV_ONLY
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
								(No second ArgvReceived()!)
					second app:	Launch() fails with B_ALREADY_RUNNING
*/
static
void
CommonLaunchTest18(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1, false, true,
			   B_SINGLE_LAUNCH | B_ARGV_ONLY);
	install_type(fileType1, appType1);
	// launch app 1
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_ALREADY_RUNNING);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
//	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
//	if (caller.SupportsRefs() && !caller.SupportsArgv())
//		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
}

/*
	@case 19		launch the app two times: B_SINGLE_LAUNCH
	@results		first app:	{Message,Argv,Refs}Received()*, ReadyToRun(),
								{Message,Argv,Refs}Received()*, QuitRequested()
					second app:	Launch() fails with B_ALREADY_RUNNING
*/
static
void
CommonLaunchTest19(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1, false, true,
			   B_SINGLE_LAUNCH);
	install_type(fileType1, appType1);
	// launch app 1
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_ALREADY_RUNNING);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
}

/*
	@case 20		launch two apps with the same signature:
					B_SINGLE_LAUNCH | B_ARGV_ONLY
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	ArgvReceived(), ReadyToRun(), QuitRequested()
*/
static
void
CommonLaunchTest20(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	// launch app 1
	create_app(appFile1, appType1, false, true,
			   B_SINGLE_LAUNCH | B_ARGV_ONLY);
	install_type(fileType1, appType1);
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2 (greater modification time)
	CHK(BMimeType(appType1).Delete() == B_OK);
	create_app(appFile2, appType1, false, true,
			   B_SINGLE_LAUNCH | B_ARGV_ONLY);
	set_file_time(appFile2, time(NULL) + 1);
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_OK);
	entry_ref ref2 = ref_for_team(team2);
	CHK(ref_for_path(appFile2) == ref2);
	check_app_type(appType1, appFile2);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
//	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
//	if (caller.SupportsRefs() && !caller.SupportsArgv())
//		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
	// checks 2
	cookie = 0;
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller2, team2, cookie, &ref2));
//	CHK(context.CheckMessageMessages(caller2, team2, cookie));
	CHK(context.CheckArgvMessage(caller2, team2, cookie, &ref2));
//	if (caller.SupportsRefs() && !caller.SupportsArgv())
//		CHK(context.CheckRefsMessage(caller2, team2, cookie));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_TERMINATED));
}

/*
	@case 21		launch two apps with the same signature: B_SINGLE_LAUNCH
	@results		first app:	{Message,Argv,Refs}Received()*, ReadyToRun(),
								QuitRequested()
					second app:	{Message,Argv,Refs}Received()*, ReadyToRun(),
								QuitRequested()
*/
static
void
CommonLaunchTest21(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	// launch app 1
	create_app(appFile1, appType1, false, true,
			   B_SINGLE_LAUNCH);
	install_type(fileType1, appType1);
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2 (greater modification time)
	CHK(BMimeType(appType1).Delete() == B_OK);
	create_app(appFile2, appType1, false, true,
			   B_SINGLE_LAUNCH);
	set_file_time(appFile2, time(NULL) + 1);
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_OK);
	entry_ref ref2 = ref_for_team(team2);
	CHK(ref_for_path(appFile2) == ref2);
	check_app_type(appType1, appFile2);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
	// checks 2
	cookie = 0;
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller2, team2, cookie, &ref2));
	CHK(context.CheckMessageMessages(caller2, team2, cookie));
	CHK(context.CheckArgvMessage(caller2, team2, cookie, &ref2));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller2, team2, cookie));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_TERMINATED));
}

/*
	@case 22		launch the app two times: B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
								(No second ArgvReceived()!)
					second app:	Launch() fails with B_ALREADY_RUNNING
*/
static
void
CommonLaunchTest22(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY);
	install_type(fileType1, appType1);
	// launch app 1
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_ALREADY_RUNNING);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
//	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
//	if (caller.SupportsRefs() && !caller.SupportsArgv())
//		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
}

/*
	@case 23		launch the app two times: B_EXCLUSIVE_LAUNCH
	@results		first app:	{Message,Argv,Refs}Received()*, ReadyToRun(),
								{Message,Argv,Refs}Received()*, QuitRequested()
					second app:	Launch() fails with B_ALREADY_RUNNING
*/
static
void
CommonLaunchTest23(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH);
	install_type(fileType1, appType1);
	// launch app 1
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_ALREADY_RUNNING);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
}

/*
	@case 24		launch two apps with the same signature:
					B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
								(No second ArgvReceived()!)
					second app:	Launch() fails with B_ALREADY_RUNNING
*/
static
void
CommonLaunchTest24(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	// launch app 1
	create_app(appFile1, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY);
	install_type(fileType1, appType1);
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2 (greater modification time)
	CHK(BMimeType(appType1).Delete() == B_OK);
	create_app(appFile2, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY);
	set_file_time(appFile2, time(NULL) + 1);
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_ALREADY_RUNNING);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
//	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
//	if (caller.SupportsRefs() && !caller.SupportsArgv())
//		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
}

/*
	@case 25		launch two apps with the same signature:
					B_EXCLUSIVE_LAUNCH
	@results		first app:	{Message,Argv,Refs}Received()*, ReadyToRun(),
								{Message,Argv,Refs}Received()*, QuitRequested()
					second app:	Launch() fails with B_ALREADY_RUNNING
*/
static
void
CommonLaunchTest25(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	// launch app 1
	create_app(appFile1, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH);
	install_type(fileType1, appType1);
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2 (greater modification time)
	CHK(BMimeType(appType1).Delete() == B_OK);
	create_app(appFile2, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH);
	set_file_time(appFile2, time(NULL) + 1);
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_ALREADY_RUNNING);
	entry_ref ref2 = ref_for_path(appFile2);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref2));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
}

/*
	@case 26		launch two apps with the same signature:
					first: B_EXCLUSIVE_LAUNCH,
					second: B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY =>
	@results		first app:	{Message,Argv,Refs}Received()*, ReadyToRun(),
								QuitRequested()
					second app:	Launch() fails with B_ALREADY_RUNNING
*/
static
void
CommonLaunchTest26(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	// launch app 1
	create_app(appFile1, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH);
	install_type(fileType1, appType1);
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2 (greater modification time)
	CHK(BMimeType(appType1).Delete() == B_OK);
	create_app(appFile2, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY);
	set_file_time(appFile2, time(NULL) + 1);
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_ALREADY_RUNNING);
	entry_ref ref2 = ref_for_path(appFile2);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
}

/*
	@case 27		launch two apps with the same signature:
					first: B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY,
					second: B_EXCLUSIVE_LAUNCH
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
								(No second ArgvReceived()!)
					second app:	Launch() fails with B_ALREADY_RUNNING
*/
static
void
CommonLaunchTest27(LaunchCaller &caller)
{
	LaunchCaller &caller2 = caller.Clone();
	LaunchContext context;
	BRoster roster;
	// launch app 1
	create_app(appFile1, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY);
	install_type(fileType1, appType1);
	team_id team1;
	CHK(context(caller, fileType1, &team1) == B_OK);
	entry_ref ref1 = ref_for_team(team1);
	CHK(ref_for_path(appFile1) == ref1);
	check_app_type(appType1, appFile1);
	context.WaitForMessage(team1, MSG_STARTED);
	// launch app 2 (greater modification time)
	CHK(BMimeType(appType1).Delete() == B_OK);
	create_app(appFile2, appType1, false, true,
			   B_EXCLUSIVE_LAUNCH);
	set_file_time(appFile2, time(NULL) + 1);
	team_id team2;
	CHK(context(caller2, fileType1, &team2) == B_ALREADY_RUNNING);
	// checks 1
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team1, cookie, &ref1));
//	CHK(context.CheckMessageMessages(caller, team1, cookie));
	CHK(context.CheckArgvMessage(caller, team1, cookie, &ref1));
//	if (caller.SupportsRefs() && !caller.SupportsArgv())
//		CHK(context.CheckRefsMessage(caller, team1, cookie));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team1, cookie, MSG_TERMINATED));
}

/*
	@case 28		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint pointing to void,
					no app with this signature exists
	@results		Should return B_LAUNCH_FAILED_APP_NOT_FOUND and unset the
					app type's app hint.
*/
static
void
CommonLaunchTest28(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	set_type_app_hint(appType1, appFile1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_LAUNCH_FAILED_APP_NOT_FOUND);
	entry_ref appHint;
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_ENTRY_NOT_FOUND);
}

/*
	@case 29		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint pointing to a cyclic
					link, no app with this signature exists
	@results		Should return
					Haiku: B_LAUNCH_FAILED_APP_NOT_FOUND and unset the app
					type's app hint.
					R5: B_ENTRY_NOT_FOUND or B_LAUNCH_FAILED_NO_RESOLVE_LINK.
*/
static
void
CommonLaunchTest29(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	set_type_app_hint(appType1, appFile1);
	install_type(fileType1, appType1);
	system((string("ln -s ") + appFile1 + " " + appFile1).c_str());
	team_id team;
	entry_ref appHint;
#if TEST_R5
	if (caller.SupportsRefs()) {
		CHK(context(caller, fileType1, &team)
			== B_LAUNCH_FAILED_NO_RESOLVE_LINK);
	} else
		CHK(context(caller, fileType1, &team) == B_ENTRY_NOT_FOUND);
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_OK);
	CHK(appHint == ref_for_path(appFile1, false));
#else
	CHK(context(caller, fileType1, &team) == B_LAUNCH_FAILED_APP_NOT_FOUND);
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_ENTRY_NOT_FOUND);
#endif
}

/*
	@case 30		installed type mimeType, preferred app, app type installed,
					preferred app type has an app hint that points to an app
					without a signature, app will pass a different signature
					to the BApplication constructor
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should remove the
					incorrect app hint on the app type.
					BRoster::GetRunningAppInfo() should return an app_info
					with the signature passed to the BApplication constructor.
*/
static
void
CommonLaunchTest30(LaunchCaller &caller)
{
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, NULL);
	set_type_app_hint(appType1, appFile1);
	entry_ref appHint;
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_OK);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
// Haiku: We unset the app hint for the app type. R5 leaves it untouched.
#ifdef TEST_R5
	check_app_type(appType1, appFile1);
#else
	CHK(BMimeType(appType1).GetAppHint(&appHint) == B_ENTRY_NOT_FOUND);
#endif
	context.WaitForMessage(team, MSG_STARTED, true);
	app_info appInfo;
	CHK(roster.GetRunningAppInfo(team, &appInfo) == B_OK);
// R5 keeps appType1, Haiku updates the signature
#ifdef TEST_R5
	CHK(!strcmp(appInfo.signature, appType1));
#else
	CHK(!strcmp(appInfo.signature, kDefaultTestAppSignature));
#endif
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	if (caller.SupportsRefs() && !caller.SupportsArgv())
		CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

typedef void commonTestFunction(LaunchCaller &caller);
static commonTestFunction *commonTestFunctions[] = {
	CommonLaunchTest1, CommonLaunchTest2, CommonLaunchTest3,
	CommonLaunchTest4, CommonLaunchTest5, CommonLaunchTest6,
	CommonLaunchTest7, CommonLaunchTest8, CommonLaunchTest9,
	CommonLaunchTest10, CommonLaunchTest11, CommonLaunchTest12,
	CommonLaunchTest13, CommonLaunchTest14, CommonLaunchTest15,
	CommonLaunchTest16, CommonLaunchTest17, CommonLaunchTest18,
	CommonLaunchTest19, CommonLaunchTest20, CommonLaunchTest21,
	CommonLaunchTest22, CommonLaunchTest23, CommonLaunchTest24,
	CommonLaunchTest25, CommonLaunchTest26, CommonLaunchTest27,
	CommonLaunchTest28, CommonLaunchTest29, CommonLaunchTest30
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
	BMessage message;
	CHK(roster.Launch((const char*)NULL, &message, NULL) == B_BAD_VALUE);
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
	BMessage message;
	CHK(roster.Launch("invalid/mine/type", &message, NULL) == B_BAD_VALUE);
}

// LaunchTypeCaller1
class LaunchTypeCaller1 : public LaunchCaller {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BMessage *message = (messages ? (BMessage*)messages->ItemAt(0L)
									  : NULL);
		BRoster roster;
		return roster.Launch(type, message, team);
	}

	virtual LaunchCaller *CloneInternal()
	{
		return new LaunchTypeCaller1;
	}
};

/*
	status_t Launch(const char *mimeType, BMessage *initialMsgs,
					team_id *appTeam) const
	@case 3			common cases 1-14
*/
void LaunchTester::LaunchTestA3()
{
	LaunchTypeCaller1 caller;
	for (int32 i = 0; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t Launch(const char *mimeType, BMessage *initialMsgs,
					team_id *appTeam) const
	@case 4			installed type mimeType, preferred app, app type not
					installed, app has signature, NULL initialMsg
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestA4()
{
	LaunchTypeCaller1 caller;
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, NULL, LaunchContext::kStandardArgc,
				LaunchContext::kStandardArgv, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
//	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const char *mimeType, BList *messageList,
					team_id *appTeam) const
	@case 1			mimeType is NULL
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestB1()
{
	BRoster roster;
	BList list;
	CHK(roster.Launch((const char*)NULL, &list, NULL) == B_BAD_VALUE);
}

/*
	status_t Launch(const char *mimeType, BMessage *initialMsgs,
					team_id *appTeam) const
	@case 2			mimeType is invalid
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestB2()
{
	BRoster roster;
	BList list;
	CHK(roster.Launch("invalid/mine/type", &list, NULL) == B_BAD_VALUE);
}

// LaunchTypeCaller2
class LaunchTypeCaller2 : public LaunchCaller {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		return roster.Launch(type, messages, team);
	}
	virtual int32 SupportsMessages() const { return 1000; }

	virtual LaunchCaller *CloneInternal()
	{
		return new LaunchTypeCaller2;
	}
};

/*
	status_t Launch(const char *mimeType, BList *messageList,
					team_id *appTeam) const
	@case 3			common cases 1-14
*/
void LaunchTester::LaunchTestB3()
{
	LaunchTypeCaller2 caller;
	for (int32 i = 0; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t Launch(const char *mimeType, BList *messageList,
					team_id *appTeam) const
	@case 4			installed type mimeType, preferred app, app type not
					installed, app has signature, NULL messageList
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestB4()
{
	LaunchTypeCaller2 caller;
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, NULL, LaunchContext::kStandardArgc,
				LaunchContext::kStandardArgv, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
//	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const char *mimeType, BList *messageList,
					team_id *appTeam) const
	@case 5			installed type mimeType, preferred app, app type not
					installed, app has signature, empty messageList
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestB5()
{
	LaunchTypeCaller2 caller;
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	team_id team;
	BList list;
	CHK(context(caller, fileType1, &list, LaunchContext::kStandardArgc,
				LaunchContext::kStandardArgv, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
//	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const char *mimeType, int argc, char **args,
					team_id *appTeam) const
	@case 1			mimeType is NULL or argc > 0 and args is NULL
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestC1()
{
	BRoster roster;
	char *argv[] = { "Hey!" };
	CHK(roster.Launch((const char*)NULL, 1, argv, NULL) == B_BAD_VALUE);
	CHK(roster.Launch((const char*)NULL, 1, (char**)NULL, NULL)
		== B_BAD_VALUE);
}

/*
	status_t Launch(const char *mimeType, int argc, char **args,
					team_id *appTeam) const
	@case 2			mimeType is invalid
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestC2()
{
	BRoster roster;
	BList list;
	char *argv[] = { "Hey!" };
	CHK(roster.Launch("invalid/mine/type", 1, argv, NULL) == B_BAD_VALUE);
}

// LaunchTypeCaller3
class LaunchTypeCaller3 : public LaunchCaller {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		return roster.Launch(type, argc, const_cast<char**>(argv), team);
	}
	virtual int32 SupportsMessages() const { return 0; }

	virtual LaunchCaller *CloneInternal()
	{
		return new LaunchTypeCaller3;
	}
};

/*
	status_t Launch(const char *mimeType, int argc, char **args,
					team_id *appTeam) const
	@case 3			common cases 1-14
*/
void LaunchTester::LaunchTestC3()
{
	LaunchTypeCaller3 caller;
	for (int32 i = 0; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t Launch(const char *mimeType, int argc, char **args,
					team_id *appTeam) const
	@case 4			installed type mimeType, preferred app, app type not
					installed, app has signature, NULL args, argc is 0
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestC4()
{
	LaunchTypeCaller3 caller;
	LaunchContext context;
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	team_id team;
	CHK(context(caller, fileType1, NULL, 0, (const char**)NULL, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);
	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref, 0, NULL));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
//	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

// SimpleFileCaller1
class SimpleFileCaller1 : public LaunchCaller {
public:
	SimpleFileCaller1() : fRef() {}
	SimpleFileCaller1(const entry_ref &ref) : fRef(ref) {}
	virtual ~SimpleFileCaller1() {}
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		BMessage *message = (messages ? (BMessage*)messages->ItemAt(0L)
									  : NULL);
		return roster.Launch(&fRef, message, team);
	}
	virtual bool SupportsRefs() const { return true; }
	virtual const entry_ref *Ref() const { return &fRef; }

	virtual LaunchCaller *CloneInternal()
	{
		return new SimpleFileCaller1;
	}

protected:
	entry_ref fRef;
};

// FileWithTypeCaller1
class FileWithTypeCaller1 : public SimpleFileCaller1 {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		BMessage *message = (messages ? (BMessage*)messages->ItemAt(0L)
									  : NULL);
		fRef = create_file(testFile1, type);
		return roster.Launch(&fRef, message, team);
	}

	virtual LaunchCaller *CloneInternal()
	{
		return new FileWithTypeCaller1;
	}
};

// SniffFileTypeCaller1
class SniffFileTypeCaller1 : public SimpleFileCaller1 {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		BMessage *message = (messages ? (BMessage*)messages->ItemAt(0L)
									  : NULL);
		fRef = create_file(testFile1, type, NULL, NULL, "UnIQe pAtTeRn");
		install_type(fileType1, NULL, "1.0 [0] ('UnIQe pAtTeRn')");
		return roster.Launch(&fRef, message, team);
	}

	virtual LaunchCaller *CloneInternal()
	{
		return new SniffFileTypeCaller1;
	}
};

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 1			ref is NULL
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestD1()
{
	BRoster roster;
	BMessage message;
	CHK(roster.Launch((const entry_ref*)NULL, &message, NULL) == B_BAD_VALUE);
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 2			ref doesn't refer to an existing entry
	@results		Should return B_ENTRY_NOT_FOUND.
*/
void LaunchTester::LaunchTestD2()
{
	BRoster roster;
	BMessage message;
	entry_ref fileRef(ref_for_path(testFile1));
	CHK(roster.Launch(&fileRef, &message, NULL) == B_ENTRY_NOT_FOUND);
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 3			ref is valid, file has type and preferred app, app type is
					not installed, app exists and has signature
	@results		Should return B_OK and set team to the ID of the team
					running the file's (not the file type's) preferred
					application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestD3()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, appType2));
	SimpleFileCaller1 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile2) == ref);
	check_app_type(appType2, appFile2);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 4			ref is valid, file has no type, but preferred app,
					app type is not installed, app exists and has signature
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestD4()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	SimpleFileCaller1 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 5			ref is valid, file has type and app hint, the type's
					preferred app type is not installed, app exists and has
					signature
	@results		Should return B_OK and set team to the ID of the team
					running the file type's preferred application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestD5()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, NULL, appFile2));
	SimpleFileCaller1 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 6			ref is valid, file has type, the type's preferred app
					type is not installed, app exists and has signature, file
					has executable permission, but is not executable
	@results		Should return B_LAUNCH_FAILED_EXECUTABLE.
					Should not set the app hint on the app or file type.
*/
void LaunchTester::LaunchTestD6()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1));
	system((string("chmod a+x ") + testFile1).c_str());
	BMessage message;
	team_id team;
	CHK(roster.Launch(&fileRef, &message, &team)
		== B_LAUNCH_FAILED_EXECUTABLE);
	CHK(BMimeType(appType1).IsInstalled() == false);
	CHK(BMimeType(fileType1).GetAppHint(&fileRef) == B_ENTRY_NOT_FOUND);
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 7			ref is valid and refers to a link to a file, file has type,
					the type's preferred app type is not installed,
					app exists and has signature
	@results		Should return B_OK and set team to the ID of the team
					running the file type's preferred application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestD7()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	create_file(testFile1, fileType1);
	system((string("ln -s ") + testFile1 + " " + testLink1).c_str());
	entry_ref fileRef(ref_for_path(testFile1, false));
	entry_ref linkRef(ref_for_path(testLink1, false));
	SimpleFileCaller1 caller(linkRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie, &fileRef));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 8			ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					Launch(const char*, entry_ref*) cases 4-16
					(== common cases 2-14)
*/
void LaunchTester::LaunchTestD8()
{
	FileWithTypeCaller1 caller;
	for (int32 i = 0; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 9			ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					Launch(const char*, entry_ref*) cases 3-16
					(== common cases 1-14)
*/
void LaunchTester::LaunchTestD9()
{
	SniffFileTypeCaller1 caller;
	for (int32 i = 1; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 10		ref is valid, file has no type, but preferred app, app
					type is not installed, app exists and has signature,
					NULL initialMessage
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestD10()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	SimpleFileCaller1 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, NULL, LaunchContext::kStandardArgc,
				LaunchContext::kStandardArgv, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 11		ref is valid and refers to a cyclic link
	@results		Should return B_LAUNCH_FAILED_NO_RESOLVE_LINK.
*/
void LaunchTester::LaunchTestD11()
{
	BRoster roster;
	system((string("ln -s ") + testLink1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1, false));
	BMessage message;
	team_id team;
	CHK(roster.Launch(&linkRef, &message, &team)
		== B_LAUNCH_FAILED_NO_RESOLVE_LINK);
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 12		ref is valid and refers to a link to void
	@results		Should return B_LAUNCH_FAILED_NO_RESOLVE_LINK.
*/
void LaunchTester::LaunchTestD12()
{
	BRoster roster;
	system((string("ln -s ") + testFile1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1, false));
	BMessage message;
	team_id team;
	CHK(roster.Launch(&linkRef, &message, &team)
		== B_LAUNCH_FAILED_NO_RESOLVE_LINK);
}

/*
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage,
					team_id *app_team) const
	@case 13		ref is valid and refers to an executable without signature
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable.
*/
void LaunchTester::LaunchTestD13()
{
	BRoster roster;
	create_app(appFile1, NULL);
	entry_ref fileRef(ref_for_path(appFile1));
	SimpleFileCaller1 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, appType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	context.WaitForMessage(team, MSG_STARTED, true);
	app_info appInfo;
	CHK(roster.GetRunningAppInfo(team, &appInfo) == B_OK);
	CHK(!strcmp(appInfo.signature, kDefaultTestAppSignature));

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
//	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckArgsMessage(caller, team, cookie, &ref, NULL,
								 0, NULL, MSG_MAIN_ARGS));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
//	CHK(context.CheckArgsMessage(caller, team, cookie, &ref, NULL,
//								 LaunchContext::kStandardArgc,
//								 LaunchContext::kStandardArgv,
//								 MSG_ARGV_RECEIVED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

// SimpleFileCaller2
class SimpleFileCaller2 : public LaunchCaller {
public:
	SimpleFileCaller2() : fRef() {}
	SimpleFileCaller2(const entry_ref &ref) : fRef(ref) {}
	virtual ~SimpleFileCaller2() {}
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		return roster.Launch(&fRef, messages, team);
	}
	virtual int32 SupportsMessages() const { return 1000; }
	virtual bool SupportsRefs() const { return true; }
	virtual const entry_ref *Ref() const { return &fRef; }

	virtual LaunchCaller *CloneInternal()
	{
		return new SimpleFileCaller2;
	}

protected:
	entry_ref fRef;
};

// FileWithTypeCaller2
class FileWithTypeCaller2 : public SimpleFileCaller2 {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		fRef = create_file(testFile1, type);
		return roster.Launch(&fRef, messages, team);
	}

	virtual LaunchCaller *CloneInternal()
	{
		return new FileWithTypeCaller2;
	}
};

// SniffFileTypeCaller2
class SniffFileTypeCaller2 : public SimpleFileCaller2 {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		fRef = create_file(testFile1, type, NULL, NULL, "UnIQe pAtTeRn");
		install_type(fileType1, NULL, "1.0 [0] ('UnIQe pAtTeRn')");
		return roster.Launch(&fRef, messages, team);
	}

	virtual LaunchCaller *CloneInternal()
	{
		return new SniffFileTypeCaller2;
	}
};

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 1			ref is NULL
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestE1()
{
	BRoster roster;
	BList list;
	CHK(roster.Launch((const entry_ref*)NULL, &list, NULL) == B_BAD_VALUE);
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 2			ref doesn't refer to an existing entry
	@results		Should return B_ENTRY_NOT_FOUND.
*/
void LaunchTester::LaunchTestE2()
{
	BRoster roster;
	BMessage message;
	entry_ref fileRef(ref_for_path(testFile1));
	BList list;
	CHK(roster.Launch(&fileRef, &list, NULL) == B_ENTRY_NOT_FOUND);
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 3			ref is valid, file has type and preferred app, app type is
					not installed, app exists and has signature
	@results		Should return B_OK and set team to the ID of the team
					running the file's (not the file type's) preferred
					application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestE3()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, appType2));
	SimpleFileCaller2 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile2) == ref);
	check_app_type(appType2, appFile2);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 4			ref is valid, file has no type, but preferred app,
					app type is not installed, app exists and has signature
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestE4()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	SimpleFileCaller2 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 5			ref is valid, file has type and app hint, the type's
					preferred app type is not installed, app exists and has
					signature
	@results		Should return B_OK and set team to the ID of the team
					running the file type's preferred application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestE5()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, NULL, appFile2));
	SimpleFileCaller2 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 6			ref is valid, file has type, the type's preferred app
					type is not installed, app exists and has signature, file
					has executable permission, but is not executable
	@results		Should return B_LAUNCH_FAILED_EXECUTABLE.
					Should not set the app hint on the app or file type.
*/
void LaunchTester::LaunchTestE6()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1));
	system((string("chmod a+x ") + testFile1).c_str());
	BList list;
	team_id team;
	CHK(roster.Launch(&fileRef, &list, &team) == B_LAUNCH_FAILED_EXECUTABLE);
	CHK(BMimeType(appType1).IsInstalled() == false);
	CHK(BMimeType(fileType1).GetAppHint(&fileRef) == B_ENTRY_NOT_FOUND);
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 7			ref is valid and refers to a link to a file, file has type,
					the type's preferred app type is not installed,
					app exists and has signature
	@results		Should return B_OK and set team to the ID of the team
					running the file type's preferred application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestE7()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	create_file(testFile1, fileType1);
	system((string("ln -s ") + testFile1 + " " + testLink1).c_str());
	entry_ref fileRef(ref_for_path(testFile1, false));
	entry_ref linkRef(ref_for_path(testLink1, false));
	SimpleFileCaller2 caller(linkRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie, &fileRef));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 8			ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					Launch(const char*, entry_ref*) cases 4-16
					(== common cases 2-14)
*/
void LaunchTester::LaunchTestE8()
{
	FileWithTypeCaller2 caller;
	for (int32 i = 0; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 9			ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					Launch(const char*, entry_ref*) cases 3-16
					(== common cases 1-14)
*/
void LaunchTester::LaunchTestE9()
{
	SniffFileTypeCaller2 caller;
	for (int32 i = 1; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 10		ref is valid, file has no type, but preferred app, app
					type is not installed, app exists and has signature,
					NULL messageList
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestE10()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	SimpleFileCaller2 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, NULL, LaunchContext::kStandardArgc,
				LaunchContext::kStandardArgv, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 11		ref is valid, file has no type, but preferred app, app
					type is not installed, app exists and has signature,
					empty messageList
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestE11()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	SimpleFileCaller2 caller(fileRef);
	LaunchContext context;
	team_id team;
	BList list;
	CHK(context(caller, fileType1, &list, LaunchContext::kStandardArgc,
				LaunchContext::kStandardArgv, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 12		ref is valid and refers to a cyclic link
	@results		Should return B_LAUNCH_FAILED_NO_RESOLVE_LINK.
*/
void LaunchTester::LaunchTestE12()
{
	BRoster roster;
	system((string("ln -s ") + testLink1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1, false));
	BMessage message;
	team_id team;
	BList list;
	CHK(roster.Launch(&linkRef, &list, &team)
		== B_LAUNCH_FAILED_NO_RESOLVE_LINK);
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 13		ref is valid and refers to a link to void
	@results		Should return B_LAUNCH_FAILED_NO_RESOLVE_LINK.
*/
void LaunchTester::LaunchTestE13()
{
	BRoster roster;
	system((string("ln -s ") + testFile1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1, false));
	BMessage message;
	team_id team;
	BList list;
	CHK(roster.Launch(&linkRef, &list, &team)
		== B_LAUNCH_FAILED_NO_RESOLVE_LINK);
}

/*
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam) const
	@case 14		ref is valid and refers to an executable without signature
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable.
*/
void LaunchTester::LaunchTestE14()
{
	BRoster roster;
	create_app(appFile1, NULL);
	entry_ref fileRef(ref_for_path(appFile1));
	SimpleFileCaller2 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, appType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	context.WaitForMessage(team, MSG_STARTED, true);
	app_info appInfo;
	CHK(roster.GetRunningAppInfo(team, &appInfo) == B_OK);
	CHK(!strcmp(appInfo.signature, kDefaultTestAppSignature));

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
//	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckArgsMessage(caller, team, cookie, &ref, NULL,
								 0, NULL, MSG_MAIN_ARGS));
	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
//	CHK(context.CheckArgsMessage(caller, team, cookie, &ref, NULL,
//								 LaunchContext::kStandardArgc,
//								 LaunchContext::kStandardArgv,
//								 MSG_ARGV_RECEIVED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

// SimpleFileCaller3
class SimpleFileCaller3 : public LaunchCaller {
public:
	SimpleFileCaller3() : fRef() {}
	SimpleFileCaller3(const entry_ref &ref) : fRef(ref) {}
	virtual ~SimpleFileCaller3() {}
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		return roster.Launch(&fRef, argc, argv, team);
	}
	virtual int32 SupportsMessages() const { return 0; }
	virtual bool SupportsRefs() const { return true; }
	virtual const entry_ref *Ref() const { return &fRef; }

	virtual LaunchCaller *CloneInternal()
	{
		return new SimpleFileCaller3;
	}

protected:
	entry_ref fRef;
};

// FileWithTypeCaller3
class FileWithTypeCaller3 : public SimpleFileCaller3 {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		fRef = create_file(testFile1, type);
		return roster.Launch(&fRef, argc, argv, team);
	}

	virtual LaunchCaller *CloneInternal()
	{
		return new FileWithTypeCaller3;
	}
};

// SniffFileTypeCaller3
class SniffFileTypeCaller3 : public SimpleFileCaller3 {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		BRoster roster;
		fRef = create_file(testFile1, type, NULL, NULL, "UnIQe pAtTeRn");
		install_type(fileType1, NULL, "1.0 [0] ('UnIQe pAtTeRn')");
		return roster.Launch(&fRef, argc, argv, team);
	}

	virtual LaunchCaller *CloneInternal()
	{
		return new SniffFileTypeCaller3;
	}
};

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 1			ref is NULL
	@results		Should return B_BAD_VALUE.
*/
void LaunchTester::LaunchTestF1()
{
	BRoster roster;
	char *argv[] = { "Hey!" };
	CHK(roster.Launch((const entry_ref*)NULL, 1, argv, NULL) == B_BAD_VALUE);
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 2			ref doesn't refer to an existing entry
	@results		Should return B_ENTRY_NOT_FOUND.
*/
void LaunchTester::LaunchTestF2()
{
	BRoster roster;
	BMessage message;
	entry_ref fileRef(ref_for_path(testFile1));
	char *argv[] = { "Hey!" };
	CHK(roster.Launch(&fileRef, 1, argv, NULL) == B_ENTRY_NOT_FOUND);
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 3			ref is valid, file has type and preferred app, app type is
					not installed, app exists and has signature
	@results		Should return B_OK and set team to the ID of the team
					running the file's (not the file type's) preferred
					application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestF3()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, appType2));
	SimpleFileCaller3 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile2) == ref);
	check_app_type(appType2, appFile2);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
//	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 4			ref is valid, file has no type, but preferred app,
					app type is not installed, app exists and has signature
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it.
*/
void LaunchTester::LaunchTestF4()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	SimpleFileCaller3 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
//	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 5			ref is valid, file has type and app hint, the type's
					preferred app type is not installed, app exists and has
					signature
	@results		Should return B_OK and set team to the ID of the team
					running the file type's preferred application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestF5()
{
	BRoster roster;
	create_app(appFile1, appType1);
	create_app(appFile2, appType2);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1, NULL, appFile2));
	SimpleFileCaller3 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
//	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 6			ref is valid, file has type, the type's preferred app
					type is not installed, app exists and has signature, file
					has executable permission, but is not executable
	@results		Should return B_LAUNCH_FAILED_EXECUTABLE.
					Should not set the app hint on the app or file type.
*/
void LaunchTester::LaunchTestF6()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	entry_ref fileRef(create_file(testFile1, fileType1));
	system((string("chmod a+x ") + testFile1).c_str());
	team_id team;
	char *argv[] = { "Hey!" };
	CHK(roster.Launch(&fileRef, 1, argv, &team) == B_LAUNCH_FAILED_EXECUTABLE);
	CHK(BMimeType(appType1).IsInstalled() == false);
	CHK(BMimeType(fileType1).GetAppHint(&fileRef) == B_ENTRY_NOT_FOUND);
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 7			ref is valid and refers to a link to a file, file has type,
					the type's preferred app type is not installed,
					app exists and has signature
	@results		Should return B_OK and set team to the ID of the team
					running the file type's preferred application's executable.
					Should install the app type and set the app hint on it.
*/
void LaunchTester::LaunchTestF7()
{
	BRoster roster;
	create_app(appFile1, appType1);
	install_type(fileType1, appType1);
	create_file(testFile1, fileType1);
	system((string("ln -s ") + testFile1 + " " + testLink1).c_str());
	entry_ref fileRef(ref_for_path(testFile1, false));
	entry_ref linkRef(ref_for_path(testLink1, false));
	SimpleFileCaller3 caller(linkRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
//	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckArgsMessage(caller, team, cookie, &ref, &fileRef,
								 LaunchContext::kStandardArgc,
								 LaunchContext::kStandardArgv, MSG_MAIN_ARGS));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckArgsMessage(caller, team, cookie, &ref, &fileRef,
								 LaunchContext::kStandardArgc,
								 LaunchContext::kStandardArgv,
								 MSG_ARGV_RECEIVED));
//	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 8			ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					Launch(const char*, entry_ref*) cases 4-16
					(== common cases 2-14)
*/
void LaunchTester::LaunchTestF8()
{
	FileWithTypeCaller3 caller;
	for (int32 i = 0; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 9			ref is valid, file has no type, sniffing results in a type,
					type is set on file,
					Launch(const char*, entry_ref*) cases 3-16
					(== common cases 1-14)
*/
void LaunchTester::LaunchTestF9()
{
	SniffFileTypeCaller3 caller;
	for (int32 i = 1; i < commonTestFunctionCount; i++) {
		NextSubTest();
		(*commonTestFunctions[i])(caller);
		tearDown();
		setUp();
	}
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 10		ref is valid, file has no type, but preferred app, app
					type is not installed, app exists and has signature,
					NULL args, argc is 0
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it. argv are ignored.
*/
void LaunchTester::LaunchTestF10()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	SimpleFileCaller3 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, NULL, 0, NULL, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref, 0, NULL));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 11		ref is valid, file has no type, but preferred app, app
					type is not installed, app exists and has signature,
					NULL args, argc > 0
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable. Should install the
					app type and set the app hint on it. argv are ignored.
*/
void LaunchTester::LaunchTestF11()
{
	BRoster roster;
	create_app(appFile1, appType1);
	entry_ref fileRef(create_file(testFile1, NULL, appType1));
	SimpleFileCaller3 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, fileType1, NULL, 1, NULL, &team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	check_app_type(appType1, appFile1);

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref, 0, NULL));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckRefsMessage(caller, team, cookie));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 12		ref is valid and refers to a cyclic link
	@results		Should return B_LAUNCH_FAILED_NO_RESOLVE_LINK.
*/
void LaunchTester::LaunchTestF12()
{
	BRoster roster;
	system((string("ln -s ") + testLink1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1, false));
	BMessage message;
	team_id team;
	CHK(roster.Launch(&linkRef, LaunchContext::kStandardArgc,
					  LaunchContext::kStandardArgv, &team)
		== B_LAUNCH_FAILED_NO_RESOLVE_LINK);
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 13		ref is valid and refers to a link to void
	@results		Should return B_LAUNCH_FAILED_NO_RESOLVE_LINK.
*/
void LaunchTester::LaunchTestF13()
{
	BRoster roster;
	system((string("ln -s ") + testFile1 + " " + testLink1).c_str());
	entry_ref linkRef(ref_for_path(testLink1, false));
	BMessage message;
	team_id team;
	CHK(roster.Launch(&linkRef, LaunchContext::kStandardArgc,
					  LaunchContext::kStandardArgv, &team)
		== B_LAUNCH_FAILED_NO_RESOLVE_LINK);
}

/*
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam) const
	@case 14		ref is valid and refers to an executable without signature
	@results		Should return B_OK and set team to the ID of the team
					running the application's executable.
*/
void LaunchTester::LaunchTestF14()
{
	BRoster roster;
	create_app(appFile1, NULL);
	entry_ref fileRef(ref_for_path(appFile1));
	SimpleFileCaller3 caller(fileRef);
	LaunchContext context;
	team_id team;
	CHK(context(caller, appType1, context.StandardMessages(),
				LaunchContext::kStandardArgc, LaunchContext::kStandardArgv,
				&team) == B_OK);
	entry_ref ref = ref_for_team(team);
	CHK(ref_for_path(appFile1) == ref);
	context.WaitForMessage(team, MSG_STARTED, true);
	app_info appInfo;
	CHK(roster.GetRunningAppInfo(team, &appInfo) == B_OK);
	CHK(!strcmp(appInfo.signature, kDefaultTestAppSignature));

	context.Terminate();
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_STARTED));
//	CHK(context.CheckMainArgsMessage(caller, team, cookie, &ref));
	CHK(context.CheckArgsMessage(caller, team, cookie, &ref, NULL,
								 LaunchContext::kStandardArgc,
								 LaunchContext::kStandardArgv,
								 MSG_MAIN_ARGS));
//	CHK(context.CheckMessageMessages(caller, team, cookie));
//	CHK(context.CheckArgvMessage(caller, team, cookie, &ref));
	CHK(context.CheckArgsMessage(caller, team, cookie, &ref, NULL,
								 LaunchContext::kStandardArgc,
								 LaunchContext::kStandardArgv,
								 MSG_ARGV_RECEIVED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller, team, cookie, MSG_TERMINATED));
}


Test* LaunchTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestA2);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestA3);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestA4);

	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB1);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB2);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB3);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB4);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestB5);

	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestC1);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestC2);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestC3);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestC4);

	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD1);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD2);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD3);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD4);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD5);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD6);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD7);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD8);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD9);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD10);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD11);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD12);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestD13);

	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE1);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE2);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE3);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE4);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE5);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE6);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE7);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE8);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE9);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE10);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE11);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE12);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE13);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestE14);

	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF1);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF2);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF3);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF4);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF5);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF6);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF7);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF8);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF9);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF10);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF11);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF12);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF13);
	ADD_TEST4(BRoster, SuiteOfTests, LaunchTester, LaunchTestF14);

	return SuiteOfTests;
}


//------------------------------------------------------------------------------
//	RosterWatchingTester.cpp
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
#include "RosterWatchingTester.h"
#include "LaunchTesterHelper.h"
#include "RosterTestAppDefs.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

static const char *testerSignature
	= "application/x-vnd.obos-roster-watching-test";
static const char *appType1	= "application/x-vnd.obos-roster-watching-app1";
static const char *appType2	= "application/x-vnd.obos-roster-watching-app2";
static const char *appType3	= "application/x-vnd.obos-roster-watching-app3";
static const char *appType4	= "application/x-vnd.obos-roster-watching-app4";
//static const char *appType5	= "application/x-vnd.obos-roster-watching-app5";

static const char *testDir		= "/tmp/testdir";
static const char *appFile1		= "/tmp/testdir/app1";
static const char *appFile2		= "/tmp/testdir/app2";
static const char *appFile3		= "/tmp/testdir/app3";
static const char *appFile4		= "/tmp/testdir/app4";
//static const char *appFile5		= "/tmp/testdir/app5";


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
entry_ref
create_app(const char *filename, const char *signature,
		   bool install = false, bool makeExecutable = true,
		   uint32 appFlags = B_SINGLE_LAUNCH)
{
	BString testApp;
	CHK(find_test_app("RosterWatchingTestApp1", &testApp) == B_OK);
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
	return ref_for_path(filename);
}

// app_info_for_team
static
app_info
app_info_for_team(team_id team)
{
	app_info info;
	CHK(be_roster->GetRunningAppInfo(team, &info) == B_OK);
	return info;
}

// check_watching_message
void
check_watching_message(LaunchContext &context, team_id team, int32 &cookie,
					   const app_info &info, uint32 messageCode)
{
	// wait for and get the message
	CHK(context.WaitForMessage(team, MSG_MESSAGE_RECEIVED, false,
							   B_INFINITE_TIMEOUT, cookie));
	BMessage *container = context.NextMessageFrom(team, cookie);
	CHK(container != NULL);
	CHK(container->what == MSG_MESSAGE_RECEIVED);
	BMessage message;
	CHK(container->FindMessage("message", &message) == B_OK);
	// check the message
if (message.what != messageCode)
printf("message.what: %.4s vs messageCode: %.4s\n", (char*)&message.what,
(char*)&messageCode);
	CHK(message.what == messageCode);
	// team
	team_id foundTeam;
	CHK(message.FindInt32("be:team", &foundTeam) == B_OK);
	CHK(foundTeam == info.team);
	// thread
	thread_id thread;
	CHK(message.FindInt32("be:thread", &thread) == B_OK);
	CHK(thread == info.thread);
	// signature
	const char *signature = NULL;
	CHK(message.FindString("be:signature", &signature) == B_OK);
	CHK(!strcmp(signature, info.signature));
	// ref
	entry_ref ref;	
	CHK(message.FindRef("be:ref", &ref) == B_OK);
	CHK(ref == info.ref);
	// flags
	uint32 flags;
	CHK(message.FindInt32("be:flags", (int32*)&flags) == B_OK);
	CHK(flags == info.flags);
}


// setUp
void
RosterWatchingTester::setUp()
{
	RosterLaunchApp *app = new RosterLaunchApp(testerSignature);
	fApplication = app;
	app->SetHandler(new RosterBroadcastHandler);
	system((string("mkdir ") + testDir).c_str());
}

// tearDown
void
RosterWatchingTester::tearDown()
{
	BMimeType(appType1).Delete();
	BMimeType(appType2).Delete();
	BMimeType(appType3).Delete();
	BMimeType(appType4).Delete();
//	BMimeType(appType5).Delete();
	delete fApplication;
	system((string("rm -rf ") + testDir).c_str());
}

// SimpleAppLauncher
class SimpleAppLauncher : public LaunchCaller {
public:
	SimpleAppLauncher() : fRef() {}
	SimpleAppLauncher(const entry_ref &ref) : fRef(ref) {}
	virtual ~SimpleAppLauncher() {}
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team)
	{
		return be_roster->Launch(&fRef, (BMessage*)NULL, team);
	}
	virtual bool SupportsRefs() const { return true; }
	virtual const entry_ref *Ref() const { return &fRef; }

	virtual LaunchCaller *CloneInternal()
	{
		return new SimpleAppLauncher;
	}

protected:
	entry_ref fRef;
};


/*
	status_t StartWatching(BMessenger target, uint32 eventMask) const
	status_t StopWatching(BMessenger target) const
	@case 1			{Start,Stop}Watching() with invalid messenger or invalid
					flags; StopWatching() non-watching messenger =>
	@results		Should return B_OK; B_BAD_VALUE.
*/
void RosterWatchingTester::WatchingTest1()
{
	BRoster roster;
	BMessenger target;
	// not valid, not watching
	CHK(roster.StopWatching(target) == B_BAD_VALUE);
	// not valid
	CHK(roster.StartWatching(target, B_REQUEST_LAUNCHED | B_REQUEST_QUIT
							 | B_REQUEST_ACTIVATED) == B_OK);
	CHK(roster.StopWatching(target) == B_OK);
	// invalid flags
	CHK(roster.StartWatching(target, 0) == B_OK);
	CHK(roster.StopWatching(target) == B_OK);
	// valid, but not watching
	CHK(roster.StopWatching(be_app_messenger) == B_BAD_VALUE);
}

/*
	status_t StartWatching(BMessenger target, uint32 eventMask) const
	status_t StopWatching(BMessenger target) const
	@case 2			several apps, several watchers, different eventMasks
	@results		Should return B_OK...
					Watching ends, when target has become invalid and the next watching
					message is tried to be sent.
*/
void RosterWatchingTester::WatchingTest2()
{
	LaunchContext context;
	BRoster roster;
	// launch app 1
	entry_ref ref1(create_app(appFile1, appType1));
	SimpleAppLauncher caller1(ref1);
	team_id team1;
	CHK(context(caller1, appType1, &team1) == B_OK);
	context.WaitForMessage(team1, MSG_READY_TO_RUN);
	BMessenger target1(NULL, team1);
	app_info appInfo1(app_info_for_team(team1));
	CHK(roster.StartWatching(target1, B_REQUEST_LAUNCHED | B_REQUEST_QUIT
							 | B_REQUEST_ACTIVATED) == B_OK);
	//   messages: app 1
	int32 cookie1 = 0;
	CHK(context.CheckNextMessage(caller1, team1, cookie1, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller1, team1, cookie1, &ref1, false));
	CHK(context.CheckNextMessage(caller1, team1, cookie1, MSG_READY_TO_RUN));
	// launch app 2
	entry_ref ref2(create_app(appFile2, appType2, false, true,
							  B_SINGLE_LAUNCH | B_ARGV_ONLY));
	SimpleAppLauncher caller2(ref2);
	team_id team2;
	CHK(context(caller2, appType2, &team2) == B_OK);
	context.WaitForMessage(team2, MSG_READY_TO_RUN);
	BMessenger target2(context.AppMessengerFor(team2));
	CHK(target2.IsValid());
	app_info appInfo2(app_info_for_team(team2));
	CHK(roster.StartWatching(target2, B_REQUEST_LAUNCHED) == B_OK);
	//   messages: app 2
	int32 cookie2 = 0;
	CHK(context.CheckNextMessage(caller2, team2, cookie2, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller2, team2, cookie2, &ref2, false));
	CHK(context.CheckNextMessage(caller2, team2, cookie2, MSG_READY_TO_RUN));
	//   messages: app 1
	check_watching_message(context, team1, cookie1, appInfo2,
						   B_SOME_APP_LAUNCHED);
	// launch app 3
	entry_ref ref3(create_app(appFile3, appType3));
	SimpleAppLauncher caller3(ref3);
	team_id team3;
	CHK(context(caller3, appType3, &team3) == B_OK);
	context.WaitForMessage(team3, MSG_READY_TO_RUN);
	BMessenger target3(NULL, team3);
	app_info appInfo3(app_info_for_team(team3));
	CHK(roster.StartWatching(target3, B_REQUEST_QUIT) == B_OK);
	//   messages: app 3
	int32 cookie3 = 0;
	CHK(context.CheckNextMessage(caller3, team3, cookie3, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller3, team3, cookie3, &ref3, false));
	CHK(context.CheckNextMessage(caller3, team3, cookie3, MSG_READY_TO_RUN));
	//   messages: app 2
	check_watching_message(context, team2, cookie2, appInfo3,
						   B_SOME_APP_LAUNCHED);
	//   messages: app 1
	check_watching_message(context, team1, cookie1, appInfo3,
						   B_SOME_APP_LAUNCHED);
	// launch app 4
	entry_ref ref4(create_app(appFile4, appType4));
	SimpleAppLauncher caller4(ref4);
	team_id team4;
	CHK(context(caller4, appType4, &team4) == B_OK);
	context.WaitForMessage(team4, MSG_READY_TO_RUN);
	BMessenger target4(NULL, team4);
	app_info appInfo4(app_info_for_team(team4));
	//   messages: app 4
	int32 cookie4 = 0;
	CHK(context.CheckNextMessage(caller4, team4, cookie4, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller4, team4, cookie4, &ref4, false));
	CHK(context.CheckNextMessage(caller4, team4, cookie4, MSG_READY_TO_RUN));
	//   messages: app 3
	//   none
	//   messages: app 2
	check_watching_message(context, team2, cookie2, appInfo4,
						   B_SOME_APP_LAUNCHED);
	//   messages: app 1
	check_watching_message(context, team1, cookie1, appInfo4,
						   B_SOME_APP_LAUNCHED);
	// terminate app 4
	context.TerminateApp(team4);
	//   messages: app 3
	check_watching_message(context, team3, cookie3, appInfo4,
						   B_SOME_APP_QUIT);
	//   messages: app 2
	//   none
	//   messages: app 1
	check_watching_message(context, team1, cookie1, appInfo4,
						   B_SOME_APP_QUIT);
	// stop watching app 1
	CHK(roster.StopWatching(target1) == B_OK);
	// terminate app 2
	context.TerminateApp(team2);
	CHK(roster.StopWatching(target2) == B_OK);
	//   messages: app 3
	check_watching_message(context, team3, cookie3, appInfo2,
						   B_SOME_APP_QUIT);
	//   messages: app 1
	//   none
	// terminate app 3
	context.TerminateApp(team3);
// Haiku handles app termination a bit different. At the point, when the
// application unregisters itself from the registrar, its port is still
// valid.
#ifdef TEST_R5
	CHK(roster.StopWatching(target3) == B_BAD_VALUE);
#else
	CHK(roster.StopWatching(target3) == B_OK);
#endif
	//   messages: app 1
	//   none
	// remaining messages
	context.Terminate();
	// app 1
	CHK(context.CheckNextMessage(caller1, team1, cookie1, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller1, team1, cookie1, MSG_TERMINATED));
	// app 2
	CHK(context.CheckNextMessage(caller2, team2, cookie2, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller2, team2, cookie2, MSG_TERMINATED));
	// app 3
	CHK(context.CheckNextMessage(caller3, team3, cookie3, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller3, team3, cookie3, MSG_TERMINATED));
	// app 4
	CHK(context.CheckNextMessage(caller4, team4, cookie4, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller4, team4, cookie4, MSG_TERMINATED));
}

/*
	status_t StartWatching(BMessenger target, uint32 eventMask) const
	status_t StopWatching(BMessenger target) const
	@case 3			call StartWatching() twice, second time with different
					masks
	@results		Should return B_OK. The second call simply overrides the
					first one.
*/
void RosterWatchingTester::WatchingTest3()
{
	LaunchContext context;
	BRoster roster;
	// launch app 1
	entry_ref ref1(create_app(appFile1, appType1));
	SimpleAppLauncher caller1(ref1);
	team_id team1;
	CHK(context(caller1, appType1, &team1) == B_OK);
	context.WaitForMessage(team1, MSG_READY_TO_RUN);
	BMessenger target1(NULL, team1);
	app_info appInfo1(app_info_for_team(team1));
	CHK(roster.StartWatching(target1, B_REQUEST_LAUNCHED | B_REQUEST_QUIT
							 | B_REQUEST_ACTIVATED) == B_OK);
	//   messages: app 1
	int32 cookie1 = 0;
	CHK(context.CheckNextMessage(caller1, team1, cookie1, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller1, team1, cookie1, &ref1, false));
	CHK(context.CheckNextMessage(caller1, team1, cookie1, MSG_READY_TO_RUN));
	// app 1: another StartWatching() with different event mask
	CHK(roster.StartWatching(target1, B_REQUEST_QUIT) == B_OK);
	// launch app 2
	entry_ref ref2(create_app(appFile2, appType2, false, true,
							  B_SINGLE_LAUNCH | B_ARGV_ONLY));
	SimpleAppLauncher caller2(ref2);
	team_id team2;
	CHK(context(caller2, appType2, &team2) == B_OK);
	context.WaitForMessage(team2, MSG_READY_TO_RUN);
	BMessenger target2(context.AppMessengerFor(team2));
	CHK(target2.IsValid());
	app_info appInfo2(app_info_for_team(team2));
	CHK(roster.StartWatching(target2, B_REQUEST_QUIT) == B_OK);
	//   messages: app 2
	int32 cookie2 = 0;
	CHK(context.CheckNextMessage(caller2, team2, cookie2, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller2, team2, cookie2, &ref2, false));
	CHK(context.CheckNextMessage(caller2, team2, cookie2, MSG_READY_TO_RUN));
	//   messages: app 1
	// none
	// app 2: another StartWatching() with different event mask
	CHK(roster.StartWatching(target2, B_REQUEST_LAUNCHED) == B_OK);
	// launch app 3
	entry_ref ref3(create_app(appFile3, appType3));
	SimpleAppLauncher caller3(ref3);
	team_id team3;
	CHK(context(caller3, appType3, &team3) == B_OK);
	context.WaitForMessage(team3, MSG_READY_TO_RUN);
	BMessenger target3(NULL, team3);
	app_info appInfo3(app_info_for_team(team3));
	//   messages: app 3
	int32 cookie3 = 0;
	CHK(context.CheckNextMessage(caller3, team3, cookie3, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller3, team3, cookie3, &ref3, false));
	CHK(context.CheckNextMessage(caller3, team3, cookie3, MSG_READY_TO_RUN));
	//   messages: app 2
	check_watching_message(context, team2, cookie2, appInfo3,
						   B_SOME_APP_LAUNCHED);
	//   messages: app 1
	//   none
	// terminate app 3
	context.TerminateApp(team3);
	//   messages: app 3
	//   none
	//   messages: app 2
	//   none
	//   messages: app 1
	check_watching_message(context, team1, cookie1, appInfo3,
						   B_SOME_APP_QUIT);
	// terminate app 2
	context.TerminateApp(team2);
	CHK(roster.StopWatching(target2) == B_OK);
	//   messages: app 1
	check_watching_message(context, team1, cookie1, appInfo2,
						   B_SOME_APP_QUIT);
	// remaining messages
	context.Terminate();
	// app 1
	CHK(context.CheckNextMessage(caller1, team1, cookie1, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller1, team1, cookie1, MSG_TERMINATED));
	// app 2
	CHK(context.CheckNextMessage(caller2, team2, cookie2, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller2, team2, cookie2, MSG_TERMINATED));
	// app 3
	CHK(context.CheckNextMessage(caller3, team3, cookie3, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller3, team3, cookie3, MSG_TERMINATED));
}


Test* RosterWatchingTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, RosterWatchingTester, WatchingTest1);
	ADD_TEST4(BRoster, SuiteOfTests, RosterWatchingTester, WatchingTest2);
	ADD_TEST4(BRoster, SuiteOfTests, RosterWatchingTester, WatchingTest3);

	return SuiteOfTests;
}


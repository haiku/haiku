//------------------------------------------------------------------------------
//	BroadcastTester.cpp
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
#include "BroadcastTester.h"
#include "LaunchTesterHelper.h"
#include "RosterTestAppDefs.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

static const char *testerSignature
	= "application/x-vnd.obos-roster-broadcast-test";
static const char *appType1	= "application/x-vnd.obos-roster-broadcast-app1";
static const char *appType2	= "application/x-vnd.obos-roster-broadcast-app2";
static const char *appType3	= "application/x-vnd.obos-roster-broadcast-app3";
static const char *appType4	= "application/x-vnd.obos-roster-broadcast-app4";

static const char *testDir		= "/tmp/testdir";
static const char *appFile1		= "/tmp/testdir/app1";
static const char *appFile2		= "/tmp/testdir/app2";
static const char *appFile3		= "/tmp/testdir/app3";
static const char *appFile4		= "/tmp/testdir/app4";


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
	CHK(find_test_app("RosterBroadcastTestApp1", &testApp) == B_OK);
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


// setUp
void
BroadcastTester::setUp()
{
	RosterLaunchApp *app = new RosterLaunchApp(testerSignature);
	fApplication = app;
	app->SetHandler(new RosterBroadcastHandler);
	system((string("mkdir ") + testDir).c_str());
}

// tearDown
void
BroadcastTester::tearDown()
{
	BMimeType(appType1).Delete();
	BMimeType(appType2).Delete();
	BMimeType(appType3).Delete();
	BMimeType(appType4).Delete();
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
	status_t Broadcast(BMessage *message) const
	@case 1			NULL message
	@results		Should return B_BAD_VALUE.
*/
void BroadcastTester::BroadcastTestA1()
{
// R5: crashes when passing a NULL message.
#ifndef TEST_R5
	CHK(be_roster->Broadcast(NULL) == B_BAD_VALUE);
#endif
}

/*
	status_t Broadcast(BMessage *message) const
	@case 2			valid message, several apps, one is B_ARGV_ONLY
	@results		Should return B_OK and send the message to all (including
					the B_ARGV_ONLY) apps. Replies go to be_app_messenger.
*/
void BroadcastTester::BroadcastTestA2()
{
	LaunchContext context;
	BRoster roster;
	// launch app 1
	entry_ref ref1(create_app(appFile1, appType1));
	SimpleAppLauncher caller1(ref1);
	team_id team1;
	CHK(context(caller1, appType1, &team1) == B_OK);
	// launch app 2
	entry_ref ref2(create_app(appFile2, appType2, false, true,
							  B_SINGLE_LAUNCH | B_ARGV_ONLY));
	SimpleAppLauncher caller2(ref2);
	team_id team2;
	CHK(context(caller2, appType2, &team2) == B_OK);
	// launch app 3
	entry_ref ref3(create_app(appFile3, appType3));
	SimpleAppLauncher caller3(ref3);
	team_id team3;
	CHK(context(caller3, appType3, &team3) == B_OK);
	// launch app 4
	entry_ref ref4(create_app(appFile4, appType4));
	SimpleAppLauncher caller4(ref4);
	team_id team4;
	CHK(context(caller4, appType4, &team4) == B_OK);
	// wait for the apps to run
	context.WaitForMessage(team1, MSG_READY_TO_RUN);
	context.WaitForMessage(team2, MSG_READY_TO_RUN);
	context.WaitForMessage(team3, MSG_READY_TO_RUN);
	context.WaitForMessage(team4, MSG_READY_TO_RUN);
	// broadcast a message
	BMessage message(MSG_1);
	CHK(roster.Broadcast(&message) == B_OK);
	// wait for the apps to report the receipt of the message
	context.WaitForMessage(team1, MSG_MESSAGE_RECEIVED);
	context.WaitForMessage(team2, MSG_MESSAGE_RECEIVED);
	context.WaitForMessage(team3, MSG_MESSAGE_RECEIVED);
	context.WaitForMessage(team4, MSG_MESSAGE_RECEIVED);
	// check the messages
	context.Terminate();
	// app 1
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller1, team1, cookie, &ref1, false));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller1, team1, cookie, &message));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_2));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_TERMINATED));
	// app 2
	cookie = 0;
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller2, team2, cookie, &ref2, false));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller2, team2, cookie, &message));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_2));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_TERMINATED));
	// app 3
	cookie = 0;
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller3, team3, cookie, &ref3, false));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller3, team3, cookie, &message));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_2));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_TERMINATED));
	// app 4
	cookie = 0;
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller4, team4, cookie, &ref4, false));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller4, team4, cookie, &message));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_2));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_TERMINATED));
}

/*
	status_t Broadcast(BMessage *message, BMessenger replyTo) const
	@case 1			NULL message
	@results		Should return B_BAD_VALUE.
*/
void BroadcastTester::BroadcastTestB1()
{
// R5: crashes when passing a NULL message.
#ifndef TEST_R5
	BMessenger replyTo(dynamic_cast<RosterLaunchApp*>(be_app)->Handler());
	CHK(be_roster->Broadcast(NULL, replyTo) == B_BAD_VALUE);
#endif
}

/*
	status_t Broadcast(BMessage *message, BMessenger replyTo) const
	@case 2			valid message, several apps, one is B_ARGV_ONLY
	@results		Should return B_OK and send the message to all (including
					the B_ARGV_ONLY) apps. Replies go to the specified
					messenger.
*/
void BroadcastTester::BroadcastTestB2()
{
	LaunchContext context;
	BRoster roster;
	// launch app 1
	entry_ref ref1(create_app(appFile1, appType1));
	SimpleAppLauncher caller1(ref1);
	team_id team1;
	CHK(context(caller1, appType1, &team1) == B_OK);
	// launch app 2
	entry_ref ref2(create_app(appFile2, appType2, false, true,
							  B_SINGLE_LAUNCH | B_ARGV_ONLY));
	SimpleAppLauncher caller2(ref2);
	team_id team2;
	CHK(context(caller2, appType2, &team2) == B_OK);
	// launch app 3
	entry_ref ref3(create_app(appFile3, appType3));
	SimpleAppLauncher caller3(ref3);
	team_id team3;
	CHK(context(caller3, appType3, &team3) == B_OK);
	// launch app 4
	entry_ref ref4(create_app(appFile4, appType4));
	SimpleAppLauncher caller4(ref4);
	team_id team4;
	CHK(context(caller4, appType4, &team4) == B_OK);
	// wait for the apps to run
	context.WaitForMessage(team1, MSG_READY_TO_RUN);
	context.WaitForMessage(team2, MSG_READY_TO_RUN);
	context.WaitForMessage(team3, MSG_READY_TO_RUN);
	context.WaitForMessage(team4, MSG_READY_TO_RUN);
	// broadcast a message
	BMessage message(MSG_1);
	BMessenger replyTo(dynamic_cast<RosterLaunchApp*>(be_app)->Handler());
	CHK(roster.Broadcast(&message, replyTo) == B_OK);
	// wait for the apps to report the receipt of the message
	context.WaitForMessage(team1, MSG_MESSAGE_RECEIVED);
	context.WaitForMessage(team2, MSG_MESSAGE_RECEIVED);
	context.WaitForMessage(team3, MSG_MESSAGE_RECEIVED);
	context.WaitForMessage(team4, MSG_MESSAGE_RECEIVED);
	// check the messages
	context.Terminate();
	// app 1
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller1, team1, cookie, &ref1, false));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller1, team1, cookie, &message));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_REPLY));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_TERMINATED));
	// app 2
	cookie = 0;
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller2, team2, cookie, &ref2, false));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller2, team2, cookie, &message));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_REPLY));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_TERMINATED));
	// app 3
	cookie = 0;
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller3, team3, cookie, &ref3, false));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller3, team3, cookie, &message));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_REPLY));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_TERMINATED));
	// app 4
	cookie = 0;
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller4, team4, cookie, &ref4, false));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller4, team4, cookie, &message));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_REPLY));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_TERMINATED));
}

/*
	status_t Broadcast(BMessage *message, BMessenger replyTo) const
	@case 3			valid message, several apps, one is B_ARGV_ONLY,
					invalid replyTo
	@results		Should return B_OK and send the message to all (including
					the B_ARGV_ONLY) apps. Replies go to the roster!
*/
void BroadcastTester::BroadcastTestB3()
{
	LaunchContext context;
	BRoster roster;
	// launch app 1
	entry_ref ref1(create_app(appFile1, appType1));
	SimpleAppLauncher caller1(ref1);
	team_id team1;
	CHK(context(caller1, appType1, &team1) == B_OK);
	// launch app 2
	entry_ref ref2(create_app(appFile2, appType2, false, true,
							  B_SINGLE_LAUNCH | B_ARGV_ONLY));
	SimpleAppLauncher caller2(ref2);
	team_id team2;
	CHK(context(caller2, appType2, &team2) == B_OK);
	// launch app 3
	entry_ref ref3(create_app(appFile3, appType3));
	SimpleAppLauncher caller3(ref3);
	team_id team3;
	CHK(context(caller3, appType3, &team3) == B_OK);
	// launch app 4
	entry_ref ref4(create_app(appFile4, appType4));
	SimpleAppLauncher caller4(ref4);
	team_id team4;
	CHK(context(caller4, appType4, &team4) == B_OK);
	// wait for the apps to run
	context.WaitForMessage(team1, MSG_READY_TO_RUN);
	context.WaitForMessage(team2, MSG_READY_TO_RUN);
	context.WaitForMessage(team3, MSG_READY_TO_RUN);
	context.WaitForMessage(team4, MSG_READY_TO_RUN);
	// broadcast a message
	BMessage message(MSG_1);
	BMessenger replyTo;
	CHK(roster.Broadcast(&message, replyTo) == B_OK);
	// wait for the apps to report the receipt of the message
	context.WaitForMessage(team1, MSG_MESSAGE_RECEIVED);
	context.WaitForMessage(team2, MSG_MESSAGE_RECEIVED);
	context.WaitForMessage(team3, MSG_MESSAGE_RECEIVED);
	context.WaitForMessage(team4, MSG_MESSAGE_RECEIVED);
	// check the messages
	context.Terminate();
	// app 1
	int32 cookie = 0;
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller1, team1, cookie, &ref1, false));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller1, team1, cookie, &message));
//	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_2));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller1, team1, cookie, MSG_TERMINATED));
	// app 2
	cookie = 0;
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller2, team2, cookie, &ref2, false));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller2, team2, cookie, &message));
//	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_2));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller2, team2, cookie, MSG_TERMINATED));
	// app 3
	cookie = 0;
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller3, team3, cookie, &ref3, false));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller3, team3, cookie, &message));
//	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_2));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller3, team3, cookie, MSG_TERMINATED));
	// app 4
	cookie = 0;
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_STARTED));
	CHK(context.CheckMainArgsMessage(caller4, team4, cookie, &ref4, false));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_READY_TO_RUN));
	CHK(context.CheckMessageMessage(caller4, team4, cookie, &message));
//	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_2));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_QUIT_REQUESTED));
	CHK(context.CheckNextMessage(caller4, team4, cookie, MSG_TERMINATED));
}


Test* BroadcastTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, BroadcastTester, BroadcastTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, BroadcastTester, BroadcastTestA2);

	ADD_TEST4(BRoster, SuiteOfTests, BroadcastTester, BroadcastTestB1);
	ADD_TEST4(BRoster, SuiteOfTests, BroadcastTester, BroadcastTestB2);
	ADD_TEST4(BRoster, SuiteOfTests, BroadcastTester, BroadcastTestB3);

	return SuiteOfTests;
}


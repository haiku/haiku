//------------------------------------------------------------------------------
//	AppRunTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <OS.h>
#include <Application.h>
#include <Handler.h>
#include <Looper.h>
#include <String.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "AppRunner.h"
#include "AppRunTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// check_output
static
void
check_output(AppRunner &runner, const char *expectedOutput)
{
	BString buffer;
	CHK(runner.GetOutput(&buffer) == B_OK);
if (buffer != expectedOutput)
printf("result is `%s', but should be `%s'\n", buffer.String(),
expectedOutput);
	CHK(buffer == expectedOutput);
}

/*
	thread_id Run()
	@case 1			launch the app two times: B_MULTIPLE_LAUNCH | B_ARGV_ONLY,
					no command line args, send B_QUIT_REQUESTED to both of
					them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	ReadyToRun(), QuitRequested()
*/
void AppRunTester::RunTest1()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 = output1;
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp1") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 2			launch the app two times: B_MULTIPLE_LAUNCH | B_ARGV_ONLY,
					command line args, send B_QUIT_REQUESTED to both of
					them
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	ArgvReceived(), ReadyToRun(), QuitRequested()
*/
void AppRunTester::RunTest2()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: c d e\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp1", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp1", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 3			launch the app two times: B_MULTIPLE_LAUNCH,
					no command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	ReadyToRun(), QuitRequested()
*/
void AppRunTester::RunTest3()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 = output1;
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp2") == B_OK);
	CHK(runner2.Run("AppRunTestApp2") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 4			launch the app two times: B_MULTIPLE_LAUNCH,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	ArgvReceived(), ReadyToRun(), QuitRequested()
*/
void AppRunTester::RunTest4()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: c d e\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp2", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp2", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 5			launch the app two times: B_SINGLE_LAUNCH | B_ARGV_ONLY,
					no command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest5()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp3") == B_OK);
	CHK(runner2.Run("AppRunTestApp3") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 6			launch the app two times: B_SINGLE_LAUNCH | B_ARGV_ONLY,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest6()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp3", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp3", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 7			launch the app two times: B_SINGLE_LAUNCH,
					no command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest7()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp4") == B_OK);
	CHK(runner2.Run("AppRunTestApp4") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 8			launch the app two times: B_SINGLE_LAUNCH,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), ArgvReceived(),
					QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest8()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::ArgvReceived()\n"
		"args: c d e\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp4", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp4", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 9			launch two apps with the same signature:
					B_SINGLE_LAUNCH | B_ARGV_ONLY,
					no command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	ReadyToRun(), QuitRequested()
*/
void AppRunTester::RunTest9()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 = output1;
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp3") == B_OK);
	CHK(runner2.Run("AppRunTestApp3a") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 10		launch two apps with the same signature:
					B_SINGLE_LAUNCH | B_ARGV_ONLY,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	ArgvReceived(), ReadyToRun(), QuitRequested()
*/
void AppRunTester::RunTest10()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: c d e\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp3", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp3a", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 11		launch two apps with the same signature:
					B_SINGLE_LAUNCH,
					no command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	ReadyToRun(), QuitRequested()
*/
void AppRunTester::RunTest11()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 = output1;
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp4") == B_OK);
	CHK(runner2.Run("AppRunTestApp4a") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 12		launch two apps with the same signature:
					B_SINGLE_LAUNCH,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	ArgvReceived(), ReadyToRun(), QuitRequested()
*/
void AppRunTester::RunTest12()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: c d e\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp4", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp4a", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 13		launch the app two times: B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY,
					no command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest13()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp5") == B_OK);
	CHK(runner2.Run("AppRunTestApp5") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 14		launch the app two times: B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest14()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp5", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp5", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 15		launch the app two times: B_EXCLUSIVE_LAUNCH,
					no command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest15()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp6") == B_OK);
	CHK(runner2.Run("AppRunTestApp6") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 16		launch the app two times: B_EXCLUSIVE_LAUNCH,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), ArgvReceived(),
					QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest16()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::ArgvReceived()\n"
		"args: c d e\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp6", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp6", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 17		launch two apps with the same signature:
					B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY,
					no command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest17()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp5") == B_OK);
	CHK(runner2.Run("AppRunTestApp5a") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 18		launch two apps with the same signature:
					B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest18()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp5", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp5a", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 19		launch two apps with the same signature: B_EXCLUSIVE_LAUNCH,
					no command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest19()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp6") == B_OK);
	CHK(runner2.Run("AppRunTestApp6a") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 20		launch two apps with the same signature: B_EXCLUSIVE_LAUNCH,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), ArgvReceived(),
					QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest20()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::ArgvReceived()\n"
		"args: c d e\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp6", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp6a", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 21		launch the app two times: first: B_EXCLUSIVE_LAUNCH,
					second: B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), ArgvReceived(),
					QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest21()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::ArgvReceived()\n"
		"args: c d e\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp6", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp5", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}

/*
	thread_id Run()
	@case 22		launch the app two times:
					first: B_EXCLUSIVE_LAUNCH | B_ARGV_ONLY,
					second: B_EXCLUSIVE_LAUNCH,
					command line args, send B_QUIT_REQUESTED to both of them
	@results		first app:	ArgvReceived(), ReadyToRun(), QuitRequested()
					second app:	quits
*/
void AppRunTester::RunTest22()
{
	BApplication app("application/x-vnd.obos-app-run-test");
	const char *output1 =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ArgvReceived()\n"
		"args: a b\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n";
	const char *output2 =
		"error: 80002004\n"
		"InitCheck(): 80002004\n";
	// run the apps
	AppRunner runner1, runner2;
	CHK(runner1.Run("AppRunTestApp5", "a b") == B_OK);
	CHK(runner2.Run("AppRunTestApp6", "c d e") == B_OK);
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	// get the outputs and compare the results
	check_output(runner1, output1);
	check_output(runner2, output2);
}


Test* AppRunTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest1);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest2);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest3);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest4);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest5);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest6);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest7);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest8);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest9);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest10);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest11);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest12);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest13);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest14);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest15);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest16);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest17);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest18);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest19);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest20);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest21);
	ADD_TEST4(BApplication, SuiteOfTests, AppRunTester, RunTest22);

	return SuiteOfTests;
}




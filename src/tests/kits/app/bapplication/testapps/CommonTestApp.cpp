// CommonTestApp.cpp

#include <stdio.h>

#include <OS.h>

#include "CommonTestApp.h"

// constructor
CommonTestApp::CommonTestApp(const char *signature)
			 : BApplication(signature),
			   fQuitOnSecondTry(false),
			   fQuitter(-1),
			   fQuittingDelay(0),
			   fQuittingTries(1)
{
}

// constructor
CommonTestApp::CommonTestApp(const char *signature, status_t *result)
			 : BApplication(signature, result)
{
}

// destructor
CommonTestApp::~CommonTestApp()
{
	if (fQuitter >= 0) {
		int32 result;
		wait_for_thread(fQuitter, &result);
	}
}

// ArgvReceived
void
CommonTestApp::ArgvReceived(int32 argc, char **argv)
{
	report("BApplication::ArgvReceived()\n");
	if (argc > 1) {
		report("args:");
		for (int32 i = 1; i < argc; i++)
			report(" %s", argv[i]);
		report("\n");
	}
}

// QuitRequested
bool
CommonTestApp::QuitRequested()
{
	report("BApplication::QuitRequested()\n");
	static bool firstTry = true;
	if (firstTry && fQuitOnSecondTry) {
		firstTry = false;
		return false;
	}
	return BApplication::QuitRequested();
}

// ReadyToRun
void
CommonTestApp::ReadyToRun()
{
	report("BApplication::ReadyToRun()\n");
}

// Run
thread_id
CommonTestApp::Run()
{
	thread_id result = BApplication::Run();
	report("BApplication::Run() done: %d\n", (result == find_thread(NULL)));
	return result;
}

// SetQuittingPolicy
void
CommonTestApp::SetQuittingPolicy(bool onSecondTry)
{
	fQuitOnSecondTry = onSecondTry;
}

// RunQuitterThread
status_t
CommonTestApp::RunQuitterThread(bigtime_t delay, int32 tries)
{
	status_t error = B_OK;
	fQuittingDelay = delay;
	fQuittingTries = tries;
	// spawn the thread
	fQuitter = spawn_thread(&_QuitterEntry, "quitter thread",
							B_NORMAL_PRIORITY, this);
	if (fQuitter < 0)
		error = fQuitter;
	if (error == B_OK)
		error = resume_thread(fQuitter);
	// cleanup on error
	if (error != B_OK && fQuitter >= 0) {
		kill_thread(fQuitter);
		fQuitter = -1;
	}
	return error;
}

// _QuitterEntry
int32
CommonTestApp::_QuitterEntry(void *data)
{
	int32 result = 0;
	if (CommonTestApp *app = (CommonTestApp*)data)
		result = app->_QuitterLoop();
	return result;
}

// _QuitterLoop
int32
CommonTestApp::_QuitterLoop()
{
	for (; fQuittingTries > 0; fQuittingTries--) {
		snooze(fQuittingDelay);
		PostMessage(B_QUIT_REQUESTED, this);
	}
	return 0;
}

static const char *kAppRunnerTeamPort = "app runner team port";

static port_id outputPort = -1;

// init_connection
status_t
init_connection()
{
	status_t error = B_OK;
	// create a port
	outputPort = create_port(10, "common test app port");
	if (outputPort < 0)
		error = outputPort;
	// find the remote port
	port_id port = -1;
	if (error == B_OK) {
		port = find_port(kAppRunnerTeamPort);
		if (port < 0)
			error = port;
	}
	// send the port ID
	if (error == B_OK) {
		ssize_t written = write_port(port, outputPort, &be_app_messenger,
									 sizeof(BMessenger));
		if (written < 0)
			error = written;
	}
	return error;
}

// report
void
report(const char *format,...)
{
	va_list args;
	va_start(args, format);
	vreport(format, args);
	va_end(args);
}

// vreport
void
vreport(const char *format, va_list args)
{
	char buffer[10240];
	vsprintf(buffer, format, args);
	int32 length = strlen(buffer);
	write_port(outputPort, 0, buffer, length);
}


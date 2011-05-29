// CommonTestApp.cpp

#include <stdio.h>
#include <cstring>

#include <OS.h>

#include "CommonTestApp.h"

// constructor
CommonTestApp::CommonTestApp(const char *signature)
			 : BApplication(signature),
			   fQuitOnSecondTry(false),
			   fEventThread(-1),
			   fEventDelay(0),
			   fEventCount(0),
			   fEventHandler(NULL),
			   fMessageHandler(NULL),
			   fReportDestruction(false)
{
}

// constructor
CommonTestApp::CommonTestApp(const char *signature, status_t *result)
			 : BApplication(signature, result),
			   fQuitOnSecondTry(false),
			   fEventThread(-1),
			   fEventDelay(0),
			   fEventCount(0),
			   fEventHandler(NULL),
			   fMessageHandler(NULL),
			   fReportDestruction(false)
{
}

// destructor
CommonTestApp::~CommonTestApp()
{
	if (fEventThread >= 0) {
		int32 result;
		wait_for_thread(fEventThread, &result);
	}
	if (fReportDestruction)
		report("BApplication::~BApplication()\n");
	delete fEventHandler;
	delete fMessageHandler;
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

// MessageReceived
void
CommonTestApp::MessageReceived(BMessage *message)
{
	if (fMessageHandler)
		fMessageHandler->MessageReceived(message);
	BApplication::MessageReceived(message);
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

// SetReportDestruction
void
CommonTestApp::SetReportDestruction(bool reportDestruction)
{
	fReportDestruction = reportDestruction;
}

// RunEventThread
status_t
CommonTestApp::RunEventThread(bigtime_t delay, int32 count,
							  EventHandler *handler)
{
	status_t error = B_OK;
	fEventDelay = delay;
	fEventCount = count;
	fEventHandler = handler;
	// spawn the thread
	fEventThread = spawn_thread(&_EventThreadEntry, "event thread",
								B_NORMAL_PRIORITY, this);
	if (fEventThread < 0)
		error = fEventThread;
	if (error == B_OK)
		error = resume_thread(fEventThread);
	// cleanup on error
	if (error != B_OK && fEventThread >= 0) {
		kill_thread(fEventThread);
		fEventThread = -1;
	}
	return error;
}

// SetMessageHandler
void
CommonTestApp::SetMessageHandler(BHandler *handler)
{
	delete fMessageHandler;
	fMessageHandler = handler;
}

// _EventThreadEntry
int32
CommonTestApp::_EventThreadEntry(void *data)
{
	int32 result = 0;
	if (CommonTestApp *app = (CommonTestApp*)data)
		result = app->_EventLoop();
	return result;
}

// _EventLoop
int32
CommonTestApp::_EventLoop()
{
	for (; fEventCount > 0; fEventCount--) {
		snooze(fEventDelay);
		if (fEventHandler)
			fEventHandler->HandleEvent(this);
	}
	return 0;
}

static const char *kAppRunnerTeamPort = "app runner team port";
static bool connectionEstablished = false;
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
	connectionEstablished = (error == B_OK);
	return error;
}

// report
void
report(const char *format,...)
{
	va_list args;
	va_start(args, format);
	if (connectionEstablished)
		vreport(format, args);
	else
		vprintf(format, args);
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


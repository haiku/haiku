// TestApp.cpp

#include "TestApp.h"

// TestHandler

// MessageReceived
void
TestHandler::MessageReceived(BMessage *message)
{
	// clone and push it
	BMessage *clone = new BMessage(*message);
	fQueue.Lock();
	fQueue.AddMessage(clone);
	fQueue.Unlock();
}

// Queue
BMessageQueue &
TestHandler::Queue()
{
	return fQueue;
}


// TestApp

// constructor
TestApp::TestApp(const char *signature)
	   : BApplication(signature),
		 fAppThread(B_ERROR),
		 fHandler()
{
	AddHandler(&fHandler);
	Unlock();
}

// Init
status_t
TestApp::Init()
{
	status_t error = B_OK;
	fAppThread = spawn_thread(&_AppThreadStart, "query app",
							  B_NORMAL_PRIORITY, this);
	if (fAppThread < 0)
		error = fAppThread;
	else {
		error = resume_thread(fAppThread);
		if (error != B_OK)
			kill_thread(fAppThread);
	}
	if (error != B_OK)
		fAppThread = B_ERROR;
	return error;
}

// Terminate
void
TestApp::Terminate()
{
	PostMessage(B_QUIT_REQUESTED, this);
	int32 result;
	wait_for_thread(fAppThread, &result);
}

// ReadyToRun
void
TestApp::ReadyToRun()
{
}

// Handler
TestHandler &
TestApp::Handler()
{
	return fHandler;
}

// _AppThreadStart
int32
TestApp::_AppThreadStart(void *data)
{
	if (TestApp *app = (TestApp*)data) {
		app->Lock();
		app->Run();
	}
	return 0;
}


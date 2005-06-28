// TestApp.cpp

#include <stdio.h>
#include <string.h>

#include <Autolock.h>

#include <TestApp.h>

// TestHandler

// MessageReceived
void
BTestHandler::MessageReceived(BMessage *message)
{
	// clone and push it
	BMessage *clone = new BMessage(*message);
	fQueue.Lock();
	fQueue.AddMessage(clone);
	fQueue.Unlock();
}

// Queue
BMessageQueue &
BTestHandler::Queue()
{
	return fQueue;
}


// TestApp

static status_t sInitError;

// constructor
BTestApp::BTestApp(const char *signature)
	   : BApplication(signature, &sInitError),
		 fAppThread(B_ERROR),
		 fHandlers()
{
	if (sInitError != B_OK) {
		fprintf(stderr, "BTestApp::BTestApp(): Failed to create BApplication: "
			"%s\n", strerror(sInitError));
		exit(1);
	}

	CreateTestHandler();
	Unlock();
}

// destructor
BTestApp::~BTestApp()
{
	int32 count = fHandlers.CountItems();
	for (int32 i = count - 1; i >= 0; i--)
		DeleteTestHandler(TestHandlerAt(i));
}

// Init
status_t
BTestApp::Init()
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
BTestApp::Terminate()
{
	PostMessage(B_QUIT_REQUESTED, this);
	int32 result;
	wait_for_thread(fAppThread, &result);
}

// ReadyToRun
void
BTestApp::ReadyToRun()
{
}

// CreateTestHandler
BTestHandler *
BTestApp::CreateTestHandler()
{
	BTestHandler *handler = new BTestHandler;
	Lock();
	AddHandler(handler);
	fHandlers.AddItem(handler);
	Unlock();
	return handler;
}

// DeleteTestHandler
bool
BTestApp::DeleteTestHandler(BTestHandler *handler)
{
	bool result = false;
	Lock();
	result = fHandlers.RemoveItem(handler);
	if (result)
		RemoveHandler(handler);
	Unlock();
	if (result)
		delete handler;
	return result;
}

// Handler
BTestHandler &
BTestApp::Handler()
{
	// The returned handler must never passed to DeleteTestHandler() by the
	// caller!
	return *TestHandlerAt(0);
}

// TestHandlerAt
BTestHandler *
BTestApp::TestHandlerAt(int32 index)
{
	BAutolock _lock(this);
	return (BTestHandler*)fHandlers.ItemAt(index);
}

// _AppThreadStart
int32
BTestApp::_AppThreadStart(void *data)
{
	if (BTestApp *app = (BTestApp*)data) {
		app->Lock();
		app->Run();
	}
	return 0;
}


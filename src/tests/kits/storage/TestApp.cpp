/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "TestApp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Autolock.h>

void
BTestHandler::MessageReceived(BMessage *message)
{
	// clone and push it
	BMessage *clone = new BMessage(*message);
	fQueue.Lock();
	fQueue.AddMessage(clone);
	fQueue.Unlock();
}


BMessageQueue &
BTestHandler::Queue()
{
	return fQueue;
}


static status_t sInitError;


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


BTestApp::~BTestApp()
{
	int32 count = fHandlers.CountItems();
	for (int32 i = count - 1; i >= 0; i--)
		DeleteTestHandler(TestHandlerAt(i));
}


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


BTestHandler &
BTestApp::Handler()
{
	// The returned handler must never passed to DeleteTestHandler() by the
	// caller!
	return *TestHandlerAt(0);
}


BTestHandler *
BTestApp::TestHandlerAt(int32 index)
{
	BAutolock _lock(this);
	return (BTestHandler*)fHandlers.ItemAt(index);
}


int32
BTestApp::_AppThreadStart(void *data)
{
	if (BTestApp *app = (BTestApp*)data) {
		app->Lock();
		app->Run();
	}
	return 0;
}

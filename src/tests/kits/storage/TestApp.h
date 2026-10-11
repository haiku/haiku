/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _STORAGE_TEST_APP_H_
#define _STORAGE_TEST_APP_H_


#include <Application.h>
#include <ObjectList.h>
#include <MessageQueue.h>


class BTestHandler : public BHandler {
public:
	virtual void MessageReceived(BMessage *message);
	BMessageQueue &Queue();

private:
	BMessageQueue	fQueue;
};


class BTestApp : public BApplication {
public:
	BTestApp(const char *signature);
	virtual ~BTestApp();

	status_t Init();
	void Terminate();

	virtual void ReadyToRun();

	BTestHandler *CreateTestHandler();
	bool DeleteTestHandler(BTestHandler *handler);

	BTestHandler &Handler();
	BTestHandler *TestHandlerAt(int32 index);

private:
	static int32 _AppThreadStart(void *data);

private:
	typedef BObjectList<BTestHandler> TestHandlers;
	thread_id		fAppThread;
	TestHandlers	fHandlers;
};

#endif

// TestApp.h

#ifndef _sk_test_app_h_
#define _sk_test_app_h_

#include <Application.h>
#include <MessageQueue.h>

// TestHandler

class TestHandler : public BHandler {
public:
	virtual void MessageReceived(BMessage *message);
	BMessageQueue &Queue();

private:
	BMessageQueue	fQueue;
};


// TestApp

class TestApp : public BApplication {
public:
	TestApp(const char *signature);

	status_t Init();
	void Terminate();

	virtual void ReadyToRun();

	TestHandler &Handler();

private:
	static int32 _AppThreadStart(void *data);

private:
	thread_id		fAppThread;
	TestHandler	fHandler;
};

#endif	// _sk_test_app_h_

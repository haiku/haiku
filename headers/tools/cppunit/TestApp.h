// TestApp.h

#ifndef _beos_test_app_h_
#define _beos_test_app_h_

#include <Application.h>
#include <List.h>
#include <MessageQueue.h>

// TestHandler

class BTestHandler : public BHandler {
public:
	virtual void MessageReceived(BMessage *message);
	BMessageQueue &Queue();

private:
	BMessageQueue	fQueue;
};


// TestApp

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
	thread_id		fAppThread;
	BList			fHandlers;
};

#endif	// _beos_test_app_h_

// MessageRunnerTestHelpers.cpp

#include <stdio.h>

#include <Autolock.h>

#include "MessageRunnerTestHelpers.h"

enum {
	JITTER	= 10000,
};

// constructor
MessageRunnerTestHandler::MessageRunnerTestHandler()
	: BHandler("message runner test handler"),
	  fReplyCount()
{
}

// destructor
MessageRunnerTestHandler::~MessageRunnerTestHandler()
{
}

// MessageReceived
void
MessageRunnerTestHandler::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_REPLY:
			fReplyCount++;
			break;
	}
}


///////////////////////////
// MessageRunnerTestLooper

struct MessageRunnerTestLooper::MessageInfo {
	bigtime_t	time;
};

// constructor
MessageRunnerTestLooper::MessageRunnerTestLooper()
	: BLooper(),
	  fMessageInfos()
{
}

// destructor
MessageRunnerTestLooper::~MessageRunnerTestLooper()
{
	for (int32 i = 0; MessageInfo *info = MessageInfoAt(i); i++)
		delete info;
}

// MessageReceived
void
MessageRunnerTestLooper::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_RUNNER_MESSAGE:
		{
			MessageInfo *info = new MessageInfo;
			info->time = system_time();
			fMessageInfos.AddItem(info);
			message->SendReply(MSG_REPLY);
			break;
		}
	}
}

// CheckMessages
bool
MessageRunnerTestLooper::CheckMessages(bigtime_t startTime, bigtime_t interval,
									   int32 count)
{
	return CheckMessages(0, startTime, interval, count);
}

// CheckMessages
bool
MessageRunnerTestLooper::CheckMessages(int32 skip, bigtime_t startTime,
									   bigtime_t interval, int32 count)
{
	BAutolock _lock(this);
	bool result = (fMessageInfos.CountItems() == count + skip);
if (!result) {
printf("message counts don't match: %ld vs. %ld\n", fMessageInfos.CountItems(),
count + skip);
}
	for (int32 i = 0; result && i < count; i++) {
		MessageInfo *info = MessageInfoAt(i + skip);
		bigtime_t expectedTime = startTime + (i + 1) * interval;
		result = (expectedTime - JITTER < info->time
				  && info->time < expectedTime + JITTER);
if (!result)
printf("message out of time: %lld vs. %lld\n", info->time, expectedTime);
	}
	return result;
}

// MessageInfoAt
MessageRunnerTestLooper::MessageInfo*
MessageRunnerTestLooper::MessageInfoAt(int32 index) const
{
	return static_cast<MessageInfo*>(fMessageInfos.ItemAt(index));
}


////////////////////////
// MessageRunnerTestApp

// constructor
MessageRunnerTestApp::MessageRunnerTestApp(const char *signature)
	: BApplication(signature),
	  fThread(-1),
	  fReplyCount(0),
	  fLooper(NULL),
	  fHandler(NULL)
{
	// create a looper
	fLooper = new MessageRunnerTestLooper;
	fLooper->Run();
	// create a handler
	fHandler = new MessageRunnerTestHandler;
	AddHandler(fHandler);
	// start out message loop in a different thread
	Unlock();
	fThread = spawn_thread(_ThreadEntry, "message runner app thread",
						   B_NORMAL_PRIORITY, this);
	resume_thread(fThread);
}

// destructor
MessageRunnerTestApp::~MessageRunnerTestApp()
{
	// quit the looper
	fLooper->Lock();
	fLooper->Quit();
	// shut down our message loop
	BMessage reply;
	PostMessage(B_QUIT_REQUESTED);
	int32 dummy;
	wait_for_thread(fThread, &dummy);
	// delete the handler
	Lock();
	RemoveHandler(fHandler);
	delete fHandler;
}

// MessageReceived
void
MessageRunnerTestApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_REPLY:
			fReplyCount++;
			break;
	}
}

// QuitRequested
bool
MessageRunnerTestApp::QuitRequested()
{
	return true;
}

// _ThreadEntry
int32
MessageRunnerTestApp::_ThreadEntry(void *data)
{
	MessageRunnerTestApp *app = static_cast<MessageRunnerTestApp*>(data);
	app->Lock();
	app->Run();
	return 0;
}


// MessageRunnerTestHelpers.h

#ifndef MESSAGE_RUNNER_TEST_HELPERS_H
#define MESSAGE_RUNNER_TEST_HELPERS_H

#include <Application.h>
#include <List.h>

enum {
	MSG_RUNNER_MESSAGE	= 'rmsg',
	MSG_REPLY			= 'rply',
};

// MessageRunnerTestHandler
class MessageRunnerTestHandler : public BHandler {
public:
	MessageRunnerTestHandler();
	~MessageRunnerTestHandler();

	virtual void MessageReceived(BMessage *message);

	int32 CountReplies() const { return fReplyCount; }

private:
	int32					fReplyCount;
};

// MessageRunnerTestLooper
class MessageRunnerTestLooper : public BLooper {
public:
	MessageRunnerTestLooper();
	~MessageRunnerTestLooper();

	virtual void MessageReceived(BMessage *message);

	bool CheckMessages(bigtime_t startTime, bigtime_t interval, int32 count);
	bool CheckMessages(int32 skip, bigtime_t startTime, bigtime_t interval,
					   int32 count);

private:
	struct MessageInfo;

private:
	MessageInfo *MessageInfoAt(int32 index) const;

private:
	BList	fMessageInfos;
};

// MessageRunnerTestApp
class MessageRunnerTestApp : public BApplication {
public:
	MessageRunnerTestApp(const char *signature);
	~MessageRunnerTestApp();

	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested();

	int32 CountReplies() const { return fReplyCount; }

	MessageRunnerTestLooper *TestLooper() const { return fLooper; }
	MessageRunnerTestHandler *TestHandler() const { return fHandler; }

private:
	static int32 _ThreadEntry(void *data);

private:
	thread_id					fThread;
	int32						fReplyCount;
	MessageRunnerTestLooper		*fLooper;
	MessageRunnerTestHandler	*fHandler;
};

#endif	// MESSAGE_RUNNER_TEST_HELPERS_H

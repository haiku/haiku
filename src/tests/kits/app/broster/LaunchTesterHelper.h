//	LaunchTesterHelper.h

#ifndef LAUNCH_TESTER_HELPER_H
#define LAUNCH_TESTER_HELPER_H

#include <Application.h>
#include <Locker.h>
#include <MessageQueue.h>
#include <OS.h>

// LaunchCaller
class LaunchCaller {
public:
	virtual status_t operator()(const char *type, team_id *team) = 0;
};

// LaunchContext
class LaunchContext {
public:
	LaunchContext(LaunchCaller &caller);
	~LaunchContext();

	status_t operator()(const char *type, team_id *team);

	void HandleMessage(BMessage *message);

	void Terminate();

	team_id TeamAt(int32 index) const;

	BMessage *NextMessageFrom(team_id team, int32 &cookie,
							  bigtime_t *time = NULL);
	bool CheckNextMessage(team_id team, int32 &cookie, uint32 what);

private:
	class Message;
	class AppInfo;

private:
	AppInfo *AppInfoAt(int32 index) const;
	AppInfo *AppInfoFor(team_id team) const;
	AppInfo *CreateAppInfo(team_id team, const BMessenger *messenger = NULL);
	AppInfo *CreateAppInfo(BMessenger messenger);
	void TerminateApp(AppInfo *info);

	int32 Terminator();

	static int32 AppThreadEntry(void *data);
	static int32 TerminatorEntry(void *data);

private:
	LaunchCaller	&fCaller;
	BList			fAppInfos;
	mutable BLocker	fLock;
	thread_id		fAppThread;
	thread_id		fTerminator;
	bool			fTerminating;
};

// RosterLaunchApp
class RosterLaunchApp : public BApplication {
public:
	RosterLaunchApp(const char *signature);

	virtual void MessageReceived(BMessage *message);

	BMessageQueue &MessageQueue();

	void SetLaunchContext(LaunchContext *context);
	LaunchContext *GetLaunchContext() const;

private:
	BMessageQueue	fMessageQueue;
	LaunchContext	*fLaunchContext;
};

#endif	// LAUNCH_TESTER_HELPER_H

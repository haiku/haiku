//	LaunchTesterHelper.h

#ifndef LAUNCH_TESTER_HELPER_H
#define LAUNCH_TESTER_HELPER_H

#include <Application.h>
#include <List.h>
#include <Locker.h>
#include <MessageQueue.h>
#include <OS.h>

enum {
	MSG_1	= 'msg1',
	MSG_2	= 'msg2',
	MSG_3	= 'msg3',
};

// LaunchCaller
class LaunchCaller {
public:
	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team) = 0;
	virtual int32 SupportsMessages() const { return 1; }
	virtual bool SupportsArgv() const { return SupportsMessages() == 0; }
	virtual bool SupportsRefs() const { return false; }
	virtual const entry_ref *Ref() const { return NULL; }
};

// LaunchContext
class LaunchContext {
public:
	LaunchContext(LaunchCaller &caller);
	~LaunchContext();

	status_t operator()(const char *type, team_id *team);
	status_t operator()(const char *type, BList *messages, int32 argc,
						const char **argv, team_id *team);

	void HandleMessage(BMessage *message);

	void Terminate();

	team_id TeamAt(int32 index) const;

	BMessage *NextMessageFrom(team_id team, int32 &cookie,
							  bigtime_t *time = NULL);
	bool CheckNextMessage(team_id team, int32 &cookie, uint32 what);
	bool CheckArgvMessage(team_id team, int32 &cookie,
						  const entry_ref *appRef, bool useRef = true);
	bool CheckArgvMessage(team_id team, int32 &cookie, const entry_ref *appRef,
						  int32 argc, const char **argv, bool useRef = true);
	bool CheckArgvMessage(team_id team, int32 &cookie, const entry_ref *appRef,
						  const entry_ref *ref, int32 argc, const char **argv);
	bool RemoveStandardArgvMessage(team_id team, const entry_ref *appRef);
	bool RemoveArgvMessage(team_id team, const entry_ref *appRef, int32 argc,
						   const char **argv);
	bool CheckMessageMessages(team_id team, int32 &cookie);
	bool CheckMessageMessage(team_id team, int32 &cookie, int32 index);
	bool CheckMessageMessage(team_id team, int32 &cookie,
							 const BMessage *message);
	bool CheckRefsMessage(team_id team, int32 &cookie);
	bool CheckRefsMessage(team_id team, int32 &cookie, const entry_ref *refs,
						  int32 count = 1);
	bool RemoveRefsMessage(team_id team);

	BList *StandardMessages();

public:
	static const char *kStandardArgv[];
	static const int32 kStandardArgc;

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
	BList			fStandardMessages;
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

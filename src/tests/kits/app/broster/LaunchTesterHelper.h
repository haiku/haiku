//	LaunchTesterHelper.h

#ifndef LAUNCH_TESTER_HELPER_H
#define LAUNCH_TESTER_HELPER_H

#include <Application.h>
#include <List.h>
#include <Locker.h>
#include <MessageQueue.h>
#include <OS.h>

// LaunchCaller
class LaunchCaller {
public:
	LaunchCaller() : fNext(NULL) {}
	virtual ~LaunchCaller() { if (fNext) delete fNext;}

	virtual status_t operator()(const char *type, BList *messages, int32 argc,
								const char **argv, team_id *team) = 0;
	virtual int32 SupportsMessages() const { return 1; }
	virtual bool SupportsArgv() const { return SupportsMessages() == 0; }
	virtual bool SupportsRefs() const { return false; }
	virtual const entry_ref *Ref() const { return NULL; }

	virtual LaunchCaller &Clone()
	{
		LaunchCaller *newCaller = CloneInternal();
		newCaller->fNext = fNext;
		fNext = newCaller;
		return *newCaller;
	}

	virtual LaunchCaller *CloneInternal() = 0;

private:
	LaunchCaller	*fNext;
};

// LaunchContext
class LaunchContext {
public:
	LaunchContext();
	~LaunchContext();

	status_t operator()(LaunchCaller &caller, const char *type, team_id *team);
	status_t operator()(LaunchCaller &caller, const char *type,
						BList *messages, int32 argc, const char **argv,
						team_id *team);

	void HandleMessage(BMessage *message);

	void Terminate();
	void TerminateApp(team_id team, bool wait = true);

	team_id TeamAt(int32 index) const;

	BMessenger AppMessengerFor(team_id team) const;

	BMessage *NextMessageFrom(team_id team, int32 &cookie,
							  bigtime_t *time = NULL);
	bool CheckNextMessage(LaunchCaller &caller, team_id team, int32 &cookie,
						  uint32 what);
	bool CheckMainArgsMessage(LaunchCaller &caller, team_id team,
							  int32 &cookie, const entry_ref *appRef,
							  bool useRef = true);
	bool CheckMainArgsMessage(LaunchCaller &caller, team_id team,
							  int32 &cookie, const entry_ref *appRef,
							  int32 argc, const char **argv,
							  bool useRef = true);
	bool CheckArgvMessage(LaunchCaller &caller, team_id team, int32 &cookie,
						  const entry_ref *appRef, bool useRef = true);
	bool CheckArgvMessage(LaunchCaller &caller, team_id team, int32 &cookie,
						  const entry_ref *appRef,
						  int32 argc, const char **argv, bool useRef = true);
	bool CheckArgvMessage(LaunchCaller &caller, team_id team, int32 &cookie,
						  const entry_ref *appRef, const entry_ref *ref,
						  int32 argc, const char **argv);
	bool CheckArgsMessage(LaunchCaller &caller, team_id team, int32 &cookie,
						  const entry_ref *appRef, const entry_ref *ref,
						  int32 argc, const char **argv, uint32 messageCode);
	bool CheckMessageMessages(LaunchCaller &caller, team_id team,
							  int32 &cookie);
	bool CheckMessageMessage(LaunchCaller &caller, team_id team, int32 &cookie,
							 int32 index);
	bool CheckMessageMessage(LaunchCaller &caller, team_id team, int32 &cookie,
							 const BMessage *message);
	bool CheckRefsMessage(LaunchCaller &caller, team_id team, int32 &cookie);
	bool CheckRefsMessage(LaunchCaller &caller, team_id team, int32 &cookie,
						  const entry_ref *refs, int32 count = 1);

	bool WaitForMessage(uint32 messageCode, bool fromNow = false,
						bigtime_t timeout = B_INFINITE_TIMEOUT);
	bool WaitForMessage(team_id team, uint32 messageCode, bool fromNow = false,
						bigtime_t timeout = B_INFINITE_TIMEOUT,
						int32 startIndex = 0);

	BList *StandardMessages();

public:
	static const char *kStandardArgv[];
	static const int32 kStandardArgc;

private:
	class Message;
	class Sleeper;
	class AppInfo;

private:
	AppInfo *AppInfoAt(int32 index) const;
	AppInfo *AppInfoFor(team_id team) const;
	AppInfo *CreateAppInfo(team_id team, const BMessenger *messenger = NULL);
	AppInfo *CreateAppInfo(BMessenger messenger);
	void TerminateApp(AppInfo *info);

	Message *FindMessage(uint32 messageCode);
	void AddSleeper(Sleeper *sleeper);
	void RemoveSleeper(Sleeper *sleeper);
	void NotifySleepers(uint32 messageCode);

	int32 Terminator();

	static int32 AppThreadEntry(void *data);
	static int32 TerminatorEntry(void *data);

private:
	BList			fAppInfos;
	BList			fSleepers;
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
	~RosterLaunchApp();

	virtual void MessageReceived(BMessage *message);

	void SetLaunchContext(LaunchContext *context);
	LaunchContext *GetLaunchContext() const;

	BHandler *Handler() { return fHandler; }
	void SetHandler(BHandler *handler);

private:
	LaunchContext	*fLaunchContext;
	BHandler		*fHandler;
};

// RosterBroadcastHandler
class RosterBroadcastHandler : public BHandler {
public:
	RosterBroadcastHandler();

	virtual void MessageReceived(BMessage *message);
};

#endif	// LAUNCH_TESTER_HELPER_H

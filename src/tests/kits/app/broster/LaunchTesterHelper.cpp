//	LaunchTesterHelper.cpp

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <List.h>
#include <Message.h>
#include <Messenger.h>

#include "LaunchTesterHelper.h"
#include "RosterTestAppDefs.h"

/////////////////
// LaunchContext

class LaunchContext::Message {
public:
	BMessage	message;
	bigtime_t	when;
};

class LaunchContext::AppInfo {
public:
	AppInfo(team_id team)
		: fTeam(team),
		  fMessenger(),
		  fMessages(),
		  fTerminating(false)
	{
	}

	~AppInfo()
	{
		for (int32 i = 0; LaunchContext::Message *message = MessageAt(i); i++)
			delete message;
	}

	team_id Team() const
	{
		return fTeam;
	}

	void SetMessenger(BMessenger messenger)
	{
		fMessenger = messenger;
	}

	BMessenger Messenger() const
	{
		return fMessenger;
	}

	void Terminate()
	{
		if (!fTerminating && fMessenger.IsValid())
		{
			fTerminating = true;
			fMessenger.SendMessage(B_QUIT_REQUESTED);
		}
	}

	void AddMessage(const BMessage &_message)
	{
		LaunchContext::Message *message = new LaunchContext::Message;
		message->message = _message;
		message->when = system_time();
		fMessages.AddItem(message);
	}

	LaunchContext::Message *MessageAt(int32 index) const
	{
		return (LaunchContext::Message*)fMessages.ItemAt(index);
	}

private:
	team_id		fTeam;
	BMessenger	fMessenger;
	BList		fMessages;
	bool		fTerminating;
};

// constructor
LaunchContext::LaunchContext(LaunchCaller &caller)
	: fCaller(caller),
	  fAppInfos(),
	  fLock(),
	  fAppThread(B_ERROR),
	  fTerminator(B_ERROR),
	  fTerminating(false)
{
	RosterLaunchApp *app = dynamic_cast<RosterLaunchApp*>(be_app);
	app->SetLaunchContext(this);
	app->Unlock();
	fAppThread = spawn_thread(AppThreadEntry, "app thread", B_NORMAL_PRIORITY,
							  this);
	if (fAppThread >= 0) {
		status_t error = resume_thread(fAppThread);
		if (error != B_OK)
			printf("ERROR: Couldn't resume app thread: %s\n", strerror(error));
	} else
		printf("ERROR: Couldn't spawn app thread: %s\n", strerror(fAppThread));
}

// destructor
LaunchContext::~LaunchContext()
{
	Terminate();
	// cleanup
	for (int32 i = 0; AppInfo *info = AppInfoAt(i); i++)
		delete info;
}

// ()
status_t
LaunchContext::operator()(const char *type, team_id *team)
{
	BAutolock _lock(fLock);
	status_t result = fCaller(type, team);
	if (result == B_OK) {
		if (team)
			CreateAppInfo(*team);
		else {
			// Note: Here's a race condition. If Terminate() is called, before
			// we've received the first message from the test app, then
			// there's a good chance that we quit the application before
			// having received any message (or at least not all messages)
			// from the test app. We wait some time to minimize the chance...
			snooze(100000);
		}
	}
	return result;
}

// HandleMessage
void
LaunchContext::HandleMessage(BMessage *message)
{
	BAutolock _lock(fLock);
	switch (message->what) {
		case MSG_STARTED:
		case MSG_TERMINATED:
		case MSG_ARGV_RECEIVED:
		case MSG_REFS_RECEIVED:
		case MSG_MESSAGE_RECEIVED:
		case MSG_QUIT_REQUESTED:
		case MSG_READY_TO_RUN:
		{
			BMessenger messenger = message->ReturnAddress();
			AppInfo *info = CreateAppInfo(messenger);
			info->AddMessage(*message);
			if (fTerminating)
				TerminateApp(info);
			break;
		}
		default:
			break;
	}
}

// Terminate
void
LaunchContext::Terminate()
{
	fLock.Lock();
	if (!fTerminating) {
		fTerminating = true;
		// tell all test apps to quit
		for (int32 i = 0; AppInfo *info = AppInfoAt(i); i++)
			TerminateApp(info);
		// start terminator
		fTerminator = spawn_thread(TerminatorEntry, "terminator",
								   B_NORMAL_PRIORITY, this);
		if (fTerminator >= 0) {
			status_t error = resume_thread(fTerminator);
			if (error != B_OK) {
				printf("ERROR: Couldn't resume terminator thread: %s\n",
					   strerror(error));
			}
		} else {
			printf("ERROR: Couldn't spawn terminator thread: %s\n",
				   strerror(fTerminator));
		}
	}
	fLock.Unlock();
	// wait for the app to terminate
	int32 dummy;
	wait_for_thread(fAppThread, &dummy);
	wait_for_thread(fTerminator, &dummy);
}

// TeamAt
team_id
LaunchContext::TeamAt(int32 index) const
{
	BAutolock _lock(fLock);
	team_id team = B_ERROR;
	if (AppInfo *info = AppInfoAt(index))
		team = info->Team();
	return team;
}

// NextMessageFrom
BMessage*
LaunchContext::NextMessageFrom(team_id team, int32 &cookie, bigtime_t *time)
{
	BAutolock _lock(fLock);
	BMessage *message = NULL;
	if (AppInfo *info = AppInfoFor(team)) {
		if (Message *contextMessage = info->MessageAt(cookie++)) {
			message = new BMessage(contextMessage->message);
			delete contextMessage;
		}
	}
	return message;
}

// CheckNextMessage
bool
LaunchContext::CheckNextMessage(team_id team, int32 &cookie, uint32 what)
{
	BMessage *message = NextMessageFrom(team, cookie);
	return (message && message->what == what);
}

// AppInfoAt
LaunchContext::AppInfo*
LaunchContext::AppInfoAt(int32 index) const
{
	return (AppInfo*)fAppInfos.ItemAt(index);
}

// AppInfoFor
LaunchContext::AppInfo*
LaunchContext::AppInfoFor(team_id team) const
{
	for (int32 i = 0; AppInfo *info = AppInfoAt(i); i++) {
		if (info->Team() == team)
			return info;
	}
	return NULL;
}

// CreateAppInfo
LaunchContext::AppInfo*
LaunchContext::CreateAppInfo(BMessenger messenger)
{
	return CreateAppInfo(messenger.Team(), &messenger);
}

// CreateAppInfo
LaunchContext::AppInfo*
LaunchContext::CreateAppInfo(team_id team, const BMessenger *messenger)
{
	AppInfo *info = AppInfoFor(team);
	if (!info) {
		info = new AppInfo(team);
		fAppInfos.AddItem(info);
	}
	if (messenger && !info->Messenger().IsValid())
		info->SetMessenger(*messenger);
	return info;
}

// TerminateApp
void
LaunchContext::TerminateApp(AppInfo *info)
{
	if (info)
		info->Terminate();
}

// Terminator
int32
LaunchContext::Terminator()
{
	bool allGone = false;
	while (!allGone) {
		allGone = true;
		fLock.Lock();
		for (int32 i = 0; AppInfo *info = AppInfoAt(i); i++) {
			team_info teamInfo;
			allGone &= (get_team_info(info->Team(), &teamInfo) != B_OK);
		}
		fLock.Unlock();
		if (!allGone)
			snooze(10000);
	}
	be_app->PostMessage(B_QUIT_REQUESTED, be_app);
	return 0;
}

// AppThreadEntry
int32
LaunchContext::AppThreadEntry(void *)
{
	be_app->Lock();
	be_app->Run();
	return 0;
}

// TerminatorEntry
int32
LaunchContext::TerminatorEntry(void *data)
{
	return ((LaunchContext*)data)->Terminator();
}


///////////////////
// RosterLaunchApp

// constructor
RosterLaunchApp::RosterLaunchApp(const char *signature)
	: BApplication(signature),
	  fMessageQueue(),
	  fLaunchContext(NULL)
{
}

// MessageReceived
void
RosterLaunchApp::MessageReceived(BMessage *message)
{
	if (fLaunchContext)
		fLaunchContext->HandleMessage(message);
}

// MessageQueue
BMessageQueue&
RosterLaunchApp::MessageQueue()
{
	return fMessageQueue;
}

// SetLaunchContext
void
RosterLaunchApp::SetLaunchContext(LaunchContext *context)
{
	fLaunchContext = context;
}

// GetLaunchContext
LaunchContext *
RosterLaunchApp::GetLaunchContext() const
{
	return fLaunchContext;
}


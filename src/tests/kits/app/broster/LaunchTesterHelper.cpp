//	LaunchTesterHelper.cpp

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Entry.h>
#include <List.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>

#include "LaunchTesterHelper.h"
#include "RosterTestAppDefs.h"

/////////////////
// LaunchContext

const char *LaunchContext::kStandardArgv[] = {
	"Some", "nice", "arguments"
};
const int32 LaunchContext::kStandardArgc
	= sizeof(kStandardArgv) / sizeof(const char*);

// Message
class LaunchContext::Message {
public:
	BMessage	message;
	bigtime_t	when;
};

// AppInfo
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

	LaunchContext::Message *RemoveMessage(int32 index)
	{
		return (LaunchContext::Message*)fMessages.RemoveItem(index);
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
	for (int32 i = 0;
		 BMessage *message = (BMessage*)fStandardMessages.ItemAt(i);
		 i++) {
		delete message;
	}
}

// ()
status_t
LaunchContext::operator()(const char *type, team_id *team)
{
	BMessage message1(MSG_1);
	BMessage message2(MSG_2);
	BMessage message3(MSG_3);
	BList messages;
	messages.AddItem(&message1);
	messages.AddItem(&message2);
	messages.AddItem(&message3);
	return (*this)(type, &messages, kStandardArgc, kStandardArgv, team);
}

// ()
status_t
LaunchContext::operator()(const char *type, BList *messages, int32 argc,
						  const char **argv, team_id *team)
{
	BAutolock _lock(fLock);
	status_t result = fCaller(type, messages, argc, argv, team);
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
//printf("LaunchContext::HandleMessage(%6ld: %.4s)\n",
//message->ReturnAddress().Team(), (char*)&message->what);
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
		if (Message *contextMessage = info->MessageAt(cookie++))
			message = &contextMessage->message;
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

// CheckArgvMessage
bool
LaunchContext::CheckArgvMessage(team_id team, int32 &cookie,
								const entry_ref *appRef, bool useRef)
{
	bool result = true;
	if (fCaller.SupportsArgv()) {
		result = CheckArgvMessage(team, cookie, appRef, kStandardArgc,
								  kStandardArgv, useRef);
	}
	return result;
}

// CheckArgvMessage
bool
LaunchContext::CheckArgvMessage(team_id team, int32 &cookie,
								const entry_ref *appRef, int32 argc,
								const char **argv, bool useRef)
{
	const entry_ref *ref = (useRef ? fCaller.Ref() : NULL);
	return CheckArgvMessage(team, cookie, appRef, ref , argc, argv);
}

// CheckArgvMessage
bool
LaunchContext::CheckArgvMessage(team_id team, int32 &cookie,
								const entry_ref *appRef,
								const entry_ref *ref , int32 argc,
								const char **argv)
{
	BMessage *message = NextMessageFrom(team, cookie);
	bool result = (message && message->what == MSG_ARGV_RECEIVED);
	int32 additionalRef = (ref ? 1 : 0);
	// check argc
	int32 foundArgc = -1;
	if (result) {
		result = (message->FindInt32("argc", &foundArgc) == B_OK
				  && foundArgc == argc + 1 + additionalRef);
if (!result)
printf("argc differ: %ld vs %ld + 1 + %ld\n", foundArgc, argc, additionalRef);
	}
	// check argv[0] (the app file name)
	if (result) {
		BPath path;
		const char *arg = NULL;
		result = (path.SetTo(appRef) == B_OK
				  && message->FindString("argv", 0, &arg) == B_OK
				  && path == arg);
if (!result)
printf("app paths differ: `%s' vs `%s'\n", arg, path.Path());
	}
	// check remaining argv
	for (int32 i = 1; result && i < argc; i++) {
		const char *arg = NULL;
		result = (message->FindString("argv", i, &arg) == B_OK
				  && !strcmp(arg, argv[i - 1]));
if (!result)
printf("arg[%ld] differ: `%s' vs `%s'\n", i, arg, argv[i - 1]);
	}
	// check additional ref
	if (result && additionalRef) {
		BPath path;
		const char *arg = NULL;
		result = (path.SetTo(ref) == B_OK
				  && message->FindString("argv", argc + 1, &arg) == B_OK
				  && path == arg);
if (!result)
printf("app paths differ: `%s' vs `%s'\n", arg, path.Path());
	}
	return result;
}

// RemoveStandardArgvMessage
bool
LaunchContext::RemoveStandardArgvMessage(team_id team, const entry_ref *appRef)
{
	return RemoveArgvMessage(team, appRef, kStandardArgc, kStandardArgv);
}

// RemoveArgvMessage
bool
LaunchContext::RemoveArgvMessage(team_id team, const entry_ref *appRef,
								 int32 argc, const char **argv)
{
	BAutolock _lock(fLock);
	bool result = true;
	if (fCaller.SupportsArgv()) {
		result = false;
		int32 cookie = 0;
		while (BMessage *message = NextMessageFrom(team, cookie)) {
			if (message->what == MSG_ARGV_RECEIVED) {
				int32 index = --cookie;
				result = CheckArgvMessage(team, cookie, appRef, argc, argv);
				delete AppInfoFor(team)->RemoveMessage(index);
				break;
			}
		}
	}
	return result;
}

// CheckMessageMessages
bool
LaunchContext::CheckMessageMessages(team_id team, int32 &cookie)
{
	BAutolock _lock(fLock);
	bool result = true;
	for (int32 i = 0; i < 3; i++)
		result &= CheckMessageMessage(team, cookie, i);
	return result;
}

// CheckMessageMessage
bool
LaunchContext::CheckMessageMessage(team_id team, int32 &cookie, int32 index)
{
	bool result = true;
	if (fCaller.SupportsMessages() > index && index < 3) {
		uint32 commands[] = { MSG_1, MSG_2, MSG_3 };
		BMessage message(commands[index]);
		result = CheckMessageMessage(team, cookie, &message);
	}
	return result;
}

// CheckMessageMessage
bool
LaunchContext::CheckMessageMessage(team_id team, int32 &cookie,
								   const BMessage *expectedMessage)
{
	BMessage *message = NextMessageFrom(team, cookie);
	bool result = (message && message->what == MSG_MESSAGE_RECEIVED);
	if (result) {
		BMessage sentMessage;
		result = (message->FindMessage("message", &sentMessage) == B_OK
				  && sentMessage.what == expectedMessage->what);
	}
	return result;
}

// CheckRefsMessage
bool
LaunchContext::CheckRefsMessage(team_id team, int32 &cookie)
{
	bool result = true;
	if (fCaller.SupportsRefs())
		result = CheckRefsMessage(team, cookie, fCaller.Ref());
	return result;
}

// CheckRefsMessage
bool
LaunchContext::CheckRefsMessage(team_id team, int32 &cookie,
								const entry_ref *refs, int32 count)
{
	BMessage *message = NextMessageFrom(team, cookie);
	bool result = (message && message->what == MSG_REFS_RECEIVED);
	if (result) {
		entry_ref ref;
		for (int32 i = 0; result && i < count; i++) {
			result = (message->FindRef("refs", i, &ref) == B_OK
					  && ref == refs[i]);
		}
	}
	return result;
}

// RemoveRefsMessage
bool
LaunchContext::RemoveRefsMessage(team_id team)
{
	BAutolock _lock(fLock);
	bool result = true;
	if (fCaller.SupportsRefs()) {
		result = false;
		int32 cookie = 0;
		while (BMessage *message = NextMessageFrom(team, cookie)) {
			if (message->what == MSG_REFS_RECEIVED) {
				int32 index = --cookie;
				result = CheckRefsMessage(team, cookie);
				delete AppInfoFor(team)->RemoveMessage(index);
				break;
			}
		}
	}
	return result;
}

// StandardMessages
BList*
LaunchContext::StandardMessages()
{
	if (fStandardMessages.IsEmpty()) {
		fStandardMessages.AddItem(new BMessage(MSG_1));
		fStandardMessages.AddItem(new BMessage(MSG_2));
		fStandardMessages.AddItem(new BMessage(MSG_3));
	}
	return &fStandardMessages;
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


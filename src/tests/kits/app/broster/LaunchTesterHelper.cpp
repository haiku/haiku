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

// Sleeper
class LaunchContext::Sleeper {
public:
	Sleeper() : fMessageCode(0), fSemaphore(-1) {}

	~Sleeper()
	{
		delete_sem(fSemaphore);
	}

	status_t Init(uint32 messageCode)
	{
		fMessageCode = messageCode;
		fSemaphore = create_sem(0, "sleeper sem");
		return (fSemaphore >= 0 ? B_OK : fSemaphore);
	}

	uint32 MessageCode() const { return fMessageCode; }

	status_t Sleep(bigtime_t timeout = B_INFINITE_TIMEOUT)
	{
		return acquire_sem_etc(fSemaphore, 1, B_RELATIVE_TIMEOUT, timeout);
	}

	status_t WakeUp()
	{
		return release_sem(fSemaphore);
	}

private:
	uint32		fMessageCode;
	sem_id		fSemaphore;
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
		NotifySleepers(_message.what);
	}

	LaunchContext::Message *RemoveMessage(int32 index)
	{
		return (LaunchContext::Message*)fMessages.RemoveItem(index);
	}

	LaunchContext::Message *MessageAt(int32 index) const
	{
		return (LaunchContext::Message*)fMessages.ItemAt(index);
	}

	LaunchContext::Message *FindMessage(uint32 messageCode,
										int32 startIndex = 0) const
	{
		LaunchContext::Message *message = NULL;
		for (int32 i = startIndex; (message = MessageAt(i)) != NULL; i++) {
			if (message->message.what == messageCode)
				break;
		}
		return message;
	}

	void AddSleeper(Sleeper *sleeper)
	{
		fSleepers.AddItem(sleeper);
	}

	void RemoveSleeper(Sleeper *sleeper)
	{
		fSleepers.RemoveItem(sleeper);
	}

	void NotifySleepers(uint32 messageCode)
	{
		for (int32 i = 0;
			 Sleeper *sleeper = (Sleeper*)fSleepers.ItemAt(i);
			 i++) {
			if (sleeper->MessageCode() == messageCode)
				sleeper->WakeUp();
		}
	}

private:
	team_id		fTeam;
	BMessenger	fMessenger;
	BList		fMessages;
	bool		fTerminating;
	BList		fSleepers;
};

// constructor
LaunchContext::LaunchContext()
	: fAppInfos(),
	  fSleepers(),
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
LaunchContext::operator()(LaunchCaller &caller, const char *type,
						  team_id *team)
{
	BMessage message1(MSG_1);
	BMessage message2(MSG_2);
	BMessage message3(MSG_3);
	BList messages;
	messages.AddItem(&message1);
	messages.AddItem(&message2);
	messages.AddItem(&message3);
	return (*this)(caller, type, &messages, kStandardArgc, kStandardArgv,
				   team);
}

// ()
status_t
LaunchContext::operator()(LaunchCaller &caller, const char *type,
						  BList *messages, int32 argc, const char **argv,
						  team_id *team)
{
	BAutolock _lock(fLock);
	status_t result = caller(type, messages, argc, argv, team);
	if (result == B_OK && team)
		CreateAppInfo(*team);
	return result;
}

// HandleMessage
void
LaunchContext::HandleMessage(BMessage *message)
{
//printf("LaunchContext::HandleMessage(%6ld: %.4s)\n",
//message->ReturnAddress().Team(), (char*)&message->what);
//if (message->what == MSG_MESSAGE_RECEIVED) {
//	BMessage sentMessage;
//	message->FindMessage("message", &sentMessage);
//	team_id sender = -1;
//	message->FindInt32("sender", &sender);
//	printf("  <- %6ld: %.4s\n", sender, (char*)&sentMessage.what);
//}

	BAutolock _lock(fLock);
	switch (message->what) {
		case MSG_STARTED:
		case MSG_TERMINATED:
		case MSG_MAIN_ARGS:
		case MSG_ARGV_RECEIVED:
		case MSG_REFS_RECEIVED:
		case MSG_MESSAGE_RECEIVED:
		case MSG_QUIT_REQUESTED:
		case MSG_READY_TO_RUN:
		case MSG_1:
		case MSG_2:
		case MSG_3:
		case MSG_REPLY:
		{
			BMessenger messenger = message->ReturnAddress();
			// add the message to the respective team's message list
			// Note: catch messages that have not been sent by us or the
			// remote team. The R5 registrar seems to send a B_REPLY message
			// sometimes.
			team_id sender = -1;
			bool dontIgnore = false;
			if (message->FindInt32("sender", &sender) != B_OK
				|| sender == be_app->Team()
				|| sender == messenger.Team()
				|| (message->FindBool("don't ignore", &dontIgnore) == B_OK)
					&& dontIgnore) {
				AppInfo *info = CreateAppInfo(messenger);
				info->AddMessage(*message);
				if (fTerminating)
					TerminateApp(info);
			}
			NotifySleepers(message->what);
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

// TerminateApp
void
LaunchContext::TerminateApp(team_id team, bool wait)
{
	fLock.Lock();
	if (AppInfo *info = AppInfoFor(team))
		TerminateApp(info);
	fLock.Unlock();
	if (wait)
		WaitForMessage(team, MSG_TERMINATED);
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

// AppMessengerFor
BMessenger
LaunchContext::AppMessengerFor(team_id team) const
{
	BAutolock _lock(fLock);
	BMessenger result;
	if (AppInfo *info = AppInfoFor(team)) {
		// We need to do some hacking.
		BMessenger messenger;
		struct fake_messenger {
			port_id	fPort;
			int32	fHandlerToken;
			team_id	fTeam;
			int32	extra0;
			int32	extra1;
			bool	fPreferredTarget;
			bool	extra2;
			bool	extra3;
			bool	extra4;
		} &fake = *(fake_messenger*)&messenger;
		status_t error = B_OK;
		fake.fTeam = team;
		// find app looper port
		bool found = false;
		int32 cookie = 0;
		port_info info;
		while (error == B_OK && !found) {
			error = get_next_port_info(fake.fTeam, &cookie, &info);
			found = (error == B_OK
					 && (!strcmp("AppLooperPort", info.name)
					 	 || !strcmp("rAppLooperPort", info.name)));
		}
		// init messenger
		if (error == B_OK) {
			fake.fPort = info.port;
			fake.fHandlerToken = 0;
			fake.fPreferredTarget = true;
		}
		if (error == B_OK)
			result = messenger;
	}
	return result;
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
LaunchContext::CheckNextMessage(LaunchCaller &caller, team_id team,
								int32 &cookie, uint32 what)
{
	BMessage *message = NextMessageFrom(team, cookie);
	return (message && message->what == what);
}

// CheckMainArgsMessage
bool
LaunchContext::CheckMainArgsMessage(LaunchCaller &caller, team_id team,
								int32 &cookie, const entry_ref *appRef,
								bool useRef)
{
	int32 argc = 0;
	const char **argv = NULL;
	if (caller.SupportsArgv()) {
		argc = kStandardArgc;
		argv = kStandardArgv;
	}
	return CheckMainArgsMessage(caller, team, cookie, appRef, argc, argv,
								useRef);
}

// CheckMainArgsMessage
bool
LaunchContext::CheckMainArgsMessage(LaunchCaller &caller, team_id team,
								int32 &cookie, const entry_ref *appRef,
								int32 argc, const char **argv, bool useRef)
{
	useRef &= caller.SupportsArgv() && argv && argc > 0;
	const entry_ref *ref = (useRef ? caller.Ref() : NULL);
	return CheckArgsMessage(caller, team, cookie, appRef, ref, argc, argv,
							MSG_MAIN_ARGS);
}

// CheckArgvMessage
bool
LaunchContext::CheckArgvMessage(LaunchCaller &caller, team_id team,
								int32 &cookie, const entry_ref *appRef,
								bool useRef)
{
	bool result = true;
	if (caller.SupportsArgv()) {
		result = CheckArgvMessage(caller, team, cookie, appRef, kStandardArgc,
								  kStandardArgv, useRef);
	}
	return result;
}

// CheckArgvMessage
bool
LaunchContext::CheckArgvMessage(LaunchCaller &caller, team_id team,
								int32 &cookie, const entry_ref *appRef,
								int32 argc, const char **argv, bool useRef)
{
	const entry_ref *ref = (useRef ? caller.Ref() : NULL);
	return CheckArgvMessage(caller, team, cookie, appRef, ref , argc, argv);
}

// CheckArgvMessage
bool
LaunchContext::CheckArgvMessage(LaunchCaller &caller, team_id team,
								int32 &cookie, const entry_ref *appRef,
								const entry_ref *ref , int32 argc,
								const char **argv)
{
	return CheckArgsMessage(caller, team, cookie, appRef, ref, argc, argv,
							MSG_ARGV_RECEIVED);
}

// CheckArgsMessage
bool
LaunchContext::CheckArgsMessage(LaunchCaller &caller, team_id team,
								int32 &cookie, const entry_ref *appRef,
								const entry_ref *ref , int32 argc,
								const char **argv, uint32 messageCode)
{
	BMessage *message = NextMessageFrom(team, cookie);
	bool result = (message && message->what == messageCode);
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
printf("document paths differ: `%s' vs `%s'\n", arg, path.Path());
	}
	return result;
}

// CheckMessageMessages
bool
LaunchContext::CheckMessageMessages(LaunchCaller &caller, team_id team,
									int32 &cookie)
{
	BAutolock _lock(fLock);
	bool result = true;
	for (int32 i = 0; i < 3; i++)
		result &= CheckMessageMessage(caller, team, cookie, i);
	return result;
}

// CheckMessageMessage
bool
LaunchContext::CheckMessageMessage(LaunchCaller &caller, team_id team,
								   int32 &cookie, int32 index)
{
	bool result = true;
	if (caller.SupportsMessages() > index && index < 3) {
		uint32 commands[] = { MSG_1, MSG_2, MSG_3 };
		BMessage message(commands[index]);
		result = CheckMessageMessage(caller, team, cookie, &message);
	}
	return result;
}

// CheckMessageMessage
bool
LaunchContext::CheckMessageMessage(LaunchCaller &caller, team_id team,
								   int32 &cookie,
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
LaunchContext::CheckRefsMessage(LaunchCaller &caller, team_id team,
								int32 &cookie)
{
	bool result = true;
	if (caller.SupportsRefs())
		result = CheckRefsMessage(caller, team, cookie, caller.Ref());
	return result;
}

// CheckRefsMessage
bool
LaunchContext::CheckRefsMessage(LaunchCaller &caller, team_id team,
								int32 &cookie, const entry_ref *refs,
								int32 count)
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

// WaitForMessage
bool
LaunchContext::WaitForMessage(uint32 messageCode, bool fromNow,
							  bigtime_t timeout)
{
	status_t error = B_ERROR;
	fLock.Lock();
	error = B_OK;
	if (fromNow || !FindMessage(messageCode)) {
		// add sleeper
		Sleeper *sleeper = new Sleeper;
		error = sleeper->Init(messageCode);
		if (error == B_OK) {
			AddSleeper(sleeper);
			fLock.Unlock();
			// sleep
			error = sleeper->Sleep(timeout);
			fLock.Lock();
			// remove sleeper
			RemoveSleeper(sleeper);
			delete sleeper;
		}
	}
	fLock.Unlock();
	return (error == B_OK);
}

// WaitForMessage
bool
LaunchContext::WaitForMessage(team_id team, uint32 messageCode, bool fromNow,
							  bigtime_t timeout, int32 startIndex)
{
	status_t error = B_ERROR;
	fLock.Lock();
	if (AppInfo *info = AppInfoFor(team)) {
		error = B_OK;
		if (fromNow || !info->FindMessage(messageCode, startIndex)) {
			// add sleeper
			Sleeper *sleeper = new Sleeper;
			error = sleeper->Init(messageCode);
			if (error == B_OK) {
				info->AddSleeper(sleeper);
				fLock.Unlock();
				// sleep
				error = sleeper->Sleep(timeout);
				fLock.Lock();
				// remove sleeper
				info->RemoveSleeper(sleeper);
				delete sleeper;
			}
		}
	}
	fLock.Unlock();
	return (error == B_OK);
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

// FindMessage
LaunchContext::Message*
LaunchContext::FindMessage(uint32 messageCode)
{
	BAutolock _lock(fLock);
	Message *message = NULL;
	AppInfo *info = NULL;
	for (int32 i = 0; !message && (info = AppInfoAt(i)) != NULL; i++)
		message = info->FindMessage(messageCode);
	return message;
}

// AddSleeper
void
LaunchContext::AddSleeper(Sleeper *sleeper)
{
	fSleepers.AddItem(sleeper);
}

// RemoveSleeper
void
LaunchContext::RemoveSleeper(Sleeper *sleeper)
{
	fSleepers.RemoveItem(sleeper);
}

// NotifySleepers
void
LaunchContext::NotifySleepers(uint32 messageCode)
{
	for (int32 i = 0; Sleeper *sleeper = (Sleeper*)fSleepers.ItemAt(i); i++) {
		if (sleeper->MessageCode() == messageCode)
			sleeper->WakeUp();
	}
}


///////////////////
// RosterLaunchApp

// constructor
RosterLaunchApp::RosterLaunchApp(const char *signature)
	: BApplication(signature),
	  fLaunchContext(NULL),
	  fHandler(NULL)
{
}

// destructor
RosterLaunchApp::~RosterLaunchApp()
{
	SetHandler(NULL);
}

// MessageReceived
void
RosterLaunchApp::MessageReceived(BMessage *message)
{
	if (fLaunchContext)
		fLaunchContext->HandleMessage(message);
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

// SetHandler
void
RosterLaunchApp::SetHandler(BHandler *handler)
{
	Lock();
	if (fHandler) {
		RemoveHandler(fHandler);
		delete fHandler;
		fHandler = NULL;
	}
	if (handler) {
		fHandler = handler;
		AddHandler(handler);
	}
	Unlock();
}


//////////////////////////
// RosterBroadcastHandler

// constructor
RosterBroadcastHandler::RosterBroadcastHandler()
{
}

// MessageReceived
void
RosterBroadcastHandler::MessageReceived(BMessage *message)
{
	RosterLaunchApp *app = dynamic_cast<RosterLaunchApp*>(be_app);
	if (LaunchContext *launchContext = app->GetLaunchContext()) {
		message->AddInt32("original what", (int32)message->what);
		message->what = MSG_REPLY;
		launchContext->HandleMessage(message);
	}
}


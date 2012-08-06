/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Copyright 2005-2009, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */


#include <map>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <AppMisc.h>
#include <AutoDeleter.h>
#include <Autolock.h>
#include <Catalog.h>
#include <debug_support.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Invoker.h>
#include <Locale.h>
#include <Path.h>

#include <MessengerPrivate.h>
#include <RegistrarDefs.h>
#include <RosterPrivate.h>
#include <Server.h>
#include <StringList.h>

#include <util/DoublyLinkedList.h>


#define HANDOVER_USE_GDB 1
//#define HANDOVER_USE_DEBUGGER 1

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DebugServer"

#define USE_GUI true
	// define to false if the debug server shouldn't use GUI (i.e. an alert)

//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TRACE(x) debug_printf x
#else
#	define TRACE(x) ;
#endif


using std::map;
using std::nothrow;


static const char *kSignature = "application/x-vnd.Haiku-debug_server";


static void
KillTeam(team_id team, const char *appName = NULL)
{
	// get a team info to verify the team still lives
	team_info info;
	if (!appName) {
		status_t error = get_team_info(team, &info);
		if (error != B_OK) {
			debug_printf("debug_server: KillTeam(): Error getting info for "
				"team %ld: %s\n", team, strerror(error));
			info.args[0] = '\0';
		}

		appName = info.args;
	}

	debug_printf("debug_server: Killing team %ld (%s)\n", team, appName);

	kill_team(team);
}


// #pragma mark -


class DebugMessage : public DoublyLinkedListLinkImpl<DebugMessage> {
public:
	DebugMessage()
	{
	}

	void SetCode(debug_debugger_message code)		{ fCode = code; }
	debug_debugger_message Code() const				{ return fCode; }

	debug_debugger_message_data &Data()				{ return fData; }
	const debug_debugger_message_data &Data() const	{ return fData; }

private:
	debug_debugger_message		fCode;
	debug_debugger_message_data	fData;
};

typedef DoublyLinkedList<DebugMessage>	DebugMessageList;


class TeamDebugHandler : public BLocker {
public:
	TeamDebugHandler(team_id team);
	~TeamDebugHandler();

	status_t Init(port_id nubPort);

	team_id Team() const;

	status_t PushMessage(DebugMessage *message);

private:
	status_t _PopMessage(DebugMessage *&message);

	thread_id _EnterDebugger();
	status_t _SetupGDBArguments(BStringList &arguments, bool usingConsoled);
	void _KillTeam();

	bool _HandleMessage(DebugMessage *message);

	void _LookupSymbolAddress(debug_symbol_lookup_context *lookupContext,
		const void *address, char *buffer, int32 bufferSize);
	void _PrintStackTrace(thread_id thread);
	void _NotifyAppServer(team_id team);
	void _NotifyRegistrar(team_id team, bool openAlert, bool stopShutdown);

	status_t _InitGUI();

	static status_t _HandlerThreadEntry(void *data);
	status_t _HandlerThread();

	bool _ExecutableNameEquals(const char *name) const;
	bool _IsAppServer() const;
	bool _IsInputServer() const;
	bool _IsRegistrar() const;
	bool _IsGUIServer() const;

	static const char *_LastPathComponent(const char *path);
	static team_id _FindTeam(const char *name);
	static bool _AreGUIServersAlive();

private:
	DebugMessageList		fMessages;
	sem_id					fMessageCountSem;
	team_id					fTeam;
	team_info				fTeamInfo;
	char					fExecutablePath[B_PATH_NAME_LENGTH];
	thread_id				fHandlerThread;
	debug_context			fDebugContext;
};


class TeamDebugHandlerRoster : public BLocker {
private:
	TeamDebugHandlerRoster()
		:
		BLocker("team debug handler roster")
	{
	}

public:
	static TeamDebugHandlerRoster *CreateDefault()
	{
		if (!sRoster)
			sRoster = new(nothrow) TeamDebugHandlerRoster;

		return sRoster;
	}

	static TeamDebugHandlerRoster *Default()
	{
		return sRoster;
	}

	bool AddHandler(TeamDebugHandler *handler)
	{
		if (!handler)
			return false;

		BAutolock _(this);

		fHandlers[handler->Team()] = handler;

		return true;
	}

	TeamDebugHandler *RemoveHandler(team_id team)
	{
		BAutolock _(this);

		TeamDebugHandler *handler = NULL;

		TeamDebugHandlerMap::iterator it = fHandlers.find(team);
		if (it != fHandlers.end()) {
			handler = it->second;
			fHandlers.erase(it);
		}

		return handler;
	}

	TeamDebugHandler *HandlerFor(team_id team)
	{
		BAutolock _(this);

		TeamDebugHandler *handler = NULL;

		TeamDebugHandlerMap::iterator it = fHandlers.find(team);
		if (it != fHandlers.end())
			handler = it->second;

		return handler;
	}

	status_t DispatchMessage(DebugMessage *message)
	{
		if (!message)
			return B_BAD_VALUE;

		ObjectDeleter<DebugMessage> messageDeleter(message);

		team_id team = message->Data().origin.team;

		// get the responsible team debug handler
		BAutolock _(this);

		TeamDebugHandler *handler = HandlerFor(team);
		if (!handler) {
			// no handler yet, we need to create one
			handler = new(nothrow) TeamDebugHandler(team);
			if (!handler) {
				KillTeam(team);
				return B_NO_MEMORY;
			}

			status_t error = handler->Init(message->Data().origin.nub_port);
			if (error != B_OK) {
				delete handler;
				KillTeam(team);
				return error;
			}

			if (!AddHandler(handler)) {
				delete handler;
				KillTeam(team);
				return B_NO_MEMORY;
			}
		}

		// hand over the message to it
		handler->PushMessage(message);
		messageDeleter.Detach();

		return B_OK;
	}

private:
	typedef map<team_id, TeamDebugHandler*>	TeamDebugHandlerMap;

	static TeamDebugHandlerRoster	*sRoster;

	TeamDebugHandlerMap				fHandlers;
};

TeamDebugHandlerRoster *TeamDebugHandlerRoster::sRoster = NULL;


class DebugServer : public BServer {
public:
	DebugServer(status_t &error);

	status_t Init();

	virtual bool QuitRequested();

private:
	static status_t _ListenerEntry(void *data);
	status_t _Listener();

	void _DeleteTeamDebugHandler(TeamDebugHandler *handler);

private:
	typedef map<team_id, TeamDebugHandler*>	TeamDebugHandlerMap;

	port_id				fListenerPort;
	thread_id			fListener;
	bool				fTerminating;
};


// #pragma mark -


TeamDebugHandler::TeamDebugHandler(team_id team)
	:
	BLocker("team debug handler"),
	fMessages(),
	fMessageCountSem(-1),
	fTeam(team),
	fHandlerThread(-1)
{
	fDebugContext.nub_port = -1;
	fDebugContext.reply_port = -1;

	fExecutablePath[0] = '\0';
}


TeamDebugHandler::~TeamDebugHandler()
{
	// delete the message count semaphore and wait for the thread to die
	if (fMessageCountSem >= 0)
		delete_sem(fMessageCountSem);

	if (fHandlerThread >= 0 && find_thread(NULL) != fHandlerThread) {
		status_t result;
		wait_for_thread(fHandlerThread, &result);
	}

	// destroy debug context
	if (fDebugContext.nub_port >= 0)
		destroy_debug_context(&fDebugContext);

	// delete the remaining messages
	while (DebugMessage *message = fMessages.Head()) {
		fMessages.Remove(message);
		delete message;
	}
}


status_t
TeamDebugHandler::Init(port_id nubPort)
{
	// get the team info for the team
	status_t error = get_team_info(fTeam, &fTeamInfo);
	if (error != B_OK) {
		debug_printf("debug_server: TeamDebugHandler::Init(): Failed to get "
			"info for team %ld: %s\n", fTeam, strerror(error));
		return error;
	}

	// get the executable path
	error = BPrivate::get_app_path(fTeam, fExecutablePath);
	if (error != B_OK) {
		debug_printf("debug_server: TeamDebugHandler::Init(): Failed to get "
			"executable path of team %ld: %s\n", fTeam, strerror(error));

		fExecutablePath[0] = '\0';
	}

	// init a debug context for the handler
	error = init_debug_context(&fDebugContext, fTeam, nubPort);
	if (error != B_OK) {
		debug_printf("debug_server: TeamDebugHandler::Init(): Failed to init "
			"debug context for team %ld, port %ld: %s\n", fTeam, nubPort,
			strerror(error));
		return error;
	}

	// set team flags
	debug_nub_set_team_flags message;
	message.flags = B_TEAM_DEBUG_PREVENT_EXIT;

	send_debug_message(&fDebugContext, B_DEBUG_MESSAGE_SET_TEAM_FLAGS, &message,
		sizeof(message), NULL, 0);

	// create the message count semaphore
	char name[B_OS_NAME_LENGTH];
	snprintf(name, sizeof(name), "team %ld message count", fTeam);
	fMessageCountSem = create_sem(0, name);
	if (fMessageCountSem < 0) {
		debug_printf("debug_server: TeamDebugHandler::Init(): Failed to create "
			"message count semaphore: %s\n", strerror(fMessageCountSem));
		return fMessageCountSem;
	}

	// spawn the handler thread
	snprintf(name, sizeof(name), "team %ld handler", fTeam);
	fHandlerThread = spawn_thread(&_HandlerThreadEntry, name, B_NORMAL_PRIORITY,
		this);
	if (fHandlerThread < 0) {
		debug_printf("debug_server: TeamDebugHandler::Init(): Failed to spawn "
			"handler thread: %s\n", strerror(fHandlerThread));
		return fHandlerThread;
	}

	resume_thread(fHandlerThread);

	return B_OK;
}


team_id
TeamDebugHandler::Team() const
{
	return fTeam;
}


status_t
TeamDebugHandler::PushMessage(DebugMessage *message)
{
	BAutolock _(this);

	fMessages.Add(message);
	release_sem(fMessageCountSem);

	return B_OK;
}


status_t
TeamDebugHandler::_PopMessage(DebugMessage *&message)
{
	// acquire the semaphore
	status_t error;
	do {
		error = acquire_sem(fMessageCountSem);
	} while (error == B_INTERRUPTED);

	if (error != B_OK)
		return error;

	// get the message
	BAutolock _(this);

	message = fMessages.Head();
	fMessages.Remove(message);

	return B_OK;
}


status_t
TeamDebugHandler::_SetupGDBArguments(BStringList &arguments, bool usingConsoled)
{
	// prepare the argument vector
	BString teamString;
	teamString.SetToFormat("--pid=%ld", fTeam);

	status_t error;
	BPath terminalPath;
	if (usingConsoled) {
		error = find_directory(B_SYSTEM_BIN_DIRECTORY, &terminalPath);
		if (error != B_OK) {
			debug_printf("debug_server: can't find system-bin directory: %s\n",
				strerror(error));
			return error;
		}
		error = terminalPath.Append("consoled");
		if (error != B_OK) {
			debug_printf("debug_server: can't append to system-bin path: %s\n",
				strerror(error));
			return error;
		}
	} else {
		error = find_directory(B_SYSTEM_APPS_DIRECTORY, &terminalPath);
		if (error != B_OK) {
			debug_printf("debug_server: can't find system-apps directory: %s\n",
				strerror(error));
			return error;
		}
		error = terminalPath.Append("Terminal");
		if (error != B_OK) {
			debug_printf("debug_server: can't append to system-apps path: %s\n",
				strerror(error));
			return error;
		}
	}

	arguments.MakeEmpty();
	if (!arguments.Add(terminalPath.Path()))
		return B_NO_MEMORY;

	if (!usingConsoled) {
		BString windowTitle;
		windowTitle.SetToFormat("Debug of Team %ld: %s", fTeam,
			_LastPathComponent(fExecutablePath));
		if (!arguments.Add("-t") || !arguments.Add(windowTitle))
			return B_NO_MEMORY;
	}

	BPath gdbPath;
	error = find_directory(B_SYSTEM_BIN_DIRECTORY, &gdbPath);
	if (error != B_OK) {
		debug_printf("debug_server: can't find system-bin directory: %s\n",
			strerror(error));
		return error;
	}
	error = gdbPath.Append("gdb");
	if (error != B_OK) {
		debug_printf("debug_server: can't append to system-bin path: %s\n",
			strerror(error));
		return error;
	}
	if (!arguments.Add(gdbPath.Path()) || !arguments.Add(teamString))
		return B_NO_MEMORY;

	if (strlen(fExecutablePath) > 0 && !arguments.Add(fExecutablePath))
		return B_NO_MEMORY;

	return B_OK;
}


thread_id
TeamDebugHandler::_EnterDebugger()
{
	TRACE(("debug_server: TeamDebugHandler::_EnterDebugger(): team %ld\n",
		fTeam));

	// prepare a debugger handover
	TRACE(("debug_server: TeamDebugHandler::_EnterDebugger(): preparing "
		"debugger handover for team %ld...\n", fTeam));

	status_t error = send_debug_message(&fDebugContext,
		B_DEBUG_MESSAGE_PREPARE_HANDOVER, NULL, 0, NULL, 0);
	if (error != B_OK) {
		debug_printf("debug_server: Failed to prepare debugger handover: %s\n",
			strerror(error));
		return error;
	}

	BStringList arguments;
	const char *argv[16];
	int argc = 0;

	bool debugInConsoled = _IsGUIServer() || !_AreGUIServersAlive();
#ifdef HANDOVER_USE_GDB

	error = _SetupGDBArguments(arguments, debugInConsoled);
	if (error != B_OK) {
		debug_printf("debug_server: Failed to set up gdb arguments: %s\n",
			strerror(error));
		return error;
	}

	// start the terminal
	TRACE(("debug_server: TeamDebugHandler::_EnterDebugger(): starting  "
		"terminal (debugger) for team %ld...\n", fTeam));

#elif defined(HANDOVER_USE_DEBUGGER)
	if (debugInConsoled) {
		error = _SetupGDBArguments(arguments, debugInConsoled);
		if (error != B_OK) {
			debug_printf("debug_server: Failed to set up gdb arguments: %s\n",
				strerror(error));
			return error;
		}
	} else {
		// prepare the argument vector
		BPath debuggerPath;
		error = find_directory(B_SYSTEM_APPS_DIRECTORY, &debuggerPath);
		if (error != B_OK) {
			debug_printf("debug_server: can't find system-apps directory: %s\n",
				strerror(error));
			return error;
		}
		error = debuggerPath.Append("Debugger");
		if (error != B_OK) {
			debug_printf("debug_server: can't append to system-apps path: %s\n",
				strerror(error));
			return error;
		}
		if (!arguments.Add(debuggerPath.Path()))
			return B_NO_MEMORY;

		BString debuggerParam;
		debuggerParam.SetToFormat("%ld", fTeam);
		if (!arguments.Add("--team") || !arguments.Add(debuggerParam))
			return B_NO_MEMORY;

		// start the debugger
		TRACE(("debug_server: TeamDebugHandler::_EnterDebugger(): starting  "
			"graphical debugger for team %ld...\n", fTeam));
	}
#endif

	for (int32 i = 0; i < arguments.CountStrings(); i++)
		argv[argc++] = arguments.StringAt(i).String();
	argv[argc] = NULL;

	thread_id thread = load_image(argc, argv, (const char**)environ);
	if (thread < 0) {
		debug_printf("debug_server: Failed to start debugger: %s\n",
			strerror(thread));
		return thread;
	}
	resume_thread(thread);

	TRACE(("debug_server: TeamDebugHandler::_EnterDebugger(): debugger started "
		"for team %ld: thread: %ld\n", fTeam, thread));

	return thread;
}


void
TeamDebugHandler::_KillTeam()
{
	KillTeam(fTeam, fTeamInfo.args);
}


bool
TeamDebugHandler::_HandleMessage(DebugMessage *message)
{
	// This method is called only for the first message the debugger gets for
	// a team. That means only a few messages are actually possible, while
	// others wouldn't trigger the debugger in the first place. So we deal with
	// all of them the same way, by popping up an alert.
	TRACE(("debug_server: TeamDebugHandler::_HandleMessage(): team %ld, code: "
		"%ld\n", fTeam, (int32)message->Code()));

	thread_id thread = message->Data().origin.thread;

	// get some user-readable message
	char buffer[512];
	switch (message->Code()) {
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			// This shouldn't happen.
			debug_printf("debug_server: Got a spurious "
				"B_DEBUGGER_MESSAGE_TEAM_DELETED message for team %ld\n",
				fTeam);
			return true;

		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
			get_debug_exception_string(
				message->Data().exception_occurred.exception, buffer,
				sizeof(buffer));
			break;

		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
		{
			// get the debugger() message
			void *messageAddress = message->Data().debugger_call.message;
			char messageBuffer[128];
			status_t error = B_OK;
			ssize_t bytesRead = debug_read_string(&fDebugContext,
				messageAddress, messageBuffer, sizeof(messageBuffer));
			if (bytesRead < 0)
				error = bytesRead;

			if (error == B_OK) {
				sprintf(buffer, "Debugger call: `%s'", messageBuffer);
			} else {
				snprintf(buffer, sizeof(buffer), "Debugger call: %p "
					"(Failed to read message: %s)", messageAddress,
					strerror(error));
			}
			break;
		}

		default:
			get_debug_message_string(message->Code(), buffer, sizeof(buffer));
			break;
	}

	debug_printf("debug_server: Thread %ld entered the debugger: %s\n", thread,
		buffer);

	_PrintStackTrace(thread);

	bool kill = true;

	// ask the user whether to debug or kill the team
	if (_IsGUIServer()) {
		// App server, input server, or registrar. We always debug those.
		kill = false;
	} else if (USE_GUI && _AreGUIServersAlive() && _InitGUI() == B_OK) {
		// normal app -- tell the user
		_NotifyAppServer(fTeam);
		_NotifyRegistrar(fTeam, true, false);

		BString buffer(
			B_TRANSLATE("The application:\n\n      %app\n\n"
			"has encountered an error which prevents it from continuing. Haiku "
			"will terminate the application and clean up."));
		buffer.ReplaceFirst("%app", fTeamInfo.args);

		// TODO: It would be nice if the alert would go away automatically
		// if someone else kills our teams.
		BAlert *alert = new BAlert(NULL, buffer.String(),
			B_TRANSLATE("Debug"), B_TRANSLATE("OK"), NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		int32 result = alert->Go();
		kill = (result == 1);
		_NotifyRegistrar(fTeam, false, !kill);
	}

	return kill;
}


void
TeamDebugHandler::_LookupSymbolAddress(
	debug_symbol_lookup_context *lookupContext, const void *address,
	char *buffer, int32 bufferSize)
{
	// lookup the symbol
	void *baseAddress;
	char symbolName[1024];
	char imageName[B_PATH_NAME_LENGTH];
	bool exactMatch;
	bool lookupSucceeded = false;
	if (lookupContext) {
		status_t error = debug_lookup_symbol_address(lookupContext, address,
			&baseAddress, symbolName, sizeof(symbolName), imageName,
			sizeof(imageName), &exactMatch);
		lookupSucceeded = (error == B_OK);
	}

	if (lookupSucceeded) {
		// we were able to look something up
		if (strlen(symbolName) > 0) {
			// we even got a symbol
			snprintf(buffer, bufferSize, "%s + %#lx%s", symbolName,
				(addr_t)address - (addr_t)baseAddress,
				(exactMatch ? "" : " (closest symbol)"));

		} else {
			// no symbol: image relative address
			snprintf(buffer, bufferSize, "(%s + %#lx)", imageName,
				(addr_t)address - (addr_t)baseAddress);
		}

	} else {
		// lookup failed: find area containing the IP
		bool useAreaInfo = false;
		area_info info;
		ssize_t cookie = 0;
		while (get_next_area_info(fTeam, &cookie, &info) == B_OK) {
			if ((addr_t)info.address <= (addr_t)address
				&& (addr_t)info.address + info.size > (addr_t)address) {
				useAreaInfo = true;
				break;
			}
		}

		if (useAreaInfo) {
			snprintf(buffer, bufferSize, "(%s + %#lx)", info.name,
				(addr_t)address - (addr_t)info.address);
		} else if (bufferSize > 0)
			buffer[0] = '\0';
	}
}


void
TeamDebugHandler::_PrintStackTrace(thread_id thread)
{
	// print a stacktrace
	void *ip = NULL;
	void *stackFrameAddress = NULL;
	status_t error = debug_get_instruction_pointer(&fDebugContext, thread, &ip,
		&stackFrameAddress);

	if (error == B_OK) {
		// create a symbol lookup context
		debug_symbol_lookup_context *lookupContext = NULL;
		error = debug_create_symbol_lookup_context(fTeam, &lookupContext);
		if (error != B_OK) {
			debug_printf("debug_server: Failed to create symbol lookup "
				"context: %s\n", strerror(error));
		}

		// lookup the IP
		char symbolBuffer[2048];
		_LookupSymbolAddress(lookupContext, ip, symbolBuffer,
			sizeof(symbolBuffer) - 1);

		debug_printf("stack trace, current PC %p  %s:\n", ip, symbolBuffer);

		for (int32 i = 0; i < 50; i++) {
			debug_stack_frame_info stackFrameInfo;

			error = debug_get_stack_frame(&fDebugContext, stackFrameAddress,
				&stackFrameInfo);
			if (error < B_OK || stackFrameInfo.parent_frame == NULL)
				break;

			// lookup the return address
			_LookupSymbolAddress(lookupContext, stackFrameInfo.return_address,
				symbolBuffer, sizeof(symbolBuffer) - 1);

			debug_printf("  (%p)  %p  %s\n", stackFrameInfo.frame,
				stackFrameInfo.return_address, symbolBuffer);

			stackFrameAddress = stackFrameInfo.parent_frame;
		}

		// delete the symbol lookup context
		if (lookupContext)
			debug_delete_symbol_lookup_context(lookupContext);
	}
}


void
TeamDebugHandler::_NotifyAppServer(team_id team)
{
	// This will remove any kWindowScreenFeels of the application, so that
	// the debugger alert is visible on screen
	BRoster::Private roster;
	roster.ApplicationCrashed(team);
}


void
TeamDebugHandler::_NotifyRegistrar(team_id team, bool openAlert,
	bool stopShutdown)
{
	BMessage notify(BPrivate::B_REG_TEAM_DEBUGGER_ALERT);
	notify.AddInt32("team", team);
	notify.AddBool("open", openAlert);
	notify.AddBool("stop shutdown", stopShutdown);

	BRoster::Private roster;
	BMessage reply;
	roster.SendTo(&notify, &reply, false);
}


status_t
TeamDebugHandler::_InitGUI()
{
	DebugServer *app = dynamic_cast<DebugServer*>(be_app);
	BAutolock _(app);
	return app->InitGUIContext();
}


status_t
TeamDebugHandler::_HandlerThreadEntry(void *data)
{
	return ((TeamDebugHandler*)data)->_HandlerThread();
}


status_t
TeamDebugHandler::_HandlerThread()
{
	TRACE(("debug_server: TeamDebugHandler::_HandlerThread(): team %ld\n",
		fTeam));

	// get initial message
	TRACE(("debug_server: TeamDebugHandler::_HandlerThread(): team %ld: "
		"getting message...\n", fTeam));

	DebugMessage *message;
	status_t error = _PopMessage(message);
	bool kill;
	if (error == B_OK) {
		// handle the message
		kill = _HandleMessage(message);
		delete message;
	} else {
		debug_printf("TeamDebugHandler::_HandlerThread(): Failed to pop "
			"initial message: %s", strerror(error));
		kill = true;
	}

	bool isGuiServer = _IsGUIServer();

	// kill the team or hand it over to the debugger
	thread_id debuggerThread = -1;
	if (kill) {
		// The team shall be killed. Since that is also the handling in case
		// an error occurs while handing over the team to the debugger, we do
		// nothing here.
	} else if ((debuggerThread = _EnterDebugger()) >= 0) {
		// wait for the "handed over" or a "team deleted" message
		bool terminate = false;
		do {
			error = _PopMessage(message);
			if (error != B_OK) {
				debug_printf("TeamDebugHandler::_HandlerThread(): Failed to "
					"pop message: %s", strerror(error));
				kill = true;
				break;
			}

			if (message->Code() == B_DEBUGGER_MESSAGE_HANDED_OVER) {
				// The team has successfully been handed over to the debugger.
				// Nothing to do.
				terminate = true;
			} else if (message->Code() == B_DEBUGGER_MESSAGE_TEAM_DELETED) {
				// The team died. Nothing to do.
				terminate = true;
			} else {
				// Some message we can ignore. The debugger will take care of
				// it.

				// check whether the debugger thread still lives
				thread_info threadInfo;
				if (get_thread_info(debuggerThread, &threadInfo) != B_OK) {
					// the debugger is gone
					debug_printf("debug_server: The debugger for team %ld "
						"seems to be gone.", fTeam);

					kill = true;
					terminate = true;
				}
			}

			delete message;
		} while (!terminate);
	} else
		kill = true;

	if (kill) {
		// kill the team
		_KillTeam();
	}

	// remove this handler from the roster and delete it
	TeamDebugHandlerRoster::Default()->RemoveHandler(fTeam);

	if (isGuiServer) {
		// wait till debugging is done
		status_t dummy;
		wait_for_thread(debuggerThread, &dummy);

		// find the registrar port
		port_id rosterPort = find_port(BPrivate::get_roster_port_name());
		port_info info;
		BMessenger messenger;
		if (rosterPort >= 0 && get_port_info(rosterPort, &info) == B_OK) {
			BMessenger::Private(messenger).SetTo(info.team, rosterPort,
				B_PREFERRED_TOKEN);
		}
		messenger.SendMessage(kMsgRestartAppServer);
	}

	delete this;

	return B_OK;
}


bool
TeamDebugHandler::_ExecutableNameEquals(const char *name) const
{
	return strcmp(_LastPathComponent(fExecutablePath), name) == 0;
}


bool
TeamDebugHandler::_IsAppServer() const
{
	return _ExecutableNameEquals("app_server");
}


bool
TeamDebugHandler::_IsInputServer() const
{
	return _ExecutableNameEquals("input_server");
}


bool
TeamDebugHandler::_IsRegistrar() const
{
	return _ExecutableNameEquals("registrar");
}


bool
TeamDebugHandler::_IsGUIServer() const
{
	// app or input server
	return _IsAppServer() || _IsInputServer() || _IsRegistrar();
}


const char *
TeamDebugHandler::_LastPathComponent(const char *path)
{
	const char *lastSlash = strrchr(path, '/');
	return lastSlash ? lastSlash + 1 : path;
}


team_id
TeamDebugHandler::_FindTeam(const char *name)
{
	// Iterate through all teams and check their executable name.
	int32 cookie = 0;
	team_info teamInfo;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		entry_ref ref;
		if (BPrivate::get_app_ref(teamInfo.team, &ref) == B_OK) {
			if (strcmp(ref.name, name) == 0)
				return teamInfo.team;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


bool
TeamDebugHandler::_AreGUIServersAlive()
{
	return _FindTeam("app_server") >= 0 && _FindTeam("input_server") >= 0
		&& _FindTeam("registrar");
}


// #pragma mark -


DebugServer::DebugServer(status_t &error)
	:
	BServer(kSignature, false, &error),
	fListenerPort(-1),
	fListener(-1),
	fTerminating(false)
{
}


status_t
DebugServer::Init()
{
	// create listener port
	fListenerPort = create_port(10, "kernel listener");
	if (fListenerPort < 0)
		return fListenerPort;

	// spawn the listener thread
	fListener = spawn_thread(_ListenerEntry, "kernel listener",
		B_NORMAL_PRIORITY, this);
	if (fListener < 0)
		return fListener;

	// register as default debugger
	// TODO: could set default flags
	status_t error = install_default_debugger(fListenerPort);
	if (error != B_OK)
		return error;

	// resume the listener
	resume_thread(fListener);

	return B_OK;
}


bool
DebugServer::QuitRequested()
{
	// Never give up, never surrender. ;-)
	return false;
}


status_t
DebugServer::_ListenerEntry(void *data)
{
	return ((DebugServer*)data)->_Listener();
}


status_t
DebugServer::_Listener()
{
	while (!fTerminating) {
		// receive the next debug message
		DebugMessage *message = new DebugMessage;
		int32 code;
		ssize_t bytesRead;
		do {
			bytesRead = read_port(fListenerPort, &code, &message->Data(),
				sizeof(debug_debugger_message_data));
		} while (bytesRead == B_INTERRUPTED);

		if (bytesRead < 0) {
			debug_printf("debug_server: Failed to read from listener port: "
				"%s. Terminating!\n", strerror(bytesRead));
			exit(1);
		}
TRACE(("debug_server: Got debug message: team: %ld, code: %ld\n",
message->Data().origin.team, code));

		message->SetCode((debug_debugger_message)code);

		// dispatch the message
		TeamDebugHandlerRoster::Default()->DispatchMessage(message);
	}

	return B_OK;
}


// #pragma mark -


int
main()
{
	status_t error;

	// for the time being let the debug server print to the syslog
	int console = open("/dev/dprintf", O_RDONLY);
	if (console < 0) {
		debug_printf("debug_server: Failed to open console: %s\n",
			strerror(errno));
	}
	dup2(console, STDOUT_FILENO);
	dup2(console, STDERR_FILENO);
	close(console);

	// create the team debug handler roster
	if (!TeamDebugHandlerRoster::CreateDefault()) {
		debug_printf("debug_server: Failed to create team debug handler "
			"roster.\n");
		exit(1);
	}

	// create application
	DebugServer server(error);
	if (error != B_OK) {
		debug_printf("debug_server: Failed to create BApplication: %s\n",
			strerror(error));
		exit(1);
	}

	// init application
	error = server.Init();
	if (error != B_OK) {
		debug_printf("debug_server: Failed to init application: %s\n",
			strerror(error));
		exit(1);
	}

	server.Run();

	return 0;
}

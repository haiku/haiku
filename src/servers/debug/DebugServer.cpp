/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <map>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Application.h>
#include <Invoker.h>
#include <debugger.h>

#include <util/DoublyLinkedList.h>

static const char *kSignature = "application/x-vnd.haiku-debug-server";

// message what codes
enum {
	DEBUG_MESSAGE	= 'dbgm',
	ALERT_MESSAGE	= 'alrt',
};

class DebugServer;
class TeamDebugHandler;

// DebugMessage
class DebugMessage : public DoublyLinkedListLinkImpl<DebugMessage> {
public:
	DebugMessage()
	{
	}

	void SetCode(int32 code)						{ fCode = code; }
	int32 Code() const								{ return fCode; }

	debug_debugger_message_data &Data()				{ return fData; }
	const debug_debugger_message_data &Data() const	{ return fData; }

private:
	int32						fCode;
	debug_debugger_message_data	fData;
};

typedef DoublyLinkedList<DebugMessage>	DebugMessageList;

// ThreadDebugHandler
class ThreadDebugHandler : public BHandler {
public:
	ThreadDebugHandler(TeamDebugHandler *teamHandler, thread_id thread);
	~ThreadDebugHandler();

	thread_id Thread() const;

	bool HandleMessage(DebugMessage *message);

	virtual void MessageReceived(BMessage *message);

private:
	TeamDebugHandler	*fTeamHandler;
	thread_id			fThread;
	BInvoker			*fInvoker;
};


// TeamDebugHandler
class TeamDebugHandler {
public:
	TeamDebugHandler(DebugServer *debugServer, team_id team);
	~TeamDebugHandler();

	team_id Team() const;

	bool HandleMessage(DebugMessage *message);

	void EnterDebugger();
	void KillTeam();

private:
	void _ContinueThread(thread_id thread);
	void _DeleteThreadHandler(ThreadDebugHandler *handler);

private:
	typedef map<team_id, ThreadDebugHandler*>	ThreadDebugHandlerMap;

	DebugServer				*fDebugServer;
	team_id					fTeam;
	port_id					fNubPort;
	ThreadDebugHandlerMap	fThreadDebugHandlers;
};

// DebugServer
class DebugServer : public BApplication {
public:
	DebugServer(status_t &error);

	status_t Init();

	virtual void MessageReceived(BMessage *message);

	void EnterDebugger(TeamDebugHandler *handler);
	void KillTeam(TeamDebugHandler *handler);

private:
	static status_t _ListenerEntry(void *data);
	status_t _Listener();

	void _DeleteTeamDebugHandler(TeamDebugHandler *handler);

private:
	typedef map<team_id, TeamDebugHandler*>	TeamDebugHandlerMap;

	port_id				fListenerPort;
	thread_id			fListener;
	bool				fTerminating;

	TeamDebugHandlerMap	fTeamDebugHandlers;
	DebugMessageList	fDebugMessages;
};


// #pragma mark -

// constructor
ThreadDebugHandler::ThreadDebugHandler(TeamDebugHandler *teamHandler,
	thread_id thread)
	: fTeamHandler(teamHandler),
	  fThread(thread),
	  fInvoker(NULL)
{
}

// destructor
ThreadDebugHandler::~ThreadDebugHandler()
{
	delete fInvoker;
}

// Thread
thread_id
ThreadDebugHandler::Thread() const
{
	return fThread;
}

// HandleMessage
bool
ThreadDebugHandler::HandleMessage(DebugMessage *message)
{
	if (!fInvoker)
		fInvoker = new BInvoker(new BMessage(ALERT_MESSAGE), this);

	// get some user-readable message
	char buffer[512];
	if (message->Code() == B_DEBUGGER_MESSAGE_THREAD_STOPPED) {
		get_debug_why_stopped_string(message->Data().thread_stopped.why, buffer,
			sizeof(buffer));
	} else if (message->Code() == B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED) {
		get_debug_exception_string(message->Data().exception_occurred.exception,
			buffer, sizeof(buffer));
	} else {
		// We shouldn't be here.
		sprintf(buffer, "Debug messages %ld", (int32)message->Code());
	}

	// TODO: This would be the point to pop up an asynchronous alert.
	// We just print the error and send a message to ourselves.
	printf("debug_server: Thread %ld entered the debugger: %s\n", fThread,
		buffer);
	BMessage alertMessage(*fInvoker->Message());
	alertMessage.AddInt32("which", 1);
	fInvoker->Invoke(&alertMessage);

	return false;
}

// MessageReceived
void
ThreadDebugHandler::MessageReceived(BMessage *message)
{
	if (message->what == ALERT_MESSAGE) {
printf("debug_server: ALERT_MESSAGE received for thread %ld\n", fThread);
		int32 which;
		if (message->FindInt32("which", &which) != B_OK)
			which = 1;

		if (which == 0)
			fTeamHandler->EnterDebugger();
		else
			fTeamHandler->KillTeam();
	}
}


// #pragma mark -

// constructor
TeamDebugHandler::TeamDebugHandler(DebugServer *debugServer, team_id team)
	: fDebugServer(debugServer),
	  fTeam(team),
	  fNubPort(-1),
	  fThreadDebugHandlers()
{
}

// destructor
TeamDebugHandler::~TeamDebugHandler()
{
	// delete all thread debug handlers.
	for (ThreadDebugHandlerMap::iterator it = fThreadDebugHandlers.begin();
		 it != fThreadDebugHandlers.end();
		 it = fThreadDebugHandlers.begin()) {
		_DeleteThreadHandler(it->second);
	}
}

// Team
team_id
TeamDebugHandler::Team() const
{
	return fTeam;
}

// HandleMessage
bool
TeamDebugHandler::HandleMessage(DebugMessage *message)
{
	thread_id thread = message->Data().origin.thread;

	switch (message->Code()) {
		case B_DEBUGGER_MESSAGE_THREAD_STOPPED:
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
		{
			fNubPort = message->Data().origin.nub_port;

			// create a thread debug handler
			ThreadDebugHandler *handler
				= new ThreadDebugHandler(this, thread);
			fThreadDebugHandlers[thread] = handler;
			fDebugServer->AddHandler(handler);

			// let the handler deal with the message
			if (handler->HandleMessage(message))
				_DeleteThreadHandler(handler);

			break;
		}

		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			return true;

		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			// a thread is gone: nothing to do
			break;

		case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
			// A signal doesn't usually trigger the debugger.
			// Just let the thread continue and see what happens next.
		case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
		case B_DEBUGGER_MESSAGE_POST_SYSCALL:
		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
			fNubPort = message->Data().origin.nub_port;
			_ContinueThread(thread);
			break;
	}

	return fThreadDebugHandlers.empty();
}

// EnterDebugger
void
TeamDebugHandler::EnterDebugger()
{
	fDebugServer->EnterDebugger(this);
}

// KillTeam
void
TeamDebugHandler::KillTeam()
{
	fDebugServer->KillTeam(this);
}

// _ContinueThread
void
TeamDebugHandler::_ContinueThread(thread_id thread)
{
	debug_nub_run_thread message;
	message.thread = thread;
	message.handle_event = B_THREAD_DEBUG_HANDLE_EVENT;

	while (true) {
		status_t error = write_port(fNubPort, B_DEBUG_MESSAGE_RUN_THREAD,
			&message, sizeof(message));
		if (error == B_OK)
			return;

		if (error != B_INTERRUPTED) {
			fprintf(stderr, "debug_server: Failed to run thread %ld: %s\n",
				thread, strerror(error));
			return;
		}
	}
}

// _DeleteThreadHandler
void
TeamDebugHandler::_DeleteThreadHandler(ThreadDebugHandler *handler)
{
	if (!handler)
		return;

	// remove handler from the map
	thread_id thread = handler->Thread();
	ThreadDebugHandlerMap::iterator it = fThreadDebugHandlers.find(thread);
	if (it != fThreadDebugHandlers.end())
		fThreadDebugHandlers.erase(it);

	// remove handler from the debug server
	if (handler->Looper())
		handler->Looper()->RemoveHandler(handler);

	delete handler;
}


// #pragma mark -

// constructor
DebugServer::DebugServer(status_t &error)
	: BApplication(kSignature, &error),
	  fListenerPort(-1),
	  fListener(-1),
	  fTerminating(false),
	  fTeamDebugHandlers(),
	  fDebugMessages()
{
}

// Init
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
	status_t error = install_default_debugger(fListenerPort);
	if (error != B_OK)
		return error;

	// resume the listener
	resume_thread(fListener);

	return B_OK;
}

// MessageReceived
void
DebugServer::MessageReceived(BMessage *message)
{
	if (message->what == DEBUG_MESSAGE) {
		// get the next debug message
		if (DebugMessage *message = fDebugMessages.Head()) {
			fDebugMessages.Remove(message);

			// get the responsible team debug handler
			team_id team = message->Data().origin.team;
			TeamDebugHandlerMap::iterator it
				= fTeamDebugHandlers.find(team);
			TeamDebugHandler *handler;
			if (it == fTeamDebugHandlers.end()) {
				handler = new TeamDebugHandler(this, team);
				fTeamDebugHandlers[team] = handler;
			} else
				handler = it->second;

			// let the handler deal with the message
			if (handler->HandleMessage(message))
				_DeleteTeamDebugHandler(handler);

			delete message;
		}
	} else
		BApplication::MessageReceived(message);
}

// EnterDebugger
void
DebugServer::EnterDebugger(TeamDebugHandler *handler)
{
	// TODO: Start gdb and hand over the team.
	KillTeam(handler);
}

// KillTeam
void
DebugServer::KillTeam(TeamDebugHandler *handler)
{

	team_id team = handler->Team();

	fprintf(stderr, "debug_server: Killing team %ld\n", team);

	kill_team(team);

	_DeleteTeamDebugHandler(handler);

	// TODO: We should actually iterate through the debug message list and
	// remove all messages for the team.
}

// _ListenerEntry
status_t
DebugServer::_ListenerEntry(void *data)
{
	return ((DebugServer*)data)->_Listener();
}

// _Listener
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
			fprintf(stderr, "debug_server: Failed to read from listener port: "
				"%s\n", strerror(bytesRead));
			exit(1);
		}

		message->SetCode(code);

		// queue the message and send notify the app
		Lock();
		fDebugMessages.Add(message);
		Unlock();

		PostMessage(DEBUG_MESSAGE);
	}

	return B_OK;
}

// _DeleteTeamDebugHandler
void
DebugServer::_DeleteTeamDebugHandler(TeamDebugHandler *handler)
{
	if (!handler)
		return;

	// remove handler from the map
	team_id team = handler->Team();
	TeamDebugHandlerMap::iterator it = fTeamDebugHandlers.find(team);
	if (it != fTeamDebugHandlers.end())
		fTeamDebugHandlers.erase(it);
	
	delete handler;
}


// #pragma mark -

// main
int
main()
{
	status_t error;

	// for the time being let the debug server print to the syslog
	int console = open("/dev/dprintf", O_RDONLY);
	if (console < 0) {
		fprintf(stderr, "debug_server: Failed to open console: %s\n",
			strerror(errno));
	}
	dup2(console, STDOUT_FILENO);
	dup2(console, STDERR_FILENO);
	close(console);

	// create application
	DebugServer server(error);
	if (error != B_OK) {
		fprintf(stderr, "debug_server: Failed to create BApplication: %s\n",
			strerror(error));
		exit(1);
	}

	// init application
	error = server.Init();
	if (error != B_OK) {
		fprintf(stderr, "debug_server: Failed to init application: %s\n",
			strerror(error));
		exit(1);
	}

	server.Run();

	return 0;
}

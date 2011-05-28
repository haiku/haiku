// AppRunner.cpp

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <Autolock.h>
#include <Entry.h>
#include <Messenger.h>
#include <String.h>

#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

#include "AppRunner.h"

static const char *kAppRunnerTeamPort = "app runner team port";

// constructor
AppRunner::AppRunner(bool requestQuitOnDestruction)
		 : fOutputLock(),
		   fRemotePort(-1),
		   fOutput(),
		   fReader(-1),
		   fTeam(-1),
		   fRef(),
		   fMessenger(),
		   fRequestQuitOnDestruction(requestQuitOnDestruction)
{
}

// destructor
AppRunner::~AppRunner()
{
	if (fRequestQuitOnDestruction)
		WaitFor(true);
	if (fReader >= 0) {
		int32 result;
		wait_for_thread(fReader, &result);
	}
}

// Run
status_t
AppRunner::Run(const char *command, const char *args, bool findCommand)
{
	status_t error = (HasQuitted() ? B_OK : B_ERROR);
	// get the app path
	BString appPath;
	if (findCommand) {
		find_test_app(command, &appPath);
		command = appPath.String();
	}
	get_ref_for_path(command, &fRef);
	// add args, i.e. compose the command line
	BString cmdLine(command);
	if (args) {
		cmdLine += " ";
		cmdLine += args;
	}
	// lock the team port
	bool teamPortLocked = false;
	if (error == B_OK) {
		teamPortLocked = _LockTeamPort();
		if (!teamPortLocked)
			error = B_ERROR;
	}
	// run the command
	if (error == B_OK) {
		cmdLine += " &";
		if (system(cmdLine.String()) != 0)
			error = errno;
	}
	// read the port ID
	if (error == B_OK) {
		fRemotePort = _ReadPortID(fMessenger);
		if (fRemotePort < 0)
			error = fRemotePort;
	}
	// unlock the team port
	if (teamPortLocked)
		_UnlockTeamPort();
	// get the team ID
	if (error == B_OK) {
		port_info info;
		error = get_port_info(fRemotePort, &info);
		fTeam = info.team;
	}
	// spawn the reader thread
	if (error == B_OK) {
		fReader = spawn_thread(&_ReaderEntry, "AppRunner reader",
							   B_NORMAL_PRIORITY, (void*)this);
		if (fReader >= 0)
			error = resume_thread(fReader);
		else
			error = fReader;
	}
	// cleanup on error
	if (error != B_OK) {
		if (fReader >= 0) {
			kill_thread(fReader);
			fReader = -1;
		}
	}
	return error;
}

// HasQuitted
bool
AppRunner::HasQuitted()
{
	port_info info;
	return (get_port_info(fRemotePort, &info) != B_OK);
}

// WaitFor
void
AppRunner::WaitFor(bool requestQuit)
{
	if (!HasQuitted() && requestQuit)
		RequestQuit();
	while (!HasQuitted())
		snooze(10000);
}

// Team
team_id
AppRunner::Team()
{
	return fTeam;
}

// AppLooperPort
port_id
AppRunner::AppLooperPort()
{
	struct messenger_hack {
		port_id	fPort;
		int32	fHandlerToken;
		team_id	fTeam;
		int32	extra0;
		int32	extra1;
		bool	fPreferredTarget;
		bool	extra2;
		bool	extra3;
		bool	extra4;
	};
	return ((messenger_hack*)&fMessenger)->fPort;
}

// GetRef
status_t
AppRunner::GetRef(entry_ref *ref)
{
	status_t error = (ref ? B_OK : B_ERROR);
	if (error == B_OK)
		*ref = fRef;
	return error;
}

// RequestQuit
status_t
AppRunner::RequestQuit()
{
	status_t error = B_OK;
	if (fTeam >= 0) {
		BMessenger messenger(fMessenger);
		error = messenger.SendMessage(B_QUIT_REQUESTED);
	} else
		error = fTeam;
	return error;
}

// GetOutput
status_t
AppRunner::GetOutput(BString *buffer)
{
	status_t error = (buffer ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BAutolock locker(fOutputLock);
		size_t size = fOutput.BufferLength();
		const void *output = fOutput.Buffer();
		if (size > 0)
			buffer->SetTo((const char*)output, size);
		else
			*buffer = "";
	}
	return error;
}

// ReadOutput
ssize_t
AppRunner::ReadOutput(void *buffer, size_t size)
{
	BAutolock locker(fOutputLock);
	return fOutput.Read(buffer, size);
}

// ReadOutputAt
ssize_t
AppRunner::ReadOutputAt(off_t position, void *buffer, size_t size)
{
	BAutolock locker(fOutputLock);
	return fOutput.ReadAt(position, buffer, size);
}

// _ReaderEntry
int32
AppRunner::_ReaderEntry(void *data)
{
	int32 result = 0;
	if (AppRunner *me = (AppRunner*)data)
		result = me->_ReaderLoop();
	return result;
}

// _ReaderLoop
int32
AppRunner::_ReaderLoop()
{
	char buffer[10240];
	port_id port = fRemotePort;
	status_t error = B_OK;
	while (error == B_OK) {
		int32 code;
		ssize_t bytes = read_port(port, &code, buffer, sizeof(buffer) - 1);
		if (bytes > 0) {
			// write the buffer
			BAutolock locker(fOutputLock);
			off_t oldPosition = fOutput.Seek(0, SEEK_END);
			fOutput.Write(buffer, bytes);
			fOutput.Seek(oldPosition, SEEK_SET);
		} else if (bytes < 0)
			error = bytes;
	}
	fRemotePort = -1;
	return 0;
}

// _LockTeamPort
bool
AppRunner::_LockTeamPort()
{
	bool result = fTeamPortLock.Lock();
	// lazy port creation
	if (result && fTeamPort < 0) {
		fTeamPort = create_port(5, kAppRunnerTeamPort);
		if (fTeamPort < 0) {
			fTeamPortLock.Unlock();
			result = false;
		}
	}
	return result;
}

// _UnlockTeamPort
void
AppRunner::_UnlockTeamPort()
{
	fTeamPortLock.Unlock();
}

// _ReadPortID
port_id
AppRunner::_ReadPortID(BMessenger &messenger)
{
	port_id port = -1;
	ssize_t size = read_port(fTeamPort, &port, &messenger, sizeof(BMessenger));
	if (size < 0)
		port = size;
	return port;
}


// fTeamPort
port_id	AppRunner::fTeamPort = -1;

// fTeamPortLock
BLocker	AppRunner::fTeamPortLock;


// find_test_app
status_t
find_test_app(const char *testApp, BString *path)
{
	status_t error = (testApp && path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		*path = BTestShell::GlobalTestDir();
		path->CharacterEscape(" \t\n!\"'`$&()?*+{}[]<>|", '\\');
		*path += "/../kits/app/";
		*path += testApp;
		#ifdef TEST_R5
			*path += "_r5";
		#endif
	}
	return error;
}

// find_test_app
status_t
find_test_app(const char *testApp, entry_ref *ref)
{
	status_t error = (testApp && ref ? B_OK : B_BAD_VALUE);
	BString path;
	if (error == B_OK)
		error = find_test_app(testApp, &path);
	if (error == B_OK)
		error = get_ref_for_path(path.String(), ref);
	return error;
}


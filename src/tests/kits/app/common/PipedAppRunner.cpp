// PipedAppRunner.cpp

#include <errno.h>
#include <unistd.h>

#include <Autolock.h>
#include <String.h>

#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

#include "PipedAppRunner.h"

// constructor
PipedAppRunner::PipedAppRunner()
			  : fOutputLock(),
				fPipe(NULL),
				fOutput(),
				fReader(-1)
{
}

// destructor
PipedAppRunner::~PipedAppRunner()
{
	if (fReader >= 0) {
		_ClosePipe();
		int32 result;
		wait_for_thread(fReader, &result);
	}
}

// Run
status_t
PipedAppRunner::Run(const char *command, const char *args, bool findCommand)
{
	status_t error = (HasQuitted() ? B_OK : B_ERROR);
	// get the app path
	BString appPath;
	if (findCommand) {
		appPath = BTestShell::GlobalTestDir();
		appPath.CharacterEscape(" \t\n!\"'`$&()?*+{}[]<>|", '\\');
		appPath += "/";
		appPath += command;
		#ifdef TEST_R5
			appPath += "_r5";
		#endif
		command = appPath.String();
	}
	// add args, i.e. compose the command line
	BString cmdLine(command);
	if (args) {
		cmdLine += " ";
		cmdLine += args;
	}
	// run the command
	if (error == B_OK) {
		fPipe = popen(cmdLine.String(), "r");
		if (!fPipe)
			error = errno;
	}
	// spawn the reader thread
	if (error == B_OK) {
		fReader = spawn_thread(&_ReaderEntry, "PipedAppRunner reader",
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
		if (fPipe) {
			pclose(fPipe);
			fPipe = NULL;
		}
	}
	return error;
}

// HasQuitted
bool
PipedAppRunner::HasQuitted()
{
	BAutolock locker(fOutputLock);
	return !fPipe;
}

// WaitFor
void
PipedAppRunner::WaitFor()
{
	while (!HasQuitted())
		snooze(10000);
}

// GetOutput
status_t
PipedAppRunner::GetOutput(BString *buffer)
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
PipedAppRunner::ReadOutput(void *buffer, size_t size)
{
	BAutolock locker(fOutputLock);
	return fOutput.Read(buffer, size);
}

// ReadOutputAt
ssize_t
PipedAppRunner::ReadOutputAt(off_t position, void *buffer, size_t size)
{
	BAutolock locker(fOutputLock);
	return fOutput.ReadAt(position, buffer, size);
}

// _ReaderEntry
int32
PipedAppRunner::_ReaderEntry(void *data)
{
	int32 result = 0;
	if (PipedAppRunner *me = (PipedAppRunner*)data)
		result = me->_ReaderLoop();
	return result;
}

// _ReaderLoop
int32
PipedAppRunner::_ReaderLoop()
{
	char buffer[10240];
	fOutputLock.Lock();
	FILE *pipe = fPipe;
	fOutputLock.Unlock();
	while (!feof(pipe)) {
		size_t bytes = fread(buffer, 1, sizeof(buffer), pipe);
		if (bytes > 0) {
			BAutolock locker(fOutputLock);
			off_t oldPosition = fOutput.Seek(0, SEEK_END);
			fOutput.Write(buffer, bytes);
			fOutput.Seek(oldPosition, SEEK_SET);
		}
	}
	_ClosePipe();
	return 0;
}

// _ClosePipe
void
PipedAppRunner::_ClosePipe()
{
	if (fOutputLock.Lock()) {
		if (fPipe) {
			pclose(fPipe);
			fPipe = NULL;
		}
		fOutputLock.Unlock();
	}
}


// AppRunner.cpp

#include <errno.h>
#include <unistd.h>

#include <Autolock.h>

#include "AppRunner.h"

// constructor
AppRunner::AppRunner()
		 : fOutputLock(),
		   fPipe(NULL),
		   fOutput(),
		   fReader(-1)
{
}

// destructor
AppRunner::~AppRunner()
{
	if (fReader >= 0) {
		_ClosePipe();
		int32 result;
		wait_for_thread(fReader, &result);
	}
}

// Run
status_t
AppRunner::Run(const char *command)
{
	status_t error = (HasQuitted() ? B_OK : B_ERROR);
	// run the command
	if (error == B_OK) {
		fPipe = popen(command, "r");
		if (!fPipe)
			error = errno;
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
		if (fPipe) {
			pclose(fPipe);
			fPipe = NULL;
		}
	}
	return error;
}

// HasQuitted
bool
AppRunner::HasQuitted()
{
	BAutolock locker(fOutputLock);
	return !fPipe;
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
AppRunner::_ClosePipe()
{
	if (fOutputLock.Lock()) {
		if (fPipe) {
			pclose(fPipe);
			fPipe = NULL;
		}
		fOutputLock.Unlock();
	}
}


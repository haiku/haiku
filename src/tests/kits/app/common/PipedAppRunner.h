// PipedAppRunner.h

#ifndef PIPED_APP_RUNNER_H
#define PIPED_APP_RUNNER_H

#include <stdio.h>

#include <DataIO.h>
#include <Locker.h>

class PipedAppRunner {
public:
	PipedAppRunner();
	~PipedAppRunner();

	status_t Run(const char *command, const char *args = NULL,
				 bool findCommand = true);
	bool HasQuitted();
	void WaitFor();

	status_t GetOutput(BString *buffer);
	ssize_t ReadOutput(void *buffer, size_t size);
	ssize_t ReadOutputAt(off_t position, void *buffer, size_t size);

private:
	static int32 _ReaderEntry(void *data);
	int32 _ReaderLoop();
	void _ClosePipe();

private:
	BLocker		fOutputLock;
	FILE		*fPipe;
	BMallocIO	fOutput;
	thread_id	fReader;
};

#endif	// PIPED_APP_RUNNER_H

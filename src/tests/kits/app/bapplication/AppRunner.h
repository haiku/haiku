// AppRunner.h

#ifndef APP_RUNNER_H
#define APP_RUNNER_H

#include <stdio.h>

#include <DataIO.h>
#include <Locker.h>

class AppRunner {
public:
	AppRunner();
	~AppRunner();

	status_t Run(const char *command);
	bool HasQuitted();

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

#endif	// APP_RUNNER_H

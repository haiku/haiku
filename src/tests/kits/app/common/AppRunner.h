// AppRunner.h

#ifndef APP_RUNNER_H
#define APP_RUNNER_H

#include <stdio.h>

#include <DataIO.h>
#include <Entry.h>
#include <Locker.h>

class AppRunner {
public:
	AppRunner(bool requestQuitOnDestruction = false);
	~AppRunner();

	status_t Run(const char *command, const char *args = NULL,
				 bool findCommand = true);
	bool HasQuitted();
	void WaitFor(bool requestQuit = false);
	team_id Team();
	port_id AppLooperPort();
	status_t GetRef(entry_ref *ref);
	status_t RequestQuit();

	status_t GetOutput(BString *buffer);
	ssize_t ReadOutput(void *buffer, size_t size);
	ssize_t ReadOutputAt(off_t position, void *buffer, size_t size);

private:
	static int32 _ReaderEntry(void *data);
	int32 _ReaderLoop();

	static bool _LockTeamPort();
	static void _UnlockTeamPort();
	static port_id _ReadPortID(BMessenger &messenger);

private:
	BLocker		fOutputLock;
	port_id		fRemotePort;
	BMallocIO	fOutput;
	thread_id	fReader;
	team_id		fTeam;
	entry_ref	fRef;
	BMessenger	fMessenger;
	bool		fRequestQuitOnDestruction;

	static port_id	fTeamPort;
	static BLocker	fTeamPortLock;
};

status_t find_test_app(const char *testApp, BString *path);
status_t find_test_app(const char *testApp, entry_ref *ref);

#endif	// APP_RUNNER_H

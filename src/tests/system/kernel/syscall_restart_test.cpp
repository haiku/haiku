#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <OS.h>


enum run_mode {
	RUN_IGNORE_SIGNAL,
	RUN_HANDLE_SIGNAL,
	RUN_HANDLE_SIGNAL_RESTART
};


class Test {
public:
	Test(const char* name)
		: fName(name)
	{
	}

	virtual ~Test()
	{
	}

	bool Run(run_mode mode)
	{
		fRunMode = mode;
		fSignalHandlerCalled = false;

		status_t error = Prepare();
		if (error != B_OK)
			return Error("Failed to prepare test: %s", strerror(error));

		thread_id thread = spawn_thread(_ThreadEntry, fName, B_NORMAL_PRIORITY,
			this);
		if (thread < 0)
			return Error("Failed to spawn thread: %s\n", strerror(thread));

		resume_thread(thread);

		// ...
		// * interrupt without restart
		// * interrupt with restart

		snooze(100000);
		kill(thread, SIGINT);

		PrepareFinish();

		status_t result;
		wait_for_thread(thread, &result);

		if (result != (Interrupted() ? B_INTERRUPTED : B_OK)) {
			return Error("Unexpected syscall return value: %s\n",
				strerror(result));
		}

		if ((RunMode() == RUN_IGNORE_SIGNAL) == fSignalHandlerCalled) {
			if (RunMode() == RUN_IGNORE_SIGNAL)
				return Error("Handler was called but shouldn't have been!");
			else
				return Error("Handler was not called!");
		}

		return Finish(Interrupted());
	}

	void Run()
	{
		printf("%s\n", fName);

		struct {
			const char*	name;
			run_mode	mode;
		} tests[] = {
			{ "ignore signal", RUN_IGNORE_SIGNAL },
			{ "handle signal no restart", RUN_HANDLE_SIGNAL },
			{ "handle signal restart", RUN_HANDLE_SIGNAL_RESTART },
			{}
		};

		for (int i = 0; tests[i].name != NULL; i++) {
			printf("  %-30s: ", tests[i].name);
			fflush(stdout);
			ClearError();
			if (Run(tests[i].mode))
				printf("ok\n");
			else
				printf("failed (%s)\n", fError);

			Cleanup();
		}
	}

	run_mode RunMode() const { return fRunMode; }
	bool Interrupted() const { return RunMode() == RUN_HANDLE_SIGNAL; }
	bigtime_t TimeWaited() const { return fTimeWaited; }

protected:
	virtual status_t Prepare()
	{
		return B_OK;
	}

	virtual status_t DoSyscall() = 0;

	virtual void HandleSignal()
	{
	}

	virtual void PrepareFinish()
	{
	}

	virtual bool Finish(bool interrupted)
	{
		return true;
	}

	virtual void Cleanup()
	{
	}

	bool Error(const char* format,...)
	{
		va_list args;
		va_start(args, format);
		vsnprintf(fError, sizeof(fError), format, args);
		va_end(args);

		return false;
	}

	bool Check(bool condition, const char* format,...)
	{
		if (condition)
			return true;

		va_list args;
		va_start(args, format);
		vsnprintf(fError, sizeof(fError), format, args);
		va_end(args);

		return false;
	}

	void ClearError()
	{
		fError[0] = '\0';
	}

private:
	static status_t _ThreadEntry(void* data)
	{
		return ((Test*)data)->_TestThread();
	}

	static void _SignalHandler(int signal, char* userData)
	{
		Test* self = (Test*)userData;

		self->fSignalHandlerCalled = true;
		self->HandleSignal();
	}

	status_t _TestThread()
	{
		// install handler
		struct sigaction action;
		if (RunMode() == RUN_IGNORE_SIGNAL)
			action.sa_handler = SIG_IGN;
		else
			action.sa_handler = (void (*)(int))_SignalHandler;

		action.sa_flags = RunMode() == RUN_HANDLE_SIGNAL_RESTART
			? SA_RESTART : 0;

		sigemptyset(&action.sa_mask);
		action.sa_userdata = this;

		sigaction(SIGINT, &action, NULL);

		bigtime_t startTime = system_time();
		status_t status = DoSyscall();
		fTimeWaited = system_time() - startTime;

		return status;
	}

private:
	const char*	fName;
	run_mode	fRunMode;
	bool		fSignalHandlerCalled;
	bigtime_t	fTimeWaited;
	char		fError[1024];
};


class SnoozeTest : public Test {
public:
	SnoozeTest()
		: Test("snooze")
	{
	}

	virtual status_t DoSyscall()
	{
		return snooze(1000000);
	}

	virtual bool Finish(bool interrupted)
	{
		if (interrupted)
			return Check(TimeWaited() < 200000, "waited too long");

		return Check(TimeWaited() > 900000 && TimeWaited() < 1100000,
			"waited %lld us instead of 1000000 us", TimeWaited());
	}
};


class ReadTest : public Test {
public:
	ReadTest()
		: Test("read")
	{
	}

	virtual status_t Prepare()
	{
		fBytesRead = -1;
		fFDs[0] = -1;
		fFDs[1] = -1;

		if (pipe(fFDs) != 0)
			return errno;

		return B_OK;
	}

	virtual status_t DoSyscall()
	{
		char buffer[256];
		fBytesRead = read(fFDs[0], buffer, sizeof(buffer));

		return fBytesRead < 0 ? errno : B_OK;
	}

	virtual void PrepareFinish()
	{
		write(fFDs[1], "Ingo", 4);
	}

	virtual bool Finish(bool interrupted)
	{
		if (interrupted)
			return Check(fBytesRead < 0, "unexpected read");

		return Check(fBytesRead == 4, "should have read 4 bytes, read only %ld "
			"bytes", fBytesRead);
	}

	virtual void Cleanup()
	{
		close(fFDs[0]);
		close(fFDs[1]);
	}

private:
	bigtime_t	fTimeWaited;
	ssize_t		fBytesRead;
	int			fFDs[2];
};


class WriteTest : public Test {
public:
	WriteTest()
		: Test("write")
	{
	}

	virtual status_t Prepare()
	{
		fBytesWritten = -1;
		fFDs[0] = -1;
		fFDs[1] = -1;

		if (pipe(fFDs) != 0)
			return errno;

		// fill pipe
		fcntl(fFDs[1], F_SETFL, O_NONBLOCK);
		while (write(fFDs[1], "a", 1) == 1);

		return B_OK;
	}

	virtual status_t DoSyscall()
	{
		// blocking wait
		fcntl(fFDs[1], F_SETFL, 0);
		fBytesWritten = write(fFDs[1], "Ingo", 4);

		return fBytesWritten < 0 ? errno : B_OK;
	}

	virtual void PrepareFinish()
	{
		char buffer[256];
		read(fFDs[0], buffer, sizeof(buffer));
	}

	virtual bool Finish(bool interrupted)
	{
		if (interrupted)
			return Check(fBytesWritten < 0, "unexpected write");

		return Check(fBytesWritten == 4, "should have written 4 bytes, wrote only %ld "
			"bytes", fBytesWritten);
	}

	virtual void Cleanup()
	{
		close(fFDs[0]);
		close(fFDs[1]);
	}

private:
	ssize_t		fBytesWritten;
	int			fFDs[2];
};


class AcquireSwitchSemTest : public Test {
public:
	AcquireSwitchSemTest(bool useSwitch)
		: Test(useSwitch ? "switch_sem" : "acquire_sem"),
		fSwitch(useSwitch)
	{
	}

	virtual status_t Prepare()
	{
		fSemaphore = create_sem(0, "test sem");

		return (fSemaphore >= 0 ? B_OK : fSemaphore);
	}

	virtual status_t DoSyscall()
	{
		if (fSwitch)
			return switch_sem(-1, fSemaphore);

		return acquire_sem(fSemaphore);
	}

	virtual void PrepareFinish()
	{
		release_sem(fSemaphore);
	}

/*
	virtual bool Finish(bool interrupted)
	{
//		int32 semCount = -1;
//		get_sem_count(fSemaphore, &semCount);

		if (interrupted)
			return true;

		return Check(fBytesWritten == 4, "should have written 4 bytes, wrote only %ld "
			"bytes", fBytesWritten);
	}
*/

	virtual void Cleanup()
	{
		delete_sem(fSemaphore);
	}

protected:
	sem_id		fSemaphore;
	bool		fSwitch;
};


class AcquireSwitchSemEtcTest : public Test {
public:
	AcquireSwitchSemEtcTest(bool useSwitch)
		: Test(useSwitch ? "switch_sem_etc" : "acquire_sem_etc"),
		fSwitch(useSwitch)
	{
	}

	virtual status_t Prepare()
	{
		fSemaphore = create_sem(0, "test sem");

		return fSemaphore >= 0 ? B_OK : fSemaphore;
	}

	virtual status_t DoSyscall()
	{
		status_t status;
		if (fSwitch) {
			status = switch_sem_etc(-1, fSemaphore, 1, B_RELATIVE_TIMEOUT,
				1000000);
		} else {
			status = acquire_sem_etc(fSemaphore, 1, B_RELATIVE_TIMEOUT,
				1000000);
		}

		if (!Interrupted() && status == B_TIMED_OUT)
			return B_OK;

		return status;
	}

	virtual bool Finish(bool interrupted)
	{
		if (interrupted)
			return Check(TimeWaited() < 200000, "waited too long");

		return Check(TimeWaited() > 900000 && TimeWaited() < 1100000,
			"waited %lld us instead of 1000000 us", TimeWaited());
	}

	virtual void Cleanup()
	{
		delete_sem(fSemaphore);
	}

protected:
	sem_id		fSemaphore;
	bool		fSwitch;
};


class AcceptTest : public Test {
public:
	AcceptTest()
		: Test("accept")
	{
	}

	virtual status_t Prepare()
	{
		fAcceptedSocket = -1;

		fServerSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (fServerSocket < 0) {
			fprintf(stderr, "Could not open server socket: %s\n",
				strerror(errno));
			return errno;
		}

		int reuse = 1;
		if (setsockopt(fServerSocket, SOL_SOCKET, SO_REUSEADDR, &reuse,
				sizeof(int)) == -1) {
			fprintf(stderr, "Could not make server socket reusable: %s\n",
				strerror(errno));
			return errno;
		}

		memset(&fServerAddress, 0, sizeof(sockaddr_in));
		fServerAddress.sin_family = AF_INET;
		fServerAddress.sin_addr.s_addr = INADDR_LOOPBACK;

		if (bind(fServerSocket, (struct sockaddr *)&fServerAddress,
				sizeof(struct sockaddr)) == -1) {
			fprintf(stderr, "Could not bind server socket: %s\n",
				strerror(errno));
			return errno;
		}

		socklen_t length = sizeof(sockaddr_in);
		getsockname(fServerSocket, (sockaddr*)&fServerAddress,
			&length);

		if (listen(fServerSocket, 10) == -1) {
			fprintf(stderr, "Could not listen on server socket: %s\n",
				strerror(errno));
			return errno;
		}

		return B_OK;
	}

	virtual status_t DoSyscall()
	{
		sockaddr_in clientAddress;
		socklen_t length = sizeof(struct sockaddr_in);

		fAcceptedSocket = accept(fServerSocket,
			(struct sockaddr *)&clientAddress, &length);
		if (fAcceptedSocket == -1)
			return errno;

		return B_OK;
	}

	virtual void PrepareFinish()
	{
		if (Interrupted())
			return;

		int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (clientSocket == -1) {
			fprintf(stderr, "Could not open client socket: %s\n",
				strerror(errno));
			return;
		}

		if (connect(clientSocket, (struct sockaddr *)&fServerAddress,
				sizeof(struct sockaddr)) == -1) {
			fprintf(stderr, "Could not connect to server socket: %s\n",
				strerror(errno));
		}

		close(clientSocket);
	}

	virtual bool Finish(bool interrupted)
	{
		if (interrupted)
			return Check(fAcceptedSocket < 0, "got socket");

		return Check(fAcceptedSocket >= 0, "got no socket");
	}

	virtual void Cleanup()
	{
		close(fAcceptedSocket);
		close(fServerSocket);
	}

protected:
	int			fServerSocket;
	sockaddr_in	fServerAddress;
	int			fAcceptedSocket;
};


class ReceiveTest : public Test {
public:
	ReceiveTest()
		: Test("recv")
	{
	}

	virtual status_t Prepare()
	{
		fBytesRead = -1;
		fAcceptedSocket = -1;
		fClientSocket = -1;

		fServerSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (fServerSocket < 0) {
			fprintf(stderr, "Could not open server socket: %s\n",
				strerror(errno));
			return errno;
		}

		int reuse = 1;
		if (setsockopt(fServerSocket, SOL_SOCKET, SO_REUSEADDR, &reuse,
				sizeof(int)) == -1) {
			fprintf(stderr, "Could not make server socket reusable: %s\n",
				strerror(errno));
			return errno;
		}

		memset(&fServerAddress, 0, sizeof(sockaddr_in));
		fServerAddress.sin_family = AF_INET;
		fServerAddress.sin_addr.s_addr = INADDR_LOOPBACK;

		if (bind(fServerSocket, (struct sockaddr *)&fServerAddress,
				sizeof(struct sockaddr)) == -1) {
			fprintf(stderr, "Could not bind server socket: %s\n",
				strerror(errno));
			return errno;
		}

		socklen_t length = sizeof(sockaddr_in);
		getsockname(fServerSocket, (sockaddr*)&fServerAddress,
			&length);

		if (listen(fServerSocket, 10) == -1) {
			fprintf(stderr, "Could not listen on server socket: %s\n",
				strerror(errno));
			return errno;
		}

		fClientSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (fClientSocket == -1) {
			fprintf(stderr, "Could not open client socket: %s\n",
				strerror(errno));
			return errno;
		}

		fcntl(fClientSocket, F_SETFL, O_NONBLOCK);

		if (connect(fClientSocket, (struct sockaddr *)&fServerAddress,
				sizeof(struct sockaddr)) == -1) {
			if (errno != EINPROGRESS) {
				fprintf(stderr, "Could not connect to server socket: %s\n",
					strerror(errno));
				return errno;
			}
		}

		sockaddr_in clientAddress;
		length = sizeof(struct sockaddr_in);

		fAcceptedSocket = accept(fServerSocket,
			(struct sockaddr *)&clientAddress, &length);
		if (fAcceptedSocket == -1)
			return errno;

		fcntl(fClientSocket, F_SETFL, 0);

		snooze(100000);

		return B_OK;
	}

	virtual status_t DoSyscall()
	{
		char buffer[256];
		fBytesRead = recv(fAcceptedSocket, buffer, sizeof(buffer), 0);

		return fBytesRead < 0 ? errno : B_OK;
	}

	virtual void PrepareFinish()
	{
		write(fClientSocket, "Axel", 4);
	}

	virtual bool Finish(bool interrupted)
	{
		if (interrupted)
			return Check(fBytesRead < 0, "unexpected read");

		return Check(fBytesRead == 4, "should have read 4 bytes, read only %ld "
			"bytes", fBytesRead);
	}

	virtual void Cleanup()
	{
		close(fAcceptedSocket);
		close(fServerSocket);
		close(fClientSocket);
	}

protected:
	int			fServerSocket;
	sockaddr_in	fServerAddress;
	int			fAcceptedSocket;
	int			fClientSocket;
	ssize_t		fBytesRead;
};


int
main()
{
	Test* tests[] = {
		new SnoozeTest,
		new ReadTest,
		new WriteTest,
		new AcquireSwitchSemTest(false),
		new AcquireSwitchSemTest(true),
		new AcquireSwitchSemEtcTest(false),
		new AcquireSwitchSemEtcTest(true),
		new AcceptTest,
		new ReceiveTest,
		NULL
	};

	for (int i = 0; tests[i] != NULL; i++)
		tests[i]->Run();

	return 0;
}

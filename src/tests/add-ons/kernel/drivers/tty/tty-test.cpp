/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <algorithm>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <OS.h>

#include <util/DoublyLinkedList.h>

//	Test cases:
//	
//	1. unblock on close:
//	
//	* unblock, when the same cookie is closed
//	  - read
//	  - write (with, without ECHO)
//	* unblock write operations, when the other tty is closed
//	* unblock slave operations, when the master is closed
//	
//	
//	2. select:
//	
//	* notify read, write, if ready when select()ing
//	* notify read, write, error on close of the other TTY
//    (the select() behavior when closing the same TTY is undefined)
//	* notify read after pending write
//	  - with ECHO
//	  - without ECHO
//	* notify write after pending read and full buffer
//	  - with ECHO
//	  - without ECHO
//	* don't notify when there was a pending read/write
//
//
// TODO: There are no ECHO tests yet, since I don't know how to turn it on.

#define CHK(condition)		assert(condition)
#define RUN_TEST(test)		(test)->Run()
#define CHK_ALIVE(thread)	CHK((thread)->IsAlive())
#define CHK_DEAD(thread)	CHK(!(thread)->IsAlive())

// FDSet
struct FDSet : fd_set {
	FDSet()
	{
		Clear();
	}

	void Clear()
	{
		FD_ZERO(this);
		fCount = 0;
	}

	void Add(int fd)
	{
		if (fd < 0 || fd >= FD_SETSIZE) {
			fprintf(stderr, "FDSet::Add(%d): Invalid FD.\n", fd);
			return;
		}

		FD_SET(fd, this);
		if (fd >= fCount)
			fCount = fd + 1;
	}

	int Count() const
	{
		return fCount;
	}

	void UpdateCount()
	{
		if (fCount > 0) {
			for (int i = fCount - 1; i >= 0; i--) {
				if (FD_ISSET(i, this)) {
					fCount = i + 1;
					return;
				}
			}
			fCount = 0;
		}
	}

	bool operator==(const FDSet &other) const
	{
		if (fCount != other.fCount)
			return false;

		for (int i = 0; i < fCount; i++) {
			if (FD_ISSET(i, this) != FD_ISSET(i, &other))
				return false;
		}

		return true;
	}

	bool operator!=(const FDSet &other) const
	{
		return !(*this == other);
	}

private:
	int	fCount;
};

// SelectSet
class SelectSet {
public:
	SelectSet() {}
	~SelectSet() {}

	void Clear()
	{
		fReadSet.Clear();
		fWriteSet.Clear();
		fErrorSet.Clear();
	}

	void AddReadFD(int fd)
	{
		fReadSet.Add(fd);
	}

	void AddWriteFD(int fd)
	{
		fWriteSet.Add(fd);
	}

	void AddErrorFD(int fd)
	{
		fErrorSet.Add(fd);
	}

	int Select(bigtime_t timeout = -1)
	{
		int count = max_c(max_c(fReadSet.Count(), fWriteSet.Count()),
			fErrorSet.Count());
		fd_set *readSet = (fReadSet.Count() > 0 ? &fReadSet : NULL);
		fd_set *writeSet = (fWriteSet.Count() > 0 ? &fWriteSet : NULL);
		fd_set *errorSet = (fErrorSet.Count() > 0 ? &fErrorSet : NULL);

		timeval tv = { timeout / 1000000, timeout % 1000000 };

		int result = select(count, readSet, writeSet, errorSet,
			(timeout >= 0 ? &tv : NULL));
		if (result <= 0) {
			Clear();
		} else {
			fReadSet.UpdateCount();
			fWriteSet.UpdateCount();
			fErrorSet.UpdateCount();
		}

		return result;
	}

	bool operator==(const SelectSet &other) const
	{
		return (fReadSet == other.fReadSet
			&& fWriteSet == other.fWriteSet
			&& fErrorSet == other.fErrorSet);
	}

	bool operator!=(const SelectSet &other) const
	{
		return !(*this == other);
	}

private:
	FDSet	fReadSet;
	FDSet	fWriteSet;
	FDSet	fErrorSet;
};

// Runnable
class Runnable {
public:
	virtual ~Runnable() {}
	virtual int32 Run() = 0;
};

// Caller
template<typename Type>
class Caller : public Runnable {
public:
	Caller(Type *object, int32 (Type::*function)())
		: fObject(object),
		  fFunction(function)
	{
	}

	virtual int32 Run()
	{
		return (fObject->*fFunction)();
	}

private:
	Type	*fObject;
	int32	(Type::*fFunction)();
};

// create_caller
template<typename Type>
static inline Runnable *
create_caller(Type *object, int32 (Type::*function)())
{
	return new Caller<Type>(object, function);
}

#define CALLER(object, function)	create_caller(object, function)

// Thread
struct Thread : public DoublyLinkedListLinkImpl<Thread> {
public:
	Thread(Runnable *runnable, const char *name)
		: fRunnable(runnable)
	{
		fThread = spawn_thread(_Entry, name, B_NORMAL_PRIORITY, this);
		if (fThread < 0) {
			sprintf("Failed to spawn thread: %s\n", strerror(fThread));
			exit(1);
		}
	}

	~Thread()
	{
		Kill();
		delete fRunnable;
	}

	void Resume()
	{
		resume_thread(fThread);
	}

	void WaitFor()
	{
		status_t result;
		wait_for_thread(fThread, &result);
	}

	void Kill()
	{
		kill_thread(fThread);
	}

	bool IsAlive()
	{
		thread_info info;
		return (get_thread_info(fThread, &info) == B_OK);
	}

private:
	static int32 _Entry(void *data)
	{
		return ((Thread*)data)->fRunnable->Run();
	}

	thread_id	fThread;
	Runnable	*fRunnable;
};

// TestCase
class TestCase {
public:
	TestCase() {}
	virtual ~TestCase()
	{
		while (Thread *thread = fThreads.Head()) {
			fThreads.Remove(thread);
			delete thread;
		}
	}

	void Run()
	{
		Test();
		delete this;
	}

protected:
	virtual void Test() = 0;

	Thread *CreateThread(Runnable *runnable, const char *name)
	{
		Thread *thread = new Thread(runnable, name);
		fThreads.Add(thread);
		return thread;
	}

	static void WriteUntilBlock(int fd)
	{
		// set non-blocking I/O
		if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
			fprintf(stderr, "WriteUntilBlock(): Failed to set non-blocking I/O "
				"mode: %s\n", strerror(errno));
			exit(1);
		}

		// write till blocking
		char buffer[1024];
		memset(buffer, 'A', sizeof(buffer));
		ssize_t bytesWritten;
		do {
			bytesWritten = write(fd, buffer, sizeof(buffer));
		} while (bytesWritten > 0 || errno == B_INTERRUPTED);

		if (bytesWritten < 0 && errno != B_WOULD_BLOCK) {
			fprintf(stderr, "WriteUntilBlock(): Writing failed: %s\n",
				strerror(errno));
			exit(1);
		}

		// reset to blocking I/O
		if (fcntl(fd, F_SETFL, 0) < 0) {
			fprintf(stderr, "WriteUntilBlock(): Failed to set blocking I/O "
				"mode: %s\n", strerror(errno));
			exit(1);
		}
	}

	static void ReadDontFail(int fd, int32 size)
	{
		char buffer[1024];

		while (size > 0) {
			ssize_t bytesRead;
			do {
				int32 toRead = sizeof(buffer);
				if (toRead > size)
					toRead = size;

				bytesRead = read(fd, buffer, toRead);
			} while (bytesRead < 0 && errno == B_INTERRUPTED);

			if (bytesRead < 0) {
				fprintf(stderr, "ReadDontFail(): Failed to read: %s\n",
					strerror(errno));
				exit(1);
			}

			size -= bytesRead;
		}
	}

	static void WriteDontFail(int fd, int32 size)
	{
		char buffer[1024];
		memset(buffer, 'A', sizeof(buffer));

		while (size > 0) {
			ssize_t bytesWritten;
			do {
				int32 toWrite = sizeof(buffer);
				if (toWrite > size)
					toWrite = size;

				bytesWritten = write(fd, buffer, toWrite);
			} while (bytesWritten < 0 && errno == B_INTERRUPTED);

			if (bytesWritten < 0) {
				fprintf(stderr, "WriteDontFail(): Failed to write: %s\n",
					strerror(errno));
				exit(1);
			}

			size -= bytesWritten;
		}
	}

	void SetEcho(int fd)
	{
		// TODO: How to set echo mode?
	}

private:
	typedef DoublyLinkedList<Thread> ThreadList;

	ThreadList	fThreads;
};


// open_tty
static int
open_tty(int index, bool master)
{
	if (index < 0 || index >= 16)
		fprintf(stderr, "open_tty(%d, %d): Bad index!\n", index, master);

	char path[32];
	sprintf(path, "/dev/%ct/r%x", (master ? 'p' : 't'), index);
	// we do not want the slave to be our controlling tty
	int fd = open(path, O_RDWR | (master ? 0 : O_NOCTTY));

	if (fd < 0) {
		fprintf(stderr, "Failed to open tty `%s': %s\n", path,
			strerror(errno));
		exit(1);
	}

	return fd;
}


static void
close_tty(int &fd)
{
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
}


// #pragma mark -

// TestUnblockOnCloseRead
class TestUnblockOnCloseRead : public TestCase {
public:
	TestUnblockOnCloseRead(bool master, bool crossOver)
		:
		fMaster(-1),
		fSlave(-1),
		fTestMaster(master),
		fCrossOver(crossOver)
	{
		printf("TestUnblockOnCloseRead(%d, %d)\n", master, crossOver);
	}


protected:
	virtual ~TestUnblockOnCloseRead()
	{
		close_tty(fMaster);
		close_tty(fSlave);
	}

	virtual void Test()
	{
		fMaster = open_tty(0, true);
		fSlave = open_tty(0, false);

		Thread *thread = CreateThread(
			CALLER(this, &TestUnblockOnCloseRead::_Reader), "reader");
		thread->Resume();

		snooze(100000);
		CHK_ALIVE(thread);

		if (fCrossOver)
			close_tty(fTestMaster ? fSlave : fMaster);
		else
			close_tty(fTestMaster ? fMaster : fSlave);

		snooze(100000);
		CHK_DEAD(thread);
	}

private:
	int32 _Reader()
	{
		char buffer[32];
		ssize_t bytesRead = read((fTestMaster ? fMaster : fSlave), buffer,
			sizeof(buffer));

		CHK((bytesRead < 0));

		return 0;
	}

private:
	int		fMaster;
	int		fSlave;
	bool	fTestMaster;
	bool	fCrossOver;
};

// TestUnblockOnCloseWrite
class TestUnblockOnCloseWrite : public TestCase {
public:
	TestUnblockOnCloseWrite(bool master, bool crossOver, bool echo)
		:
		fMaster(-1),
		fSlave(-1),
		fTestMaster(master),
		fCrossOver(crossOver),
		fEcho(echo)
	{
		printf("TestUnblockOnCloseWrite(%d, %d, %d)\n", master, crossOver,
			echo);
	}


protected:
	virtual ~TestUnblockOnCloseWrite()
	{
		close_tty(fMaster);
		close_tty(fSlave);
	}

	virtual void Test()
	{
		fMaster = open_tty(0, true);
		fSlave = open_tty(0, false);

		if (fEcho)
			SetEcho((fTestMaster ? fSlave : fMaster));

		WriteUntilBlock((fTestMaster ? fMaster : fSlave));

		Thread *thread = CreateThread(
			CALLER(this, &TestUnblockOnCloseWrite::_Writer), "writer");
		thread->Resume();

		snooze(100000);
		CHK_ALIVE(thread);

		if (fCrossOver)
			close_tty(fTestMaster ? fSlave : fMaster);
		else
			close_tty(fTestMaster ? fMaster : fSlave);

		snooze(100000);
		CHK_DEAD(thread);
	}

private:
	int32 _Writer()
	{
		char buffer[32];
		memset(buffer, 'A', sizeof(buffer));
		ssize_t bytesWritten = write((fTestMaster ? fMaster : fSlave), buffer,
			sizeof(buffer));

		CHK((bytesWritten < 0));

		return 0;
	}

private:
	int		fMaster;
	int		fSlave;
	bool	fTestMaster;
	bool	fCrossOver;
	bool	fEcho;
};

// TestSelectAlreadyReady
class TestSelectAlreadyReady : public TestCase {
public:
	TestSelectAlreadyReady(bool master, bool write)
		:
		fMaster(-1),
		fSlave(-1),
		fTestMaster(master),
		fWrite(write)
	{
		printf("TestSelectAlreadyReady(%d, %d)\n", master, write);
	}


protected:
	virtual ~TestSelectAlreadyReady()
	{
		close_tty(fMaster);
		close_tty(fSlave);
	}

	virtual void Test()
	{
		fMaster = open_tty(0, true);
		fSlave = open_tty(0, false);

		if (!fWrite)
			WriteDontFail((fTestMaster ? fSlave : fMaster), 1);

		Thread *thread = CreateThread(
			CALLER(this, &TestSelectAlreadyReady::_Selector), "selector");
		thread->Resume();

		snooze(100000);
		CHK_DEAD(thread);
	}

private:
	int32 _Selector()
	{
		SelectSet selectSet;
		SelectSet compareSet;
		if (fWrite) {
			selectSet.AddWriteFD((fTestMaster ? fMaster : fSlave));
			compareSet.AddWriteFD((fTestMaster ? fMaster : fSlave));
		} else {
			selectSet.AddReadFD((fTestMaster ? fMaster : fSlave));
			compareSet.AddReadFD((fTestMaster ? fMaster : fSlave));
		}

		int result = selectSet.Select();
		CHK(result > 0);
		CHK(selectSet == compareSet);

		return 0;
	}

private:
	int		fMaster;
	int		fSlave;
	bool	fTestMaster;
	bool	fWrite;
};

// TestSelectNotifyOnClose
class TestSelectNotifyOnClose : public TestCase {
public:
	TestSelectNotifyOnClose(bool master)
		: fMaster(-1),
		  fSlave(-1),
		  fTestMaster(master)
	{
		printf("TestSelectNotifyOnClose(%d)\n", master);
	}


protected:
	virtual ~TestSelectNotifyOnClose()
	{
		close_tty(fMaster);
		close_tty(fSlave);
	}

	virtual void Test()
	{
		fMaster = open_tty(0, true);
		fSlave = open_tty(0, false);

		WriteUntilBlock((fTestMaster ? fMaster : fSlave));

		Thread *thread = CreateThread(
			CALLER(this, &TestSelectNotifyOnClose::_Selector), "selector");
		thread->Resume();

		snooze(100000);
		CHK_ALIVE(thread);

		close_tty((fTestMaster ? fSlave : fMaster));

		snooze(100000);
		CHK_DEAD(thread);
	}

private:
	int32 _Selector()
	{
		int fd = (fTestMaster ? fMaster : fSlave);

		SelectSet selectSet;
		selectSet.AddReadFD(fd);
		selectSet.AddWriteFD(fd);
		selectSet.AddErrorFD(fd);

		// In case the slave is closed while we select() on the master, only a
		// `write' event will arrive.
		SelectSet compareSet;
		if (!fTestMaster) {
			compareSet.AddReadFD(fd);
			compareSet.AddErrorFD(fd);
		}
		compareSet.AddWriteFD(fd);

		int result = selectSet.Select();
		CHK(result > 0);
		CHK(selectSet == compareSet);

		return 0;
	}

private:
	int		fMaster;
	int		fSlave;
	bool	fTestMaster;
};

// TestSelectNotifyAfterPending
class TestSelectNotifyAfterPending : public TestCase {
public:
	TestSelectNotifyAfterPending(bool master, bool write, bool unblock)
		:
		fMaster(-1),
		fSlave(-1),
		fTestMaster(master),
		fWrite(write),
		fUnblock(unblock)
	{
		printf("TestSelectNotifyAfterPending(%d, %d, %d)\n", master, write,
			unblock);
	}


protected:
	virtual ~TestSelectNotifyAfterPending()
	{
		close_tty(fMaster);
		close_tty(fSlave);
	}

	virtual void Test()
	{
		fMaster = open_tty(0, true);
		fSlave = open_tty(0, false);

		if (fWrite)
			WriteUntilBlock((fTestMaster ? fMaster : fSlave));

		Thread *readWriter = CreateThread(
			CALLER(this, &TestSelectNotifyAfterPending::_ReadWriter),
			"read-writer");
		Thread *selector = CreateThread(
			CALLER(this, &TestSelectNotifyAfterPending::_Selector), "selector");

		readWriter->Resume();
		selector->Resume();

		snooze(100000);
		CHK_ALIVE(readWriter);
		CHK_ALIVE(selector);

		if (fUnblock) {
			// unblock the read-writer and the selector
			if (fWrite)
				ReadDontFail((fTestMaster ? fSlave : fMaster), 2);
			else
				WriteDontFail((fTestMaster ? fSlave : fMaster), 2);

			snooze(100000);
			CHK_DEAD(readWriter);
			CHK_DEAD(selector);

		} else {
			// unblock the read-writer, but not the selector
			if (fWrite)
				ReadDontFail((fTestMaster ? fSlave : fMaster), 1);
			else
				WriteDontFail((fTestMaster ? fSlave : fMaster), 1);

			snooze(100000);
			CHK_DEAD(readWriter);
			CHK_ALIVE(selector);

			close_tty((fTestMaster ? fMaster : fSlave));

			snooze(100000);
			CHK_DEAD(selector);
		}
	}

private:
	int32 _ReadWriter()
	{
		int fd = (fTestMaster ? fMaster : fSlave);
		if (fWrite)
			WriteDontFail(fd, 1);
		else
			ReadDontFail(fd, 1);

		return 0;
	}

	int32 _Selector()
	{
		int fd = (fTestMaster ? fMaster : fSlave);

		SelectSet selectSet;
		SelectSet compareSet;

		if (fWrite) {
			selectSet.AddWriteFD(fd);
			compareSet.AddWriteFD(fd);
		} else {
			selectSet.AddReadFD(fd);
			compareSet.AddReadFD(fd);
		}

		int result = selectSet.Select();
		CHK(result > 0);
		CHK(selectSet == compareSet);

		return 0;
	}

private:
	int		fMaster;
	int		fSlave;
	bool	fTestMaster;
	bool	fWrite;
	bool	fUnblock;
};

// TestIoctlFIONRead
class TestIoctlFIONRead : public TestCase {
public:
	TestIoctlFIONRead(bool master)
		:
		fMaster(-1),
		fSlave(-1),
		fTestMaster(master)
	{
		printf("TestIoctlFIONRead(%d)\n", master);
	}


protected:
	virtual ~TestIoctlFIONRead()
	{
		close_tty(fMaster);
		close_tty(fSlave);
	}

	virtual void Test()
	{
		fMaster = open_tty(0, true);
		fSlave = open_tty(0, false);
		
		int fd = (fTestMaster ? fMaster : fSlave);
		status_t err;
		int toRead = -1;

		errno = 0;
		err = ioctl(fd, FIONREAD, NULL);
		CHK(err == -1);
		printf("e: %s\n", strerror(errno));
		// should be CHK(errno == EINVAL); !!
		CHK(errno == EFAULT);

		errno = 0;
		err = ioctl(fd, FIONREAD, (void *)1);
		CHK(err == -1);
		printf("e: %s\n", strerror(errno));
		CHK(errno == EFAULT);

		errno = 0;

		err = ioctl(fd, FIONREAD, &toRead);
		printf("e: %s\n", strerror(errno));
		//CHK(err == 0);
		//CHK(toRead == 0);

		WriteDontFail((fTestMaster ? fSlave : fMaster), 1);

		errno = 0;

		err = ioctl(fd, FIONREAD, &toRead);
		printf("e: %d\n", err);
		CHK(err == 0);
		CHK(toRead == 1);

		WriteUntilBlock((fTestMaster ? fSlave : fMaster));

		err = ioctl(fd, FIONREAD, &toRead);
		CHK(err == 0);
		CHK(toRead > 1);

		close_tty((fTestMaster ? fSlave : fMaster));

		err = ioctl(fd, FIONREAD, &toRead);
		CHK(err == 0);
		CHK(toRead > 1);

		ReadDontFail(fd, toRead);

		err = ioctl(fd, FIONREAD, &toRead);
		CHK(err == 0);
		CHK(toRead == 0);

	}

private:
	int		fMaster;
	int		fSlave;
	bool	fTestMaster;
};



// #pragma mark -

int
main()
{
	// unblock tests

	RUN_TEST(new TestUnblockOnCloseRead(true,  false));
	RUN_TEST(new TestUnblockOnCloseRead(false, false));
	RUN_TEST(new TestUnblockOnCloseRead(false, true));

	RUN_TEST(new TestUnblockOnCloseWrite(true,  false, false));
	RUN_TEST(new TestUnblockOnCloseWrite(false, false, false));
	RUN_TEST(new TestUnblockOnCloseWrite(true,  true,  false));
	RUN_TEST(new TestUnblockOnCloseWrite(false, true,  false));
// TODO: How to enable echo mode?
//	RUN_TEST(new TestUnblockOnCloseWrite(true,  false, true));
//	RUN_TEST(new TestUnblockOnCloseWrite(false, false, true));
//	RUN_TEST(new TestUnblockOnCloseWrite(true,  true,  true));
//	RUN_TEST(new TestUnblockOnCloseWrite(false, true,  true));

	// select tests

	RUN_TEST(new TestSelectAlreadyReady(true,  true));
	RUN_TEST(new TestSelectAlreadyReady(false, true));
	RUN_TEST(new TestSelectAlreadyReady(true,  false));
	RUN_TEST(new TestSelectAlreadyReady(false, false));

	RUN_TEST(new TestSelectNotifyOnClose(true));
	RUN_TEST(new TestSelectNotifyOnClose(false));

	RUN_TEST(new TestSelectNotifyAfterPending(false, false, false));
	RUN_TEST(new TestSelectNotifyAfterPending(false, false, true));
	RUN_TEST(new TestSelectNotifyAfterPending(false, true,  false));
	RUN_TEST(new TestSelectNotifyAfterPending(false, true,  true));
	RUN_TEST(new TestSelectNotifyAfterPending(true,  false, false));
	RUN_TEST(new TestSelectNotifyAfterPending(true,  false, true));
	RUN_TEST(new TestSelectNotifyAfterPending(true,  true,  false));
	RUN_TEST(new TestSelectNotifyAfterPending(true,  true,  true));

	RUN_TEST(new TestIoctlFIONRead(true));
	//RUN_TEST(new TestIoctlFIONRead(false));

	return 0;
}

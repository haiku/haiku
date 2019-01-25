/*
 * Copyright 2004-2010, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 * Original code from ZipOMatic by jonas.sundstrom@kirilla.com
 */
#include "ExpanderThread.h"


#include <errno.h>
#include <image.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include <Messenger.h>
#include <Path.h>


const char* ExpanderThreadName = "ExpanderThread";


ExpanderThread::ExpanderThread(BMessage* refs_message, BMessenger* messenger)
	:
	GenericThread(ExpanderThreadName, B_NORMAL_PRIORITY, refs_message),
	fWindowMessenger(messenger),
	fThreadId(-1),
	fStdIn(-1),
	fStdOut(-1),
	fStdErr(-1),
	fExpanderOutput(NULL),
	fExpanderError(NULL)
{
	SetDataStore(new BMessage(*refs_message));
		// leak?
	// prevents bug with B_SIMPLE_DATA
	// (drag&drop messages)
}


ExpanderThread::~ExpanderThread()
{
	delete fWindowMessenger;
}


status_t
ExpanderThread::ThreadStartup()
{
	status_t status = B_OK;
	entry_ref srcRef;
	entry_ref destRef;
	BString cmd;

	if ((status = GetDataStore()->FindRef("srcRef", &srcRef)) != B_OK)
		return status;

	if ((status = GetDataStore()->FindRef("destRef", &destRef)) == B_OK) {
		BPath path(&destRef);
		chdir(path.Path());
	}

	if ((status = GetDataStore()->FindString("cmd", &cmd)) != B_OK)
		return status;

	BPath path(&srcRef);
	BString pathString(path.Path());
	pathString.CharacterEscape("\\\"$`", '\\');
	pathString.Prepend("\"");
	pathString.Append("\"");
	cmd.ReplaceAll("%s", pathString.String());

	int32 argc = 3;
	const char** argv = new const char * [argc + 1];

	argv[0] = strdup("/bin/sh");
	argv[1] = strdup("-c");
	argv[2] = strdup(cmd.String());
	argv[argc] = NULL;

	fThreadId = PipeCommand(argc, argv, fStdIn, fStdOut, fStdErr);

	delete [] argv;

	if (fThreadId < 0)
		return fThreadId;

	// lower the command priority since it is a background task.
	set_thread_priority(fThreadId, B_LOW_PRIORITY);

	resume_thread(fThreadId);

	int flags = fcntl(fStdOut, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fStdOut, F_SETFL, flags);
	flags = fcntl(fStdErr, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fStdErr, F_SETFL, flags);

	fExpanderOutput = fdopen(fStdOut, "r");
	fExpanderError = fdopen(fStdErr, "r");

	return B_OK;
}


status_t
ExpanderThread::ExecuteUnit(void)
{
	// read output and error from command
	// send it to window

	BMessage message('outp');
	bool outputAdded = false;
	for (int32 i = 0; i < 50; i++) {
		char* output_string = fgets(fExpanderOutputBuffer , LINE_MAX,
			fExpanderOutput);
		if (output_string == NULL)
			break;

		message.AddString("output", output_string);
		outputAdded = true;
	}
	if (outputAdded)
		fWindowMessenger->SendMessage(&message);

	if (feof(fExpanderOutput))
		return EOF;

	char* error_string = fgets(fExpanderOutputBuffer, LINE_MAX,
		fExpanderError);
	if (error_string != NULL && strcmp(error_string, "\n")) {
		BMessage message('errp');
		message.AddString("error", error_string);
		fWindowMessenger->SendMessage(&message);
	}

	// streams are non blocking, sleep every 100ms
	snooze(100000);

	return B_OK;
}


void
ExpanderThread::PushInput(BString text)
{
	text += "\n";
	write(fStdIn, text.String(), text.Length());
}


status_t
ExpanderThread::ThreadShutdown(void)
{
	close(fStdIn);
	close(fStdOut);
	close(fStdErr);

	return B_OK;
}


void
ExpanderThread::ThreadStartupFailed(status_t status)
{
	fprintf(stderr, "ExpanderThread::ThreadStartupFailed() : %s\n",
		strerror(status));

	Quit();
}


void
ExpanderThread::ExecuteUnitFailed(status_t status)
{
	if (status == EOF) {
		// thread has finished, been quit or killed, we don't know
		fWindowMessenger->SendMessage(new BMessage('exit'));
	} else {
		// explicit error - communicate error to Window
		fWindowMessenger->SendMessage(new BMessage('exrr'));
	}

	Quit();
}


void
ExpanderThread::ThreadShutdownFailed(status_t status)
{
	fprintf(stderr, "ExpanderThread::ThreadShutdownFailed() %s\n",
		strerror(status));
}


status_t
ExpanderThread::ProcessRefs(BMessage *msg)
{
	return B_OK;
}


thread_id
ExpanderThread::PipeCommand(int argc, const char** argv, int& in, int& out,
	int& err, const char** envp)
{
	// This function written by Peter Folk <pfolk@uni.uiuc.edu>
	// and published in the BeDevTalk FAQ
	// http://www.abisoft.com/faq/BeDevTalk_FAQ.html#FAQ-209

	// Save current FDs
	int old_out = dup(1);
	int old_err = dup(2);

	int filedes[2];
	
	// create new pipe FDs as stdout, stderr
	pipe(filedes);  dup2(filedes[1], 1); close(filedes[1]);
	out = filedes[0]; // Read from out, taken from cmd's stdout
	pipe(filedes);  dup2(filedes[1], 2); close(filedes[1]);
	err = filedes[0]; // Read from err, taken from cmd's stderr

	// taken from pty.cpp
	// create a tty for stdin, as utilities don't generally use stdin
	int master = posix_openpt(O_RDWR);
	if (master < 0)
		return -1;

	int slave;
	const char* ttyName;
	if (grantpt(master) != 0 || unlockpt(master) != 0
		|| (ttyName = ptsname(master)) == NULL
		|| (slave = open(ttyName, O_RDWR | O_NOCTTY)) < 0) {
		close(master);
		return -1;
	}

	int pid = fork();
	if (pid < 0) {
		close(master);
		close(slave);
		return -1;
	}

	// child
	if (pid == 0) {
		close(master);

		setsid();
		if (ioctl(slave, TIOCSCTTY, NULL) != 0)
			return -1;

		dup2(slave, 0); 
		close(slave);

		// "load" command.
		execv(argv[0], (char *const *)argv);

		// shouldn't return
		return -1;
	}

	// parent
	close (slave);
	in = master;

	// Restore old FDs
	close(1); dup(old_out); close(old_out);
	close(2); dup(old_err); close(old_err);

	// Theoretically I should do loads of error checking, but
	// the calls aren't very likely to fail, and that would
	// muddy up the example quite a bit. YMMV.

	return pid;
}


status_t
ExpanderThread::SuspendExternalExpander()
{
	thread_info info;
	status_t status = get_thread_info(fThreadId, &info);

	if (status == B_OK)
		return send_signal(-fThreadId, SIGSTOP);
	else
		return status;
}


status_t
ExpanderThread::ResumeExternalExpander()
{
	thread_info info;
	status_t status = get_thread_info(fThreadId, &info);

	if (status == B_OK)
		return send_signal(-fThreadId, SIGCONT);
	else
		return status;
}


status_t
ExpanderThread::InterruptExternalExpander()
{
	thread_info info;
	status_t status = get_thread_info(fThreadId, &info);

	if (status == B_OK) {
		status = send_signal(-fThreadId, SIGINT);
		WaitOnExternalExpander();
	}

	return status;
}


status_t
ExpanderThread::WaitOnExternalExpander()
{
	thread_info info;
	status_t status = get_thread_info(fThreadId, &info);

	if (status == B_OK)
		return wait_for_thread(fThreadId, &status);
	else
		return status;
}

/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 *		Peter Folk <pfolk@uni.uiuc.edu>
 */


#include "ZipperThread.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <Catalog.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Locker.h>
#include <Message.h>
#include <Path.h>
#include <Volume.h>

#include <private/tracker/tracker_private.h>

#include "ZipOMaticMisc.h"
#include "ZipOMaticWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "file:ZipperThread.cpp"


ZipperThread::ZipperThread(BMessage* refsMessage, BWindow* window)
	:
	GenericThread("ZipperThread", B_NORMAL_PRIORITY, refsMessage),
	fWindowMessenger(window),
	fZipProcess(-1),
	fStdIn(-1),
	fStdOut(-1),
	fStdErr(-1),
	fOutputFile(NULL)
{
	fThreadDataStore = new BMessage(*refsMessage);
}


ZipperThread::~ZipperThread()
{
}


status_t
ZipperThread::ThreadStartup()
{
	type_code type = B_REF_TYPE;
	int32 refCount = 0;
	entry_ref ref;
	entry_ref lastRef;
	bool sameFolder = true;

	status_t status = fThreadDataStore->GetInfo("refs", &type, &refCount);
	if (status != B_OK || refCount < 1) {
		_SendMessageToWindow(ZIPPO_THREAD_EXIT_ERROR);
		Quit();
		return B_ERROR;
	}

	for	(int index = 0; index < refCount; index++) {
		fThreadDataStore->FindRef("refs", index, &ref);

		if (index > 0) {
			if (lastRef.directory != ref.directory) {
				sameFolder = false;
				break;
			}
		}
		lastRef = ref;
	}

	entry_ref dirRef;
	bool gotDirRef = false;

	status = fThreadDataStore->FindRef("dir_ref", 0, &dirRef);
	if (status == B_OK) {
		BEntry dirEntry(&dirRef);
		BNode dirNode(&dirRef);

		if (dirEntry.InitCheck() == B_OK
			&& dirEntry.Exists()
			&& dirNode.InitCheck() == B_OK
			&& dirNode.IsDirectory())
			gotDirRef = true;
	}

	if (gotDirRef) {
		BEntry entry(&dirRef);
		BPath path;
		entry.GetPath(&path);
		chdir(path.Path());
	} else if (sameFolder) {
		BEntry entry(&lastRef);
		BPath path;
		entry.GetParent(&entry);
		entry.GetPath(&path);
		chdir(path.Path());
	} else {
		BPath path;
		if (find_directory(B_DESKTOP_DIRECTORY, &path) == B_OK)
			chdir(path.Path());
	}

	BString archiveName;

	if (refCount > 1)
		archiveName = B_TRANSLATE("Archive");
	else
		archiveName = lastRef.name;

	int index = 1;
	for (;; index++) {
		BString tryName = archiveName;

		if (index != 1)
			tryName << " " << index;

		tryName << ".zip";

		BEntry entry(tryName.String());
		if (!entry.Exists()) {
			archiveName = tryName;
			entry.GetRef(&fOutputEntryRef);
			break;
		}
	}

	int32 argc = refCount + 3;
	const char** argv = new const char* [argc + 1];

	argv[0] = strdup("/bin/zip");
	argv[1] = strdup("-ry");
	argv[2] = strdup(archiveName.String());

	for (int index = 0; index < refCount; index++) {
		fThreadDataStore->FindRef("refs", index, &ref);

		if (gotDirRef || sameFolder) {
			argv[3 + index]	= strdup(ref.name);
		} else {
			BPath path(&ref);
			BString file = path.Path();
			argv[3 + index]	= strdup(path.Path());
		}
	}

	argv[argc] = NULL;

	fZipProcess = _PipeCommand(argc, argv, fStdIn, fStdOut, fStdErr);

	delete [] argv;

	if (fZipProcess < 0)
		return fZipProcess;

	resume_thread(fZipProcess);

	fOutputFile = fdopen(fStdOut, "r");
	if (fOutputFile == NULL)
		return errno;

	_SendMessageToWindow(ZIPPO_TASK_DESCRIPTION, "archive_filename",
		archiveName.String());
	_SendMessageToWindow(ZIPPO_LINE_OF_STDOUT, "zip_output",
		B_TRANSLATE("Preparing to archive"));

	return B_OK;
}


status_t
ZipperThread::ExecuteUnit()
{
	char buffer[4096];

	char* output = fgets(buffer, sizeof(buffer) - 1, fOutputFile);
	if (output == NULL)
		return EOF;

	char* newLine = strrchr(output, '\n');
	if (newLine != NULL)
		*newLine = '\0';

	if (!strncmp("  a", output, 3)) {
		output[2] = 'A';
		_SendMessageToWindow(ZIPPO_LINE_OF_STDOUT, "zip_output", output + 2);
	} else if (!strncmp("up", output, 2)) {
		output[0] = 'U';
		_SendMessageToWindow(ZIPPO_LINE_OF_STDOUT, "zip_output", output);
	} else {
		_SendMessageToWindow(ZIPPO_LINE_OF_STDOUT, "zip_output", output);
	}

	return B_OK;
}


status_t
ZipperThread::ThreadShutdown()
{
	close(fStdIn);
	close(fStdOut);
	close(fStdErr);

	_SelectInTracker();

	return B_OK;
}


void
ZipperThread::ThreadStartupFailed(status_t status)
{
	fprintf(stderr, "ZipperThread::ThreadStartupFailed(): %s\n",
		strerror(status));
	_SendMessageToWindow(ZIPPO_THREAD_EXIT_ERROR);

	Quit();
}


void
ZipperThread::ExecuteUnitFailed(status_t status)
{
	if (status == EOF) {
		fprintf(stderr, "ZipperThread::ExecuteUnitFailed(): EOF\n");
		// thread has finished, been quit or killed, we don't know
		_SendMessageToWindow(ZIPPO_THREAD_EXIT);
	} else {
		fprintf(stderr, "ZipperThread::ExecuteUnitFailed(): %s\n",
			strerror(status));
		// explicit error - communicate error to Window
		_SendMessageToWindow(ZIPPO_THREAD_EXIT_ERROR);
	}

	Quit();
}


void
ZipperThread::ThreadShutdownFailed(status_t status)
{
	fprintf(stderr, "ZipperThread::ThreadShutdownFailed(): %s\n",
		strerror(status));
}


void
ZipperThread::_MakeShellSafe(BString* string)
{
	string->CharacterEscape("\"$`", '\\');
	string->Prepend("\"");
	string->Append("\"");
}


thread_id
ZipperThread::_PipeCommand(int argc, const char** argv, int& in, int& out,
	int& err, const char** envp)
{
	static BLocker lock;

	if (lock.Lock()) {
		// This function was originally written by Peter Folk
		// <pfolk@uni.uiuc.edu> and published in the BeDevTalk FAQ
		// http://www.abisoft.com/faq/BeDevTalk_FAQ.html#FAQ-209

		thread_id thread;

		// Save current FDs
		int oldIn = dup(STDIN_FILENO);
		int oldOut = dup(STDOUT_FILENO);
		int oldErr = dup(STDERR_FILENO);

		int inPipe[2], outPipe[2], errPipe[2];

		// Create new pipe FDs as stdin, stdout, stderr
		if (pipe(inPipe) < 0)
			goto err1;
		if (pipe(outPipe) < 0)
			goto err2;
		if (pipe(errPipe) < 0)
			goto err3;

		errno = 0;

		// replace old stdin/stderr/stdout
		dup2(inPipe[0], STDIN_FILENO);
		close(inPipe[0]);
		dup2(outPipe[1], STDOUT_FILENO);
		close(outPipe[1]);
		dup2(errPipe[1], STDERR_FILENO);
		close(errPipe[1]);

		if (errno == 0) {
			in = inPipe[1];		// Write to in, appears on cmd's stdin
			out = outPipe[0];	// Read from out, taken from cmd's stdout
			err = errPipe[0];	// Read from err, taken from cmd's stderr

			// execute command
			thread = load_image(argc, argv, envp);
		} else {
			thread = errno;
		}

		// Restore old FDs
		dup2(oldIn, STDIN_FILENO);
		close(oldIn);
		dup2(oldOut, STDOUT_FILENO);
		close(oldOut);
		dup2(oldErr, STDERR_FILENO);
		close(oldErr);

		lock.Unlock();
		return thread;

	err3:
		close(outPipe[0]);
		close(outPipe[1]);
	err2:
		close(inPipe[0]);
		close(inPipe[1]);
	err1:
		close(oldIn);
		close(oldOut);
		close(oldErr);

		lock.Unlock();
		return errno;
	} else {
		return B_ERROR;
	}
}


void
ZipperThread::_SendMessageToWindow(uint32 what, const char* name,
	const char* value)
{
	BMessage msg(what);
	if (name != NULL && value != NULL)
		msg.AddString(name, value);

	fWindowMessenger.SendMessage(&msg);
}


status_t
ZipperThread::SuspendExternalZip()
{
	thread_info info;
	status_t status = get_thread_info(fZipProcess, &info);

	if (status == B_OK && !strcmp(info.name, "zip"))
		return suspend_thread(fZipProcess);

	return status;
}


status_t
ZipperThread::ResumeExternalZip()
{
	thread_info info;
	status_t status = get_thread_info(fZipProcess, &info);

	if (status == B_OK && !strcmp(info.name, "zip"))
		return resume_thread(fZipProcess);

	return status;
}


status_t
ZipperThread::InterruptExternalZip()
{
	thread_info info;
	status_t status = get_thread_info(fZipProcess, &info);

	if (status == B_OK && !strcmp(info.name, "zip")) {
		status = send_signal(fZipProcess, SIGINT);
		WaitOnExternalZip();
		return status;
	}

	return status;
}


status_t
ZipperThread::WaitOnExternalZip()
{
	thread_info info;
	status_t status = get_thread_info(fZipProcess, &info);

	if (status == B_OK && !strcmp(info.name, "zip"))
		return wait_for_thread(fZipProcess, &status);

	return status;
}


status_t
ZipperThread::_SelectInTracker()
{
	entry_ref parentRef;
	BEntry entry(&fOutputEntryRef);

	if (!entry.Exists())
		return B_ENTRY_NOT_FOUND;

	entry.GetParent(&entry);
	entry.GetRef(&parentRef);

	BMessenger trackerMessenger(kTrackerSignature);
	if (!trackerMessenger.IsValid())
		return B_ERROR;

	BMessage request, reply;
	request.what = B_REFS_RECEIVED;
	request.AddRef("refs", &parentRef);
	status_t status = trackerMessenger.SendMessage(&request, &reply);
	if (status != B_OK)
		return status;

	// Wait 0.3 seconds to give Tracker time to populate.
	snooze(300000);

	request.MakeEmpty();
	request.what = BPrivate::kSelect;
	request.AddRef("refs", &fOutputEntryRef);

	reply.MakeEmpty();
	return trackerMessenger.SendMessage(&request, &reply);
}

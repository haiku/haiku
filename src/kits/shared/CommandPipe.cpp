/*
 * Copyright 2007 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ramshankar, v.ramshankar@gmail.com
 */

//! BCommandPipe class to handle reading shell output
//  (stdout/stderr) of other programs into memory.
#include "CommandPipe.h"

#include <stdlib.h>
#include <unistd.h>

#include <image.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>


BCommandPipe::BCommandPipe()
	: fOutDesOpen(false)
	, fErrDesOpen(false)
{
}


BCommandPipe::~BCommandPipe()
{
	FlushArgs();
}


status_t
BCommandPipe::AddArg(const char* arg)
{
	return (fArgList.AddItem(reinterpret_cast<void*>(strdup(arg))) == true ?
		B_OK : B_ERROR);
}


void
BCommandPipe::PrintToStream() const
{
	for (int32 i = 0L; i < fArgList.CountItems(); i++)
		printf("%s ", (char*)fArgList.ItemAtFast(i));

	printf("\n");
}


void
BCommandPipe::FlushArgs()
{
	// Delete all arguments from the list
	for (int32 i = 0; i < fArgList.CountItems(); i++)
		free(fArgList.RemoveItem(0L));

	fArgList.MakeEmpty();
	Close();
}


void
BCommandPipe::Close()
{
	if (fErrDesOpen) {
		close(fErrDes[0]);
		fErrDesOpen = false;
	}
	
	if (fOutDesOpen) {
		close(fOutDes[0]);
		fOutDesOpen = false;
	}
}


const char**
BCommandPipe::Argv(int32& _argc) const
{
	// *** Warning *** Freeing is left to caller!! Indicated in Header
	int32 argc = fArgList.CountItems();
	const char **argv = (const char**)malloc((argc + 1) * sizeof(char*));
	for (int32 i = 0; i < argc; i++)
		argv[i] = (const char*)fArgList.ItemAtFast(i);
	
	argv[argc] = NULL;
	_argc = argc;
	return argv;
}


// #pragma mark -


thread_id
BCommandPipe::PipeAll(int* outAndErrDes) const
{
	// This function pipes both stdout and stderr to the same filedescriptor
	// (outdes)
	int oldstdout;
	int oldstderr;
	pipe(outAndErrDes);
	oldstdout = dup(STDOUT_FILENO);
	oldstderr = dup(STDERR_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	dup2(outAndErrDes[1], STDOUT_FILENO);
	dup2(outAndErrDes[1], STDERR_FILENO);
	
	// Construct the argv vector
	int32 argc = fArgList.CountItems();
	const char **argv = (const char**)malloc((argc + 1) * sizeof(char*));
	for (int32 i = 0; i < argc; i++)
		argv[i] = (const char*)fArgList.ItemAtFast(i);
	
	argv[argc] = NULL;
	
	// Load the app image... and pass the args
	thread_id appThread = load_image((int)argc, argv, const_cast<
		const char**>(environ));

	dup2(oldstdout, STDOUT_FILENO);
	dup2(oldstderr, STDERR_FILENO);
	close(oldstdout);
	close(oldstderr);

	delete[] argv;
	
	return appThread;
}


thread_id
BCommandPipe::Pipe(int* outdes, int* errdes) const
{
	int oldstdout;
	int oldstderr;
	pipe(outdes);
	pipe(errdes);
	oldstdout = dup(STDOUT_FILENO);
	oldstderr = dup(STDERR_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	dup2(outdes[1], STDOUT_FILENO);
	dup2(errdes[1], STDERR_FILENO);
	
	// Construct the argv vector
	int32 argc = fArgList.CountItems();
	const char **argv = (const char**)malloc((argc + 1) * sizeof(char*));
	for (int32 i = 0; i < argc; i++)
		argv[i] = (const char*)fArgList.ItemAtFast(i);
	
	argv[argc] = NULL;
	
	// Load the app image... and pass the args
	thread_id appThread = load_image((int)argc, argv, const_cast<
		const char**>(environ));

	dup2(oldstdout, STDOUT_FILENO);
	dup2(oldstderr, STDERR_FILENO);
	close(oldstdout);
	close(oldstderr);
	
	delete[] argv;

	return appThread;
}


thread_id
BCommandPipe::Pipe(int* outdes) const
{
	// Redirects only output (stdout) to caller, stderr is closed
	int errdes[2];
	thread_id tid = Pipe(outdes, errdes);
	close(errdes[0]);
	close(errdes[1]);
	return tid;
}


thread_id
BCommandPipe::PipeInto(FILE** _out, FILE** _err)
{
	Close();
	thread_id tid = Pipe(fOutDes, fErrDes);
	
	resume_thread(tid);
	
	close(fErrDes[1]);
	close(fOutDes[1]);
	
	fOutDesOpen = true;
	fErrDesOpen = true;

	*_out = fdopen(fOutDes[0], "r");
	*_err = fdopen(fErrDes[0], "r");
	
	return tid;
}


thread_id
BCommandPipe::PipeInto(FILE** _outAndErr)
{
	Close();
	thread_id tid = PipeAll(fOutDes);
	
	if (tid == B_ERROR || tid == B_NO_MEMORY)
		return tid;
	
	resume_thread(tid);
	
	close(fOutDes[1]);
	fOutDesOpen = true;
	
	*_outAndErr = fdopen(fOutDes[0], "r");
	return tid;
}


// #pragma mark -


void
BCommandPipe::Run()
{
	// Runs the command without bothering to redirect streams, this is similar
	// to system() but uses pipes and wait_for_thread.... Synchronous.
	int outdes[2], errdes[2];
	status_t exitCode;
	wait_for_thread(Pipe(outdes, errdes), &exitCode);

	close(outdes[0]);
	close(errdes[0]);
	close(outdes[1]);
	close(errdes[1]);
}


void
BCommandPipe::RunAsync()
{
	// Runs the command without bothering to redirect streams, this is similar
	// to system() but uses pipes.... Asynchronous.
	Close();
	FILE* f = NULL;
	PipeInto(&f);
	fclose(f);
}


// #pragma mark -


BString
BCommandPipe::ReadLines(FILE* file, bool* cancel, BMessenger& target,
	const BMessage& message, const BString& stringFieldName)
{
	// Reads output of file, line by line. The entire output is returned
	// and as each line is being read "target" (if any) is informed,
	// with "message" i.e. AddString (stringFieldName, <line read from pipe>)
	
	// "cancel" cancels the reading process, when it becomes true (unless its
	// waiting on fgetc()) and I don't know how to cancel the waiting fgetc()
	// call.
	
	BString result;
	BString line;
	BMessage updateMsg(message);

	while (!feof(file)) {
		if (cancel != NULL && *cancel == true)
			break;
		
		unsigned char c = fgetc(file);
		
		if (c != 255) {
			line << (char)c;
			result << (char)c;
		}
		
		if (c == '\n') {
			updateMsg.RemoveName(stringFieldName.String());
			updateMsg.AddString(stringFieldName.String(), line);
			target.SendMessage(&updateMsg);
			line = "";
		}
	}
	
	return result;
}


BCommandPipe&
BCommandPipe::operator<<(const char* _arg)
{
	AddArg(_arg);
	return *this;
}


BCommandPipe&
BCommandPipe::operator<<(const BString& _arg)
{
	AddArg(_arg.String());
	return *this;
}


BCommandPipe&
BCommandPipe::operator<<(const BCommandPipe& _arg)
{
	int32 argc;
	const char** argv = _arg.Argv(argc);
	for (int32 i = 0; i < argc; i++)
		AddArg(argv[i]);
	
	return *this;
}


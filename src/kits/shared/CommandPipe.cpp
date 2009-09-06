/*
 * Copyright 2007 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ramshankar, v.ramshankar@gmail.com
 *		Stephan Aßmus <superstippi@gmx.de>
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
	:
	fStdOutOpen(false),
	fStdErrOpen(false)
{
}


BCommandPipe::~BCommandPipe()
{
	FlushArgs();
}


status_t
BCommandPipe::AddArg(const char* arg)
{
	if (arg == NULL || arg[0] == '\0')
		return B_BAD_VALUE;

	char* argCopy = strdup(arg);
	if (argCopy == NULL)
		return B_NO_MEMORY;

	if (!fArgList.AddItem(reinterpret_cast<void*>(argCopy))) {
		free(argCopy);
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
BCommandPipe::PrintToStream() const
{
	for (int32 i = 0L; i < fArgList.CountItems(); i++)
		printf("%s ", reinterpret_cast<char*>(fArgList.ItemAtFast(i)));

	printf("\n");
}


void
BCommandPipe::FlushArgs()
{
	// Delete all arguments from the list
	for (int32 i = fArgList.CountItems() - 1; i >= 0; i--)
		free(fArgList.ItemAtFast(i));
	fArgList.MakeEmpty();

	Close();
}


void
BCommandPipe::Close()
{
	if (fStdErrOpen) {
		close(fStdErr[0]);
		fStdErrOpen = false;
	}

	if (fStdOutOpen) {
		close(fStdOut[0]);
		fStdOutOpen = false;
	}
}


const char**
BCommandPipe::Argv(int32& argc) const
{
	// NOTE: Freeing is left to caller as indicated in the header!
	argc = fArgList.CountItems();
	const char** argv = reinterpret_cast<const char**>(
		malloc((argc + 1) * sizeof(char*)));
	for (int32 i = 0; i < argc; i++)
		argv[i] = reinterpret_cast<const char*>(fArgList.ItemAtFast(i));

	argv[argc] = NULL;
	return argv;
}


// #pragma mark -


thread_id
BCommandPipe::PipeAll(int* stdOutAndErr) const
{
	// This function pipes both stdout and stderr to the same filedescriptor
	// (stdOut)
	int oldStdOut;
	int oldStdErr;
	pipe(stdOutAndErr);
	oldStdOut = dup(STDOUT_FILENO);
	oldStdErr = dup(STDERR_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	// TODO: This looks broken, using "stdOutAndErr[1]" twice!
	dup2(stdOutAndErr[1], STDOUT_FILENO);
	dup2(stdOutAndErr[1], STDERR_FILENO);

	// Construct the argv vector
	int32 argc;
	const char** argv = Argv(argc);

	// Load the app image... and pass the args
	thread_id appThread = load_image((int)argc, argv,
		const_cast<const char**>(environ));

	dup2(oldStdOut, STDOUT_FILENO);
	dup2(oldStdErr, STDERR_FILENO);
	close(oldStdOut);
	close(oldStdErr);

	free(argv);

	return appThread;
}


thread_id
BCommandPipe::Pipe(int* stdOut, int* stdErr) const
{
	int oldStdOut;
	int oldStdErr;
	pipe(stdOut);
	pipe(stdErr);
	oldStdOut = dup(STDOUT_FILENO);
	oldStdErr = dup(STDERR_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	dup2(stdOut[1], STDOUT_FILENO);
	dup2(stdErr[1], STDERR_FILENO);

	// Construct the argv vector
	int32 argc;
	const char** argv = Argv(argc);

	// Load the app image... and pass the args
	thread_id appThread = load_image((int)argc, argv, const_cast<
		const char**>(environ));

	dup2(oldStdOut, STDOUT_FILENO);
	dup2(oldStdErr, STDERR_FILENO);
	close(oldStdOut);
	close(oldStdErr);

	free(argv);

	return appThread;
}


thread_id
BCommandPipe::Pipe(int* stdOut) const
{
	// Redirects only output (stdout) to caller, stderr is closed
	int stdErr[2];
	thread_id tid = Pipe(stdOut, stdErr);
	close(stdErr[0]);
	close(stdErr[1]);
	return tid;
}


thread_id
BCommandPipe::PipeInto(FILE** _out, FILE** _err)
{
	Close();

	thread_id tid = Pipe(fStdOut, fStdErr);
	if (tid >= 0)
		resume_thread(tid);

	close(fStdErr[1]);
	close(fStdOut[1]);

	fStdOutOpen = true;
	fStdErrOpen = true;

	*_out = fdopen(fStdOut[0], "r");
	*_err = fdopen(fStdErr[0], "r");

	return tid;
}


thread_id
BCommandPipe::PipeInto(FILE** _outAndErr)
{
	Close();

	thread_id tid = PipeAll(fStdOut);
	if (tid >= 0)
		resume_thread(tid);

	close(fStdOut[1]);
	fStdOutOpen = true;

	*_outAndErr = fdopen(fStdOut[0], "r");
	return tid;
}


// #pragma mark -


void
BCommandPipe::Run()
{
	// Runs the command without bothering to redirect streams, this is similar
	// to system() but uses pipes and wait_for_thread.... Synchronous.
	int stdOut[2], stdErr[2];
	status_t exitCode;
	wait_for_thread(Pipe(stdOut, stdErr), &exitCode);

	close(stdOut[0]);
	close(stdErr[0]);
	close(stdOut[1]);
	close(stdErr[1]);
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


status_t
BCommandPipe::ReadLines(FILE* file, LineReader* lineReader)
{
	// Reads output of file, line by line. Each line is passed to lineReader
	// for inspection, and the IsCanceled() method is repeatedly called.

	if (file == NULL || lineReader == NULL)
		return B_BAD_VALUE;

	BString line;

	while (!feof(file)) {
		if (lineReader->IsCanceled())
			return B_CANCELED;

		unsigned char c = fgetc(file);
			// TODO: fgetc() blocks, is there a way to make it timeout?

		if (c != 255)
			line << (char)c;

		if (c == '\n') {
			status_t ret = lineReader->ReadLine(line);
			if (ret != B_OK)
				return ret;
			line = "";
		}
	}

	return B_OK;
}


BString
BCommandPipe::ReadLines(FILE* file)
{
	class AllLinesReader : public LineReader {
	public:
		AllLinesReader()
			:
			fResult("")
		{
		}

		virtual bool IsCanceled()
		{
			return false;
		}

		virtual status_t ReadLine(const BString& line)
		{
			int lineLength = line.Length();
			int resultLength = fResult.Length();
			fResult << line;
			if (fResult.Length() != lineLength + resultLength)
				return B_NO_MEMORY;
			return B_OK;
		}

		BString Result() const
		{
			return fResult;
		}

	private:
		BString fResult;
	} lineReader;

	ReadLines(file, &lineReader);

	return lineReader.Result();
}


// #pragma mark -


BCommandPipe&
BCommandPipe::operator<<(const char* arg)
{
	AddArg(arg);
	return *this;
}


BCommandPipe&
BCommandPipe::operator<<(const BString& arg)
{
	AddArg(arg.String());
	return *this;
}


BCommandPipe&
BCommandPipe::operator<<(const BCommandPipe& arg)
{
	int32 argc;
	const char** argv = arg.Argv(argc);
	for (int32 i = 0; i < argc; i++)
		AddArg(argv[i]);

	return *this;
}


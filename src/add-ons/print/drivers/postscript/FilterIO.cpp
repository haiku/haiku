/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Ithamar R. Adema <ithamar.adema@team-embedded.nl>
*/


#include "FilterIO.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <image.h>

#include <String.h>


FilterIO::FilterIO(const BString& cmdline)
	:
	BDataIO()
{
	BString cmd(cmdline);
	const char* argv[4];

	argv[0] = strdup("/bin/sh");
	argv[1] = strdup("-c");
	argv[2] = strdup(cmd.String());
	argv[3] = NULL;

	InitData(3, argv);

	free((void*)argv[0]);
	free((void*)argv[1]);
	free((void*)argv[2]);
}


FilterIO::FilterIO(int argc, const char **argv, const char **envp)
	:
	BDataIO()
{
	InitData(argc, argv, envp);
}


status_t
FilterIO::InitData(int argc, const char** argv, const char** envp)
{
	fStdIn = fStdOut = fStdErr = -1;
	fInitErr = B_OK;

	fThreadId = PipeCommand(argc, argv, fStdIn, fStdOut, fStdErr, envp);
	if (fThreadId < 0)
		fInitErr = fThreadId;

	// lower the command priority since it is a background task.
	set_thread_priority(fThreadId, B_LOW_PRIORITY);
	resume_thread(fThreadId);

	return fInitErr;
}


FilterIO::~FilterIO()
{
	::close(fStdIn);
	::close(fStdOut);
	::close(fStdErr);
}


ssize_t
FilterIO::Read(void* buffer, size_t size)
{
	return ::read(fStdOut, buffer, size);
}


ssize_t
FilterIO::Write(const void* buffer, size_t size)
{
	return ::write(fStdIn, buffer, size);
}


thread_id
FilterIO::PipeCommand(int argc, const char** argv, int& in, int& out, int& err,
	const char** envp)
{
	// This function written by Peter Folk <pfolk@uni.uiuc.edu>
	// and published in the BeDevTalk FAQ
	// http://www.abisoft.com/faq/BeDevTalk_FAQ.html#FAQ-209

	if (!envp)
		envp = (const char**)environ;

	// Save current FDs
	int old_in  =  dup(0);
	int old_out  =  dup(1);
	int old_err  =  dup(2);

	int filedes[2];

	// Create new pipe FDs as stdin, stdout, stderr
	pipe(filedes);  dup2(filedes[0], 0); close(filedes[0]);
	in = filedes[1];  // Write to in, appears on cmd's stdin
	pipe(filedes);  dup2(filedes[1], 1); close(filedes[1]);
	out = filedes[0]; // Read from out, taken from cmd's stdout
	pipe(filedes);  dup2(filedes[1], 2); close(filedes[1]);
	err = filedes[0]; // Read from err, taken from cmd's stderr

	// "load" command.
	thread_id ret  =  load_image(argc, argv, envp);
	if (ret < B_OK)
		goto cleanup;

	// thread ret is now suspended.

	setpgid(ret, ret);

cleanup:
	// Restore old FDs
	close(0); dup(old_in); close(old_in);
	close(1); dup(old_out); close(old_out);
	close(2); dup(old_err); close(old_err);

	/* Theoretically I should do loads of error checking, but
	   the calls aren't very likely to fail, and that would 
	   muddy up the example quite a bit.  YMMV. */

	return ret;
}

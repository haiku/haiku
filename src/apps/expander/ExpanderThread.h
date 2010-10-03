/*
 * Copyright 2004-2010, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 * Original code from ZipOMatic by jonas.sundstrom@kirilla.com
 */
#ifndef __EXPANDERTHREAD_H__
#define __EXPANDERTHREAD_H__

#include <Message.h>
#include <Volume.h>
#include <String.h>
#include <OS.h>
#include <FindDirectory.h>

#include "GenericThread.h"
#include <stdio.h>
#include <stdlib.h>

extern const char * ExpanderThreadName;

class ExpanderThread : public GenericThread
{
	public:
		ExpanderThread(BMessage *refs_message, BMessenger *messenger);
		~ExpanderThread();

		status_t SuspendExternalExpander();
		status_t ResumeExternalExpander();
		status_t InterruptExternalExpander();
		status_t WaitOnExternalExpander();
		void PushInput(BString text);

	private:

		virtual status_t ThreadStartup();
		virtual status_t ExecuteUnit();
		virtual status_t ThreadShutdown();

		virtual void ThreadStartupFailed(status_t a_status);
		virtual void ExecuteUnitFailed(status_t a_status);
		virtual void ThreadShutdownFailed(status_t a_status);

		status_t ProcessRefs(BMessage *	msg);

		thread_id PipeCommand(int argc, const char **argv,
			int & in, int & out, int & err,
			const char **envp = (const char **) environ);

		BMessenger * 	fWindowMessenger;

		thread_id		fThreadId;
		int				fStdIn;
		int				fStdOut;
		int				fStdErr;
		FILE *			fExpanderOutput;
		FILE *			fExpanderError;
		char			fExpanderOutputBuffer[LINE_MAX];
};

#endif // __EXPANDERTHREAD_H__

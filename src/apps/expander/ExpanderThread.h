/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// ExpanderThread.h
//
// Copyright (c) 2004 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef __EXPANDERTHREAD_H__
#define __EXPANDERTHREAD_H__

#include <Message.h>
#include <Volume.h>
#include <String.h>
#include <OS.h> 
#include <FindDirectory.h> 

#include <GenericThread.h>
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
		BString			fExpanderOutputString;
		char *			fExpanderOutputBuffer;
};

#endif // __EXPANDERTHREAD_H__

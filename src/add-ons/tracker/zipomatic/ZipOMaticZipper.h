/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas.sundstrom@kirilla.com
 */
#ifndef ZIPPER_THREAD_H
#define ZIPPER_THREAD_H


#include "GenericThread.h"

#include <Messenger.h>

#include <stdio.h>

class BMessage;
class BWindow;


class ZipperThread : public GenericThread {
	public:
							ZipperThread(BMessage* refsMessage, BWindow* window);
							~ZipperThread();

				status_t	SuspendExternalZip();
				status_t	ResumeExternalZip();
				status_t	InterruptExternalZip();
				status_t	WaitOnExternalZip();

	private:
		virtual status_t	ThreadStartup();
		virtual status_t	ExecuteUnit();
		virtual status_t	ThreadShutdown();

		virtual void		ThreadStartupFailed(status_t a_status);
		virtual void		ExecuteUnitFailed(status_t a_status);
		virtual void		ThreadShutdownFailed(status_t a_status);

				status_t	ProcessRefs(BMessage* msg);
				void		_MakeShellSafe(BString* string);

				thread_id	_PipeCommand(int argc, const char** argv, 
								int& in, int& out, int& err,
								const char** envp = (const char**)environ);

				void		_SendMessageToWindow(uint32 what,
								const char* name  =  NULL, const char* value = NULL);

				BMessenger	fWindowMessenger;
				thread_id	fZipProcess;
				int			m_std_in;
				int			m_std_out;
				int			m_std_err;
				FILE*		fOutputFile;
};

#endif	// ZIPPER_THREAD_H


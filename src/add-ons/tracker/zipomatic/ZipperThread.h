/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */
#ifndef _ZIPPER_THREAD_H
#define _ZIPPER_THREAD_H


#include <stdio.h>
#include <stdlib.h>

#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>
#include <Window.h>

#include "GenericThread.h"


class ZipperThread : public GenericThread {
public:
							ZipperThread(BMessage* refsMessage,
								BWindow* window);
							~ZipperThread();

			status_t		SuspendExternalZip();
			status_t		ResumeExternalZip();
			status_t		InterruptExternalZip();
			status_t		WaitOnExternalZip();

protected:
	virtual	status_t		ThreadStartup();
	virtual	status_t		ExecuteUnit();
	virtual	status_t		ThreadShutdown();

	virtual	void			ThreadStartupFailed(status_t status);
	virtual	void			ExecuteUnitFailed(status_t status);
	virtual	void			ThreadShutdownFailed(status_t status);

private:
			void			_MakeShellSafe(BString* string);

			thread_id		_PipeCommand(int argc, const char** argv,
								int& in, int& out, int& err,
								const char** envp = (const char**)environ);

			void			_SendMessageToWindow(uint32 what,
								const char* name = NULL,
								const char* value = NULL);

			status_t		_SelectInTracker();

			BMessenger		fWindowMessenger;
			thread_id		fZipProcess;
			int				fStdIn;
			int				fStdOut;
			int				fStdErr;
			FILE*			fOutputFile;
			entry_ref		fOutputEntryRef;
};

#endif	// _ZIPPER_THREAD_H


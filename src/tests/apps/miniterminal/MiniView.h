#ifndef _MINI_TERMINAL_H_
#define _MINI_TERMINAL_H_

#include <MessageFilter.h>

#include "ViewBuffer.h"
#include "Console.h"

class MiniView : public ViewBuffer {
public:
							MiniView(BRect frame);
virtual						~MiniView();

		void				Start();
		status_t			OpenTTY();
		status_t			SpawnThreads();
virtual	void				KeyDown(const char *bytes, int32 numBytes);

private:
static	int32				ConsoleWriter(void *arg);
static	int32				ExecuteShell(void *arg);
static	filter_result		MessageFilter(BMessage *message, BHandler **target, BMessageFilter *filter);

		Console				*fConsole;
		int					fMasterFD;
		int					fSlaveFD;
		thread_id			fShellProcess;
		thread_id			fConsoleWriter;
};

#endif

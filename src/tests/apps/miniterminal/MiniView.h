#ifndef _MINI_TERMINAL_H_
#define _MINI_TERMINAL_H_

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

		Console				*fConsole;
		int					fMasterFD;
		int					fSlaveFD;
		thread_id			fShellProcess;
		thread_id			fConsoleWriter;
};

#endif

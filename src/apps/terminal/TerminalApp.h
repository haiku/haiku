#ifndef TERMINAL_APP_H
#define TERMINAL_APP_H

#include <Application.h>
#include <Message.h>

class TerminalWindow;

class TerminalApp
	: public BApplication
{
public:
					TerminalApp();
	virtual void 	MessageReceived(BMessage *message);
	virtual	void	ArgvReceived(int32 argc, const char *argv[], const char * cwd);
	virtual void	RefsReceived(BMessage *message);
	virtual void	ReadyToRun();

	virtual	void	DispatchMessage(BMessage *an_event, BHandler *handler);

			int32	NumberOfWindows();
			void	OpenTerminal();
			void	OpenTerminal(entry_ref * ref);
			void	CloseTerminal();
	
private:
	int32			fWindowCount;
	int32			fNext_Terminal_Window;
		
};

extern TerminalApp * terminal_app;

#endif // TERMINAL_APP_H


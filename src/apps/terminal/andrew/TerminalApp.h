#ifndef TERMINAL_APP_H
#define TERMINAL_APP_H

#include <Application.h>
#include <Message.h>
#include <Roster.h>
#include <cassert>

class TerminalWindow;

class TerminalApp
	: public BApplication
{
public:
					TerminalApp();
	virtual         ~TerminalApp();
	virtual void 	MessageReceived(BMessage *message);
	virtual	void	ArgvReceived(int32 argc, char * const argv[], const char * cwd);
	virtual	void	ArgvReceived(int32 argc, char **argv) { assert(false); }
	virtual void	RefsReceived(BMessage *message);
	virtual void	ReadyToRun();

	virtual	void	DispatchMessage(BMessage *an_event, BHandler *handler);

			void	OpenTerminal(BMessage * message = 0);
private:
			void	PrintUsage(const char * execname);
	bool            fWindowOpened;
	bool            fArgvOkay;
};

extern TerminalApp * terminal_app;

#endif // TERMINAL_APP_H


#include <iostream>
#include <Autolock.h>
#include <Path.h>
#include <Point.h>
#include "Constants.h"
#include "TerminalApp.h"
#include "TerminalWindow.h"

BPoint windowPoint(7,26);
BPoint cascadeOffset(15,15);

TerminalApp * terminal_app;

TerminalApp::TerminalApp()
	: BApplication(APP_SIGNATURE)
{
	fWindowCount = 0;
	fNext_Terminal_Window = 1;
	terminal_app = this;
}

void TerminalApp::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if ( msg->what == B_ARGV_RECEIVED ) {
		int32 argc;
		if (msg->FindInt32("argc",&argc) != B_OK) {
			argc = 0;
		}
		const char ** argv = new (const char*)[argc];
		for (int arg = 0; (arg < argc) ; arg++) {
			if (msg->FindString("argv",arg,&argv[arg]) != B_OK) {
				argv[arg] = "";
			}
		}
		const char * cwd;
		if (msg->FindString("cwd",&cwd) != B_OK) {
			cwd = "";
		}
		ArgvReceived(argc, argv, cwd);
	} else {
		BApplication::DispatchMessage(msg,handler);
	}
}


void
TerminalApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case TERMINAL_START_NEW_TERMINAL:
			OpenTerminal();
		break;
		case B_SILENT_RELAUNCH:
			OpenTerminal();
		break;
		default:
			BApplication::MessageReceived(message);
		break;
	} 
}

void
TerminalApp::OpenTerminal()
{
	new TerminalWindow(windowPoint,fNext_Terminal_Window++);
	windowPoint += cascadeOffset; // TODO: wrap around screen
	fWindowCount++;
}

void
TerminalApp::OpenTerminal(entry_ref * ref)
{
	new TerminalWindow(ref,fNext_Terminal_Window++);
	fWindowCount++;
}

void
TerminalApp::CloseTerminal()
{
	fWindowCount--;
	if (fWindowCount == 0) {
		BAutolock lock(this);
		Quit();
	}
}

void
TerminalApp::RefsReceived(BMessage *message)
{
	int32		refNum;
	entry_ref	ref;
	status_t	err;
	
	refNum = 0;
	do {
		err = message->FindRef("refs", refNum, &ref);
		if (err != B_OK)
			return;
		OpenTerminal(&ref);
		refNum++;
	} while (true);
}

// TODO: find the arguments for Terminal and implement them all
void
TerminalApp::ArgvReceived(int32 argc, const char *argv[], const char * cwd)
{
	const char * execname = (argc >= 1 ? argv[0] : "");
	cout << execname << ": command line arguments not yet implemented" << endl;
	return;
}

void 
TerminalApp::ReadyToRun() 
{
	if (fWindowCount == 0) {
		OpenTerminal();
	}
}

int32
TerminalApp::NumberOfWindows()
{
 	return fWindowCount;
}

int
main()
{
	TerminalApp	terminal;
	terminal.Run();
	return 0;
}

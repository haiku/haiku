#include <Autolock.h>
#include <Path.h>
#include <Point.h>

#include <Constants.h>
#include <TerminalApp.h>
#include <TerminalWindow.h>

#include <stdio.h>
#include <getopt.h>

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

void
TerminalApp::PrintUsage(const char * execname) {
	if (execname == 0) {
		execname = "Terminal";
	}
	fprintf(stderr,"Usage: %s [OPTIONS] [SHELL]\n",execname);
	fprintf(stderr,"Open a terminal window.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"  -curbg     COLOR  use COLOR as the cursor background color\n");
	fprintf(stderr,"  -curfg     COLOR  use COLOR as the cursor foreground (text) color\n");
	fprintf(stderr,"  -bg        COLOR  use COLOR as the background color\n");
	fprintf(stderr,"  -fg        COLOR  use COLOR as the foreground (text) color\n");
	fprintf(stderr,"  -g, -geom  NxM    use geometry N columns by M rows\n");
	fprintf(stderr,"  -h, -help         print this help\n");
	fprintf(stderr,"  -m, -meta         pass through the META key to the shell\n");
	fprintf(stderr,"  -p, -pref  FILE   use FILE as a Terminal preference file\n");
	fprintf(stderr,"  -selbg     COLOR  use COLOR as the selection background color\n");
	fprintf(stderr,"  -selfg     COLOR  use COLOR as the selection foreground (text) color\n");
	fprintf(stderr,"  -t, -title TITLE  use TITLE as the window title\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Report bugs to shatty@myrealbox.com\n");
	fprintf(stderr,"\n");
}

struct option curbg_opt = { "curbg", required_argument, 0, 1 } ;
struct option curfg_opt = { "curfg", required_argument, 0, 2 } ;
struct option bg_opt = { "bg", required_argument, 0, 3 } ;
struct option fg_opt = { "fg", required_argument, 0, 4 } ;
struct option geom_opt = { "geom", required_argument, 0, 'g' } ;
struct option help_opt = { "help", no_argument, 0, 'h' } ;
struct option meta_opt = { "meta", no_argument, 0, 'm' } ;
struct option pref_opt = { "pref", required_argument, 0, 'p' } ;
struct option selbg_opt = { "selbg", required_argument, 0, 5 } ;
struct option selfg_opt = { "selfg", required_argument, 0, 6 } ;
struct option title_opt = { "title", required_argument, 0, 't' } ;

struct option options[] =
 { curbg_opt, curfg_opt, bg_opt, fg_opt, 
   geom_opt, help_opt, meta_opt, pref_opt,
   selbg_opt, selfg_opt, title_opt, 0
 };

// TODO: implement the arguments for Terminal
void
TerminalApp::ArgvReceived(int32 argc, const char *argv[], const char * cwd)
{
	if (argc <= 1) {
		OpenTerminal();
	} else if (argv[1][0] != '-') {
		argc--;
		printf("execvp(%ld,%s ...)\n",argc,argv[1]);
		OpenTerminal();
	} else {
		const char * execname = (argc >= 1 ? argv[0] : "");
		const char * title = 0;
		int indexptr = 0;
		int ch;
		char * const * optargv = (char * const *)argv;
		while ((ch = getopt_long_only(argc, optargv, "g:hmp:t:", options, &indexptr)) != -1) {
			switch (ch) {
			case 'h':
				PrintUsage(execname);
				return;
			break;
			case 't':
				title = optarg;
				printf("title is %s\n", title);
			break;
			case ':':
				switch (optopt) {
				case 't':
					printf("-t without title\n");
				break;
				default:
					printf("-%c missing argument\n", optopt);
				}
			break;
			case '?':
				printf("unknown arg %c\n", optopt);
			break;
			default:
				printf("%s\n",argv[optind]);
				return;
			}
		}
		OpenTerminal();
	}
	return;
}

void 
TerminalApp::ReadyToRun() 
{
	if (fWindowCount == 0) {
		Quit();
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

#include <Autolock.h>
#include <Path.h>
#include <Point.h>
#include <Alert.h>

#include <Constants.h>
#include <TerminalApp.h>
#include <TerminalWindow.h>

#include <stdio.h>
#include <getopt.h>

TerminalApp * terminal_app;

TerminalApp::TerminalApp()
	: BApplication(APP_SIGNATURE)
{
	fWindowOpened = false;
	fArgvOkay = true;
	terminal_app = this;
}

TerminalApp::~TerminalApp()
{
	
}

void TerminalApp::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if ( msg->what == B_ARGV_RECEIVED ) {
		int32 argc;
		if (msg->FindInt32("argc",&argc) != B_OK) {
			argc = 0;
		}
		char ** argv = new (char*)[argc];
		for (int arg = 0; (arg < argc) ; arg++) {
			BString string;
			if (msg->FindString("argv",arg,&string) != B_OK) {
				argv[arg] = "";
			} else {
				char * chars = new char[string.Length()];
				strcpy(chars,string.String());
				argv[arg] = chars;
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
		default:
			BApplication::MessageReceived(message);
		break;
	} 
}

void
TerminalApp::OpenTerminal(BMessage * message)
{
	TerminalWindow * terminal = new TerminalWindow(message);
	(void)terminal;
	fWindowOpened = true;
}

#include <Roster.h>

void
TerminalApp::RefsReceived(BMessage *message)
{
	int32 i = 0;
	entry_ref ref;
	if (IsLaunching()) {
		// peel off the first ref and open it ourselves
		if (message->FindRef("refs",i++,&ref) == B_OK) {
			BMessage file(OPEN_TERMINAL);
			file.AddRef("refs",&ref);
			OpenTerminal(&file);
		}
	}
	// handle any other refs by launching them as separate apps
	while (message->FindRef("refs",i++,&ref) == B_OK) {
		BMessage * file = new BMessage(OPEN_TERMINAL);
		file->AddRef("refs",&ref);
		be_roster->Launch(APP_SIGNATURE,file);
	}
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
   selbg_opt, selfg_opt, title_opt, {0}
 };

status_t
string2color (const char * name, rgb_color * color) {
	if (!name || !color) {
		return B_BAD_VALUE;
	}
	if (strcmp("black",name) == 0) {
		color->red = 0;
		color->green = 0;
		color->blue = 0;
		return B_OK;
	}
	if (strcmp("red",name) == 0) {
		color->red = 128;
		color->green = 0;
		color->blue = 0;
		return B_OK;
	}
	return B_ERROR;
}

// TODO: implement the arguments for Terminal
void
TerminalApp::ArgvReceived(int32 argc, char * const argv[], const char * cwd)
{
	BMessage terminal(OPEN_TERMINAL);
	fArgvOkay = false;
	if (argc > 1) {
		if (argv[1][0] == '-') {
			const char * execname = (argc >= 1 ? argv[0] : "");
			int indexptr = 0;
			int ch;
			char * const * optargv = argv;
			while ((ch = getopt_long_only(argc, optargv, "hg:mp:t:", options, &indexptr)) != -1) {
				switch (ch) {
				case 'h':
					PrintUsage(execname);
					return;
				break;
				case 'g':
					printf("geometry is %s = ",optarg);
					int m, n;
					if ((sscanf(optarg,"%dx%d",&m,&n) != 2) || (m < 0) || (n < 0)) {
						printf("??\n");
						printf("geometry must be of the format MxN where M and N are positive integers\n");
						return;
					}
					printf("%d,%d\n",m,n);
					terminal.AddInt32("columns",m);
					terminal.AddInt32("rows",n);					
				break;
				case 'm':
					printf("pass meta through to shell\n");
					terminal.AddBool("meta",true);
				break;
				case 't':
					printf("title is %s\n",optarg);
					terminal.AddString("title",optarg);
				break;
				case 'p': {
					printf("prefs file is %s\n",optarg);
					BPath pref;
					if (optarg[0] == '/') {
						pref.SetTo(optarg);
					} else {
						pref.SetTo(cwd,optarg);
					}
					entry_ref ref;
					switch (get_ref_for_path(pref.Path(),&ref)) {
						case B_OK:
						break;
						case B_ENTRY_NOT_FOUND:
							printf("could not find entry for prefs file '%s'\n",optarg);
							return;
						break;
						case B_NO_MEMORY:
							printf("not enough memory in get_ref_for_path\n");
							return;
						break;
						default:
							printf("unknown error in get_ref_for_path\n");
							return;
					}
					terminal.AddRef("refs",&ref);
				}
				break;					
				case ':':
					switch (optopt) {
					case 't':
						printf("-t option must be specified with a title\n");
						return;
					break;
					default:
						printf("-%c missing argument\n", optopt);
						return;
					}
				break;
				case '?':
					// getopt prints error message
					return;
				break;
				default:
					switch (options[indexptr].val) {
					case 1: { // curbg
						printf("curbg = %s\n",optarg);
						rgb_color color;
						if (string2color(optarg,&color) != B_OK) {
							printf("invalid color specifier for curbg\n");
							return;
						}
						terminal.AddData("curbg",B_RGB_32_BIT_TYPE,
						                  &color,sizeof(color),true,1);
					}
					break;
					case 2: { // curfg
						printf("curfg = %s\n",optarg);
						rgb_color color;
						if (string2color(optarg,&color) != B_OK) {
							printf("invalid color specifier for curfg\n");
							return;
						}
						terminal.AddData("curfg",B_RGB_32_BIT_TYPE,
						                  &color,sizeof(color),true,1);
					}
					break;
					case 3: { // bg
						printf("bg = %s\n",optarg);
						rgb_color color;
						if (string2color(optarg,&color) != B_OK) {
							printf("invalid color specifier for bg\n");
							return;
						}
						terminal.AddData("bg",B_RGB_32_BIT_TYPE,
						                  &color,sizeof(color),true,1);
					}
					break;
					case 4: { // fg
						printf("fg = %s\n",optarg);
						rgb_color color;
						if (string2color(optarg,&color) != B_OK) {
							printf("invalid color specifier for fg\n");
							return;
						}
						terminal.AddData("fg",B_RGB_32_BIT_TYPE,
						                  &color,sizeof(color),true,1);
					}
					break;
					case 5: { // selbg
						printf("selbg = %s\n",optarg);
						rgb_color color;
						if (string2color(optarg,&color) != B_OK) {
							printf("invalid color specifier for selbg\n");
							return;
						}
						terminal.AddData("selbg",B_RGB_32_BIT_TYPE,
						                  &color,sizeof(color),true,1);
					}
					break;
					case 6: { // selfg
						printf("selfg = %s\n",optarg);
						rgb_color color;
						if (string2color(optarg,&color) != B_OK) {
							printf("invalid color specifier for selfg\n");
							return;
						}
						terminal.AddData("selfg",B_RGB_32_BIT_TYPE,
						                  &color,sizeof(color),true,1);
					}
					break;
					default:
						printf("invalid indexptr %d\n",indexptr);
						return;
					}
				}
			}
		}
		while (optind < argc) {
			printf("adding string %s\n",argv[optind]);
			terminal.AddString("argv",argv[optind++]);
		}
	}
	OpenTerminal(&terminal);
	fArgvOkay = true;
	return;
}

void 
TerminalApp::ReadyToRun() 
{
	if (!fArgvOkay) {
		Quit();
		return;
	} 
	if (!fWindowOpened) {
		OpenTerminal();
	}
}

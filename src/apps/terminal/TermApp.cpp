/*
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed unter the terms of the MIT license.
 */
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <malloc.h>

#include <storage/Path.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <Alert.h>
#include <Screen.h>
#include <String.h>
#include <TextView.h>

#include "TermApp.h"
#include "TermBuffer.h"
#include "TermWindow.h"
#include "TermConst.h"
#include "spawn.h"
#include "PrefHandler.h"
#include "CodeConv.h"

// Preference temporary
extern PrefHandler *gTermPref;
extern char gWindowName[];

bool usage_requested = false;
bool geometry_requested = false;
bool color_requested = false;

struct standard_args {
	char *name;
	char *longname;
	int priority;
	int nargs;
	const char *prefname;
};

struct standard_args standard_args[] = {
	{ "-h", "--help", 90, 0, NULL },
	{ "-f", "--fullscreen", 30, 0, NULL },
	{ "-p", "--preference", 80, 1, NULL },
	{ "-t", "--title", 70, 1, NULL },
	{ "-geom", "--geometry", 50, 1, NULL },
#if 0
	{ "-fg", "--text-fore-color", 40, 1, PREF_TEXT_FORE_COLOR },
	{ "-bg", "--text-back-color", 35, 1, PREF_TEXT_BACK_COLOR },
	{ "-curfg", "--cursor-fore-color", 30, 1, PREF_CURSOR_FORE_COLOR },
	{ "-curbg", "--cursor-back-color", 25, 1, PREF_CURSOR_BACK_COLOR },
	{ "-selfg", "--select-fore-color", 20, 1, PREF_SELECT_FORE_COLOR },
	{ "-selbg", "--select-back-color", 15, 1, PREF_SELECT_BACK_COLOR },
	{ "-imfg", "--im-fore-color", 10, 1, PREF_IM_FORE_COLOR },
	{ "-imbg", "--im-back-color", 5, 1, PREF_IM_BACK_COLOR },
	{ "-imsel", "--im-select-color", 0, 1, PREF_IM_SELECT_COLOR },
#endif
};

int argmatch(char **, int, char *, char *, int, char **, int *);
void sort_args(int, char **);
int text_to_rgb(char *, rgb_color *, char *);


const ulong MSG_ACTIVATE_TERM = 'msat';
const ulong MSG_TERM_IS_MINIMIZE = 'mtim';


TermApp::TermApp()
	: BApplication(TERM_SIGNATURE)
{
	fWindowNumber = FindTerminalId();

	char title[256];
	snprintf(title, sizeof(title), "Terminal %d", fWindowNumber);
	fWindowTitle = title;
	fStartFullscreen = false;

	int i = fWindowNumber / 16;
	int j = fWindowNumber % 16;

	int k =(j * 16) + (i * 64) + 50;
	int l =(j * 16)  + 50;

	fTermWindow = NULL;
	fAboutPanel = NULL;
	fTermFrame.Set(k, l, k + 50, k + 50);

	gTermPref = new PrefHandler();
}


TermApp::~TermApp()
{
	// delete gTermPref;
}


void
TermApp::ReadyToRun()
{
	// Prevent opeing window when option -h is given.
	if (usage_requested)
		return;
	
	const char *command = NULL;
	const char *encoding;
	int rows, cols;
	
	encoding = gTermPref->getString(PREF_TEXT_ENCODING);
	
	// Get encoding name (setenv TTYPE in spawn_shell functions)
	const etable *p = encoding_table;
	while (p->name) {
		if (!strcmp(p->name, encoding)) {
			encoding = p->shortname;
			break;
		}
		p++;
	}
	
	if (CommandLine.Length() > 0)
		command = CommandLine.String();
	else
		command = gTermPref->getString(PREF_SHELL);
	
	rows = gTermPref->getInt32(PREF_ROWS);
	if (rows < 1)
		gTermPref->setInt32(PREF_ROWS, rows = 1);

	cols = gTermPref->getInt32(PREF_COLS);
	if (cols < MIN_COLS)
		gTermPref->setInt32(PREF_COLS, cols = MIN_COLS);

	pfd = spawn_shell(rows, cols, command, encoding);

	// failed spawn, print stdout and open alert panel
	if (pfd == -1 ) {
		(new BAlert("alert", "Terminal couldn't start the shell. Sorry.",
			"ok", NULL, NULL, B_WIDTH_FROM_LABEL,
			B_INFO_ALERT))->Go(NULL);
		PostMessage(B_QUIT_REQUESTED);
	}

	MakeTermWindow(fTermFrame);
	// using BScreen::Frame isn't enough
	if (fStartFullscreen)
		BMessenger(fTermWindow).SendMessage(FULLSCREEN);
}


void
TermApp::Quit()
{
	if (!usage_requested){
		int status;

		kill(-sh_pid, SIGHUP);
		wait(&status);
	}

	delete gTermPref;
	BApplication::Quit();
}


void
TermApp::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Terminal\n"
		"\twritten by Kazuho Okui and Takashi Murai\n"
		"\tupdated by Kian Duffy and others\n\n"
		"\tCopyright " B_UTF8_COPYRIGHT "2003-2005, Haiku.\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE); 			
	view->SetFontAndColor(0, 8, &font);

	alert->Go();
}


void
TermApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MENU_SWITCH_TERM:
			SwitchTerm();
			break;

		case MSG_ACTIVATE_TERM:
			fTermWindow->TermWinActivate();
			break;

		case MSG_TERM_IS_MINIMIZE:
		{
			BMessage reply(B_REPLY);
			reply.AddBool("result", fTermWindow->IsMinimized());
			msg->SendReply(&reply);
			break;
		}

		default:
			BApplication::MessageReceived(msg);
			break;
	}
}


void
TermApp::ArgvReceived(int32 argc, char **argv)
{
	int skip_args = 0;
	char *value = 0;

	if (argc < 2)
		return;

	sort_args(argc, argv);
	argc = 0;
	while (argv[argc])
		argc++;

	// Print usage
	if (argmatch(argv, argc, "-help", "--help", 3, NULL, &skip_args)) {
		Usage(argv[0]);
		usage_requested = true;
		PostMessage(B_QUIT_REQUESTED);
	}

	// Start fullscreen
	if (argmatch(argv, argc, "-f", "--fullscreen", 4, NULL, &skip_args))
		fStartFullscreen = true;

	// Load preference file
	if (argmatch(argv, argc, "-p", "--preference", 4, &value, &skip_args))
		gTermPref->Open(value);

	// Set window title
	if (argmatch(argv ,argc, "-t", "--title", 3, &value, &skip_args))
		fWindowTitle = value;

    // Set window geometry
    if (argmatch(argv, argc, "-geom", "--geometry", 4, &value, &skip_args)) {
		int width, height, xpos, ypos;

		sscanf(value, "%dx%d+%d+%d", &width, &height, &xpos, &ypos);
		if (width < 0 || height < 0 || xpos < 0 || ypos < 0
			|| width >= 256 || height >= 256 || xpos >= 2048 || ypos >= 2048) {
			fprintf(stderr, "%s: invalid geometry format or value.\n", argv[0]);
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			usage_requested = true;
			PostMessage(B_QUIT_REQUESTED);
		}
		gTermPref->setInt32(PREF_COLS, width);
		gTermPref->setInt32(PREF_ROWS, height);

		fTermFrame.Set(xpos, ypos, xpos + 50, ypos + 50);
		geometry_requested = true;
    }

#if 0
	// Open '/etc/rgb.txt' file
	BFile inFile;
	off_t size = 0;

	status_t status = inFile.SetTo("/etc/rgb.txt", B_READ_ONLY);
	if (status != B_OK) {
		fprintf(stderr, "%s: Can't open /etc/rgb.txt file.\n", argv[0]);
		usage_requested = true;
		PostMessage(B_QUIT_REQUESTED);
	}

	inFile.GetSize(&size);
	char *buffer = new char [size];
	inFile.Read(buffer, size);

	// Set window, cursor, area and IM color
	for (int i = 4; i < 13; i++) {
		if (argmatch(argv, argc, standard_args[i].name, standard_args[i].longname,
				9, &value, &skip_args)) {
			rgb_color color;
			if (text_to_rgb(value, &color, buffer))
				gTermPref->setRGB(standard_args[i].prefname, color);
			else
				fprintf(stderr, "%s: invalid color string -- %s\n", argv[0], value);
		}
	}

	delete buffer;
#endif
	skip_args++;

	if (skip_args < argc) {
		// Check invalid options

		if (*argv[skip_args] == '-') {
			fprintf(stderr, "%s: invalid option `%s'\n", argv[0], argv[skip_args]);
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			usage_requested = true;
			PostMessage(B_QUIT_REQUESTED);
		}

		CommandLine += argv[skip_args++];
		while (skip_args < argc) {
			CommandLine += ' ';
			CommandLine += argv[skip_args++];
		}
	}
}

//void
//TermApp::AboutRequested(void)
//{
//  if(fAboutPanel) {
  //  fAboutPanel->Activate();
 // }
 // else {
 //   BRect rect(0, 0, 350 - 1, 170 - 1);
 //   rect.OffsetTo(fTermWindow->Frame().LeftTop());
 //   rect.OffsetBy(100, 100);
  
  //  fAboutPanel = new AboutDlg(rect, this);
 //   fAboutPanel->Show();
 // }
  
//}

void
TermApp::RefsReceived(BMessage *message) 
{ 
	// Works Only Launced by Double-Click file, or Drags file to App.
	if (!IsLaunching())
		return;

	entry_ref ref; 
	if (message->FindRef("refs", 0, &ref) != B_OK)
		return;

	BFile file; 
	if (file.SetTo(&ref, B_READ_WRITE) != B_OK)
		return;

	BNodeInfo info(&file);
	char mimetype[B_MIME_TYPE_LENGTH];
	info.GetType(mimetype);

	// if App opened by Pref file
	if (!strcmp(mimetype, PREFFILE_MIMETYPE)) {
	
		BEntry ent(&ref);
		BPath path(&ent);
		gTermPref->OpenText(path.Path());
		return;
	}

	// if App opened by Shell Script
	if (!strcmp(mimetype, "text/x-haiku-shscript")){
		// Not implemented.
		//    beep();
		return;
	}
}  


void
TermApp::MakeTermWindow(BRect &frame)
{
	fTermWindow = new TermWindow(frame, fWindowTitle.String());
	fTermWindow->Show();
}


void
TermApp::ActivateTermWindow(team_id id)
{
	BMessenger app(TERM_SIGNATURE, id);
	
	if (app.IsTargetLocal())
		fTermWindow->Activate();
	else
		app.SendMessage(MSG_ACTIVATE_TERM);
}


void
TermApp::SwitchTerm()
{
	team_id myId = be_app->Team(); // My id
	BList teams;
	be_roster->GetAppList(TERM_SIGNATURE, &teams); 
	int32 numTerms = teams.CountItems();

	if (numTerms <= 1 )
		return; //Can't Switch !!

	// Find position of mine in app teams.
	int32 i;

	for (i = 0; i < numTerms; i++) {
		if (myId == reinterpret_cast<team_id>(teams.ItemAt(i)))
			break;
	}

	do {
		if (--i < 0)
			i = numTerms - 1;
	} while (IsMinimize(reinterpret_cast<team_id>(teams.ItemAt(i))));

	// Activate switched terminal.
	ActivateTermWindow(reinterpret_cast<team_id>(teams.ItemAt(i)));
}


bool
TermApp::IsMinimize(team_id id)
{
	BMessenger app(TERM_SIGNATURE, id);

	if (app.IsTargetLocal())
		return fTermWindow->IsMinimized();

	BMessage reply;
	bool hidden;

	if (app.SendMessage(MSG_TERM_IS_MINIMIZE, &reply) != B_OK)
		return true;

	reply.FindBool("result", &hidden);
	return hidden;
}


int32
TermApp::FindTerminalId()
{
	BList teams;
	be_roster->GetAppList(TERM_SIGNATURE, &teams);
	int32 count = teams.CountItems();
	int32 numbers[count];
	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	
	for (int32 i=0; i<count; i++) {
		numbers[i] = 0;
		team_id id = (team_id)teams.ItemAt(i);
		if (id == info.team)
			continue;
		
		BMessage msg(B_GET_PROPERTY);
		BMessage reply;
		msg.AddSpecifier("Title");
		msg.AddSpecifier("Window", (int32)0);
		if (BMessenger(TERM_SIGNATURE, id).SendMessage(&msg, &reply) != B_OK)
			continue;
		BString title;
		if (reply.FindString("result", &title)!=B_OK)
			continue;
		sscanf(title.String(), "Terminal %ld", &numbers[i]);
	}
	
	for (int32 i=1; i<count; i++) {
		bool found = false;
		for (int32 j=0; j<count; j++) {
			if (i == numbers[j]) {
				found = true;
				break;
			}
		}
		if (!found)
			return i;
	}

	return count;
}


//#ifndef B_NETPOSITIVE_APP_SIGNATURE
//#define B_NETPOSITIVE_APP_SIGNATURE "application/x-vnd.Be-NPOS" 
//#endif
//
//void
//TermApp::ShowHTML(BMessage *msg)
//{
//  const char *url;
//  msg->FindString("Url", &url);
//  BMessage message;
//
//  message.what = B_NETPOSITIVE_OPEN_URL;
//  message.AddString("be:url", url);

//  be_roster->Launch(B_NETPOSITIVE_APP_SIGNATURE, &message);
//  while(!(be_roster->IsRunning(B_NETPOSITIVE_APP_SIGNATURE)))  
//    snooze(10000);
//
//  // Activate net+
//  be_roster->ActivateApp(be_roster->TeamFor(B_NETPOSITIVE_APP_SIGNATURE));
//}

#define PR(a) fprintf(stderr, a)

// Display usage to stderr
void
TermApp::Usage(char *name)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Haiku Terminal\n");
	fprintf(stderr, "Copyright 2004 Haiku, Inc.\n");
	fprintf(stderr, "Parts Copyright(C) 1999 Kazuho Okui and Takashi Murai.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s [OPTION] [SHELL]\n", name);
	
	PR("  -p,     --preference         load preference file\n");
	PR("  -t,     --title              set window title\n");
	PR("  -geom,  --geometry           set window geometry\n");
	PR("                               An example of geometry is \"80x25+100+100\"\n");
#if 0
	PR("  -fg,    --text-fore-color    set window foreground color\n");
	PR("  -bg,    --text-back-color    set window background color\n");
	PR("  -curfg, --cursor-fore-color  set cursor foreground color\n");
	PR("  -curbg, --cursor-back-color  set cursor background color\n");
	PR("  -selfg, --select-fore-color  set selection area foreground color\n");
	PR("  -selbg, --select-back-color  set selection area background color\n");
	PR("                               Examples of color are \"#FF00FF\" and \"purple\"\n");
#endif
	PR("\n");
}

int
text_to_rgb(char *name, rgb_color *color, char *buffer)
{
	if (name[0] != '#') {
		// Convert from /etc/rgb.txt.
		BString inStr(buffer);
		int32 point, offset = 0;

		// Search color name
		do {
			point = inStr.FindFirst(name, offset);
			if (point < 0)
				return false;
			offset = point + 1;
		} while(*(buffer + point -1) != '\t');

		char *p = buffer + point;
		while (*p != '\n')
			p--;
		p++;

		if (sscanf(p, "%d %d %d", (int *)&color->red, (int *)&color->green,
				(int *)&color->blue) == EOF)
			return false;

		color->alpha = 0;
	} else if (name[0] == '#') {
		// Convert from #RRGGBB format
		sscanf(name, "#%2x%2x%2x", (int *)&color->red, (int *)&color->green,
			(int *)&color->blue);
		color->alpha = 0;
	} else
		return false;

	return true;
}

// This routine copy from GNU Emacs.
// TODO: This might be a GPL licensing issue here. Investigate.
int
argmatch(char **argv, int argc, char *sstr, char *lstr,
	int minlen, char **valptr, int *skipptr)
{
	char *p = 0;
	int arglen;
	char *arg;

	// Don't access argv[argc]; give up in advance
	if (argc <= *skipptr + 1)
		return 0;

	arg = argv[*skipptr+1];
	if (arg == NULL)
		return 0;

	if (strcmp(arg, sstr) == 0) {
		if(valptr != NULL) {
			*valptr = argv[*skipptr+2];
			*skipptr += 2;
		} else
			*skipptr += 1;
		return 1;
	}

	arglen =(valptr != NULL &&(p = strchr(arg, '=')) != NULL
			? p - arg : strlen(arg));
	
	if(lstr == 0 || arglen < minlen || strncmp(arg, lstr, arglen) != 0)
		return 0;
	else
	if(valptr == NULL)
	{
		*skipptr += 1;
		return 1;
	}
	else
	if(p != NULL)
	{
		*valptr = p+1;
		*skipptr += 1;
		return 1;
	}
	else
	if(argv[*skipptr+2] != NULL)
	{
		*valptr = argv[*skipptr+2];
		*skipptr += 2;
		return 1;
	}
	else
	{
		return 0;
	}
}

// This routine copy from GNU Emacs.
// TODO: This might be a GPL licensing issue here. Investigate.
void
sort_args(int argc, char **argv)
{
	/*
		For each element of argv,
		the corresponding element of options is:
		0 for an option that takes no arguments,
		1 for an option that takes one argument, etc.
		-1 for an ordinary non-option argument.
	*/
	
	char **newargv =(char **) malloc(sizeof(char *) * argc);
	
	int *options =(int *) malloc(sizeof(int) * argc);
	int *priority =(int *) malloc(sizeof(int) * argc);
	int to = 1;
	int incoming_used = 1;
	int from;
	int i;
	//int end_of_options = argc;
	
	// Categorize all the options,
	// and figure out which argv elts are option arguments
	for(from = 1; from < argc; from++)
	{
		options[from] = -1;
		priority[from] = 0;
		if(argv[from][0] == '-')
		{
			int match, thislen;
			char *equals;
			
			// If we have found "--", don't consider any more arguments as options
			if(argv[from][1] == '-' && argv[from][2] == 0)
			{
				// Leave the "--", and everything following it, at the end.
				for(; from < argc; from++)
				{
					priority[from] = -100;
					options[from] = -1;
				}
				break;
			}
			
			// Look for a match with a known old-fashioned option.
			for(i = 0; i <(int)(sizeof(standard_args) / sizeof(standard_args[0])); i++)
				if(!strcmp(argv[from], standard_args[i].name))
				{
					options[from] = standard_args[i].nargs;
					priority[from] = standard_args[i].priority;
					if(from + standard_args[i].nargs >= argc)
						fprintf(stderr, "Option `%s' requires an argument\n", argv[from]);
					from += standard_args[i].nargs;
					goto done;
				}
			
			/*
				Look for a match with a known long option.
				MATCH is -1 if no match so far, -2 if two or more matches so far,
				>= 0(the table index of the match) if just one match so far.
			*/
			if(argv[from][1] == '-')
			{
				match = -1;
				thislen = strlen(argv[from]);
				equals = strchr(argv[from], '=');
				if(equals != 0)
					thislen = equals - argv[from];
				
				for(i = 0;i <(int )(sizeof(standard_args) / sizeof(standard_args[0])); i++)
					if(standard_args[i].longname 
						&& !strncmp(argv[from], standard_args[i].longname, thislen))
					{
						if(match == -1)
							match = i;
						else
							match = -2;
					}
					
					// If we found exactly one match, use that
					if(match >= 0)
					{
						options[from] = standard_args[match].nargs;
						priority[from] = standard_args[match].priority;
						
						// If --OPTION=VALUE syntax is used,
						// this option uses just one argv element
						if(equals != 0)
							options[from] = 0;
						if(from + options[from] >= argc)
							fprintf(stderr, "Option `%s' requires an argument\n", argv[from]);
						from += options[from];
					}
			}
			done: ;
		}
	}
	
	// Copy the arguments, in order of decreasing priority, to NEW
	newargv[0] = argv[0];
	while (incoming_used < argc) {
		int best = -1;
		int best_priority = -9999;

		// Find the highest priority remaining option.
		// If several have equal priority, take the first of them.
		for (from = 1; from < argc; from++) {
			if (argv[from] != 0 && priority[from] > best_priority) {
				best_priority = priority[from];
				best = from;
			}
			
			// Skip option arguments--they are tied to the options.
			if (options[from] > 0)
				from += options[from];
		}
		
		if (best < 0)
			abort();
		
		// Copy the highest priority remaining option, with its args, to NEW.
		// Unless it is a duplicate of the previous one
		if (!(options[best] == 0 && ! strcmp(newargv[to - 1], argv[best]))) {
			newargv[to++] = argv[best];
			for(i = 0; i < options[best]; i++)
				newargv[to++] = argv[best + i + 1];
		}
		
		incoming_used += 1 +(options[best] > 0 ? options[best] : 0);
		
		// Clear out this option in ARGV
		argv[best] = 0;
		for (i = 0; i < options[best]; i++)
			argv[best + i + 1] = 0;
	}
	
	// If duplicate options were deleted, fill up extra space with null ptrs
	while (to < argc)
		newargv[to++] = 0;
	
	memcpy(argv, newargv, sizeof(char *) * argc);
	
	free(options);
	free(newargv);
	free(priority);
}

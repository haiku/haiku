/*
 * Copyright 2001-2007, Haiku.
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed unter the terms of the MIT license.
 */


#include "TermApp.h"

#include "CodeConv.h"
#include "PrefHandler.h"
#include "TermWindow.h"
#include "TermConst.h"

#include <Alert.h>
#include <Clipboard.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <TextView.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static bool sUsageRequested = false;
static bool sGeometryRequested = false;

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
};

int argmatch(char **, int, char *, char *, int, char **, int *);
void sort_args(int, char **);


const ulong MSG_ACTIVATE_TERM = 'msat';
const ulong MSG_TERM_IS_MINIMIZE = 'mtim';


TermApp::TermApp()
	: BApplication(TERM_SIGNATURE),
	fStartFullscreen(false),
	fWindowNumber(-1),
	fTermWindow(NULL)
{
	fWindowTitle = "Terminal";
	_RegisterTerminal();

	if (fWindowNumber > 0)
		fWindowTitle << " " << fWindowNumber;

	int i = fWindowNumber / 16;
	int j = fWindowNumber % 16;
	int k = (j * 16) + (i * 64) + 50;
	int l = (j * 16)  + 50;

	fTermFrame.Set(k, l, k + 50, k + 50);
}


TermApp::~TermApp()
{
}


void
TermApp::ReadyToRun()
{
	// Prevent opeing window when option -h is given.
	if (sUsageRequested)
		return;

	status_t status = _MakeTermWindow(fTermFrame);

	// failed spawn, print stdout and open alert panel
	// TODO: This alert does never show up.
	if (status < B_OK) {
		(new BAlert("alert", "Terminal couldn't start the shell. Sorry.",
			"ok", NULL, NULL, B_WIDTH_FROM_LABEL,
			B_INFO_ALERT))->Go(NULL);
		PostMessage(B_QUIT_REQUESTED);
		return;
	}

	// using BScreen::Frame isn't enough
	if (fStartFullscreen)
		BMessenger(fTermWindow).SendMessage(FULLSCREEN);
}


void
TermApp::Quit()
{
	if (!sUsageRequested)
		_UnregisterTerminal();

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
	
	view->SetStylable(true);

	BFont font;
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
			_SwitchTerm();
			break;

		case MSG_ACTIVATE_TERM:
			fTermWindow->Activate();
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
		_Usage(argv[0]);
		sUsageRequested = true;
		PostMessage(B_QUIT_REQUESTED);
	}

	// Start fullscreen
	if (argmatch(argv, argc, "-f", "--fullscreen", 4, NULL, &skip_args))
		fStartFullscreen = true;

	// Load preference file
	if (argmatch(argv, argc, "-p", "--preference", 4, &value, &skip_args))
		PrefHandler::Default()->Open(value);

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
			sUsageRequested = true;
			PostMessage(B_QUIT_REQUESTED);
		}
		PrefHandler::Default()->setInt32(PREF_COLS, width);
		PrefHandler::Default()->setInt32(PREF_ROWS, height);

		fTermFrame.Set(xpos, ypos, xpos + 50, ypos + 50);
		sGeometryRequested = true;
   	 }

	skip_args++;

	if (skip_args < argc) {
		// Check invalid options

		if (*argv[skip_args] == '-') {
			fprintf(stderr, "%s: invalid option `%s'\n", argv[0], argv[skip_args]);
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			sUsageRequested = true;
			PostMessage(B_QUIT_REQUESTED);
		}

		fCommandLine += argv[skip_args++];
		while (skip_args < argc) {
			fCommandLine += ' ';
			fCommandLine += argv[skip_args++];
		}
	}
}


void
TermApp::RefsReceived(BMessage* message) 
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
		PrefHandler::Default()->OpenText(path.Path());
		return;
	}

	// if App opened by Shell Script
	if (!strcmp(mimetype, "text/x-haiku-shscript")){
		// Not implemented.
		//    beep();
		return;
	}
}  


status_t
TermApp::_MakeTermWindow(BRect &frame)
{
	const char *command = NULL;
	if (fCommandLine.Length() > 0)
		command = fCommandLine.String();
	else
		command = PrefHandler::Default()->getString(PREF_SHELL);

	try {
		fTermWindow = new TermWindow(frame, fWindowTitle.String(), command);
	} catch (int error) {
		return (status_t)error;	
	} catch (...) {
		return B_ERROR;
	}

	fTermWindow->Show();
	
	return B_OK;
}


void
TermApp::_ActivateTermWindow(team_id id)
{
	BMessenger app(TERM_SIGNATURE, id);
	if (app.IsTargetLocal())
		fTermWindow->Activate();
	else
		app.SendMessage(MSG_ACTIVATE_TERM);
}


void
TermApp::_SwitchTerm()
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
	} while (_IsMinimized(reinterpret_cast<team_id>(teams.ItemAt(i))));

	// Activate switched terminal.
	_ActivateTermWindow(reinterpret_cast<team_id>(teams.ItemAt(i)));
}


bool
TermApp::_IsMinimized(team_id id)
{
	BMessenger app(TERM_SIGNATURE, id);
	if (app.IsTargetLocal())
		return fTermWindow->IsMinimized();

	BMessage reply;
	if (app.SendMessage(MSG_TERM_IS_MINIMIZE, &reply) != B_OK)
		return true;

	bool hidden;
	reply.FindBool("result", &hidden);
	return hidden;
}


/*!
	Checks if all teams that have an ID-to-team mapping in the message
	are still running.
	The IDs for teams that are gone will be made available again, and
	their mapping is removed from the message.
*/
void
TermApp::_SanitizeIDs(BMessage* data, uint8* windows, ssize_t length)
{
	BList teams;
	be_roster->GetAppList(TERM_SIGNATURE, &teams);

	for (int32 i = 0; i < length; i++) {
		if (!windows[i])
			continue;

		BString id("id-");
		id << i;

		team_id team;
		if (data->FindInt32(id.String(), &team) != B_OK)
			continue;

		if (!teams.HasItem((void*)team)) {
			windows[i] = false;
			data->RemoveName(id.String());
		}
	}
}


/*!
	Removes the current fWindowNumber (ID) from the supplied array, or
	finds a free ID in it, and sets fWindowNumber accordingly.
*/
bool
TermApp::_UpdateIDs(bool set, uint8* windows, ssize_t maxLength,
	ssize_t* _length)
{
	ssize_t length = *_length;

	if (set) {
		int32 i;
		for (i = 0; i < length; i++) {
			if (!windows[i]) {
				windows[i] = true;
				fWindowNumber = i + 1;
				break;
			}
		}

		if (i == length) {
			if (length >= maxLength)
				return false;

			windows[length] = true;
			length++;
			fWindowNumber = length;
		}
	} else {
		// update information and write it back
		windows[fWindowNumber - 1] = false;
	}

	*_length = length;
	return true;
}


void
TermApp::_UpdateRegistration(bool set)
{
	if (set)
		fWindowNumber = -1;
	else if (fWindowNumber < 0)
		return;

#ifdef __HAIKU__
	// use BClipboard - it supports atomic access in Haiku
	BClipboard clipboard(TERM_SIGNATURE);

	while (true) {
		if (!clipboard.Lock())
			return;

		BMessage* data = clipboard.Data();

		const uint8* windowsData;
		uint8 windows[512];
		ssize_t length;
		if (data->FindData("ids", B_RAW_TYPE,
				(const void**)&windowsData, &length) != B_OK)
			length = 0;

		if (length > (ssize_t)sizeof(windows))
			length = sizeof(windows);
		if (length > 0)
			memcpy(windows, windowsData, length);

		_SanitizeIDs(data, windows, length);

		status_t status = B_OK;
		if (_UpdateIDs(set, windows, sizeof(windows), &length)) {
			// add/remove our ID-to-team mapping
			BString id("id-");
			id << fWindowNumber;

			if (set)
				data->AddInt32(id.String(), Team());
			else
				data->RemoveName(id.String());

			data->RemoveName("ids");
			//if (data->ReplaceData("ids", B_RAW_TYPE, windows, length) != B_OK)
			data->AddData("ids", B_RAW_TYPE, windows, length);

			status = clipboard.Commit(true);
		}

		clipboard.Unlock();

		if (status == B_OK)
			break;
	}
#else	// !__HAIKU__
	// use a file to store the IDs - unfortunately, locking
	// doesn't work on BeOS either here
	int fd = open("/tmp/terminal_ids", O_RDWR | O_CREAT);
	if (fd < 0)
		return;

	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_CUR;
	lock.l_start = 0;
	lock.l_len = -1;
	fcntl(fd, F_SETLKW, &lock);

	uint8 windows[512];
	ssize_t length = read_pos(fd, 0, windows, sizeof(windows));
	if (length < 0) {
		close(fd);
		return;
	}

	if (length > (ssize_t)sizeof(windows))
		length = sizeof(windows);

	if (_UpdateIDs(set, windows, sizeof(windows), &length))
		write_pos(fd, 0, windows, length);

	close(fd);
#endif	// !__HAIKU__
}


void
TermApp::_UnregisterTerminal()
{
	_UpdateRegistration(false);
}


void
TermApp::_RegisterTerminal()
{
	_UpdateRegistration(true);
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


void
TermApp::_Usage(char *name)
{
	fprintf(stderr, "Haiku Terminal\n"
		"Copyright 2001-2007 Haiku, Inc.\n"
		"Copyright(C) 1999 Kazuho Okui and Takashi Murai.\n"
		"\n"
		"Usage: %s [OPTION] [SHELL]\n", name);

	fprintf(stderr, "  -p,     --preference         load preference file\n"
		"  -t,     --title              set window title\n"
		"  -geom,  --geometry           set window geometry\n"
		"                               An example of geometry is \"80x25+100+100\"\n");
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

/*
 * Copyright 2001-2007, Haiku.
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed unter the terms of the MIT license.
 */


#include "TermApp.h"

#include "Arguments.h"
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


const ulong MSG_ACTIVATE_TERM = 'msat';
const ulong MSG_TERM_IS_MINIMIZE = 'mtim';


TermApp::TermApp()
	: BApplication(TERM_SIGNATURE),
	fStartFullscreen(false),
	fWindowNumber(-1),
	fTermWindow(NULL),
	fArgs(NULL)
{
	fArgs = new Arguments();

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
	delete fArgs;
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
	fArgs->Parse(argc, argv);
	
	if (fArgs->UsageRequested()) {
		_Usage(argv[0]);
		sUsageRequested = true;
		PostMessage(B_QUIT_REQUESTED);
		return;	
	}
	
	if (fArgs->Title() != NULL)
		fWindowTitle = fArgs->Title();

	fStartFullscreen = fArgs->FullScreen();
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
	try {
		fTermWindow = new TermWindow(frame, fWindowTitle.String(), new Arguments(*fArgs));
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

	fprintf(stderr,
		"  -h,     --help               print this help\n"    
		//"  -p,     --preference         load preference file\n"
		"  -t,     --title              set window title\n"
		"  -f,     --fullscreen         start fullscreen\n"		
		//"  -geom,  --geometry           set window geometry\n"
		//"                               An example of geometry is \"80x25+100+100\"\n"
		);
}


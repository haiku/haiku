/*
 * Copyright 2001-2009, Haiku, Inc. All rights reserved.
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 *
 * Distributed unter the terms of the MIT license.
 */


#include "TermApp.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Alert.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Locale.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>


#include "Arguments.h"
#include "Globals.h"
#include "PrefHandler.h"
#include "TermConst.h"
#include "TermView.h"
#include "TermWindow.h"


static bool sUsageRequested = false;
//static bool sGeometryRequested = false;

const ulong MSG_ACTIVATE_TERM = 'msat';
const ulong MSG_TERM_WINDOW_INFO = 'mtwi';


int
main()
{
	TermApp app;
	app.Run();

	return 0;
}

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Terminal TermApp"

TermApp::TermApp()
	: BApplication(TERM_SIGNATURE),
	fStartFullscreen(false),
	fWindowTitleUserDefined(false),
	fWindowNumber(-1),
	fTermWindow(NULL),
	fArgs(NULL)
{

	fArgs = new Arguments(0, NULL);

	fWindowTitle = B_TRANSLATE("Terminal");
	_RegisterTerminal();

	if (fWindowNumber > 0)
		fWindowTitle << " " << fWindowNumber;

	if (_LoadWindowPosition(&fTermFrame, &fTermWorkspaces) != B_OK) {
		int i = fWindowNumber / 16;
		int j = fWindowNumber % 16;
		int k = (j * 16) + (i * 64) + 50;
		int l = (j * 16)  + 50;

		fTermFrame.Set(k, l, k + 50, k + 50);
		fTermWorkspaces = B_CURRENT_WORKSPACE;
	}
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

	// Install a SIGCHLD signal handler, so that we will be notified, when
	// a shell exits.
	struct sigaction action;
#ifdef __HAIKU__
	action.sa_handler = (sighandler_t)_SigChildHandler;
#else
	action.sa_handler = (__signal_func_ptr)_SigChildHandler;
#endif
	sigemptyset(&action.sa_mask);
#ifdef SA_NODEFER
	action.sa_flags = SA_NODEFER;
#endif
	action.sa_userdata = this;
	if (sigaction(SIGCHLD, &action, NULL) < 0) {
		fprintf(stderr, B_TRANSLATE("sigaction() failed: %s\n"),
			strerror(errno));
		// continue anyway
	}

	// init the mouse copy'n'paste clipboard
	gMouseClipboard = new BClipboard(MOUSE_CLIPBOARD_NAME, true);

	status_t status = _MakeTermWindow(fTermFrame, fTermWorkspaces);

	// failed spawn, print stdout and open alert panel
	// TODO: This alert does never show up.
	if (status < B_OK) {
		(new BAlert("alert",
			B_TRANSLATE("Terminal couldn't start the shell. Sorry."),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_FROM_LABEL,
			B_INFO_ALERT))->Go(NULL);
		PostMessage(B_QUIT_REQUESTED);
		return;
	}

	// using BScreen::Frame isn't enough
	if (fStartFullscreen)
		BMessenger(fTermWindow).SendMessage(FULLSCREEN);
}


bool
TermApp::QuitRequested()
{
	// check whether the system is shutting down
	BMessage* message = CurrentMessage();
	bool shutdown;
	if (message != NULL && message->FindBool("_shutdown_", &shutdown) == B_OK
		&& shutdown) {
		// The system is shutting down. Quit the window synchronously. This
		// skips the checks for running processes and the "Are you sure..."
		// alert.
		if (fTermWindow->Lock())
			fTermWindow->Quit();
	}

	return BApplication::QuitRequested();
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
	TermView::AboutRequested();
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

		case MSG_TERM_WINDOW_INFO:
		{
			BMessage reply(B_REPLY);
			reply.AddBool("minimized", fTermWindow->IsMinimized());
			reply.AddInt32("workspaces", fTermWindow->Workspaces());
			msg->SendReply(&reply);
			break;
		}

		case MSG_SAVE_WINDOW_POSITION:
			_SaveWindowPosition(msg);
			break;

		case MSG_CHECK_CHILDREN:
			_HandleChildCleanup();
			break;

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

	if (fArgs->Title() != NULL) {
		fWindowTitle = fArgs->Title();
		fWindowTitleUserDefined = true;
	}

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
TermApp::_GetWindowPositionFile(BFile* file, uint32 openMode)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (status != B_OK)
		return status;

	status = path.Append("Terminal_windows");
	if (status != B_OK)
		return status;

	return file->SetTo(path.Path(), openMode);
}


status_t
TermApp::_LoadWindowPosition(BRect* frame, uint32* workspaces)
{
	status_t status;
	BMessage position;

	BFile file;
	status = _GetWindowPositionFile(&file, B_READ_ONLY);
	if (status != B_OK)
		return status;

	status = position.Unflatten(&file);

	file.Unset();

	if (status != B_OK)
		return status;

	status = position.FindRect("rect", fWindowNumber - 1, frame);
	if (status != B_OK)
		return status;

	int32 _workspaces;
	status = position.FindInt32("workspaces", fWindowNumber - 1, &_workspaces);
	if (status != B_OK)
		return status;
	if (modifiers() & B_SHIFT_KEY)
		*workspaces = _workspaces;
	else
		*workspaces = B_CURRENT_WORKSPACE;

	return B_OK;
}


status_t
TermApp::_SaveWindowPosition(BMessage* position)
{
	BFile file;
	BMessage originalSettings;

	// We append ourself to the existing settings file
	// So we have to read it, insert our BMessage, and rewrite it.

	status_t status = _GetWindowPositionFile(&file, B_READ_ONLY);
	if (status == B_OK) {
		originalSettings.Unflatten(&file);
			// No error checking on that : it fails if the settings
			// file is missing, but we can create it.

		file.Unset();
	}

	// Append the new settings
	BRect rect;
	position->FindRect("rect", &rect);
	if (originalSettings.ReplaceRect("rect", fWindowNumber - 1, rect) != B_OK)
		originalSettings.AddRect("rect", rect);

	int32 workspaces;
	position->FindInt32("workspaces", &workspaces);
	if (originalSettings.ReplaceInt32("workspaces", fWindowNumber - 1, workspaces)
			!= B_OK)
		originalSettings.AddInt32("workspaces", workspaces);

	// Resave the whole thing
	status = _GetWindowPositionFile (&file,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	return originalSettings.Flatten(&file);
}


status_t
TermApp::_MakeTermWindow(BRect &frame, uint32 workspaces)
{
	try {
		fTermWindow = new TermWindow(frame, fWindowTitle,
			fWindowTitleUserDefined, fWindowNumber, workspaces, fArgs);
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
	team_id myId = be_app->Team();
	BList teams;
	be_roster->GetAppList(TERM_SIGNATURE, &teams);

	int32 numTerms = teams.CountItems();
	if (numTerms <= 1)
		return;

	// Find our position in the app teams.
	int32 i;

	for (i = 0; i < numTerms; i++) {
		if (myId == reinterpret_cast<team_id>(teams.ItemAt(i)))
			break;
	}

	do {
		if (--i < 0)
			i = numTerms - 1;
	} while (!_IsSwitchTarget(reinterpret_cast<team_id>(teams.ItemAt(i))));

	// Activate switched terminal.
	_ActivateTermWindow(reinterpret_cast<team_id>(teams.ItemAt(i)));
}


bool
TermApp::_IsSwitchTarget(team_id id)
{
	uint32 currentWorkspace = 1L << current_workspace();

	BMessenger app(TERM_SIGNATURE, id);
	if (app.IsTargetLocal()) {
		return !fTermWindow->IsMinimized()
			&& (fTermWindow->Workspaces() & currentWorkspace) != 0;
	}

	BMessage reply;
	if (app.SendMessage(MSG_TERM_WINDOW_INFO, &reply) != B_OK)
		return false;

	bool minimized;
	int32 workspaces;
	if (reply.FindBool("minimized", &minimized) != B_OK
		|| reply.FindInt32("workspaces", &workspaces) != B_OK)
		return false;

	return !minimized && (workspaces & currentWorkspace) != 0;
}


/*!	Checks if all teams that have an ID-to-team mapping in the message
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
		id << i + 1;

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
TermApp::_HandleChildCleanup()
{
}


/*static*/ void
TermApp::_SigChildHandler(int signal, void* data)
{
	// Spawing a thread that does the actual signal handling is pretty much
	// the only safe thing to do in a multi-threaded application. The
	// interrupted thread might have been anywhere, e.g. in a critical section,
	// holding locks. If we do anything that does require locking at any point
	// (e.g. memory allocation, messaging), we risk a dead-lock or data
	// structure corruption. Spawing a thread is safe though, since its only
	// a system call.
	thread_id thread = spawn_thread(_ChildCleanupThread, "child cleanup",
		B_NORMAL_PRIORITY, ((TermApp*)data)->fTermWindow);
	if (thread >= 0)
		resume_thread(thread);
}


/*static*/ status_t
TermApp::_ChildCleanupThread(void* data)
{
	// Just drop the windowa message and let it do the actual work. This
	// saves us additional synchronization measures.
	return ((TermWindow*)data)->PostMessage(MSG_CHECK_CHILDREN);
}



void
TermApp::_Usage(char *name)
{
	fprintf(stderr, B_TRANSLATE("Haiku Terminal\n"
		"Copyright 2001-2009 Haiku, Inc.\n"
		"Copyright(C) 1999 Kazuho Okui and Takashi Murai.\n"
		"\n"
		"Usage: %s [OPTION] [SHELL]\n"), name);

	fprintf(stderr,
		B_TRANSLATE("  -h,     --help               print this help\n"
		//"  -p,     --preference         load preference file\n"
		"  -t,     --title              set window title\n"
		"  -f,     --fullscreen         start fullscreen\n")
		//"  -geom,  --geometry           set window geometry\n"
		//"                               An example of geometry is \"80x25+100+100\"\n"
		);
}


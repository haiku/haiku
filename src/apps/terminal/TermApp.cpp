/*
 * Copyright 2001-2013, Haiku, Inc. All rights reserved.
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 *
 * Distributed unter the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy, myob@users.sourceforge.net
 *		Siarzhuk Zharski, zharik@gmx.li
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
#include "TermWindow.h"


static bool sUsageRequested = false;
//static bool sGeometryRequested = false;

rgb_color TermApp::fDefaultPalette[kTermColorCount];

int
main()
{
	TermApp app;
	app.Run();

	return 0;
}

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal TermApp"

TermApp::TermApp()
	: BApplication(TERM_SIGNATURE),
	fStartFullscreen(false),
	fTermWindow(NULL),
	fArgs(NULL)
{
	fArgs = new Arguments(0, NULL);

	_InitDefaultPalette();
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
	action.sa_handler = (__sighandler_t)_SigChildHandler;
#else
	action.sa_handler = (__signal_func_ptr)_SigChildHandler;
#endif
	sigemptyset(&action.sa_mask);
#ifdef SA_NODEFER
	action.sa_flags = SA_NODEFER;
#endif
	action.sa_userdata = this;
	if (sigaction(SIGCHLD, &action, NULL) < 0) {
		fprintf(stderr, "sigaction() failed: %s\n",
			strerror(errno));
		// continue anyway
	}

	// init the mouse copy'n'paste clipboard
	gMouseClipboard = new BClipboard(MOUSE_CLIPBOARD_NAME, true);

	status_t status = _MakeTermWindow();

	// failed spawn, print stdout and open alert panel
	// TODO: This alert does never show up.
	if (status < B_OK) {
		BAlert* alert = new BAlert("alert",
			B_TRANSLATE("Terminal couldn't start the shell. Sorry."),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_FROM_LABEL,
			B_INFO_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
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
	BApplication::Quit();
}


void
TermApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ACTIVATE_TERM:
			fTermWindow->Activate();
			break;

		case MSG_CHECK_CHILDREN:
			_HandleChildCleanup();
			break;

		default:
			BApplication::MessageReceived(message);
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
	if (strcmp(mimetype, PREFFILE_MIMETYPE) == 0) {

		BEntry ent(&ref);
		BPath path(&ent);
		PrefHandler::Default()->OpenText(path.Path());
		return;
	}

	// if App opened by Shell Script
	if (strcmp(mimetype, "text/x-haiku-shscript") == 0) {
		// Not implemented.
		//    beep();
		return;
	}
}


status_t
TermApp::_MakeTermWindow()
{
	try {
		fTermWindow = new TermWindow(fWindowTitle, fArgs);
	} catch (int error) {
		return (status_t)error;
	} catch (...) {
		return B_ERROR;
	}

	fTermWindow->Show();

	return B_OK;
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


void
TermApp::_InitDefaultPalette()
{
	// 0 - 15 are system ANSI colors
	const char * keys[kANSIColorCount] = {
		PREF_ANSI_BLACK_COLOR,
		PREF_ANSI_RED_COLOR,
		PREF_ANSI_GREEN_COLOR,
		PREF_ANSI_YELLOW_COLOR,
		PREF_ANSI_BLUE_COLOR,
		PREF_ANSI_MAGENTA_COLOR,
		PREF_ANSI_CYAN_COLOR,
		PREF_ANSI_WHITE_COLOR,
		PREF_ANSI_BLACK_HCOLOR,
		PREF_ANSI_RED_HCOLOR,
		PREF_ANSI_GREEN_HCOLOR,
		PREF_ANSI_YELLOW_HCOLOR,
		PREF_ANSI_BLUE_HCOLOR,
		PREF_ANSI_MAGENTA_HCOLOR,
		PREF_ANSI_CYAN_HCOLOR,
		PREF_ANSI_WHITE_HCOLOR
	};

	rgb_color* color = fDefaultPalette;
	PrefHandler* handler = PrefHandler::Default();
	for (uint i = 0; i < kANSIColorCount; i++)
		*color++ = handler->getRGB(keys[i]);

	// 16 - 231 are 6x6x6 color "cubes" in xterm color model
	for (uint red = 0; red < 256; red += (red == 0) ? 95 : 40)
		for (uint green = 0; green < 256; green += (green == 0) ? 95 : 40)
			for (uint blue = 0; blue < 256; blue += (blue == 0) ? 95 : 40)
				(*color++).set_to(red, green, blue);

	// 232 - 255 are grayscale ramp in xterm color model
	for (uint gray = 8; gray < 240; gray += 10)
		(*color++).set_to(gray, gray, gray);
}

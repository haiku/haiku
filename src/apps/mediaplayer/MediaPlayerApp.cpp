
#include <Autolock.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <stdio.h>

#include "Constants.h"
#include "MediaPlayerApp.h"
#include "MediaPlayerWindow.h"

using namespace BPrivate;

const int cascade_left = 50;
const int cascade_top = 50;
const int cascade_step = 10;

BRect windowRect(cascade_left-cascade_step,cascade_top-cascade_step,310,120);

void cascade() {
	BScreen screen(NULL);
	BRect screenBorder = screen.Frame();
	float left = windowRect.left + cascade_step;
	if (left + windowRect.Width() > screenBorder.right) {
		left = cascade_left;
	}
	float top = windowRect.top + cascade_step;
	if (top + windowRect.Height() > screenBorder.bottom) {
		top = cascade_top;
	}
	windowRect.OffsetTo(BPoint(left,top));	
}

void uncascade() {
	BScreen screen(NULL);
	BRect screenBorder = screen.Frame();
	float left = windowRect.left - cascade_step;
	if (left < cascade_left) {
		left = screenBorder.right - windowRect.Width() - cascade_left;
		left = left - ((int)left % cascade_step) + cascade_left;
	}
	float top = windowRect.top - cascade_step;
	if (top < cascade_top) {
		top = screenBorder.bottom - windowRect.Height() - cascade_top;
		top = top - ((int)left % cascade_step) + cascade_top;
	}
	windowRect.OffsetTo(BPoint(left,top));	
}

MediaPlayerApp * media_player_app;

MediaPlayerApp::MediaPlayerApp()
	: BApplication(APP_SIGNATURE)
{
	fOpenPanel = new BFilePanel();

	fWindowCount = 0;
	media_player_app = this;
}

void MediaPlayerApp::DispatchMessage(BMessage *msg, BHandler *handler)
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
MediaPlayerApp::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case B_SILENT_RELAUNCH:
			be_roster->ActivateApp(Team());
		break;
		default:
			BApplication::MessageReceived(message);
		break;
	} 
}

void
MediaPlayerApp::OpenDocument()
{
	cascade();
	new MediaPlayerWindow(windowRect);
	fWindowCount++;
}

void
MediaPlayerApp::OpenDocument(entry_ref * ref)
{
	cascade();
	new MediaPlayerWindow(windowRect,ref);
	fWindowCount++;
}

void
MediaPlayerApp::CloseDocument()
{
	uncascade();
	fWindowCount--;
	if (fWindowCount == 0) {
		BAutolock lock(this);
		Quit();
	}
}

void
MediaPlayerApp::RefsReceived(BMessage *message)
{
	int32		refNum;
	entry_ref	ref;
	
	refNum = 0;
	while (message->FindRef("refs", refNum, &ref) == B_OK) {
		OpenDocument(&ref);
		refNum++;
	}
}

void
MediaPlayerApp::ArgvReceived(int32 argc, const char *argv[], const char * cwd)
{
	for (int i = 1 ; (i < argc) ; i++) {
		BPath path;
		if (argv[i][0] == '/') {
			path.SetTo(argv[i]);
		} else {
			path.SetTo(cwd,argv[i]);
		}
		if (path.InitCheck() != B_OK) {
			printf("path.InitCheck failed: \"");
			if (argv[i][0] == '/') {
				printf("%s",argv[i]);
			} else {
				printf("%s/%s",cwd,argv[i]);
			}
			printf("\".\n");
			continue;
		}
		
		entry_ref ref;
		if (get_ref_for_path(path.Path(), &ref) != B_OK) {
			printf("get_ref_for_path failed: \"");
			printf("%s",path.Path());
			printf("\".\n");
			continue;
		}
		OpenDocument(&ref);
	}
}

void 
MediaPlayerApp::ReadyToRun() 
{
	if (fWindowCount == 0) {
		OpenDocument();
	}
}

int32
MediaPlayerApp::NumberOfWindows()
{
	return fWindowCount;
}

int
main()
{
	MediaPlayerApp mediaPlayer;
	mediaPlayer.Run();
	return 0;
}

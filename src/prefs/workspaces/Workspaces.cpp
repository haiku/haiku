/*
 * Workspaces.cpp
 * Open BeOS version alpha 1 by François Revol revol@free.fr
 *
 * Workspaces window trick found by Minox on BeShare
 * (using B_ALL_WORKSPACES as flags in BWindow)
 * Found out that using 0xffffffff as Flags was causing the windo not to close on Alt-W
 * hey Workspaces get Flags of Window 0
 * gives 0x00008080 which makes it.
 */

#include <Application.h>
#include <Alert.h>
#include <File.h>
#include <Window.h>
#include <Screen.h>
#include <Entry.h>
#include <Roster.h>
#include <Path.h>
#include <FindDirectory.h>

#include <stdlib.h>


class WorkspacesPreferences {
	public:
		WorkspacesPreferences();
		virtual ~WorkspacesPreferences();

		BRect WindowFrame() const { return fWindowFrame; }
		BRect ScreenFrame() const { return fScreenFrame; }
		void UpdateScreenFrame();
		void SetWindowFrame(BRect);

	private:
		static const char kWorkspacesSettingFile[];
		BRect fWindowFrame, fScreenFrame;
};

class WorkspacesWindow : public BWindow {
	public:
		WorkspacesWindow(WorkspacesPreferences *fPreferences);
		virtual ~WorkspacesWindow();
	
		virtual void ScreenChanged(BRect frame,color_space mode);
		virtual void FrameMoved(BPoint origin);
		virtual void FrameResized(float width, float height);
		virtual void Zoom(BPoint origin, float width, float height);
		virtual void MessageReceived(BMessage *msg);
		virtual bool QuitRequested(void);

	private:
		WorkspacesPreferences *fPreferences;
};

class WorkspacesApp : public BApplication {
	public:
		WorkspacesApp();
		virtual ~WorkspacesApp();

		virtual void AboutRequested(void);
		virtual void ArgvReceived(int32 argc, char **argv);
		virtual void ReadyToRun(void);
		virtual bool WorkspacesApp::QuitRequested(void);

	private:
		static const char kWorkspacesAppSig[];
};

const char WorkspacesApp::kWorkspacesAppSig[] = "application/x-vnd.Be-WORK";
const char WorkspacesPreferences::kWorkspacesSettingFile[] = "Workspace_data";


WorkspacesPreferences::WorkspacesPreferences()
{
	UpdateScreenFrame();

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK) {
		path.Append(kWorkspacesSettingFile);

		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK && file.Read(&fWindowFrame, sizeof(BRect)) == sizeof(BRect)) {
			BScreen screen;
			if (screen.Frame().right >= fWindowFrame.right
				&& screen.Frame().bottom >= fWindowFrame.bottom)
				return;
		}
	}

	fWindowFrame = fScreenFrame;
	fWindowFrame.OffsetBy(-10, -10);
	fWindowFrame.left = fWindowFrame.right - 160;
	fWindowFrame.top = fWindowFrame.bottom - 120;
}


WorkspacesPreferences::~WorkspacesPreferences()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(kWorkspacesSettingFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK)
		file.Write(&fWindowFrame, sizeof(BRect));
}


void
WorkspacesPreferences::UpdateScreenFrame()
{
	BScreen screen;
	fScreenFrame = screen.Frame();
}


void
WorkspacesPreferences::SetWindowFrame(BRect f)
{
	fWindowFrame = f;

	UpdateScreenFrame();
}


//	#pragma mark -


// here is the trick :)
#define B_WORKSPACES_WINDOW 0x00008000

WorkspacesWindow::WorkspacesWindow(WorkspacesPreferences *preferences)
	: BWindow(preferences->WindowFrame(), "Workspaces", B_TITLED_WINDOW_LOOK,
 			B_NORMAL_WINDOW_FEEL, B_WORKSPACES_WINDOW | B_AVOID_FRONT, B_ALL_WORKSPACES)
{
	fPreferences = preferences;
}


WorkspacesWindow::~WorkspacesWindow()
{
	delete fPreferences;
}


void
WorkspacesWindow::ScreenChanged(BRect frame,color_space mode)
{
	BRect rect = fPreferences->WindowFrame();
	BRect old = fPreferences->ScreenFrame();

	// adjust horizontal position
	if (frame.right < rect.left || frame.right > rect.right && rect.left > old.right/2)
		MoveTo(frame.right - (old.right - rect.left),rect.top);

	// adjust vertical position
	if (frame.bottom < rect.top || frame.bottom > rect.bottom && rect.top > old.bottom/2)
		MoveTo(Frame().left,frame.bottom - (old.bottom - rect.top));
}


void
WorkspacesWindow::FrameMoved(BPoint origin)
{
	fPreferences->SetWindowFrame(Frame());
}


void
WorkspacesWindow::FrameResized(float width, float height)
{
	fPreferences->SetWindowFrame(Frame());
}


void 
WorkspacesWindow::Zoom(BPoint origin, float width, float height)
{
	BScreen screen;
	origin = screen.Frame().RightBottom();
	origin.x -= 10 + fPreferences->WindowFrame().Width();
	origin.y -= 10 + fPreferences->WindowFrame().Height();

	MoveTo(origin);
}


void
WorkspacesWindow::MessageReceived(BMessage *msg)
{
	if (msg->what == 'DATA') { // Drop from Tracker
		entry_ref ref;
		for (int i = 0; (msg->FindRef("refs", i, &ref) == B_OK); i++)
			be_roster->Launch(&ref);
	} else
		BWindow::MessageReceived(msg);
}


bool
WorkspacesWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


WorkspacesApp::WorkspacesApp() : BApplication(kWorkspacesAppSig)
{
}


WorkspacesApp::~WorkspacesApp()
{
}


void
WorkspacesApp::AboutRequested(void)
{
	// blocking !! 
	// the original does the same by the way =)
	(new BAlert("about", "OpenBeOS Workspaces\n\tby François Revol, Axel Dörfler.\n\noriginal Be version by Robert Polic", "Big Deal"))->Go();
}


void
WorkspacesApp::ArgvReceived(int32 argc, char **argv)
{
	// we are told to switch to a specific workspace
	if (argc > 1) {
		// get it from the command line...
		activate_workspace(atoi(argv[1]));
		// if the app is running, don't quit
		// but if it isn't, cancel the complete run, so it doesn't
		// open any window
		if (IsLaunching())
			Quit();
	}
}


void
WorkspacesApp::ReadyToRun(void)
{
	(new WorkspacesWindow(new WorkspacesPreferences()))->Show();
}


bool
WorkspacesApp::QuitRequested(void)
{
	return true;
}


//	#pragma mark -


int
main(int argc, char **argv)
{
	new WorkspacesApp();
	be_app->Run();
	delete be_app;
	return 0;
}


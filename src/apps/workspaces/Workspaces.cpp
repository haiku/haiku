/*
 * Copyright 2002-2008, Haiku, Inc.
 * Copyright 2002, François Revol, revol@free.fr.
 * This file is distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver "Madison" Kohl,
 *		Matt Madia
 */


#include <Alert.h>
#include <Application.h>
#include <Dragger.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <TextView.h>
#include <Window.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ViewPrivate.h>
#include <WindowPrivate.h>


static const char* kSignature = "application/x-vnd.Be-WORK";
static const char* kOldSettingFile = "Workspace_data";
static const char* kSettingsFile = "Workspaces_settings";

static const uint32 kMsgChangeCount = 'chWC';
static const uint32 kMsgToggleTitle = 'tgTt';
static const uint32 kMsgToggleBorder = 'tgBd';
static const uint32 kMsgToggleAutoRaise = 'tgAR';
static const uint32 kMsgToggleAlwaysOnTop = 'tgAT';

static const float kScreenBorderOffset = 10.0;


class WorkspacesSettings {
	public:
		WorkspacesSettings();
		virtual ~WorkspacesSettings();

		BRect WindowFrame() const { return fWindowFrame; }
		BRect ScreenFrame() const { return fScreenFrame; }

		bool AutoRaising() const { return fAutoRaising; }
		bool AlwaysOnTop() const { return fAlwaysOnTop; }
		bool HasTitle() const { return fHasTitle; }
		bool HasBorder() const { return fHasBorder; }

		void UpdateFramesForScreen(BRect screenFrame);
		void UpdateScreenFrame();

		void SetWindowFrame(BRect);
		void SetAutoRaising(bool enable) { fAutoRaising = enable; }
		void SetAlwaysOnTop(bool enable) { fAlwaysOnTop = enable; }
		void SetHasTitle(bool enable) { fHasTitle = enable; }
		void SetHasBorder(bool enable) { fHasBorder = enable; }

	private:
		status_t _Open(BFile& file, int mode);

		BRect	fWindowFrame;
		BRect	fScreenFrame;
		bool	fAutoRaising;
		bool	fAlwaysOnTop;
		bool	fHasTitle;
		bool	fHasBorder;
};

class WorkspacesView : public BView {
	public:
		WorkspacesView(BRect frame);
		WorkspacesView(BMessage* archive);
		~WorkspacesView();

		static	WorkspacesView* Instantiate(BMessage* archive);
		virtual	status_t Archive(BMessage* archive, bool deep = true) const;

		virtual void MessageReceived(BMessage* message);
		virtual void MouseMoved(BPoint where, uint32 transit,
			const BMessage* dragMessage);
		virtual void MouseDown(BPoint where);

	private:
		void _AboutRequested();
};

class WorkspacesWindow : public BWindow {
	public:
		WorkspacesWindow(WorkspacesSettings *settings);
		virtual ~WorkspacesWindow();

		virtual void ScreenChanged(BRect frame, color_space mode);
		virtual void FrameMoved(BPoint origin);
		virtual void FrameResized(float width, float height);
		virtual void Zoom(BPoint origin, float width, float height);

		virtual void MessageReceived(BMessage *msg);
		virtual bool QuitRequested();

		void SetAutoRaise(bool enable);
		bool IsAutoRaising() const { return fAutoRaising; }

	private:
		WorkspacesSettings *fSettings;
		BRect	fPreviousFrame;
		bool	fAutoRaising;
};

class WorkspacesApp : public BApplication {
	public:
		WorkspacesApp();
		virtual ~WorkspacesApp();

		virtual void AboutRequested();
		virtual void ArgvReceived(int32 argc, char **argv);
		virtual void ReadyToRun();

		void Usage(const char *programName);

	private:
		WorkspacesWindow*	fWindow;
};


WorkspacesSettings::WorkspacesSettings()
	:
	fAutoRaising(false),
	fAlwaysOnTop(false),
	fHasTitle(true),
	fHasBorder(true)
{
	UpdateScreenFrame();

	bool loaded = false;
	BScreen screen;

	BFile file;
	if (_Open(file, B_READ_ONLY) == B_OK) {
		BMessage settings;
		if (settings.Unflatten(&file) == B_OK) {
			if (settings.FindRect("window", &fWindowFrame) == B_OK
				&& settings.FindRect("screen", &fScreenFrame) == B_OK)
				loaded = true;

			settings.FindBool("auto-raise", &fAutoRaising);
			settings.FindBool("always on top", &fAlwaysOnTop);

			if (settings.FindBool("has title", &fHasTitle) != B_OK)
				fHasTitle = true;
			if (settings.FindBool("has border", &fHasBorder) != B_OK)
				fHasBorder = true;
		}
	} else {
		// try reading BeOS compatible settings
		BPath path;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
			path.Append(kOldSettingFile);
			BFile file(path.Path(), B_READ_ONLY);
			if (file.InitCheck() == B_OK
				&& file.Read(&fWindowFrame, sizeof(BRect)) == sizeof(BRect)) {
				// we now also store the frame of the screen to know
				// in which context the window frame has been chosen
				BRect frame;
				if (file.Read(&frame, sizeof(BRect)) == sizeof(BRect))
					fScreenFrame = frame;
				else
					fScreenFrame = screen.Frame();

				loaded = true;
			}
		}
	}

	if (loaded) {
		// if the current screen frame is different from the one
		// just loaded, we need to alter the window frame accordingly
		if (fScreenFrame != screen.Frame())
			UpdateFramesForScreen(screen.Frame());
	}

	if (!loaded
		|| !(screen.Frame().right + 5 >= fWindowFrame.right
			&& screen.Frame().bottom + 5 >= fWindowFrame.bottom
			&& screen.Frame().left - 5 <= fWindowFrame.left
			&& screen.Frame().top - 5 <= fWindowFrame.top)) {
		// set to some usable defaults
		fWindowFrame = fScreenFrame;
		fWindowFrame.OffsetBy(-kScreenBorderOffset, -kScreenBorderOffset);
		fWindowFrame.left = fWindowFrame.right - 160;
		fWindowFrame.top = fWindowFrame.bottom - 140;
	}
}


WorkspacesSettings::~WorkspacesSettings()
{
	// write settings file
	BFile file;
	if (_Open(file, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) != B_OK)
		return;

	BMessage settings('wksp');

	if (settings.AddRect("window", fWindowFrame) == B_OK
		&& settings.AddRect("screen", fScreenFrame) == B_OK
		&& settings.AddBool("auto-raise", fAutoRaising) == B_OK
		&& settings.AddBool("always on top", fAlwaysOnTop) == B_OK
		&& settings.AddBool("has title", fHasTitle) == B_OK
		&& settings.AddBool("has border", fHasBorder) == B_OK)
		settings.Flatten(&file);
}


status_t
WorkspacesSettings::_Open(BFile& file, int mode)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	 if (status != B_OK)
		status = find_directory(B_COMMON_SETTINGS_DIRECTORY, &path);
	 if (status != B_OK)
		return status;

	path.Append(kSettingsFile);

	status = file.SetTo(path.Path(), mode);
	if (mode == B_READ_ONLY && status == B_ENTRY_NOT_FOUND) {
		if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
			path.Append(kSettingsFile);
			status = file.SetTo(path.Path(), mode);
		}
	}

	return status;
}


void
WorkspacesSettings::UpdateFramesForScreen(BRect newScreenFrame)
{
	// don't change the position if the screen frame hasn't changed
	if (newScreenFrame == fScreenFrame)
		return;

	// adjust horizontal position
	if (fWindowFrame.right > fScreenFrame.right / 2)
		fWindowFrame.OffsetTo(newScreenFrame.right
			- (fScreenFrame.right - fWindowFrame.left), fWindowFrame.top);

	// adjust vertical position
	if (fWindowFrame.bottom > fScreenFrame.bottom / 2)
		fWindowFrame.OffsetTo(fWindowFrame.left,
			newScreenFrame.bottom - (fScreenFrame.bottom - fWindowFrame.top));

	fScreenFrame = newScreenFrame;
}


void
WorkspacesSettings::UpdateScreenFrame()
{
	BScreen screen;
	fScreenFrame = screen.Frame();
}


void
WorkspacesSettings::SetWindowFrame(BRect frame)
{
	fWindowFrame = frame;
}


//	#pragma mark -


WorkspacesView::WorkspacesView(BRect frame)
	: BView(frame, "workspaces", B_FOLLOW_ALL, kWorkspacesViewFlag)
{
	frame.OffsetTo(B_ORIGIN);
	frame.top = frame.bottom - 7;
	frame.left = frame.right - 7;
	BDragger* dragger = new BDragger(frame, this,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
}


WorkspacesView::WorkspacesView(BMessage* archive)
	: BView(archive)
{
}


WorkspacesView::~WorkspacesView()
{
}


/*static*/ WorkspacesView*
WorkspacesView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "WorkspacesView"))
		return NULL;

	return new WorkspacesView(archive);
}


status_t
WorkspacesView::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status == B_OK)
		status = archive->AddString("add_on", kSignature);
	if (status == B_OK)
		status = archive->AddString("class", "WorkspacesView");

	return status;
}


void
WorkspacesView::_AboutRequested()
{
	BAlert *alert = new BAlert("about", "Workspaces\n"
		"written by François Revol, Axel Dörfler, and Matt Madia.\n\n"
		"Copyright 2002-2008, Haiku.\n\n"
		"Send windows behind using the Option key. "
		"Move windows to front using the Control key.\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 10, &font);

	alert->Go();
}


void
WorkspacesView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			_AboutRequested();
			break;

		case kMsgChangeCount:
			be_roster->Launch("application/x-vnd.Be-SCRN");
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
WorkspacesView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (Window() == NULL || EventMask() == 0)
		return;

	// Auto-Raise

	where = ConvertToScreen(where);
	BScreen screen(Window());
	BRect frame = screen.Frame();
	if (where.x == frame.left || where.x == frame.right
		|| where.y == frame.top || where.y == frame.bottom) {
		// cursor is on screen edge
		if (Window()->Frame().Contains(where))
			Window()->Activate();
	}
}


void
WorkspacesView::MouseDown(BPoint where)
{
	int32 buttons = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) == 0)
		return;

	// open context menu

	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	menu->SetFont(be_plain_font);

	// TODO: alternatively change the count here directly?
	BMenuItem* changeItem = new BMenuItem("Change Workspace Count"
		B_UTF8_ELLIPSIS, new BMessage(kMsgChangeCount));
	menu->AddItem(changeItem);

	WorkspacesWindow* window = dynamic_cast<WorkspacesWindow*>(Window());
	if (window != NULL) {
		BMenuItem* item;

		menu->AddSeparatorItem();
		menu->AddItem(item = new BMenuItem("No title",
			new BMessage(kMsgToggleTitle)));
		if (window->Look() == B_MODAL_WINDOW_LOOK)
			item->SetMarked(true);
		menu->AddItem(item = new BMenuItem("No Border",
			new BMessage(kMsgToggleBorder)));
		if (window->Look() == B_NO_BORDER_WINDOW_LOOK)
			item->SetMarked(true);

		menu->AddSeparatorItem();
		menu->AddItem(item = new BMenuItem("Always On Top",
			new BMessage(kMsgToggleAlwaysOnTop)));
		if (window->Feel() == B_FLOATING_ALL_WINDOW_FEEL)
			item->SetMarked(true);
		menu->AddItem(item = new BMenuItem("Auto-Raise",
			new BMessage(kMsgToggleAutoRaise)));
		if (window->IsAutoRaising())
			item->SetMarked(true);

		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS,
			new BMessage(B_ABOUT_REQUESTED)));
		menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED)));
		menu->SetTargetForItems(window);
	}

	changeItem->SetTarget(this);
	ConvertToScreen(&where);
	menu->Go(where, true, false, true);
}


//	#pragma mark -


WorkspacesWindow::WorkspacesWindow(WorkspacesSettings *settings)
	: BWindow(settings->WindowFrame(), "Workspaces", B_TITLED_WINDOW_LOOK,
 			B_NORMAL_WINDOW_FEEL, B_AVOID_FRONT | B_WILL_ACCEPT_FIRST_CLICK,
 			B_ALL_WORKSPACES),
 	fSettings(settings),
 	fAutoRaising(false)
{
	AddChild(new WorkspacesView(Bounds()));
	fPreviousFrame = Frame();

	if (!fSettings->HasTitle())
		SetLook(B_MODAL_WINDOW_LOOK);
	else if (!fSettings->HasBorder())
		SetLook(B_NO_BORDER_WINDOW_LOOK);

	if (fSettings->AlwaysOnTop())
		SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
	else
		SetAutoRaise(fSettings->AutoRaising());
}


WorkspacesWindow::~WorkspacesWindow()
{
	delete fSettings;
}


void
WorkspacesWindow::ScreenChanged(BRect rect, color_space mode)
{
	fPreviousFrame = fSettings->WindowFrame();
		// work-around for a bug in BeOS, see explanation in FrameMoved()

	fSettings->UpdateFramesForScreen(rect);
	MoveTo(fSettings->WindowFrame().LeftTop());
}


void
WorkspacesWindow::FrameMoved(BPoint origin)
{
	if (origin == fPreviousFrame.LeftTop()) {
		// This works around a bug in BeOS; when you change the window
		// position in WorkspaceActivated() or ScreenChanged(), it will
		// send an old repositioning message *after* the FrameMoved()
		// that originated your change has arrived
		return;
	}

	fSettings->SetWindowFrame(Frame());
}


void
WorkspacesWindow::FrameResized(float width, float height)
{
	fSettings->SetWindowFrame(Frame());
}


void
WorkspacesWindow::Zoom(BPoint origin, float width, float height)
{
	BScreen screen;
	origin = screen.Frame().RightBottom();
	origin.x -= kScreenBorderOffset + fSettings->WindowFrame().Width();
	origin.y -= kScreenBorderOffset + fSettings->WindowFrame().Height();

	MoveTo(origin);
}


void
WorkspacesWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		{
			// Drop from Tracker
			entry_ref ref;
			for (int i = 0; (message->FindRef("refs", i, &ref) == B_OK); i++)
				be_roster->Launch(&ref);
			break;
		}

		case B_ABOUT_REQUESTED:
			PostMessage(message, ChildAt(0));
			break;

		case kMsgToggleBorder:
		{
			bool enable = false;
			if (Look() == B_NO_BORDER_WINDOW_LOOK)
				enable = true;

			if (enable)
				SetLook(B_TITLED_WINDOW_LOOK);
			else
				SetLook(B_NO_BORDER_WINDOW_LOOK);

			fSettings->SetHasBorder(enable);
			break;
		}

		case kMsgToggleTitle:
		{
			bool enable = false;
			if (Look() == B_MODAL_WINDOW_LOOK)
				enable = true;

			if (enable)
				SetLook(B_TITLED_WINDOW_LOOK);
			else
				SetLook(B_MODAL_WINDOW_LOOK);

			fSettings->SetHasTitle(enable);
			break;
		}

		case kMsgToggleAutoRaise:
			SetAutoRaise(!IsAutoRaising());
			SetFeel(B_NORMAL_WINDOW_FEEL);
			break;

		case kMsgToggleAlwaysOnTop:
		{
			bool enable = false;
			if (Feel() != B_FLOATING_ALL_WINDOW_FEEL)
				enable = true;

			if (enable)
				SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
			else
				SetFeel(B_NORMAL_WINDOW_FEEL);

			fSettings->SetAlwaysOnTop(enable);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
WorkspacesWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
WorkspacesWindow::SetAutoRaise(bool enable)
{
	if (enable == fAutoRaising)
		return;

	fAutoRaising = enable;
	fSettings->SetAutoRaising(enable);

	if (enable)
		ChildAt(0)->SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	else
		ChildAt(0)->SetEventMask(0);
}


//	#pragma mark -


WorkspacesApp::WorkspacesApp()
	: BApplication(kSignature)
{
	fWindow = new WorkspacesWindow(new WorkspacesSettings());
}


WorkspacesApp::~WorkspacesApp()
{
}


void
WorkspacesApp::AboutRequested()
{
	fWindow->PostMessage(B_ABOUT_REQUESTED);
}


void
WorkspacesApp::Usage(const char *programName)
{
	printf("Usage: %s [options] [workspace]\n"
		"where \"options\" is one of:\n"
		"  --notitle\t\ttitle bar removed.  border and resize kept.\n"
		"  --noborder\t\ttitle, border, and resize removed.\n"
		"  --avoidfocus\t\tprevents the window from being the target of keyboard events.\n"
		"  --alwaysontop\t\tkeeps window on top\n"
		"  --notmovable\t\twindow can't be moved around\n"
		"  --autoraise\t\tauto-raise the workspace window when it's at the screen corner\n"
		"  --help\t\tdisplay this help and exit\n"
		"and \"workspace\" is the number of the Workspace to which to switch (0-31)\n",
		programName);

	// quit only if we aren't running already
	if (IsLaunching())
		Quit();
}


void
WorkspacesApp::ArgvReceived(int32 argc, char **argv)
{
	for (int i = 1;  i < argc;  i++) {
		if (argv[i][0] == '-' && argv[i][1] == '-') {
			// evaluate --arguments
			if (!strcmp(argv[i], "--notitle"))
				fWindow->SetLook(B_MODAL_WINDOW_LOOK);
			else if (!strcmp(argv[i], "--noborder"))
				fWindow->SetLook(B_NO_BORDER_WINDOW_LOOK);
			else if (!strcmp(argv[i], "--avoidfocus"))
				fWindow->SetFlags(fWindow->Flags() | B_AVOID_FOCUS);
			else if (!strcmp(argv[i], "--notmovable"))
				fWindow->SetFlags(fWindow->Flags() | B_NOT_MOVABLE);
			else if (!strcmp(argv[i], "--alwaysontop"))
				fWindow->SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
			else if (!strcmp(argv[i], "--autoraise"))
				fWindow->SetAutoRaise(true);
			else {
				const char *programName = strrchr(argv[0], '/');
				programName = programName ? programName + 1 : argv[0];

				Usage(programName);
			}
		} else if (isdigit(*argv[i])) {
			// check for a numeric arg, if not already given
			activate_workspace(atoi(argv[i]));

			// if the app is running, don't quit
			// but if it isn't, cancel the complete run, so it doesn't
			// open any window
			if (IsLaunching())
				Quit();
		} else if (!strcmp(argv[i], "-")) {
			activate_workspace(current_workspace() - 1);

			if (IsLaunching())
				Quit();
		} else if (!strcmp(argv[i], "+")) {
			activate_workspace(current_workspace() + 1);

			if (IsLaunching())
				Quit();
		} else {
			// some unknown arguments were specified
			fprintf(stderr, "Invalid argument: %s\n", argv[i]);

			if (IsLaunching())
				Quit();
		}
	}
}


void
WorkspacesApp::ReadyToRun()
{
	fWindow->Show();
}


//	#pragma mark -


int
main(int32 argc, char **argv)
{
	WorkspacesApp app;
	app.Run();

	return 0;
}

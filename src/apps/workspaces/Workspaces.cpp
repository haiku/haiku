/*
 * Copyright 2002-2016, Haiku, Inc. All rights reserved.
 * Copyright 2002, François Revol, revol@free.fr.
 * This file is distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver "Madison" Kohl,
 *		Matt Madia
 *		Daniel Devine, devine@ddevnet.net
 */


#include <AboutWindow.h>
#include <Application.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locale.h>
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

#include <InterfacePrivate.h>
#include <ViewPrivate.h>
#include <WindowPrivate.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Workspaces"


static const char* kDeskbarItemName = "workspaces";
static const char* kSignature = "application/x-vnd.Be-WORK";
static const char* kDeskbarSignature = "application/x-vnd.Be-TSKB";
static const char* kScreenPrefletSignature = "application/x-vnd.Haiku-Screen";
static const char* kOldSettingFile = "Workspace_data";
static const char* kSettingsFile = "Workspaces_settings";

static const uint32 kMsgChangeCount = 'chWC';
static const uint32 kMsgToggleTitle = 'tgTt';
static const uint32 kMsgToggleBorder = 'tgBd';
static const uint32 kMsgToggleAutoRaise = 'tgAR';
static const uint32 kMsgToggleAlwaysOnTop = 'tgAT';
static const uint32 kMsgToggleLiveInDeskbar = 'tgDb';
static const uint32 kMsgToggleSwitchOnWheel = 'tgWh';


extern "C" _EXPORT BView* instantiate_deskbar_item(float maxWidth,
	float maxHeight);


static status_t
OpenSettingsFile(BFile& file, int mode)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		status = find_directory(B_SYSTEM_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsFile);
	if (status != B_OK)
		return status;

	status = file.SetTo(path.Path(), mode);
	if (mode == B_READ_ONLY && status == B_ENTRY_NOT_FOUND) {
		if (find_directory(B_SYSTEM_SETTINGS_DIRECTORY, &path) == B_OK
			&& path.Append(kSettingsFile) == B_OK) {
			status = file.SetTo(path.Path(), mode);
		}
	}

	return status;
}


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
		bool SettingsLoaded() const { return fLoaded; }

		void UpdateFramesForScreen(BRect screenFrame);
		void UpdateScreenFrame();

		void SetWindowFrame(BRect);
		void SetAutoRaising(bool enable) { fAutoRaising = enable; }
		void SetAlwaysOnTop(bool enable) { fAlwaysOnTop = enable; }
		void SetHasTitle(bool enable) { fHasTitle = enable; }
		void SetHasBorder(bool enable) { fHasBorder = enable; }

	private:
		BRect	fWindowFrame;
		BRect	fScreenFrame;
		bool	fAutoRaising;
		bool	fAlwaysOnTop;
		bool	fHasTitle;
		bool	fHasBorder;
		bool	fLoaded;
};

class WorkspacesView : public BView {
	public:
		WorkspacesView(BRect frame, bool showDragger);
		WorkspacesView(BMessage* archive);
		~WorkspacesView();

		static	WorkspacesView* Instantiate(BMessage* archive);
		virtual	status_t Archive(BMessage* archive, bool deep = true) const;

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void FrameMoved(BPoint newPosition);
		virtual void FrameResized(float newWidth, float newHeight);
		virtual void MessageReceived(BMessage* message);
		virtual void MouseMoved(BPoint where, uint32 transit,
			const BMessage* dragMessage);
		virtual void MouseDown(BPoint where);

		bool SwitchOnWheel() const { return fSwitchOnWheel; }
		void SetSwitchOnWheel(bool enable);

	private:
		void _AboutRequested();

		void _UpdateParentClipping();
		void _ExcludeFromParentClipping();
		void _CleanupParentClipping();

		friend class WorkspacesWindow;

		void _LoadSettings();
		void _SaveSettings();

		BView*	fParentWhichDrawsOnChildren;
		BRect	fCurrentFrame;
		bool	fSwitchOnWheel;
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
		bool IsAutoRaising() const { return fSettings->AutoRaising(); }

		float GetTabHeight() { return fSettings->HasTitle() ? fTabHeight : 0; }
		float GetBorderWidth() { return fBorderWidth; }
		float GetScreenBorderOffset() { return 2.0 * fBorderWidth; }

	private:
		WorkspacesSettings *fSettings;
		WorkspacesView *fWorkspacesView;
		float	fTabHeight;
		float	fBorderWidth;
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


//	#pragma mark - WorkspacesSettings


WorkspacesSettings::WorkspacesSettings()
	:
	fAutoRaising(false),
	fAlwaysOnTop(false),
	fHasTitle(true),
	fHasBorder(true),
	fLoaded(false)
{
	UpdateScreenFrame();

	BScreen screen;

	BFile file;
	if (OpenSettingsFile(file, B_READ_ONLY) == B_OK) {
		BMessage settings;
		if (settings.Unflatten(&file) == B_OK) {
			fLoaded = settings.FindRect("window", &fWindowFrame) == B_OK
				&& settings.FindRect("screen", &fScreenFrame) == B_OK;
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

				fLoaded = true;
			}
		}
	}

	if (fLoaded) {
		// if the current screen frame is different from the one
		// just loaded, we need to alter the window frame accordingly
		if (fScreenFrame != screen.Frame())
			UpdateFramesForScreen(screen.Frame());
	}
}


WorkspacesSettings::~WorkspacesSettings()
{
	BFile file;
	if (OpenSettingsFile(file, B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE)
			!= B_OK) {
		return;
	}

	// switch on wheel saved by view later on

	BMessage settings('wksp');
	if (settings.AddRect("window", fWindowFrame) == B_OK
		&& settings.AddRect("screen", fScreenFrame) == B_OK
		&& settings.AddBool("auto-raise", fAutoRaising) == B_OK
		&& settings.AddBool("always on top", fAlwaysOnTop) == B_OK
		&& settings.AddBool("has title", fHasTitle) == B_OK
		&& settings.AddBool("has border", fHasBorder) == B_OK) {
		settings.Flatten(&file);
	}
}


void
WorkspacesSettings::UpdateFramesForScreen(BRect newScreenFrame)
{
	// don't change the position if the screen frame hasn't changed
	if (newScreenFrame == fScreenFrame)
		return;

	// adjust horizontal position
	if (fWindowFrame.right > fScreenFrame.right / 2) {
		fWindowFrame.OffsetTo(newScreenFrame.right
			- (fScreenFrame.right - fWindowFrame.left), fWindowFrame.top);
	}

	// adjust vertical position
	if (fWindowFrame.bottom > fScreenFrame.bottom / 2) {
		fWindowFrame.OffsetTo(fWindowFrame.left,
			newScreenFrame.bottom - (fScreenFrame.bottom - fWindowFrame.top));
	}

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


//	#pragma mark - WorkspacesView


WorkspacesView::WorkspacesView(BRect frame, bool showDragger = true)
	:
	BView(frame, kDeskbarItemName, B_FOLLOW_ALL,
		kWorkspacesViewFlag | B_FRAME_EVENTS),
	fParentWhichDrawsOnChildren(NULL),
	fCurrentFrame(frame),
	fSwitchOnWheel(false)
{
	_LoadSettings();

	if (showDragger) {
		frame.OffsetTo(B_ORIGIN);
		frame.top = frame.bottom - 7;
		frame.left = frame.right - 7;
		BDragger* dragger = new BDragger(frame, this,
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		AddChild(dragger);
	}
}


WorkspacesView::WorkspacesView(BMessage* archive)
	:
	BView(archive),
	fParentWhichDrawsOnChildren(NULL),
	fCurrentFrame(Frame()),
	fSwitchOnWheel(false)
{
	_LoadSettings();

	// Just in case we are instantiated from an older archive...
	SetFlags(Flags() | B_FRAME_EVENTS);
	// Make sure the auto-raise feature didn't leave any artifacts - this is
	// not a good idea to keep enabled for a replicant.
	if (EventMask() != 0)
		SetEventMask(0);
}


WorkspacesView::~WorkspacesView()
{
	_SaveSettings();
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
	BAboutWindow* window = new BAboutWindow(
		B_TRANSLATE_SYSTEM_NAME("Workspaces"), kSignature);

	const char* authors[] = {
		"Axel Dörfler",
		"Oliver \"Madison\" Kohl",
		"Matt Madia",
		"François Revol",
		NULL
	};

	const char* extraCopyrights[] = {
		"2002 François Revol",
		NULL
	};

	const char* extraInfo = "Send windows behind using the Option key. "
		"Move windows to front using the Control key.\n";

	window->AddCopyright(2002, "Haiku, Inc.",
			extraCopyrights);
	window->AddAuthors(authors);
	window->AddExtraInfo(extraInfo);

	window->Show();
}


void
WorkspacesView::AttachedToWindow()
{
	BView* parent = Parent();
	if (parent != NULL && (parent->Flags() & B_DRAW_ON_CHILDREN) != 0) {
		fParentWhichDrawsOnChildren = parent;
		_ExcludeFromParentClipping();
	}
}


void
WorkspacesView::DetachedFromWindow()
{
	if (fParentWhichDrawsOnChildren != NULL)
		_CleanupParentClipping();
}


void
WorkspacesView::FrameMoved(BPoint newPosition)
{
	_UpdateParentClipping();
}


void
WorkspacesView::FrameResized(float newWidth, float newHeight)
{
	_UpdateParentClipping();
}


void
WorkspacesView::_UpdateParentClipping()
{
	if (fParentWhichDrawsOnChildren != NULL) {
		_CleanupParentClipping();
		_ExcludeFromParentClipping();
		fParentWhichDrawsOnChildren->Invalidate(fCurrentFrame);
		fCurrentFrame = Frame();
	}
}


void
WorkspacesView::_ExcludeFromParentClipping()
{
	// Prevent the parent view to draw over us. Do so in a way that allows
	// restoring the parent to the previous state.
	fParentWhichDrawsOnChildren->PushState();

	BRegion clipping(fParentWhichDrawsOnChildren->Bounds());
	clipping.Exclude(Frame());
	fParentWhichDrawsOnChildren->ConstrainClippingRegion(&clipping);
}


void
WorkspacesView::_CleanupParentClipping()
{
	// Restore the previous parent state. NOTE: This relies on views
	// being detached in exactly the opposite order as them being
	// attached. Otherwise we would mess up states if a sibbling view did
	// the same thing we did in AttachedToWindow()...
	fParentWhichDrawsOnChildren->PopState();
}


void
WorkspacesView::_LoadSettings()
{
	BFile file;
	if (OpenSettingsFile(file, B_READ_ONLY) == B_OK) {
		BMessage settings;
		if (settings.Unflatten(&file) == B_OK)
			settings.FindBool("switch on wheel", &fSwitchOnWheel);
	}
}


void
WorkspacesView::_SaveSettings()
{
	BFile file;
	if (OpenSettingsFile(file, B_READ_ONLY | B_CREATE_FILE) != B_OK)
		return;

	BMessage settings('wksp');
	settings.Unflatten(&file);

	if (OpenSettingsFile(file, B_WRITE_ONLY | B_ERASE_FILE) != B_OK)
		return;

	if (settings.ReplaceBool("switch on wheel", fSwitchOnWheel) != B_OK)
		settings.AddBool("switch on wheel", fSwitchOnWheel);

	settings.Flatten(&file);
}


void
WorkspacesView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			_AboutRequested();
			break;

		case B_MOUSE_WHEEL_CHANGED:
		{
			if (!fSwitchOnWheel)
				break;

			float deltaY = message->FindFloat("be:wheel_delta_y");
			if (deltaY > 0.1)
				activate_workspace(current_workspace() + 1);
			else if (deltaY < -0.1)
				activate_workspace(current_workspace() - 1);
			break;
		}

		case kMsgChangeCount:
			be_roster->Launch(kScreenPrefletSignature);
			break;

		case kMsgToggleLiveInDeskbar:
		{
			// only actually used from the replicant itself
			// since HasItem() locks up we just remove directly.
			BDeskbar deskbar;
			// we shouldn't do this here actually, but it works for now...
			deskbar.RemoveItem(kDeskbarItemName);
			break;
		}

		case kMsgToggleSwitchOnWheel:
		{
			fSwitchOnWheel = !fSwitchOnWheel;
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
WorkspacesView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	WorkspacesWindow* window = dynamic_cast<WorkspacesWindow*>(Window());
	if (window == NULL || !window->IsAutoRaising())
		return;

	// Auto-Raise

	where = ConvertToScreen(where);
	BScreen screen(window);
	BRect screenFrame = screen.Frame();
	BRect windowFrame = window->Frame();
	float tabHeight = window->GetTabHeight();
	float borderWidth = window->GetBorderWidth();

	if (where.x == screenFrame.left || where.x == screenFrame.right
			|| where.y == screenFrame.top || where.y == screenFrame.bottom) {
		// cursor is on screen edge

		// Stretch frame to also accept mouse moves over the window borders
		windowFrame.InsetBy(-borderWidth, -(tabHeight + borderWidth));

		if (windowFrame.Contains(where))
			window->Activate();
	}
}


void
WorkspacesView::MouseDown(BPoint where)
{
	// With enabled auto-raise feature, we'll get mouse messages we don't
	// want to handle here.
	if (!Bounds().Contains(where))
		return;

	int32 buttons = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) == 0)
		return;

	// open context menu

	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	menu->SetFont(be_plain_font);

	// TODO: alternatively change the count here directly?
	BMenuItem* changeItem = new BMenuItem(B_TRANSLATE("Change workspace count"
		B_UTF8_ELLIPSIS), new BMessage(kMsgChangeCount));
	menu->AddItem(changeItem);

	BMenuItem* switchItem = new BMenuItem(B_TRANSLATE("Switch on mouse wheel"),
		new BMessage(kMsgToggleSwitchOnWheel));
	menu->AddItem(switchItem);
	switchItem->SetMarked(fSwitchOnWheel);

	WorkspacesWindow *window = dynamic_cast<WorkspacesWindow*>(Window());
	if (window != NULL) {
		// inside Workspaces app
		BMenuItem* item;

		menu->AddSeparatorItem();
		menu->AddItem(item = new BMenuItem(B_TRANSLATE("Show window tab"),
			new BMessage(kMsgToggleTitle)));
		if (window->Look() == B_TITLED_WINDOW_LOOK)
			item->SetMarked(true);
		menu->AddItem(item = new BMenuItem(B_TRANSLATE("Show window border"),
			new BMessage(kMsgToggleBorder)));
		if (window->Look() == B_TITLED_WINDOW_LOOK
			|| window->Look() == B_MODAL_WINDOW_LOOK) {
			item->SetMarked(true);
		}

		menu->AddSeparatorItem();
		menu->AddItem(item = new BMenuItem(B_TRANSLATE("Always on top"),
			new BMessage(kMsgToggleAlwaysOnTop)));
		if (window->Feel() == B_FLOATING_ALL_WINDOW_FEEL)
			item->SetMarked(true);
		menu->AddItem(item = new BMenuItem(B_TRANSLATE("Auto-raise"),
			new BMessage(kMsgToggleAutoRaise)));
		if (window->IsAutoRaising())
			item->SetMarked(true);
		if (be_roster->IsRunning(kDeskbarSignature)) {
			menu->AddItem(item = new BMenuItem(
				B_TRANSLATE("Live in the Deskbar"),
				new BMessage(kMsgToggleLiveInDeskbar)));
			BDeskbar deskbar;
			item->SetMarked(deskbar.HasItem(kDeskbarItemName));
		}

		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
			new BMessage(B_QUIT_REQUESTED)));
		menu->SetTargetForItems(window);
	} else {
		// we're replicated in some way...
		BMenuItem* item;

		menu->AddSeparatorItem();

		// check which way
		BDragger *dragger = dynamic_cast<BDragger*>(ChildAt(0));
		if (dragger) {
			// replicant
			menu->AddItem(item = new BMenuItem(B_TRANSLATE("Remove replicant"),
				new BMessage(B_TRASH_TARGET)));
			item->SetTarget(dragger);
		} else {
			// Deskbar item
			menu->AddItem(item = new BMenuItem(B_TRANSLATE("Remove replicant"),
				new BMessage(kMsgToggleLiveInDeskbar)));
			item->SetTarget(this);
		}
	}

	changeItem->SetTarget(this);
	switchItem->SetTarget(this);

	ConvertToScreen(&where);
	menu->Go(where, true, true, true);
}


void
WorkspacesView::SetSwitchOnWheel(bool enable)
{
	if (enable == fSwitchOnWheel)
		return;

	fSwitchOnWheel = enable;
}


//	#pragma mark - WorkspacesWindow


WorkspacesWindow::WorkspacesWindow(WorkspacesSettings *settings)
	:
	BWindow(settings->WindowFrame(), B_TRANSLATE_SYSTEM_NAME("Workspaces"),
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_AVOID_FRONT | B_WILL_ACCEPT_FIRST_CLICK | B_CLOSE_ON_ESCAPE,
		B_ALL_WORKSPACES),
	fSettings(settings),
	fWorkspacesView(NULL)
{
	// Turn window decor on to grab decor widths.
	BMessage windowSettings;
	float borderWidth = 0;

	SetLook(B_TITLED_WINDOW_LOOK);
	if (GetDecoratorSettings(&windowSettings) == B_OK) {
		BRect tabFrame = windowSettings.FindRect("tab frame");
		borderWidth = windowSettings.FindFloat("border width");
		fTabHeight = tabFrame.Height();
		fBorderWidth = borderWidth;
	}

	if (!fSettings->SettingsLoaded()) {
		// No settings, compute a reasonable default frame.
		// We aim for previews at 10% of actual screen size, and matching the
		// aspect ratio. We then scale that down, until it fits the screen.
		// Finally, we put the window on the bottom right of the screen so the
		// auto-raise mode can be used.

		BScreen screen;

		float screenWidth = screen.Frame().Width();
		float screenHeight = screen.Frame().Height();
		float aspectRatio = screenWidth / screenHeight;

		uint32 columns, rows;
		BPrivate::get_workspaces_layout(&columns, &rows);

		// default size of ~1/10 of screen width
		float workspaceWidth = screenWidth / 10;
		float workspaceHeight = workspaceWidth / aspectRatio;

		float width = floor(workspaceWidth * columns);
		float height = floor(workspaceHeight * rows);

		// If you have too many workspaces to fit on the screen, shrink until
		// they fit.
		while (width + 2 * borderWidth > screenWidth
				|| height + 2 * borderWidth + GetTabHeight() > screenHeight) {
			width = floor(0.95 * width);
			height = floor(0.95 * height);
		}

		BRect frame = fSettings->ScreenFrame();
		frame.OffsetBy(-2.0 * borderWidth, -2.0 * borderWidth);
		frame.left = frame.right - width;
		frame.top = frame.bottom - height;
		ResizeTo(frame.Width(), frame.Height());

		// Put it in bottom corner by default.
		MoveTo(screenWidth - frame.Width() - borderWidth,
			screenHeight - frame.Height() - borderWidth);

		fSettings->SetWindowFrame(frame);
	}

	if (!fSettings->HasBorder())
		SetLook(B_NO_BORDER_WINDOW_LOOK);
	else if (!fSettings->HasTitle())
		SetLook(B_MODAL_WINDOW_LOOK);

	fWorkspacesView = new WorkspacesView(Bounds());
	AddChild(fWorkspacesView);

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
	fSettings->UpdateFramesForScreen(rect);
	MoveTo(fSettings->WindowFrame().LeftTop());
}


void
WorkspacesWindow::FrameMoved(BPoint origin)
{
	fSettings->SetWindowFrame(Frame());
}


void
WorkspacesWindow::FrameResized(float width, float height)
{
	if (!(modifiers() & B_SHIFT_KEY)) {
		BWindow::FrameResized(width, height);
		return;
	}

	uint32 columns, rows;
	BPrivate::get_workspaces_layout(&columns, &rows);

	BScreen screen;
	float screenWidth = screen.Frame().Width();
	float screenHeight = screen.Frame().Height();

	float windowAspectRatio
		= (columns * screenWidth) / (rows * screenHeight);

	float newHeight = width / windowAspectRatio;

	if (height != newHeight)
		ResizeTo(width, newHeight);

	fSettings->SetWindowFrame(Frame());
}


void
WorkspacesWindow::Zoom(BPoint origin, float width, float height)
{
	BScreen screen;
	float screenWidth = screen.Frame().Width();
	float screenHeight = screen.Frame().Height();
	float aspectRatio = screenWidth / screenHeight;

	uint32 columns, rows;
	BPrivate::get_workspaces_layout(&columns, &rows);

	float workspaceWidth = Frame().Width() / columns;
	float workspaceHeight = workspaceWidth / aspectRatio;

	width = floor(workspaceWidth * columns);
	height = floor(workspaceHeight * rows);

	while (width + 2 * GetScreenBorderOffset() > screenWidth
		|| height + 2 * GetScreenBorderOffset() + GetTabHeight()
			> screenHeight) {
		width = floor(0.95 * width);
		height = floor(0.95 * height);
	}

	ResizeTo(width, height);

	if (fSettings->AutoRaising()) {
		// The auto-raising mode makes sense only if the window is positionned
		// exactly in the bottom-right corner. If the setting is enabled, move
		// the window there.
		origin = screen.Frame().RightBottom();
		origin.x -= GetScreenBorderOffset() + width;
		origin.y -= GetScreenBorderOffset() + height;

		MoveTo(origin);
	}
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
				if (fSettings->HasTitle())
					SetLook(B_TITLED_WINDOW_LOOK);
				else
					SetLook(B_MODAL_WINDOW_LOOK);
			else
				SetLook(B_NO_BORDER_WINDOW_LOOK);

			fSettings->SetHasBorder(enable);
			break;
		}

		case kMsgToggleTitle:
		{
			bool enable = false;
			if (Look() == B_MODAL_WINDOW_LOOK
				|| Look() == B_NO_BORDER_WINDOW_LOOK)
				enable = true;

			if (enable)
				SetLook(B_TITLED_WINDOW_LOOK);
			else
				SetLook(B_MODAL_WINDOW_LOOK);

			// No matter what the setting for title, we must force the border on
			fSettings->SetHasBorder(true);
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

		case kMsgToggleLiveInDeskbar:
		{
			BDeskbar deskbar;
			if (deskbar.HasItem(kDeskbarItemName))
				deskbar.RemoveItem(kDeskbarItemName);
			else {
				fWorkspacesView->_SaveSettings();
					// save "switch on wheel" setting for replicant to load
				entry_ref ref;
				be_roster->FindApp(kSignature, &ref);
				deskbar.AddItem(&ref);
			}
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
	fSettings->SetAutoRaising(enable);

	if (enable)
		ChildAt(0)->SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	else
		ChildAt(0)->SetEventMask(0);
}


//	#pragma mark - WorkspacesApp


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
	printf(B_TRANSLATE("Usage: %s [options] [workspace]\n"
		"where \"options\" are:\n"
		"  --notitle\t\ttitle bar removed, border and resize kept\n"
		"  --noborder\t\ttitle, border, and resize removed\n"
		"  --avoidfocus\t\tprevents the window from being the target of "
		"keyboard events\n"
		"  --alwaysontop\t\tkeeps window on top\n"
		"  --notmovable\t\twindow can't be moved around\n"
		"  --autoraise\t\tauto-raise the workspace window when it's at the "
		"screen edge\n"
		"  --help\t\tdisplay this help and exit\n"
		"and \"workspace\" is the number of the Workspace to which to switch "
		"(0-31)\n"),
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
			fprintf(stderr, B_TRANSLATE("Invalid argument: %s\n"), argv[i]);

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


BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	// Calculate the correct size of the Deskbar replicant first

	BScreen screen;
	float screenWidth = screen.Frame().Width();
	float screenHeight = screen.Frame().Height();
	float aspectRatio = screenWidth / screenHeight;
	uint32 columns, rows;
	BPrivate::get_workspaces_layout(&columns, &rows);

	// We use 1px for the top and left borders (shown as double)
	// and divide the remainder equally. However, we keep in mind
	// that the actual width and height of each workspace is smaller
	// by 1px, because of bottom/right borders (shown as single).
	// When calculating workspace width, we must ensure that the assumed
	// actual workspace height is not negative. Zero is OK.

	float height = maxHeight;
	float rowHeight = floor((height - 1) / rows);
	if (rowHeight < 1)
		rowHeight = 1;

	float columnWidth = floor((rowHeight - 1) * aspectRatio) + 1;

	float width = columnWidth * columns + 1;
	if (width > maxWidth)
		width = maxWidth;

	return new WorkspacesView(BRect (0, 0, width - 1, height - 1), false);
}


//	#pragma mark -


int
main(int argc, char **argv)
{
	WorkspacesApp app;
	app.Run();

	return 0;
}

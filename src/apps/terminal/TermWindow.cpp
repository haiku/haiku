/*
 * Copyright 2007-2010, Haiku, Inc. All rights reserved.
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 *
 * Distributed under the terms of the MIT license.
 */

#include "TermWindow.h"

#include <new>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Clipboard.h>
#include <Dragger.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <PrintJob.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <UTF8.h>

#include <AutoLocker.h>

#include "ActiveProcessInfo.h"
#include "Arguments.h"
#include "AppearPrefView.h"
#include "FindWindow.h"
#include "Globals.h"
#include "PrefWindow.h"
#include "PrefHandler.h"
#include "SetTitleDialog.h"
#include "ShellParameters.h"
#include "TermConst.h"
#include "TermScrollView.h"
#include "TitlePlaceholderMapper.h"


const static int32 kMaxTabs = 6;
const static int32 kTermViewOffset = 3;

// messages constants
static const uint32 kNewTab = 'NTab';
static const uint32 kCloseView = 'ClVw';
static const uint32 kCloseOtherViews = 'CloV';
static const uint32 kIncreaseFontSize = 'InFs';
static const uint32 kDecreaseFontSize = 'DcFs';
static const uint32 kSetActiveTab = 'STab';
static const uint32 kUpdateTitles = 'UPti';
static const uint32 kEditTabTitle = 'ETti';
static const uint32 kEditWindowTitle = 'EWti';
static const uint32 kTabTitleChanged = 'TTch';
static const uint32 kWindowTitleChanged = 'WTch';
static const uint32 kUpdateSwitchTerminalsMenuItem = 'Ustm';

using namespace BPrivate ; // BCharacterSet stuff

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Terminal TermWindow"


// #pragma mark - TermViewContainerView


class TermViewContainerView : public BView {
public:
	TermViewContainerView(TermView* termView)
		:
		BView(BRect(), "term view container", B_FOLLOW_ALL, 0),
		fTermView(termView)
	{
		termView->MoveTo(kTermViewOffset, kTermViewOffset);
		BRect frame(termView->Frame());
		ResizeTo(frame.right + kTermViewOffset, frame.bottom + kTermViewOffset);
		AddChild(termView);
	}

	TermView* GetTermView() const	{ return fTermView; }

	virtual void GetPreferredSize(float* _width, float* _height)
	{
		float width, height;
		fTermView->GetPreferredSize(&width, &height);
		*_width = width + 2 * kTermViewOffset;
		*_height = height + 2 * kTermViewOffset;
	}

private:
	TermView*	fTermView;
};


// #pragma mark - SessionID


TermWindow::SessionID::SessionID(int32 id)
	:
	fID(id)
{
}


TermWindow::SessionID::SessionID(const BMessage& message, const char* field)
{
	if (message.FindInt32(field, &fID) != B_OK)
		fID = -1;
}


status_t
TermWindow::SessionID::AddToMessage(BMessage& message, const char* field) const
{
	return message.AddInt32(field, fID);
}


// #pragma mark - Session


struct TermWindow::Session {
	SessionID				id;
	int32					index;
	Title					title;
	TermViewContainerView*	containerView;

	Session(SessionID id, int32 index, TermViewContainerView* containerView)
		:
		id(id),
		index(index),
		containerView(containerView)
	{
		title.title = B_TRANSLATE("Shell ");
		title.title << index;
		title.patternUserDefined = false;
	}
};


// #pragma mark - TermWindow


TermWindow::TermWindow(const BString& title, Arguments* args)
	:
	BWindow(BRect(0, 0, 0, 0), title, B_DOCUMENT_WINDOW,
		B_CURRENT_WORKSPACE | B_QUIT_ON_WINDOW_CLOSE),
	fTitleUpdateRunner(this, BMessage(kUpdateTitles), 1000000),
	fNextSessionID(0),
	fTabView(NULL),
	fMenuBar(NULL),
	fSwitchTerminalsMenuItem(NULL),
	fEncodingMenu(NULL),
	fPrintSettings(NULL),
	fPrefWindow(NULL),
	fFindPanel(NULL),
	fSavedFrame(0, 0, -1, -1),
	fSetWindowTitleDialog(NULL),
	fSetTabTitleDialog(NULL),
	fFindString(""),
	fFindNextMenuItem(NULL),
	fFindPreviousMenuItem(NULL),
	fFindSelection(false),
	fForwardSearch(false),
	fMatchCase(false),
	fMatchWord(false),
	fFullScreen(false)
{
	// register this terminal
	fTerminalRoster.Register(Team(), this);
	fTerminalRoster.SetListener(this);
	int32 id = fTerminalRoster.ID();

	// apply the title settings
	fTitle.pattern = title;
	if (fTitle.pattern.Length() == 0) {
		fTitle.pattern = B_TRANSLATE_SYSTEM_NAME("Terminal");

		if (id >= 0)
			fTitle.pattern << " " << id + 1;

		fTitle.patternUserDefined = false;
	} else
		fTitle.patternUserDefined = true;

	fTitle.title = fTitle.pattern;
	fTitle.pattern = title;

	_TitleSettingsChanged();

	// get the saved window position and workspaces
	BRect frame;
	uint32 workspaces;
	if (_LoadWindowPosition(&frame, &workspaces) == B_OK) {
		// apply
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
		SetWorkspaces(workspaces);
	} else {
		// use computed defaults
		int i = id / 16;
		int j = id % 16;
		int k = (j * 16) + (i * 64) + 50;
		int l = (j * 16)  + 50;

		MoveTo(k, l);
	}

	// init the GUI and add a tab
	_InitWindow();
	_AddTab(args);

	// Announce our window as no longer minimized. That's not true, since it's
	// still hidden at this point, but it will be shown very soon.
	fTerminalRoster.SetWindowInfo(false, Workspaces());
}


TermWindow::~TermWindow()
{
	fTerminalRoster.Unregister();

	_FinishTitleDialog();

	if (fPrefWindow)
		fPrefWindow->PostMessage(B_QUIT_REQUESTED);

	if (fFindPanel && fFindPanel->Lock()) {
		fFindPanel->Quit();
		fFindPanel = NULL;
	}

	PrefHandler::DeleteDefault();

	for (int32 i = 0; Session* session = _SessionAt(i); i++)
		delete session;
}


void
TermWindow::SessionChanged()
{
	_UpdateSessionTitle(fTabView->Selection());
}


void
TermWindow::_InitWindow()
{
	// make menu bar
	_SetupMenu();

	// shortcuts to switch tabs
	for (int32 i = 0; i < 9; i++) {
		BMessage* message = new BMessage(kSetActiveTab);
		message->AddInt32("index", i);
		AddShortcut('1' + i, B_COMMAND_KEY, message);
	}

	AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY,
		new BMessage(MSG_MOVE_TAB_LEFT));
	AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY,
		new BMessage(MSG_MOVE_TAB_RIGHT));

	BRect textFrame = Bounds();
	textFrame.top = fMenuBar->Bounds().bottom + 1.0;

	fTabView = new SmartTabView(textFrame, "tab view", B_WIDTH_FROM_WIDEST);
	fTabView->SetListener(this);
	AddChild(fTabView);

	// Make the scroll view one pixel wider than the tab view container view, so
	// the scroll bar will look good.
	fTabView->SetInsets(0, 0, -1, 0);
}


bool
TermWindow::_CanClose(int32 index)
{
	bool warnOnExit = PrefHandler::Default()->getBool(PREF_WARN_ON_EXIT);

	if (!warnOnExit)
		return true;

	uint32 busyProcessCount = 0;
	BString busyProcessNames;
		// all names, separated by "\n\t"

	if (index != -1) {
		ShellInfo shellInfo;
		ActiveProcessInfo info;
		TermView* termView = _TermViewAt(index);
		if (termView->GetShellInfo(shellInfo)
			&& termView->GetActiveProcessInfo(info)
			&& (info.ID() != shellInfo.ProcessID()
				|| !shellInfo.IsDefaultShell())) {
			busyProcessCount++;
			busyProcessNames = info.Name();
		}
	} else {
		for (int32 i = 0; i < fSessions.CountItems(); i++) {
			ShellInfo shellInfo;
			ActiveProcessInfo info;
			TermView* termView = _TermViewAt(i);
			if (termView->GetShellInfo(shellInfo)
				&& termView->GetActiveProcessInfo(info)
				&& (info.ID() != shellInfo.ProcessID()
					|| !shellInfo.IsDefaultShell())) {
				if (++busyProcessCount > 1)
					busyProcessNames << "\n\t";
				busyProcessNames << info.Name();
			}
		}
	}

	if (busyProcessCount == 0)
		return true;

	BString alertMessage;
	if (busyProcessCount == 1) {
		// Only one pending process. Select the alert text depending on whether
		// the terminal will be closed.
		alertMessage = index == -1 || fSessions.CountItems() == 1
			? B_TRANSLATE("The process \"%1\" is still running.\n"
				"If you close the Terminal, the process will be killed.")
			: B_TRANSLATE("The process \"%1\" is still running.\n"
				"If you close the tab, the process will be killed.");
	} else {
		// multiple pending processes
		alertMessage = B_TRANSLATE(
			"The following processes are still running:\n\n"
			"\t%1\n\n"
			"If you close the Terminal, the processes will be killed.");
	}

	alertMessage.ReplaceFirst("%1", busyProcessNames);

	BAlert* alert = new BAlert(B_TRANSLATE("Really close?"),
		alertMessage, B_TRANSLATE("Close"), B_TRANSLATE("Cancel"), NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(1, B_ESCAPE);
	return alert->Go() == 0;
}


bool
TermWindow::QuitRequested()
{
	_FinishTitleDialog();

	if (!_CanClose(-1))
		return false;

	_SaveWindowPosition();

	return BWindow::QuitRequested();
}


void
TermWindow::MenusBeginning()
{
	TermView* view = _ActiveTermView();
		
	// Syncronize Encode Menu Pop-up menu and Preference.
	const BCharacterSet* charset
		= BCharacterSetRoster::GetCharacterSetByConversionID(view->Encoding());
	if (charset != NULL) {
		BString name(charset->GetPrintName());
		const char* mime = charset->GetMIMEName();
		if (mime)
			name << " (" << mime << ")";

		BMenuItem* item = fEncodingMenu->FindItem(name);
		if (item != NULL)
			item->SetMarked(true);
	}

	BFont font;
	view->GetTermFont(&font);

	float size = font.Size();

	fDecreaseFontSizeMenuItem->SetEnabled(size > 9);
	fIncreaseFontSizeMenuItem->SetEnabled(size < 18);

	BWindow::MenusBeginning();
}


/* static */
BMenu*
TermWindow::_MakeEncodingMenu()
{
	BMenu* menu = new (std::nothrow) BMenu(B_TRANSLATE("Text encoding"));
	if (menu == NULL)
		return NULL;

	BCharacterSetRoster roster;
	BCharacterSet charset;
	while (roster.GetNextCharacterSet(&charset) == B_OK) {
		int encoding = M_UTF8;
		const char* mime = charset.GetMIMEName();
		if (mime == NULL || strcasecmp(mime, "UTF-8") != 0)
			encoding = charset.GetConversionID();

		// filter out currently (???) not supported USC-2 and UTF-16
		if (encoding == B_UTF16_CONVERSION || encoding == B_UNICODE_CONVERSION)
			continue;

		BString name(charset.GetPrintName());
		if (mime)
			name << " (" << mime << ")";

		BMessage *message = new BMessage(MENU_ENCODING);
		if (message != NULL) {
			message->AddInt32("op", (int32)encoding);
			menu->AddItem(new BMenuItem(name, message));
		}
	}

	menu->SetRadioMode(true);

	return menu;
}


void
TermWindow::_SetupMenu()
{
	BLayoutBuilder::Menu<>(fMenuBar = new BMenuBar(Bounds(), "mbar"))
		// Terminal
		.AddMenu(B_TRANSLATE_SYSTEM_NAME("Terminal"))
			.AddItem(B_TRANSLATE("Switch Terminals"), MENU_SWITCH_TERM, B_TAB)
				.GetItem(fSwitchTerminalsMenuItem)
			.AddItem(B_TRANSLATE("New Terminal"), MENU_NEW_TERM, 'N')
			.AddItem(B_TRANSLATE("New tab"), kNewTab, 'T')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS), MENU_PAGE_SETUP)
			.AddItem(B_TRANSLATE("Print"), MENU_PRINT,'P')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Close window"), B_QUIT_REQUESTED, 'W',
				B_SHIFT_KEY)
			.AddItem(B_TRANSLATE("Close active tab"), kCloseView, 'W')
			.AddItem(B_TRANSLATE("Quit"), B_QUIT_REQUESTED, 'Q')
		.End()

		// Edit
		.AddMenu(B_TRANSLATE("Edit"))
			.AddItem(B_TRANSLATE("Copy"), B_COPY,'C')
			.AddItem(B_TRANSLATE("Paste"), B_PASTE,'V')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Select all"), B_SELECT_ALL, 'A')
			.AddItem(B_TRANSLATE("Clear all"), MENU_CLEAR_ALL, 'L')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS), MENU_FIND_STRING,'F')
			.AddItem(B_TRANSLATE("Find previous"), MENU_FIND_PREVIOUS, 'G',
					B_SHIFT_KEY)
				.GetItem(fFindPreviousMenuItem)
				.SetEnabled(false)
			.AddItem(B_TRANSLATE("Find next"), MENU_FIND_NEXT, 'G')
				.GetItem(fFindNextMenuItem)
				.SetEnabled(false)
			.AddSeparator()
			.AddItem(B_TRANSLATE("Window title" B_UTF8_ELLIPSIS),
				kEditWindowTitle)
		.End()

		// Settings
		.AddMenu(B_TRANSLATE("Settings"))
			.AddItem(_MakeWindowSizeMenu())
			.AddItem(fEncodingMenu = _MakeEncodingMenu())
			.AddMenu(B_TRANSLATE("Text size"))
				.AddItem(B_TRANSLATE("Increase"), kIncreaseFontSize, '+',
						B_COMMAND_KEY)
					.GetItem(fIncreaseFontSizeMenuItem)
				.AddItem(B_TRANSLATE("Decrease"), kDecreaseFontSize, '-',
						B_COMMAND_KEY)
					.GetItem(fDecreaseFontSizeMenuItem)
			.End()
			.AddSeparator()
			.AddItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS), MENU_PREF_OPEN)
			.AddSeparator()
			.AddItem(B_TRANSLATE("Save as default"), SAVE_AS_DEFAULT)
		.End()
	;

	AddChild(fMenuBar);

	_UpdateSwitchTerminalsMenuItem();
}


status_t
TermWindow::_GetWindowPositionFile(BFile* file, uint32 openMode)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (status != B_OK)
		return status;

	status = path.Append("Terminal");
	if (status != B_OK)
		return status;

	status = path.Append("Windows");
	if (status != B_OK)
		return status;

	return file->SetTo(path.Path(), openMode);
}


status_t
TermWindow::_LoadWindowPosition(BRect* frame, uint32* workspaces)
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

	int32 id = fTerminalRoster.ID();
	status = position.FindRect("rect", id, frame);
	if (status != B_OK)
		return status;

	int32 _workspaces;
	status = position.FindInt32("workspaces", id, &_workspaces);
	if (status != B_OK)
		return status;
	if (modifiers() & B_SHIFT_KEY)
		*workspaces = _workspaces;
	else
		*workspaces = B_CURRENT_WORKSPACE;

	return B_OK;
}


status_t
TermWindow::_SaveWindowPosition()
{
	BFile file;
	BMessage originalSettings;

	// Read the settings file if it exists and is a valid BMessage.
	status_t status = _GetWindowPositionFile(&file, B_READ_ONLY);
	if (status == B_OK) {
		status = originalSettings.Unflatten(&file);
		file.Unset();

		if (status != B_OK)
			status = originalSettings.MakeEmpty();

		if (status != B_OK)
			return status;
	}

	// Replace the settings
	int32 id = fTerminalRoster.ID();
	BRect rect(Frame());
	if (originalSettings.ReplaceRect("rect", id, rect) != B_OK)
		originalSettings.AddRect("rect", rect);

	int32 workspaces = Workspaces();
	if (originalSettings.ReplaceInt32("workspaces", id, workspaces) != B_OK)
		originalSettings.AddInt32("workspaces", workspaces);

	// Resave the whole thing
	status = _GetWindowPositionFile (&file,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	return originalSettings.Flatten(&file);
}


void
TermWindow::_GetPreferredFont(BFont& font)
{
	// Default to be_fixed_font
	font = be_fixed_font;

	const char* family = PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY);
	const char* style = PrefHandler::Default()->getString(PREF_HALF_FONT_STYLE);

	font.SetFamilyAndStyle(family, style);

	float size = PrefHandler::Default()->getFloat(PREF_HALF_FONT_SIZE);
	if (size < 6.0f)
		size = 6.0f;
	font.SetSize(size);
}


void
TermWindow::MessageReceived(BMessage *message)
{
	int32 encodingId;
	bool findresult;

	switch (message->what) {
		case B_COPY:
			_ActiveTermView()->Copy(be_clipboard);
			break;

		case B_PASTE:
			_ActiveTermView()->Paste(be_clipboard);
			break;

		case B_SELECT_ALL:
			_ActiveTermView()->SelectAll();
			break;

		case MENU_CLEAR_ALL:
			_ActiveTermView()->Clear();
			break;

		case MENU_SWITCH_TERM:
			_SwitchTerminal();
			break;

		case MENU_NEW_TERM:
		{
			// Set our current working directory to that of the active tab, so
			// that the new terminal and its shell inherit it.
			// Note: That's a bit lame. We should rather fork() and change the
			// CWD in the child, but since ATM there aren't any side effects of
			// changing our CWD, we save ourselves the trouble.
			ActiveProcessInfo activeProcessInfo;
			if (_ActiveTermView()->GetActiveProcessInfo(activeProcessInfo))
				chdir(activeProcessInfo.CurrentDirectory());

			app_info info;
			be_app->GetAppInfo(&info);

			// try launching two different ways to work around possible problems
			if (be_roster->Launch(&info.ref) != B_OK)
				be_roster->Launch(TERM_SIGNATURE);
			break;
		}

		case MENU_PREF_OPEN:
			if (!fPrefWindow) {
				fPrefWindow = new PrefWindow(this);
			}
			else
				fPrefWindow->Activate();
			break;

		case MSG_PREF_CLOSED:
			fPrefWindow = NULL;
			break;

		case MSG_WINDOW_TITLE_SETTING_CHANGED:
		case MSG_TAB_TITLE_SETTING_CHANGED:
			_TitleSettingsChanged();
			break;

		case MENU_FIND_STRING:
			if (!fFindPanel) {
				fFindPanel = new FindWindow(this, fFindString, fFindSelection,
					fMatchWord, fMatchCase, fForwardSearch);
			}
			else
				fFindPanel->Activate();
			break;

		case MSG_FIND:
		{
			fFindPanel->PostMessage(B_QUIT_REQUESTED);
			message->FindBool("findselection", &fFindSelection);
			if (!fFindSelection)
				message->FindString("findstring", &fFindString);
			else
				_ActiveTermView()->GetSelection(fFindString);

			if (fFindString.Length() == 0) {
				const char* errorMsg = !fFindSelection
					? B_TRANSLATE("No search string was entered.")
					: B_TRANSLATE("Nothing is selected.");
				BAlert* alert = new BAlert(B_TRANSLATE("Find failed"),
					errorMsg, B_TRANSLATE("OK"), NULL, NULL,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->SetShortcut(0, B_ESCAPE);

				alert->Go();
				fFindPreviousMenuItem->SetEnabled(false);
				fFindNextMenuItem->SetEnabled(false);
				break;
			}

			message->FindBool("forwardsearch", &fForwardSearch);
			message->FindBool("matchcase", &fMatchCase);
			message->FindBool("matchword", &fMatchWord);
			findresult = _ActiveTermView()->Find(fFindString, fForwardSearch, fMatchCase, fMatchWord);

			if (!findresult) {
				BAlert* alert = new BAlert(B_TRANSLATE("Find failed"),
					B_TRANSLATE("Text not found."),
					B_TRANSLATE("OK"), NULL, NULL,
					B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->SetShortcut(0, B_ESCAPE);
				alert->Go();
				fFindPreviousMenuItem->SetEnabled(false);
				fFindNextMenuItem->SetEnabled(false);
				break;
			}

			// Enable the menu items Find Next and Find Previous
			fFindPreviousMenuItem->SetEnabled(true);
			fFindNextMenuItem->SetEnabled(true);
			break;
		}

		case MENU_FIND_NEXT:
		case MENU_FIND_PREVIOUS:
			findresult = _ActiveTermView()->Find(fFindString,
				(message->what == MENU_FIND_NEXT) == fForwardSearch,
				fMatchCase, fMatchWord);
			if (!findresult) {
				BAlert* alert = new BAlert(B_TRANSLATE("Find failed"),
					B_TRANSLATE("Not found."), B_TRANSLATE("OK"),
					NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->SetShortcut(0, B_ESCAPE);
				alert->Go();
			}
			break;

		case MSG_FIND_CLOSED:
			fFindPanel = NULL;
			break;

		case MENU_ENCODING:
			if (message->FindInt32("op", &encodingId) == B_OK)
				_ActiveTermView()->SetEncoding(encodingId);
			break;

		case MSG_COLS_CHANGED:
		{
			int32 columns, rows;
			message->FindInt32("columns", &columns);
			message->FindInt32("rows", &rows);

			_ActiveTermView()->SetTermSize(rows, columns);

			_ResizeView(_ActiveTermView());
			break;
		}
		case MSG_HALF_FONT_CHANGED:
		case MSG_FULL_FONT_CHANGED:
		case MSG_HALF_SIZE_CHANGED:
		case MSG_FULL_SIZE_CHANGED:
		{
			BFont font;
			_GetPreferredFont(font);
			_ActiveTermView()->SetTermFont(&font);

			_ResizeView(_ActiveTermView());
			break;
		}

		case FULLSCREEN:
			if (!fSavedFrame.IsValid()) { // go fullscreen
				_ActiveTermView()->DisableResizeView();
				float mbHeight = fMenuBar->Bounds().Height() + 1;
				fSavedFrame = Frame();
				BScreen screen(this);
				for (int32 i = fTabView->CountTabs() - 1; i >=0 ; i--)
					_TermViewAt(i)->ScrollBar()->ResizeBy(0, (B_H_SCROLL_BAR_HEIGHT - 1));

				fMenuBar->Hide();
				fTabView->ResizeBy(0, mbHeight);
				fTabView->MoveBy(0, -mbHeight);
				fSavedLook = Look();
				// done before ResizeTo to work around a Dano bug (not erasing the decor)
				SetLook(B_NO_BORDER_WINDOW_LOOK);
				ResizeTo(screen.Frame().Width()+1, screen.Frame().Height()+1);
				MoveTo(screen.Frame().left, screen.Frame().top);
				fFullScreen = true;
			} else { // exit fullscreen
				_ActiveTermView()->DisableResizeView();
				float mbHeight = fMenuBar->Bounds().Height() + 1;
				fMenuBar->Show();
				for (int32 i = fTabView->CountTabs() - 1; i >=0 ; i--)
					_TermViewAt(i)->ScrollBar()->ResizeBy(0, -(B_H_SCROLL_BAR_HEIGHT - 1));
				ResizeTo(fSavedFrame.Width(), fSavedFrame.Height());
				MoveTo(fSavedFrame.left, fSavedFrame.top);
				fTabView->ResizeBy(0, -mbHeight);
				fTabView->MoveBy(0, mbHeight);
				SetLook(fSavedLook);
				fSavedFrame = BRect(0,0,-1,-1);
				fFullScreen = false;
			}
			break;

		case MSG_FONT_CHANGED:
			PostMessage(MSG_HALF_FONT_CHANGED);
			break;

		case MSG_COLOR_CHANGED:
		case MSG_COLOR_SCHEMA_CHANGED:
		{
			_SetTermColors(_ActiveTermViewContainerView());
			_ActiveTermViewContainerView()->Invalidate();
			_ActiveTermView()->Invalidate();
			break;
		}

		case SAVE_AS_DEFAULT:
		{
			BPath path;
			if (PrefHandler::GetDefaultPath(path) == B_OK)
				PrefHandler::Default()->SaveAsText(path.Path(), PREFFILE_MIMETYPE);
			break;
		}
		case MENU_PAGE_SETUP:
			_DoPageSetup();
			break;

		case MENU_PRINT:
			_DoPrint();
			break;

		case MSG_CHECK_CHILDREN:
			_CheckChildren();
			break;

		case MSG_MOVE_TAB_LEFT:
		case MSG_MOVE_TAB_RIGHT:
			_NavigateTab(_IndexOfTermView(_ActiveTermView()),
				message->what == MSG_MOVE_TAB_LEFT ? -1 : 1, true);
			break;

		case kTabTitleChanged:
		{
			// tab title changed message from SetTitleDialog
			SessionID sessionID(*message, "session");
			if (Session* session = _SessionForID(sessionID)) {
				BString title;
				if (message->FindString("title", &title) == B_OK) {
					session->title.pattern = title;
					session->title.patternUserDefined = true;
				} else {
					session->title.pattern.Truncate(0);
					session->title.patternUserDefined = false;
				}
				_UpdateSessionTitle(_IndexOfSession(session));
			}
			break;
		}

		case kWindowTitleChanged:
		{
			// window title changed message from SetTitleDialog
			BString title;
			if (message->FindString("title", &title) == B_OK) {
				fTitle.pattern = title;
				fTitle.patternUserDefined = true;
			} else {
				fTitle.pattern
					= PrefHandler::Default()->getString(PREF_WINDOW_TITLE);
				fTitle.patternUserDefined = false;
			}

			_UpdateSessionTitle(fTabView->Selection());
				// updates the window title as a side effect

			break;
		}

		case kSetActiveTab:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK
					&& index >= 0 && index < fSessions.CountItems()) {
				fTabView->Select(index);
			}
			break;
		}

		case kNewTab:
			_NewTab();
			break;

		case kCloseView:
		{
			int32 index = -1;
			SessionID sessionID(*message, "session");
			if (sessionID.IsValid()) {
				if (Session* session = _SessionForID(sessionID))
					index = _IndexOfSession(session);
			} else
				index = _IndexOfTermView(_ActiveTermView());

			if (index >= 0)
				_RemoveTab(index);

			break;
		}

		case kCloseOtherViews:
		{
			Session* session = _SessionForID(SessionID(*message, "session"));
			if (session == NULL)
				break;

			int32 count = fSessions.CountItems();
			for (int32 i = count - 1; i >= 0; i--) {
				if (_SessionAt(i) != session)
					_RemoveTab(i);
			}

			break;
		}

		case kIncreaseFontSize:
		case kDecreaseFontSize:
		{
			TermView* view = _ActiveTermView();
			BFont font;
			view->GetTermFont(&font);

			float size = font.Size();
			if (message->what == kIncreaseFontSize)
				size += 1;
			else
				size -= 1;

			// limit the font size
			if (size < 9)
				size = 9;
			if (size > 18)
				size = 18;

			font.SetSize(size);
			view->SetTermFont(&font);
			PrefHandler::Default()->setInt32(PREF_HALF_FONT_SIZE, (int32)size);

			_ResizeView(view);
			break;
		}

		case kUpdateTitles:
			_UpdateTitles();
			break;

		case kEditTabTitle:
		{
			SessionID sessionID(*message, "session");
			if (Session* session = _SessionForID(sessionID))
				_OpenSetTabTitleDialog(_IndexOfSession(session));
			break;
		}

		case kEditWindowTitle:
			_OpenSetWindowTitleDialog();
			break;

		case kUpdateSwitchTerminalsMenuItem:
			_UpdateSwitchTerminalsMenuItem();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
TermWindow::WindowActivated(bool activated)
{
	if (activated)
		_UpdateSwitchTerminalsMenuItem();
}


void
TermWindow::_SetTermColors(TermViewContainerView* containerView)
{
	PrefHandler* handler = PrefHandler::Default();
	rgb_color background = handler->getRGB(PREF_TEXT_BACK_COLOR);

	containerView->SetViewColor(background);

	TermView *termView = containerView->GetTermView();
	termView->SetTextColor(handler->getRGB(PREF_TEXT_FORE_COLOR), background);

	termView->SetSelectColor(handler->getRGB(PREF_SELECT_FORE_COLOR),
		handler->getRGB(PREF_SELECT_BACK_COLOR));
}


status_t
TermWindow::_DoPageSetup()
{
	BPrintJob job("PageSetup");

	// display the page configure panel
	status_t status = job.ConfigPage();

	// save a pointer to the settings
	fPrintSettings = job.Settings();

	return status;
}


void
TermWindow::_DoPrint()
{
	BPrintJob job("Print");
	if (fPrintSettings)
		job.SetSettings(new BMessage(*fPrintSettings));

	if (job.ConfigJob() != B_OK)
		return;

	BRect pageRect = job.PrintableRect();
	BRect curPageRect = pageRect;

	int pHeight = (int)pageRect.Height();
	int pWidth = (int)pageRect.Width();
	float w, h;
	_ActiveTermView()->GetFrameSize(&w, &h);
	int xPages = (int)ceil(w / pWidth);
	int yPages = (int)ceil(h / pHeight);

	job.BeginJob();

	// loop through and draw each page, and write to spool
	for (int x = 0; x < xPages; x++) {
		for (int y = 0; y < yPages; y++) {
			curPageRect.OffsetTo(x * pWidth, y * pHeight);
			job.DrawView(_ActiveTermView(), curPageRect, B_ORIGIN);
			job.SpoolPage();

			if (!job.CanContinue()) {
				// It is likely that the only way that the job was cancelled is
				// because the user hit 'Cancel' in the page setup window, in
				// which case, the user does *not* need to be told that it was
				// cancelled.
				// He/she will simply expect that it was done.
				return;
			}
		}
	}

	job.CommitJob();
}


void
TermWindow::_NewTab()
{
	if (fTabView->CountTabs() < kMaxTabs) {
		ActiveProcessInfo info;
		if (_ActiveTermView()->GetActiveProcessInfo(info))
			_AddTab(NULL, info.CurrentDirectory());
		else
			_AddTab(NULL);
	}
}


void
TermWindow::_AddTab(Arguments* args, const BString& currentDirectory)
{
	int argc = 0;
	const char* const* argv = NULL;
	if (args != NULL)
		args->GetShellArguments(argc, argv);
	ShellParameters shellParameters(argc, argv, currentDirectory);

	try {
		TermView* view = new TermView(
			PrefHandler::Default()->getInt32(PREF_ROWS),
			PrefHandler::Default()->getInt32(PREF_COLS),
			shellParameters,
			PrefHandler::Default()->getInt32(PREF_HISTORY_SIZE));
		view->SetListener(this);

		TermViewContainerView* containerView = new TermViewContainerView(view);
		BScrollView* scrollView = new TermScrollView("scrollView",
			containerView, view, fSessions.IsEmpty());
		if (!fFullScreen)
			scrollView->ScrollBar(B_VERTICAL)->ResizeBy(0, -(B_H_SCROLL_BAR_HEIGHT - 1));

		if (fSessions.IsEmpty())
			fTabView->SetScrollView(scrollView);

		Session* session = new Session(_NewSessionID(), _NewSessionIndex(),
			containerView);
		fSessions.AddItem(session);

		BFont font;
		_GetPreferredFont(font);
		view->SetTermFont(&font);

		int width, height;
		view->GetFontSize(&width, &height);

		float minimumHeight = -1;
		if (fMenuBar)
			minimumHeight += fMenuBar->Bounds().Height() + 1;
		if (fTabView && fTabView->CountTabs() > 0)
			minimumHeight += fTabView->TabHeight() + 1;
		SetSizeLimits(MIN_COLS * width - 1, MAX_COLS * width - 1,
			minimumHeight + MIN_ROWS * height - 1,
			minimumHeight + MAX_ROWS * height - 1);
			// TODO: The size limit computation is apparently broken, since
			// the terminal can be resized smaller than MIN_ROWS/MIN_COLS!

		// If it's the first time we're called, setup the window
		if (fTabView->CountTabs() == 0) {
			float viewWidth, viewHeight;
			containerView->GetPreferredSize(&viewWidth, &viewHeight);

			// Resize Window
			ResizeTo(viewWidth + B_V_SCROLL_BAR_WIDTH,
				viewHeight + fMenuBar->Bounds().Height() + 1);
				// NOTE: Width is one pixel too small, since the scroll view
				// is one pixel wider than its parent.
		}

		BTab* tab = new BTab;
		fTabView->AddTab(scrollView, tab);
		view->SetScrollBar(scrollView->ScrollBar(B_VERTICAL));
		view->SetMouseClipboard(gMouseClipboard);

		const BCharacterSet* charset
			= BCharacterSetRoster::FindCharacterSetByName(
				PrefHandler::Default()->getString(PREF_TEXT_ENCODING));
		if (charset != NULL)
			view->SetEncoding(charset->GetConversionID());

		_SetTermColors(containerView);

		int32 tabIndex = fTabView->CountTabs() - 1;
		fTabView->Select(tabIndex);

		_UpdateSessionTitle(tabIndex);
	} catch (...) {
		// most probably out of memory. That's bad.
		// TODO: Should cleanup, I guess

		// Quit the application if we don't have a shell already
		if (fTabView->CountTabs() == 0) {
			fprintf(stderr, "Terminal couldn't open a shell\n");
			PostMessage(B_QUIT_REQUESTED);
		}
	}
}


void
TermWindow::_RemoveTab(int32 index)
{
	_FinishTitleDialog();
		// always close to avoid confusion

	if (fSessions.CountItems() > 1) {
		if (!_CanClose(index))
			return;
		if (Session* session = (Session*)fSessions.RemoveItem(index)) {
			if (fSessions.CountItems() == 1) {
				fTabView->SetScrollView(dynamic_cast<BScrollView*>(
					_SessionAt(0)->containerView->Parent()));
			}

			delete session;
			delete fTabView->RemoveTab(index);
		}
	} else
		PostMessage(B_QUIT_REQUESTED);
}


void
TermWindow::_NavigateTab(int32 index, int32 direction, bool move)
{
	int32 count = fSessions.CountItems();
	if (count <= 1 || index < 0 || index >= count)
		return;

	int32 newIndex = (index + direction + count) % count;
	if (newIndex == index)
		return;

	if (move) {
		// move the given tab to the new index
		Session* session = (Session*)fSessions.RemoveItem(index);
		fSessions.AddItem(session, newIndex);
		fTabView->MoveTab(index, newIndex);
	}

	// activate the respective tab
	fTabView->Select(newIndex);
}


TermViewContainerView*
TermWindow::_ActiveTermViewContainerView() const
{
	return _TermViewContainerViewAt(fTabView->Selection());
}


TermViewContainerView*
TermWindow::_TermViewContainerViewAt(int32 index) const
{
	if (Session* session = _SessionAt(index))
		return session->containerView;
	return NULL;
}


TermView*
TermWindow::_ActiveTermView() const
{
	return _ActiveTermViewContainerView()->GetTermView();
}


TermView*
TermWindow::_TermViewAt(int32 index) const
{
	TermViewContainerView* view = _TermViewContainerViewAt(index);
	return view != NULL ? view->GetTermView() : NULL;
}


int32
TermWindow::_IndexOfTermView(TermView* termView) const
{
	if (!termView)
		return -1;

	// find the view
	int32 count = fTabView->CountTabs();
	for (int32 i = count - 1; i >= 0; i--) {
		if (termView == _TermViewAt(i))
			return i;
	}

	return -1;
}


TermWindow::Session*
TermWindow::_SessionAt(int32 index) const
{
	return (Session*)fSessions.ItemAt(index);
}


TermWindow::Session*
TermWindow::_SessionForID(const SessionID& sessionID) const
{
	for (int32 i = 0; Session* session = _SessionAt(i); i++) {
		if (session->id == sessionID)
			return session;
	}

	return NULL;
}


int32
TermWindow::_IndexOfSession(Session* session) const
{
	return fSessions.IndexOf(session);
}


void
TermWindow::_CheckChildren()
{
	int32 count = fSessions.CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		Session* session = _SessionAt(i);
		if (session->containerView->GetTermView()->CheckShellGone())
			NotifyTermViewQuit(session->containerView->GetTermView(), 0);
	}
}


void
TermWindow::Zoom(BPoint leftTop, float width, float height)
{
	_ActiveTermView()->DisableResizeView();
	BWindow::Zoom(leftTop, width, height);
}


void
TermWindow::FrameResized(float newWidth, float newHeight)
{
	BWindow::FrameResized(newWidth, newHeight);

	TermView* view = _ActiveTermView();
	PrefHandler::Default()->setInt32(PREF_COLS, view->Columns());
	PrefHandler::Default()->setInt32(PREF_ROWS, view->Rows());
}


void
TermWindow::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
	fTerminalRoster.SetWindowInfo(IsMinimized(), Workspaces());
}


void
TermWindow::WorkspaceActivated(int32 workspace, bool state)
{
	fTerminalRoster.SetWindowInfo(IsMinimized(), Workspaces());
}


void
TermWindow::Minimize(bool minimize)
{
	BWindow::Minimize(minimize);
	fTerminalRoster.SetWindowInfo(IsMinimized(), Workspaces());
}


void
TermWindow::TabSelected(SmartTabView* tabView, int32 index)
{
	SessionChanged();
}


void
TermWindow::TabDoubleClicked(SmartTabView* tabView, BPoint point, int32 index)
{
	if (index >= 0) {
		// clicked on a tab -- open the title dialog
		_OpenSetTabTitleDialog(index);
	} else {
		// not clicked on a tab -- create a new one
		_NewTab();
	}
}


void
TermWindow::TabMiddleClicked(SmartTabView* tabView, BPoint point, int32 index)
{
	if (index >= 0)
		_RemoveTab(index);
}


void
TermWindow::TabRightClicked(SmartTabView* tabView, BPoint point, int32 index)
{
	if (index < 0)
		return;

	TermView* termView = _TermViewAt(index);
	if (termView == NULL)
		return;

	BMessage* closeMessage = new BMessage(kCloseView);
	_SessionAt(index)->id.AddToMessage(*closeMessage, "session");

	BMessage* closeOthersMessage = new BMessage(kCloseOtherViews);
	_SessionAt(index)->id.AddToMessage(*closeOthersMessage, "session");

	BMessage* editTitleMessage = new BMessage(kEditTabTitle);
	_SessionAt(index)->id.AddToMessage(*editTitleMessage, "session");

	BPopUpMenu* popUpMenu = new BPopUpMenu("tab menu");
	BLayoutBuilder::Menu<>(popUpMenu)
		.AddItem(B_TRANSLATE("Close tab"), closeMessage)
		.AddItem(B_TRANSLATE("Close other tabs"), closeOthersMessage)
		.AddSeparator()
		.AddItem(B_TRANSLATE("Edit tab title" B_UTF8_ELLIPSIS),
			editTitleMessage)
	;

	popUpMenu->SetAsyncAutoDestruct(true);
	popUpMenu->SetTargetForItems(BMessenger(this));

	BPoint screenWhere = tabView->ConvertToScreen(point);
	BRect mouseRect(screenWhere, screenWhere);
	mouseRect.InsetBy(-4.0, -4.0);
	popUpMenu->Go(screenWhere, true, true, mouseRect, true);
}


void
TermWindow::NotifyTermViewQuit(TermView* view, int32 reason)
{
	// Since the notification can come from the view, we send a message to
	// ourselves to avoid deleting the caller synchronously.
	if (Session* session = _SessionAt(_IndexOfTermView(view))) {
		BMessage message(kCloseView);
		session->id.AddToMessage(message, "session");
		message.AddInt32("reason", reason);
		PostMessage(&message);
	}
}


void
TermWindow::SetTermViewTitle(TermView* view, const char* title)
{
	int32 index = _IndexOfTermView(view);
	if (Session* session = _SessionAt(index)) {
		session->title.pattern = title;
		session->title.patternUserDefined = true;
		_UpdateSessionTitle(index);
	}
}


void
TermWindow::TitleChanged(SetTitleDialog* dialog, const BString& title,
	bool titleUserDefined)
{
	if (dialog == fSetTabTitleDialog) {
		// tab title
		BMessage message(kTabTitleChanged);
		fSetTabTitleSession.AddToMessage(message, "session");
		if (titleUserDefined)
			message.AddString("title", title);

		PostMessage(&message);
	} else if (dialog == fSetWindowTitleDialog) {
		// window title
		BMessage message(kWindowTitleChanged);
		if (titleUserDefined)
			message.AddString("title", title);

		PostMessage(&message);
	}
}


void
TermWindow::SetTitleDialogDone(SetTitleDialog* dialog)
{
	if (dialog == fSetTabTitleDialog) {
		fSetTabTitleSession = SessionID();
		fSetTabTitleDialog = NULL;
			// assuming this is atomic
	}
}


void
TermWindow::TerminalInfosUpdated(TerminalRoster* roster)
{
	PostMessage(kUpdateSwitchTerminalsMenuItem);
}


void
TermWindow::PreviousTermView(TermView* view)
{
	_NavigateTab(_IndexOfTermView(view), -1, false);
}


void
TermWindow::NextTermView(TermView* view)
{
	_NavigateTab(_IndexOfTermView(view), 1, false);
}


void
TermWindow::_ResizeView(TermView *view)
{
	int fontWidth, fontHeight;
	view->GetFontSize(&fontWidth, &fontHeight);

	float minimumHeight = -1;
	if (fMenuBar)
		minimumHeight += fMenuBar->Bounds().Height() + 1;
	if (fTabView && fTabView->CountTabs() > 1)
		minimumHeight += fTabView->TabHeight() + 1;

	SetSizeLimits(MIN_COLS * fontWidth - 1, MAX_COLS * fontWidth - 1,
		minimumHeight + MIN_ROWS * fontHeight - 1,
		minimumHeight + MAX_ROWS * fontHeight - 1);

	float width;
	float height;
	view->Parent()->GetPreferredSize(&width, &height);
	width += B_V_SCROLL_BAR_WIDTH;
		// NOTE: Width is one pixel too small, since the scroll view
		// is one pixel wider than its parent.
	height += fMenuBar->Bounds().Height() + 1;

	ResizeTo(width, height);

	view->Invalidate();
}


/* static */
BMenu*
TermWindow::_MakeWindowSizeMenu()
{
	BMenu* menu = new (std::nothrow) BMenu(B_TRANSLATE("Window size"));
	if (menu == NULL)
		return NULL;

	const int32 windowSizes[4][2] = {
		{ 80, 25 },
		{ 80, 40 },
		{ 132, 25 },
		{ 132, 40 }
	};

	const int32 sizeNum = sizeof(windowSizes) / sizeof(windowSizes[0]);
	for (int32 i = 0; i < sizeNum; i++) {
		char label[32];
		int32 columns = windowSizes[i][0];
		int32 rows = windowSizes[i][1];
		snprintf(label, sizeof(label), "%ldx%ld", columns, rows);
		BMessage* message = new BMessage(MSG_COLS_CHANGED);
		message->AddInt32("columns", columns);
		message->AddInt32("rows", rows);
		menu->AddItem(new BMenuItem(label, message));
	}

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Full screen"),
		new BMessage(FULLSCREEN), B_ENTER));

	return menu;
}


void
TermWindow::_UpdateSwitchTerminalsMenuItem()
{
	fSwitchTerminalsMenuItem->SetEnabled(_FindSwitchTerminalTarget() >= 0);
}


void
TermWindow::_TitleSettingsChanged()
{
	if (!fTitle.patternUserDefined)
		fTitle.pattern = PrefHandler::Default()->getString(PREF_WINDOW_TITLE);

	fSessionTitlePattern = PrefHandler::Default()->getString(PREF_TAB_TITLE);

	_UpdateTitles();
}


void
TermWindow::_UpdateTitles()
{
	int32 sessionCount = fSessions.CountItems();
	for (int32 i = 0; i < sessionCount; i++)
		_UpdateSessionTitle(i);
}


void
TermWindow::_UpdateSessionTitle(int32 index)
{
	Session* session = _SessionAt(index);
	if (session == NULL)
		return;

	// get the shell and active process infos
	ShellInfo shellInfo;
	ActiveProcessInfo activeProcessInfo;
	TermView* termView = _TermViewAt(index);
	if (!termView->GetShellInfo(shellInfo)
		|| !termView->GetActiveProcessInfo(activeProcessInfo)) {
		return;
	}

	// evaluate the session title pattern
	BString sessionTitlePattern = session->title.patternUserDefined
		? session->title.pattern : fSessionTitlePattern;
	TabTitlePlaceholderMapper tabMapper(shellInfo, activeProcessInfo,
		session->index);
	const BString& sessionTitle = PatternEvaluator::Evaluate(
		sessionTitlePattern, tabMapper);

	// set the tab title
	if (sessionTitle != session->title.title) {
		session->title.title = sessionTitle;
		fTabView->TabAt(index)->SetLabel(session->title.title);
		fTabView->Invalidate();
			// Invalidate the complete tab view, since other tabs might change
			// their positions.
	}

	// If this is the active tab, also recompute the window title.
	if (index != fTabView->Selection())
		return;

	// evaluate the window title pattern
	WindowTitlePlaceholderMapper windowMapper(shellInfo, activeProcessInfo,
		fTerminalRoster.ID() + 1, sessionTitle);
	const BString& windowTitle = PatternEvaluator::Evaluate(fTitle.pattern,
		windowMapper);

	// set the window title
	if (windowTitle != fTitle.title) {
		fTitle.title = windowTitle;
		SetTitle(fTitle.title);
	}
}


void
TermWindow::_OpenSetTabTitleDialog(int32 index)
{
	// If a dialog is active, finish it.
	_FinishTitleDialog();

	BString toolTip = BString(B_TRANSLATE(
		"The pattern specifying the current tab title. The following "
			"placeholders\n"
		"can be used:\n")) << kTooTipSetTabTitlePlaceholders;
	fSetTabTitleDialog = new SetTitleDialog(
		B_TRANSLATE("Set tab title"), B_TRANSLATE("Tab title:"),
		toolTip);

	Session* session = _SessionAt(index);
	bool userDefined = session->title.patternUserDefined;
	const BString& title = userDefined
		? session->title.pattern : fSessionTitlePattern;
	fSetTabTitleSession = session->id;

	// place the dialog window directly under the tab, but keep it on screen
	BPoint location = fTabView->ConvertToScreen(
		fTabView->TabFrame(index).LeftBottom() + BPoint(0, 1));
	BRect frame(fSetTabTitleDialog->Frame().OffsetToCopy(location));
	BSize screenSize(BScreen(fSetTabTitleDialog).Frame().Size());
	fSetTabTitleDialog->MoveTo(
		BLayoutUtils::MoveIntoFrame(frame, screenSize).LeftTop());

	fSetTabTitleDialog->Go(title, userDefined, this);
}


void
TermWindow::_OpenSetWindowTitleDialog()
{
	// If a dialog is active, finish it.
	_FinishTitleDialog();

	BString toolTip = BString(B_TRANSLATE(
		"The pattern specifying the window title. The following placeholders\n"
		"can be used:\n")) << kTooTipSetTabTitlePlaceholders;
	fSetWindowTitleDialog = new SetTitleDialog(B_TRANSLATE("Set window title"),
		B_TRANSLATE("Window title:"), toolTip);

	// center the dialog in the window frame, but keep it on screen
	fSetWindowTitleDialog->CenterIn(Frame());
	BRect frame(fSetWindowTitleDialog->Frame());
	BSize screenSize(BScreen(fSetWindowTitleDialog).Frame().Size());
	fSetWindowTitleDialog->MoveTo(
		BLayoutUtils::MoveIntoFrame(frame, screenSize).LeftTop());

	fSetWindowTitleDialog->Go(fTitle.pattern, fTitle.patternUserDefined, this);
}


void
TermWindow::_FinishTitleDialog()
{
	SetTitleDialog* oldDialog = fSetTabTitleDialog;
	if (oldDialog != NULL && oldDialog->Lock()) {
		// might have been unset in the meantime, so recheck
		if (fSetTabTitleDialog == oldDialog) {
			oldDialog->Finish();
				// this also unsets the variables
		}
		oldDialog->Unlock();
		return;
	}

	oldDialog = fSetWindowTitleDialog;
	if (oldDialog != NULL && oldDialog->Lock()) {
		// might have been unset in the meantime, so recheck
		if (fSetWindowTitleDialog == oldDialog) {
			oldDialog->Finish();
				// this also unsets the variable
		}
		oldDialog->Unlock();
		return;
	}
}


void
TermWindow::_SwitchTerminal()
{
	team_id teamID = _FindSwitchTerminalTarget();
	if (teamID < 0)
		return;

	BMessenger app(TERM_SIGNATURE, teamID);
	app.SendMessage(MSG_ACTIVATE_TERM);
}


team_id
TermWindow::_FindSwitchTerminalTarget()
{
	AutoLocker<TerminalRoster> rosterLocker(fTerminalRoster);

	team_id myTeamID = Team();

	int32 numTerms = fTerminalRoster.CountTerminals();
	if (numTerms <= 1)
		return -1;

	// Find our position in the Terminal teams.
	int32 i;

	for (i = 0; i < numTerms; i++) {
		if (myTeamID == fTerminalRoster.TerminalAt(i)->team)
			break;
	}

	if (i == numTerms) {
		// we didn't find ourselves -- that shouldn't happen
		return -1;
	}

	uint32 currentWorkspace = 1L << current_workspace();

	while (true) {
		if (--i < 0)
			i = numTerms - 1;

		const TerminalRoster::Info* info = fTerminalRoster.TerminalAt(i);
		if (info->team == myTeamID) {
			// That's ourselves again. We've run through the complete list.
			return -1;
		}

		if (!info->minimized && (info->workspaces & currentWorkspace) != 0)
			return info->team;
	}
}


TermWindow::SessionID
TermWindow::_NewSessionID()
{
	return fNextSessionID++;
}


int32
TermWindow::_NewSessionIndex()
{
	for (int32 id = 1; ; id++) {
		bool used = false;

		for (int32 i = 0;
			 Session* session = _SessionAt(i); i++) {
			if (id == session->index) {
				used = true;
				break;
			}
		}

		if (!used)
			return id;
	}
}

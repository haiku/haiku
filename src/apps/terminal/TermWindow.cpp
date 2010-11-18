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
#include <Clipboard.h>
#include <Dragger.h>
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

#include "ActiveProcessInfo.h"
#include "Arguments.h"
#include "AppearPrefView.h"
#include "Encoding.h"
#include "FindWindow.h"
#include "Globals.h"
#include "PrefWindow.h"
#include "PrefHandler.h"
#include "ShellParameters.h"
#include "TermConst.h"
#include "TermScrollView.h"
#include "TitlePlaceholderMapper.h"


const static int32 kMaxTabs = 6;
const static int32 kTermViewOffset = 3;

// messages constants
const static uint32 kNewTab = 'NTab';
const static uint32 kCloseView = 'ClVw';
const static uint32 kIncreaseFontSize = 'InFs';
const static uint32 kDecreaseFontSize = 'DcFs';
const static uint32 kSetActiveTab = 'STab';
const static uint32 kUpdateTitles = 'UPti';


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
	:
	fID(-1)
{
	message.FindInt32(field, &fID);
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


TermWindow::TermWindow(BRect frame, const BString& title,
	bool isUserDefinedTitle, int32 windowIndex, uint32 workspaces,
	Arguments* args)
	:
	BWindow(frame, title, B_DOCUMENT_WINDOW,
		B_CURRENT_WORKSPACE | B_QUIT_ON_WINDOW_CLOSE, workspaces),
	fWindowIndex(windowIndex),
	fTitleUpdateRunner(this, BMessage(kUpdateTitles), 1000000),
	fNextSessionID(0),
	fTabView(NULL),
	fMenubar(NULL),
	fFilemenu(NULL),
	fEditmenu(NULL),
	fEncodingmenu(NULL),
	fHelpmenu(NULL),
	fWindowSizeMenu(NULL),
	fPrintSettings(NULL),
	fPrefWindow(NULL),
	fFindPanel(NULL),
	fSavedFrame(0, 0, -1, -1),
	fFindString(""),
	fFindNextMenuItem(NULL),
	fFindPreviousMenuItem(NULL),
	fFindSelection(false),
	fForwardSearch(false),
	fMatchCase(false),
	fMatchWord(false),
	fFullScreen(false)
{
	// apply the title settings
	fTitle.title = title;
	fTitle.pattern = title;
	fTitle.patternUserDefined = isUserDefinedTitle;
	_TitleSettingsChanged();

	_InitWindow();
	_AddTab(args);
}


TermWindow::~TermWindow()
{
	if (fPrefWindow)
		fPrefWindow->PostMessage(B_QUIT_REQUESTED);

	if (fFindPanel && fFindPanel->Lock()) {
		fFindPanel->Quit();
		fFindPanel = NULL;
	}

	PrefHandler::DeleteDefault();

	for (int32 i = 0; Session* session = (Session*)fSessions.ItemAt(i); i++)
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

	BRect textFrame = Bounds();
	textFrame.top = fMenubar->Bounds().bottom + 1.0;

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

	bool isBusy = false;
	if (index != -1)
		isBusy = _TermViewAt(index)->IsShellBusy();
	else {
		for (int32 i = 0; i < fSessions.CountItems(); i++) {
			if (_TermViewAt(i)->IsShellBusy()) {
				isBusy = true;
				break;
			}
		}
	}

	if (isBusy) {
		const char* alertMessage = index == -1 || fSessions.CountItems() == 1
			? B_TRANSLATE("A process is still running.\n"
				"If you close the Terminal, the process will be killed.")
			: B_TRANSLATE("A process is still running.\n"
				"If you close the tab, the process will be killed.");
		BAlert* alert = new BAlert(B_TRANSLATE("Really close?"),
			alertMessage, B_TRANSLATE("Close"), B_TRANSLATE("Cancel"), NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		int32 result = alert->Go();
		if (result == 1)
			return false;
	}

	return true;
}


bool
TermWindow::QuitRequested()
{
	if (!_CanClose(-1))
		return false;

	BMessage position = BMessage(MSG_SAVE_WINDOW_POSITION);
	position.AddRect("rect", Frame());
	position.AddInt32("workspaces", Workspaces());
	be_app->PostMessage(&position);

	return BWindow::QuitRequested();
}


void
TermWindow::MenusBeginning()
{
	TermView* view = _ActiveTermView();

	// Syncronize Encode Menu Pop-up menu and Preference.
	BMenuItem* item = fEncodingmenu->FindItem(
		EncodingAsString(view->Encoding()));
	if (item != NULL)
		item->SetMarked(true);

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

	int encoding;
	int i = 0;
	while (get_next_encoding(i, &encoding) == B_OK) {
		BMessage *message = new BMessage(MENU_ENCODING);
		if (message != NULL) {
			message->AddInt32("op", (int32)encoding);
			menu->AddItem(new BMenuItem(EncodingAsString(encoding),
				message));
		}
		i++;
	}

	menu->SetRadioMode(true);

	return menu;
}


void
TermWindow::_SetupMenu()
{
	// Menu bar object.
	fMenubar = new BMenuBar(Bounds(), "mbar");

	// Make File Menu.
	fFilemenu = new BMenu(B_TRANSLATE("Terminal"));
	fFilemenu->AddItem(new BMenuItem(B_TRANSLATE("Switch Terminals"),
		new BMessage(MENU_SWITCH_TERM), B_TAB));
	fFilemenu->AddItem(new BMenuItem(B_TRANSLATE("New Terminal"),
		new BMessage(MENU_NEW_TERM), 'N'));
	fFilemenu->AddItem(new BMenuItem(B_TRANSLATE("New tab"),
		new BMessage(kNewTab), 'T'));

	fFilemenu->AddSeparatorItem();
	fFilemenu->AddItem(new BMenuItem(B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS),
		new BMessage(MENU_PAGE_SETUP)));
	fFilemenu->AddItem(new BMenuItem(B_TRANSLATE("Print"),
		new BMessage(MENU_PRINT),'P'));
	fFilemenu->AddSeparatorItem();
	fFilemenu->AddItem(new BMenuItem(
		B_TRANSLATE("About Terminal" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED)));
	fFilemenu->AddSeparatorItem();
	fFilemenu->AddItem(new BMenuItem(B_TRANSLATE("Close window"),
		new BMessage(B_QUIT_REQUESTED), 'W', B_SHIFT_KEY));
	fFilemenu->AddItem(new BMenuItem(B_TRANSLATE("Close active tab"),
		new BMessage(kCloseView), 'W'));
	fFilemenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	fMenubar->AddItem(fFilemenu);

	// Make Edit Menu.
	fEditmenu = new BMenu(B_TRANSLATE("Edit"));
	fEditmenu->AddItem(new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY),'C'));
	fEditmenu->AddItem(new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE),'V'));
	fEditmenu->AddSeparatorItem();
	fEditmenu->AddItem(new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));
	fEditmenu->AddItem(new BMenuItem(B_TRANSLATE("Clear all"),
		new BMessage(MENU_CLEAR_ALL), 'L'));
	fEditmenu->AddSeparatorItem();
	fEditmenu->AddItem(new BMenuItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS),
		new BMessage(MENU_FIND_STRING),'F'));
	fFindPreviousMenuItem = new BMenuItem(B_TRANSLATE("Find previous"),
		new BMessage(MENU_FIND_PREVIOUS), 'G', B_SHIFT_KEY);
	fEditmenu->AddItem(fFindPreviousMenuItem);
	fFindPreviousMenuItem->SetEnabled(false);
	fFindNextMenuItem = new BMenuItem(B_TRANSLATE("Find next"),
		new BMessage(MENU_FIND_NEXT), 'G');
	fEditmenu->AddItem(fFindNextMenuItem);
	fFindNextMenuItem->SetEnabled(false);

	fMenubar->AddItem(fEditmenu);

	// Make Help Menu.
	fHelpmenu = new BMenu(B_TRANSLATE("Settings"));
	fWindowSizeMenu = _MakeWindowSizeMenu();

	fEncodingmenu = _MakeEncodingMenu();

	fSizeMenu = new BMenu(B_TRANSLATE("Text size"));

	fIncreaseFontSizeMenuItem = new BMenuItem(B_TRANSLATE("Increase"),
		new BMessage(kIncreaseFontSize), '+', B_COMMAND_KEY);

	fDecreaseFontSizeMenuItem = new BMenuItem(B_TRANSLATE("Decrease"),
		new BMessage(kDecreaseFontSize), '-', B_COMMAND_KEY);

	fSizeMenu->AddItem(fIncreaseFontSizeMenuItem);
	fSizeMenu->AddItem(fDecreaseFontSizeMenuItem);

	fHelpmenu->AddItem(fWindowSizeMenu);
	fHelpmenu->AddItem(fEncodingmenu);
	fHelpmenu->AddItem(fSizeMenu);
	fHelpmenu->AddSeparatorItem();
	fHelpmenu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(MENU_PREF_OPEN)));
	fHelpmenu->AddSeparatorItem();
	fHelpmenu->AddItem(new BMenuItem(B_TRANSLATE("Save as default"),
		new BMessage(SAVE_AS_DEFAULT)));
	fMenubar->AddItem(fHelpmenu);

	AddChild(fMenubar);
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

		case B_ABOUT_REQUESTED:
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;

		case MENU_CLEAR_ALL:
			_ActiveTermView()->Clear();
			break;

		case MENU_SWITCH_TERM:
			be_app->PostMessage(MENU_SWITCH_TERM);
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
				float mbHeight = fMenubar->Bounds().Height() + 1;
				fSavedFrame = Frame();
				BScreen screen(this);
				_ActiveTermView()->ScrollBar()->ResizeBy(0, (B_H_SCROLL_BAR_HEIGHT - 2));

				fMenubar->Hide();
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
				float mbHeight = fMenubar->Bounds().Height() + 1;
				fMenubar->Show();
				_ActiveTermView()->ScrollBar()->ResizeBy(0, -(B_H_SCROLL_BAR_HEIGHT - 2));
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
			TermView* termView;
			int32 index = -1;
			if (message->FindPointer("termView", (void**)&termView) == B_OK)
				index = _IndexOfTermView(termView);
			else
				index = _IndexOfTermView(_ActiveTermView());

			if (index >= 0)
				_RemoveTab(index);

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

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
TermWindow::WindowActivated(bool activated)
{
	BWindow::WindowActivated(activated);
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
	if (!fPrintSettings || (_DoPageSetup() != B_OK)) {
		(new BAlert(B_TRANSLATE("Cancel"), B_TRANSLATE("Print cancelled."),
			B_TRANSLATE("OK")))->Go();
		return;
	}

	BPrintJob job("Print");
	job.SetSettings(new BMessage(*fPrintSettings));

	BRect pageRect = job.PrintableRect();
	BRect curPageRect = pageRect;

	int pHeight = (int)pageRect.Height();
	int pWidth = (int)pageRect.Width();
	float w,h;
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
		if (fFullScreen)
			_ActiveTermView()->ScrollBar()->Show();

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
		if (fMenubar)
			minimumHeight += fMenubar->Bounds().Height() + 1;
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
				viewHeight + fMenubar->Bounds().Height() + 1);
				// NOTE: Width is one pixel too small, since the scroll view
				// is one pixel wider than its parent.
		}

		BTab* tab = new BTab;
		fTabView->AddTab(scrollView, tab);
		view->SetScrollBar(scrollView->ScrollBar(B_VERTICAL));
		view->SetMouseClipboard(gMouseClipboard);
		view->SetEncoding(EncodingID(
			PrefHandler::Default()->getString(PREF_TEXT_ENCODING)));

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
	if (fSessions.CountItems() > 1) {
		if (!_CanClose(index))
			return;
		if (Session* session = (Session*)fSessions.RemoveItem(index)) {
			if (fSessions.CountItems() == 1) {
				fTabView->SetScrollView(dynamic_cast<BScrollView*>(
					((Session*)fSessions.ItemAt(0))->containerView->Parent()));
			}

			delete session;
			delete fTabView->RemoveTab(index);
			if (fFullScreen)
				_ActiveTermView()->ScrollBar()->Hide();
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

	if (move) {
		// TODO: Move the tab!
	} else {
		index += direction;
		fTabView->Select((index + count) % count);
	}
}


TermViewContainerView*
TermWindow::_ActiveTermViewContainerView() const
{
	return _TermViewContainerViewAt(fTabView->Selection());
}


TermViewContainerView*
TermWindow::_TermViewContainerViewAt(int32 index) const
{
	if (Session* session = (Session*)fSessions.ItemAt(index))
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


void
TermWindow::_CheckChildren()
{
	int32 count = fSessions.CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		Session* session = (Session*)fSessions.ItemAt(i);
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
TermWindow::TabSelected(SmartTabView* tabView, int32 index)
{
	SessionChanged();
}


void
TermWindow::TabDoubleClicked(SmartTabView* tabView, BPoint point, int32 index)
{
	if (index >= 0) {
		// TODO: Open the change title dialog!
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

	BMessage* message = new BMessage(kCloseView);
	message->AddPointer("termview", termView);

	BPopUpMenu* popUpMenu = new BPopUpMenu("tab menu");
	popUpMenu->AddItem(new BMenuItem(B_TRANSLATE("Close tab"), message));
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
	BMessage message(kCloseView);
	message.AddPointer("termView", view);
	message.AddInt32("reason", reason);
	PostMessage(&message);
}


void
TermWindow::SetTermViewTitle(TermView* view, const char* title)
{
	int32 index = _IndexOfTermView(view);
	if (Session* session = (Session*)fSessions.ItemAt(index)) {
		session->title.pattern = title;
		session->title.patternUserDefined = true;
		_UpdateSessionTitle(index);
	}
}


void
TermWindow::PreviousTermView(TermView* view, bool move)
{
	_NavigateTab(_IndexOfTermView(view), -1, move);
}


void
TermWindow::NextTermView(TermView* view, bool move)
{
	_NavigateTab(_IndexOfTermView(view), 1, move);
}


void
TermWindow::_ResizeView(TermView *view)
{
	int fontWidth, fontHeight;
	view->GetFontSize(&fontWidth, &fontHeight);

	float minimumHeight = -1;
	if (fMenubar)
		minimumHeight += fMenubar->Bounds().Height() + 1;
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
	height += fMenubar->Bounds().Height() + 1;

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
	Session* session = (Session*)fSessions.ItemAt(index);
	if (session == NULL)
		return;

	// get the active process info
	ActiveProcessInfo activeProcessInfo;
	if (!_TermViewAt(index)->GetActiveProcessInfo(activeProcessInfo))
		return;

	// evaluate the session title pattern
	BString sessionTitlePattern = session->title.patternUserDefined
		? session->title.pattern : fSessionTitlePattern;
	TabTitlePlaceholderMapper tabMapper(activeProcessInfo, session->index);
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
	WindowTitlePlaceholderMapper windowMapper(activeProcessInfo, fWindowIndex,
		sessionTitle);
	const BString& windowTitle = PatternEvaluator::Evaluate(fTitle.pattern,
		windowMapper);

	// set the window title
	if (windowTitle != fTitle.title) {
		fTitle.title = windowTitle;
		SetTitle(fTitle.title);
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
			 Session* session = (Session*)fSessions.ItemAt(i); i++) {
			if (id == session->index) {
				used = true;
				break;
			}
		}

		if (!used)
			return id;
	}
}

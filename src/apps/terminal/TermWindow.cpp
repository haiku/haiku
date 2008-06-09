/*
 * Copyright 2007 Haiku, Inc.
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed under the terms of the MIT license.
 */

#include "TermWindow.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <Alert.h>
#include <Application.h>
#include <Clipboard.h>
#include <Dragger.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PrintJob.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>

#include "Arguments.h"
#include "Coding.h"
#include "MenuUtil.h"
#include "FindWindow.h"
#include "PrefWindow.h"
#include "PrefView.h"
#include "PrefHandler.h"
#include "SmartTabView.h"
#include "TermConst.h"
#include "TermScrollView.h"
#include "TermView.h"


const static int32 kMaxTabs = 6;
const static int32 kTermViewOffset = 3;

// messages constants
const static uint32 kNewTab = 'NTab';
const static uint32 kCloseView = 'ClVw';
const static uint32 kIncreaseFontSize = 'InFs';
const static uint32 kDecreaseFontSize = 'DcFs';


class CustomTermView : public TermView {
public:
	CustomTermView(int32 rows, int32 columns, int32 argc, const char **argv, int32 historySize = 1000);
	virtual void NotifyQuit(int32 reason);
	virtual void SetTitle(const char *title);
};


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


TermWindow::TermWindow(BRect frame, const char* title, Arguments *args)
	: BWindow(frame, title, B_DOCUMENT_WINDOW, B_CURRENT_WORKSPACE|B_QUIT_ON_WINDOW_CLOSE),
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
	fFindForwardMenuItem(NULL),
	fFindBackwardMenuItem(NULL),
	fFindSelection(false),
	fForwardSearch(false),
	fMatchCase(false),
	fMatchWord(false)
{
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
}


void
TermWindow::_InitWindow()
{
	// make menu bar
	_SetupMenu();

	AddShortcut('+', B_COMMAND_KEY, new BMessage(kIncreaseFontSize));
	AddShortcut('-', B_COMMAND_KEY, new BMessage(kDecreaseFontSize));
	
	BRect textFrame = Bounds();
	textFrame.top = fMenubar->Bounds().bottom + 1.0;

	fTabView = new SmartTabView(textFrame, "tab view");
	AddChild(fTabView);
}


void
TermWindow::MenusBeginning()
{
	// Syncronize Encode Menu Pop-up menu and Preference.
	BMenuItem *item = fEncodingmenu->FindItem(EncodingAsString(_ActiveTermView()->Encoding()));
	if (item != NULL)
		item->SetMarked(true);
	BWindow::MenusBeginning();
}


void
TermWindow::_SetupMenu()
{
	PrefHandler menuText;
	
	LoadLocaleFile(&menuText);
	
	// Menu bar object.
	fMenubar = new BMenuBar(Bounds(), "mbar");
	
	// Make File Menu.
	fFilemenu = new BMenu("Terminal");
	fFilemenu->AddItem(new BMenuItem("Switch Terminals", new BMessage(MENU_SWITCH_TERM),'G'));
	fFilemenu->AddItem(new BMenuItem("New Terminal" B_UTF8_ELLIPSIS, new BMessage(MENU_NEW_TERM), 'N'));
	fFilemenu->AddItem(new BMenuItem("New Tab", new BMessage(kNewTab), 'T'));
	
	fFilemenu->AddSeparatorItem();
	fFilemenu->AddItem(new BMenuItem("Page Setup" B_UTF8_ELLIPSIS, new BMessage(MENU_PAGE_SETUP)));
	fFilemenu->AddItem(new BMenuItem("Print", new BMessage(MENU_PRINT),'P'));
	fFilemenu->AddSeparatorItem();
	fFilemenu->AddItem(new BMenuItem("About Terminal" B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED)));
	fFilemenu->AddSeparatorItem();
	fFilemenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	fMenubar->AddItem(fFilemenu);

	// Make Edit Menu.
	fEditmenu = new BMenu ("Edit");
	fEditmenu->AddItem (new BMenuItem ("Copy", new BMessage (B_COPY),'C'));
	fEditmenu->AddItem (new BMenuItem ("Paste", new BMessage (B_PASTE),'V'));
	fEditmenu->AddSeparatorItem();
	fEditmenu->AddItem (new BMenuItem ("Select All", new BMessage (B_SELECT_ALL), 'A'));
	fEditmenu->AddItem (new BMenuItem ("Clear All", new BMessage (MENU_CLEAR_ALL), 'L'));
	fEditmenu->AddSeparatorItem();
	fEditmenu->AddItem (new BMenuItem ("Find" B_UTF8_ELLIPSIS, new BMessage (MENU_FIND_STRING),'F'));
	fFindBackwardMenuItem = new BMenuItem ("Find Backward", new BMessage (MENU_FIND_BACKWARD), '[');
	fEditmenu->AddItem(fFindBackwardMenuItem);
	fFindBackwardMenuItem->SetEnabled(false);
	fFindForwardMenuItem = new BMenuItem ("Find Forward", new BMessage (MENU_FIND_FORWARD), ']');
	fEditmenu->AddItem (fFindForwardMenuItem);
	fFindForwardMenuItem->SetEnabled(false);

	fMenubar->AddItem (fEditmenu);

	// Make Help Menu.
	fHelpmenu = new BMenu("Settings");
	fWindowSizeMenu = new BMenu("Window Size");
	_BuildWindowSizeMenu(fWindowSizeMenu);
	
	fEncodingmenu = new BMenu("Text Encoding");
	fEncodingmenu->SetRadioMode(true);
	MakeEncodingMenu(fEncodingmenu, true);
	fHelpmenu->AddItem(fWindowSizeMenu);  
	fHelpmenu->AddItem(fEncodingmenu);
	fHelpmenu->AddSeparatorItem();
	fHelpmenu->AddItem(new BMenuItem("Preferences" B_UTF8_ELLIPSIS, new BMessage(MENU_PREF_OPEN)));
	fHelpmenu->AddSeparatorItem();
	fHelpmenu->AddItem(new BMenuItem("Save as default", new BMessage(SAVE_AS_DEFAULT))); 
	fMenubar->AddItem(fHelpmenu);
	
	AddChild(fMenubar);
}


void
TermWindow::_GetPreferredFont(BFont &font)
{
	const char *family = PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY);

	font.SetFamilyAndStyle(family, NULL);
	float size = PrefHandler::Default()->getFloat(PREF_HALF_FONT_SIZE);
	if (size < 6.0f)
		size = 6.0f;
	font.SetSize(size);
	font.SetSpacing(B_FIXED_SPACING);
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
			app_info info;
			be_app->GetAppInfo(&info);
			
			// try launching two different ways to work around possible problems
			if (be_roster->Launch(&info.ref) != B_OK)
				be_roster->Launch(TERM_SIGNATURE);
			break;
		}
		
		case MENU_PREF_OPEN:
			if (!fPrefWindow)
				fPrefWindow = new PrefWindow(this);
			else
				fPrefWindow->Activate();
			break;
		
		case MSG_PREF_CLOSED:
			fPrefWindow = NULL;
			break;
	
		case MENU_FIND_STRING:
			if (!fFindPanel) {
				BRect r = Frame();
				r.left += 20;
				r.top += 20;
				r.right = r.left + 260;
				r.bottom = r.top + 190;
				fFindPanel = new FindWindow(r, this, fFindString, fFindSelection, fMatchWord, fMatchCase, fForwardSearch);
			}
			else
				fFindPanel->Activate();
			break;
	
		case MSG_FIND:
			fFindPanel->PostMessage(B_QUIT_REQUESTED);
			message->FindBool("findselection", &fFindSelection);
			if (!fFindSelection) 
				message->FindString("findstring", &fFindString);
			else 
				_ActiveTermView()->GetSelection(fFindString);

			if (fFindString.Length() == 0) {
				BAlert *alert = new BAlert("find failed", "No search string.", "Okay", NULL, 
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go();
				fFindBackwardMenuItem->SetEnabled(false);
				fFindForwardMenuItem->SetEnabled(false);
				break;
			}

			message->FindBool("forwardsearch", &fForwardSearch);
			message->FindBool("matchcase", &fMatchCase);
			message->FindBool("matchword", &fMatchWord);
			findresult = _ActiveTermView()->Find(fFindString, fForwardSearch, fMatchCase, fMatchWord);
				
			if (!findresult) {
				BAlert *alert = new BAlert("find failed", "Not Found.", "Okay", NULL,
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go();
				fFindBackwardMenuItem->SetEnabled(false);
				fFindForwardMenuItem->SetEnabled(false);
				break;
			}

			// Enable the menu items Find Forward and Find Backward
			fFindBackwardMenuItem->SetEnabled(true);
			fFindForwardMenuItem->SetEnabled(true);
			break;
		
		case MENU_FIND_FORWARD:
			findresult = _ActiveTermView()->Find(fFindString, true, fMatchCase, fMatchWord);
			if (!findresult) {
				BAlert *alert = new BAlert("find failed", "Not Found.", "Okay", NULL,
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go();
			}
			break;
	
		case MENU_FIND_BACKWARD:
			findresult = _ActiveTermView()->Find(fFindString, false, fMatchCase, fMatchWord);
			if (!findresult) {
				BAlert *alert = new BAlert("find failed", "Not Found.", "Okay", NULL,
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
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
			PrefHandler::Default()->setInt32(PREF_COLS, columns);
			PrefHandler::Default()->setInt32(PREF_ROWS, rows);
		   	
			_ActiveTermView()->SetTermSize(rows, columns, 0);
		
			_ResizeView(_ActiveTermView());
		
			BPath path;
			if (PrefHandler::GetDefaultPath(path) == B_OK)
				PrefHandler::Default()->SaveAsText(path.Path(), PREFFILE_MIMETYPE);
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
				float mbHeight = fMenubar->Bounds().Height() + 1;
				fSavedFrame = Frame();
				BScreen screen(this);
				_ActiveTermView()->ScrollBar()->Hide();
				fMenubar->Hide();
				fTabView->ResizeBy(0, mbHeight);
				fTabView->MoveBy(0, -mbHeight);
				fSavedLook = Look();
				// done before ResizeTo to work around a Dano bug (not erasing the decor)
				SetLook(B_NO_BORDER_WINDOW_LOOK);
				ResizeTo(screen.Frame().Width()+1, screen.Frame().Height()+1);
				MoveTo(screen.Frame().left, screen.Frame().top);
			} else { // exit fullscreen
				float mbHeight = fMenubar->Bounds().Height() + 1;
				fMenubar->Show();
				_ActiveTermView()->ScrollBar()->Show();
				ResizeTo(fSavedFrame.Width(), fSavedFrame.Height());
				MoveTo(fSavedFrame.left, fSavedFrame.top);
				fTabView->ResizeBy(0, -mbHeight);
				fTabView->MoveBy(0, mbHeight);
				SetLook(fSavedLook);
				fSavedFrame = BRect(0,0,-1,-1);
			}
			break;	
		
		case MSG_FONT_CHANGED:
	    	PostMessage(MSG_HALF_FONT_CHANGED);
			break;

		case MSG_COLOR_CHANGED:
		{
			_SetTermColors(_ActiveTermViewContainerView());
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

		case kNewTab:
			if (fTabView->CountTabs() < kMaxTabs)
				_AddTab(NULL);
			break;

		case kCloseView:
		{
			TermView* termView;
			if (message->FindPointer("termView", (void**)&termView) == B_OK) {
				int32 index = _IndexOfTermView(termView);
				if (index >= 0)
					_RemoveTab(index);
			}
			break;
		}
		
		case kIncreaseFontSize:		
		case kDecreaseFontSize:
		{
			message->PrintToStream();
			TermView *view = _ActiveTermView();
			BFont font;
			view->GetTermFont(&font);
			
			float size = font.Size();
			if (message->what == kIncreaseFontSize)
				size += 1;
			else
				size -= 1;
			
			// limit the font size
			if (size < 6)
				size = 6;
			else if (size > 20)
				size = 20;
				
			font.SetSize(size);	
			view->SetTermFont(&font);
			PrefHandler::Default()->setInt32(PREF_HALF_FONT_SIZE, (int32)size);
			
			_ResizeView(view);
			break;
		}

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
TermWindow::_SetTermColors(TermViewContainerView *containerView)
{
	PrefHandler* handler = PrefHandler::Default();
	rgb_color background = handler->getRGB(PREF_TEXT_BACK_COLOR);

	containerView->SetViewColor(background);

	TermView *termView = containerView->GetTermView();
	termView->SetTextColor(handler->getRGB(PREF_TEXT_FORE_COLOR), background);

	termView->SetSelectColor(handler->getRGB(PREF_SELECT_FORE_COLOR),
		handler->getRGB(PREF_SELECT_BACK_COLOR));

	termView->SetCursorColor(handler->getRGB(PREF_CURSOR_FORE_COLOR),
		handler->getRGB(PREF_CURSOR_BACK_COLOR));
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
		(new BAlert("Cancel", "Print cancelled.", "OK"))->Go();
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
    
			if (!job.CanContinue()){
				// It is likely that the only way that the job was cancelled is
			    // because the user hit 'Cancel' in the page setup window, in which
			    // case, the user does *not* need to be told that it was cancelled.
			    // He/she will simply expect that it was done.
				    return;
			}
		}
	}
	
	job.CommitJob(); 
}


void
TermWindow::_AddTab(Arguments *args)
{
	int argc = 0;
	const char *const *argv = NULL;
	if (args != NULL)
		args->GetShellArguments(argc, argv);
	
	try {
		// Note: I don't pass the Arguments class directly to the termview,
		// only to avoid adding it as a dependency: in other words, to keep
		// the TermView class as agnostic as possible about the surrounding world.
		CustomTermView *view = 
			new CustomTermView(PrefHandler::Default()->getInt32(PREF_ROWS),
							PrefHandler::Default()->getInt32(PREF_COLS),
							argc, (const char **)argv);

		TermViewContainerView *containerView = new TermViewContainerView(view);
		BScrollView *scrollView = new TermScrollView("scrollView",
			containerView, view);

		BTab *tab = new BTab;
		// TODO: Use a better name. For example, do like MacOsX's Terminal
		// and update the title using the last executed command ?
		// Or like Gnome's Terminal and use the current path ?
		fTabView->AddTab(scrollView, tab);
		tab->SetLabel("Terminal");
		view->SetScrollBar(scrollView->ScrollBar(B_VERTICAL));
		
		// Resize the vertical scrollbar to take the window gripping handle into account
		// TODO: shouldn't this be done in BScrollView itself ? At least BScrollBar does that.	
		scrollView->ScrollBar(B_VERTICAL)->ResizeBy(0, -13);
		
		view->SetEncoding(EncodingID(PrefHandler::Default()->getString(PREF_TEXT_ENCODING)));
		
		BFont font;
		_GetPreferredFont(font);	
		view->SetTermFont(&font);

		_SetTermColors(containerView);

		int width, height;
		view->GetFontSize(&width, &height);

		float minimumHeight = 0;
		if (fMenubar)
			minimumHeight += fMenubar->Bounds().Height();
		if (fTabView && fTabView->CountTabs() > 1)
			minimumHeight += fTabView->TabHeight();
		SetSizeLimits(MIN_COLS * width, MAX_COLS * width,
						minimumHeight + MIN_ROWS * height, 
						minimumHeight + MAX_ROWS * height);

		// If it's the first time we're called, setup the window
		if (fTabView->CountTabs() == 1) {
			float viewWidth, viewHeight;
			containerView->GetPreferredSize(&viewWidth, &viewHeight);

			// Resize Window
			ResizeTo(viewWidth + B_V_SCROLL_BAR_WIDTH,
					viewHeight + fMenubar->Bounds().Height());
		}
		// TODO: No fTabView->Select(tab); ?
		fTabView->Select(fTabView->CountTabs() - 1);
	} catch (...) {
		// most probably out of memory. That's bad.
		// TODO: Should cleanup, I guess
	}
}	
	
	
void
TermWindow::_RemoveTab(int32 index)
{
	if (fTabView->CountTabs() > 1)			
		delete fTabView->RemoveTab(index);
	else
		PostMessage(B_QUIT_REQUESTED);
}

TermViewContainerView*
TermWindow::_ActiveTermViewContainerView() const
{
	return _TermViewContainerViewAt(fTabView->Selection());
}


TermViewContainerView*
TermWindow::_TermViewContainerViewAt(int32 index) const
{
	// TODO: BAD HACK:
	// We should probably use the observer api to tell
	// the various "tabs" when settings are changed. Fix this.
	BScrollView* scrollView = (BScrollView*)fTabView->ViewForTab(index);
	return scrollView ? (TermViewContainerView*)scrollView->Target() : NULL;
}


TermView *
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
	// There seems to be no separate list of sessions, so we have to iterate
	// through the tabs.
	int32 count = fTabView->CountTabs();
	for (int32 i = count - 1; i >= 0; i--) {
		// get the term view
		TermView* termView = _TermViewAt(i);
		if (!termView)
			continue;

		termView->CheckShellGone();
	}
}


void
TermWindow::_ResizeView(TermView *view)
{
	int fontWidth, fontHeight;
	view->GetFontSize(&fontWidth, &fontHeight);
			
	float minimumHeight = 0;
	if (fMenubar)
		minimumHeight += fMenubar->Bounds().Height();
	if (fTabView && fTabView->CountTabs() > 1)
		minimumHeight += fTabView->TabHeight();
	
	SetSizeLimits(MIN_COLS * fontWidth, MAX_COLS * fontWidth,
					minimumHeight + MIN_ROWS * fontHeight, 
					minimumHeight + MAX_ROWS * fontHeight);
	
	float width, height;
	view->Parent()->GetPreferredSize(&width, &height);
	width += B_V_SCROLL_BAR_WIDTH;
	height += fMenubar->Bounds().Height() + 2;

	ResizeTo(width, height);
	
	view->Invalidate();
}


void
TermWindow::_BuildWindowSizeMenu(BMenu *menu)
{
	const int32 windowSizes[5][2] = {
		{ 80, 24 },
		{ 80, 25 },
		{ 80, 40 },
		{ 132, 24 },
		{ 132, 25 }	
	};
	
	const int32 sizeNum = sizeof(windowSizes) / sizeof(windowSizes[0]);
	for (int32 i = 0; i < sizeNum; i++) {
		char label[32];
		int32 columns = windowSizes[i][0];
		int32 rows = windowSizes[i][1];
		snprintf(label, sizeof(label), "%ldx%ld", columns, rows);
		BMessage *message = new BMessage(MSG_COLS_CHANGED);
		message->AddInt32("columns", columns);
		message->AddInt32("rows", rows);
		menu->AddItem(new BMenuItem(label, message));
	}
	
	menu->AddItem(new BMenuItem("Fullscreen",
					new BMessage(FULLSCREEN), B_ENTER)); 
}


// CustomTermView
CustomTermView::CustomTermView(int32 rows, int32 columns, int32 argc, const char **argv, int32 historySize)
	:
	TermView(rows, columns, argc, argv, historySize)
{
}


void
CustomTermView::NotifyQuit(int32 reason)
{
	BWindow *window = Window();
	if (window == NULL)
		window = be_app->WindowAt(0);

	// TODO: If we got this from a view in a tab not currently selected,
	// Window() will be NULL, as the view is detached.
	// So we send the message to the first application window
	// This isn't so cool, but for now, a Terminal app has only one
	// window.
	if (window != NULL) {
		BMessage message(kCloseView);
		message.AddPointer("termView", this);
		message.AddInt32("reason", reason);	
		window->PostMessage(&message);
	}
}


void
CustomTermView::SetTitle(const char *title)
{
	//Window()->SetTitle(title);
}


/*
 * Copyright 2007 Haiku, Inc.
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed under the terms of the MIT license.
 */

#include "TermWindow.h"

#include "Coding.h"
#include "ColorWindow.h"
#include "MenuUtil.h"
#include "FindWindow.h"
#include "PrefWindow.h"
#include "PrefView.h"
#include "PrefHandler.h"
#include "SmartTabView.h"
#include "TermConst.h"
#include "TermView.h"

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

#include <stdio.h>
#include <string.h>
#include <time.h>

//
// help and GPL URL
//
//#define URL_PREFIX  "file:///boot/home/config/settings/MuTerminal/help/"
//#define INDEX_FILE  "/index.html"
//#define GPL_FILE  "/gpl.html"
//#define CHLP_FILE   "file:///boot/beos/documentation/Shell%20Tools/index.html"


const static float kViewOffset = 3;
const static uint32 kNewTab = 'NTab';
const static uint32 kCloseView = 'ClVw';


class CustomTermView : public TermView {
public:
	CustomTermView(int32 rows, int32 columns, const char *command = NULL, int32 historySize = 1000);
	virtual void NotifyQuit(int32 reason);
	virtual void SetTitle(const char *title);
};


TermWindow::TermWindow(BRect frame, const char* title, const char *command)
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
	_InitWindow(command);
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


//	#pragma mark - public methods


/** Initialize Window object. */
void
TermWindow::_InitWindow(const char *command)
{
	// make menu bar
	_SetupMenu();

	BRect textFrame = Bounds();
	textFrame.top = fMenubar->Bounds().bottom + 1.0;

	fTabView = new SmartTabView(textFrame, "tab view");
	AddChild(fTabView);
	
	_NewTab(command);
}


void
TermWindow::MenusBeginning()
{
	// Syncronize Encode Menu Pop-up menu and Preference.
	BMenuItem *item = fEncodingmenu->FindItem(id2longname(_ActiveTermView()->Encoding()));
	if (item != NULL)
		item->SetMarked(true);
	BWindow::MenusBeginning();
}


void
TermWindow::_SetupMenu()
{
	PrefHandler menuText;
	
	LoadLocaleFile (&menuText);
	
	// Menu bar object.
	fMenubar = new BMenuBar(Bounds(), "mbar");
	
	// Make File Menu.
	fFilemenu = new BMenu("Terminal");
	fFilemenu->AddItem(new BMenuItem("Switch Terminals", new BMessage(MENU_SWITCH_TERM),'G'));
	fFilemenu->AddItem(new BMenuItem("New Terminal" B_UTF8_ELLIPSIS, new BMessage(MENU_NEW_TERM), 'N'));
	
	// TODO: Tabs disabled until various problems are fixed.
	// n. 1: calling "exit" from a tab closes the whole app.
	fFilemenu->AddItem(new BMenuItem("New Tab", new BMessage(kNewTab), 'T'));
	
	fFilemenu->AddSeparatorItem();
	fFilemenu->AddItem(new BMenuItem("Page Setup...", new BMessage(MENU_PAGE_SETUP)));
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
	fWindowSizeMenu->AddItem(new BMenuItem("80x24", new BMessage(EIGHTYTWENTYFOUR)));
	fWindowSizeMenu->AddItem(new BMenuItem("80x25", new BMessage(EIGHTYTWENTYFIVE)));  
	fWindowSizeMenu->AddItem(new BMenuItem("80x40", new BMessage(EIGHTYFORTY))); 
	fWindowSizeMenu->AddItem(new BMenuItem("132x24", new BMessage(ONETHREETWOTWENTYFOUR))); 
	fWindowSizeMenu->AddItem(new BMenuItem("132x25", new BMessage(ONETHREETWOTWENTYFIVE))); 
	fWindowSizeMenu->AddItem(new BMenuItem("Fullscreen", new BMessage(FULLSCREEN), B_ENTER)); 

	fEncodingmenu = new BMenu("Font Encoding");
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
TermWindow::MessageReceived(BMessage *message)
{
	int32 coding_id;
	BRect r;
	BFont halfFont;
	BFont fullFont;
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
			
		case MENU_SWITCH_TERM: {
			be_app->PostMessage(MENU_SWITCH_TERM);
			break;
		}
		case kNewTab:
			_NewTab(NULL);
			break;

		case kCloseView:
		{
			// TODO: We assume that this message was sent from the current active tab.
			// Since the implementation of BTabView uses AddChild/RemoveChild on the
			// views, the current active tab is the only one who is attached, thus
			// the only one which could send a message.
			delete fTabView->RemoveTab(fTabView->Selection());
			break;	
		}

		case MENU_NEW_TERM: {
			app_info info;
			be_app->GetAppInfo(&info);
			
			// try launching two different ways to work around possible problems
			if (be_roster->Launch(&info.ref)!=B_OK)
				be_roster->Launch(TERM_SIGNATURE);
			break;
		}
		case MENU_PREF_OPEN: {
			if (!fPrefWindow)
				fPrefWindow = new PrefWindow(this);
			else
				fPrefWindow->Activate();
			break;
		}
		case MSG_PREF_CLOSED: {
			fPrefWindow = NULL;
			break;
		}
		case MENU_FIND_STRING: {
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
		}
		case MSG_FIND: {
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
		}
		case MENU_FIND_FORWARD: {
			findresult = _ActiveTermView()->Find(fFindString, true, fMatchCase, fMatchWord);
			if (!findresult) {
				BAlert *alert = new BAlert("find failed", "Not Found.", "Okay", NULL,
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go();
			}
			break;
		}
		case MENU_FIND_BACKWARD: {
			findresult = _ActiveTermView()->Find(fFindString, false, fMatchCase, fMatchWord);
			if (!findresult) {
				BAlert *alert = new BAlert("find failed", "Not Found.", "Okay", NULL,
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go();
			}
			break;
		}
		case MSG_FIND_CLOSED:
			fFindPanel = NULL;
			break;

		case MENU_ENCODING: {
			message->FindInt32 ("op", &coding_id);
			_ActiveTermView()->SetEncoding(coding_id);
			break;
		}
		
		// Message from Preference panel.
		case MSG_ROWS_CHANGED:
		case MSG_COLS_CHANGED: {
			r = _ActiveTermView()->SetTermSize (PrefHandler::Default()->getInt32 (PREF_ROWS),
										PrefHandler::Default()->getInt32 (PREF_COLS), 0);
		
			ResizeTo (r.Width()+ B_V_SCROLL_BAR_WIDTH + kViewOffset * 2,
				r.Height()+fMenubar->Bounds().Height() + kViewOffset *2);
		
			BPath path;
			if (PrefHandler::GetDefaultPath(path) == B_OK)
				PrefHandler::Default()->SaveAsText(path.Path(), PREFFILE_MIMETYPE);
			break;
		}
		case MSG_HALF_FONT_CHANGED:
		case MSG_FULL_FONT_CHANGED:
		case MSG_HALF_SIZE_CHANGED:
		case MSG_FULL_SIZE_CHANGED: {
	
			halfFont.SetFamilyAndStyle (PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY),NULL);
			halfFont.SetSize (PrefHandler::Default()->getFloat(PREF_HALF_FONT_SIZE));
			halfFont.SetSpacing (B_FIXED_SPACING);
			
			fullFont.SetFamilyAndStyle (PrefHandler::Default()->getString(PREF_FULL_FONT_FAMILY),NULL);
			fullFont.SetSize (PrefHandler::Default()->getFloat(PREF_FULL_FONT_SIZE));
			fullFont.SetSpacing (B_FIXED_SPACING);
			
			_ActiveTermView()->SetTermFont (&halfFont, &fullFont);
			r = _ActiveTermView()->SetTermSize (0, 0, 0);
			
			int width, height;
			
			_ActiveTermView()->GetFontSize (&width, &height);
			
			SetSizeLimits (MIN_COLS * width, MAX_COLS * width,
							MIN_COLS * height, MAX_COLS * height);
			
			ResizeTo (r.Width()+ B_V_SCROLL_BAR_WIDTH + kViewOffset * 2,
			r.Height()+fMenubar->Bounds().Height() + kViewOffset * 2);
			
			_ActiveTermView()->Invalidate();
			break;
		}
		case EIGHTYTWENTYFOUR: {
			PrefHandler::Default()->setString(PREF_COLS, "80");
			PrefHandler::Default()->setString(PREF_ROWS, "24");
		   	PostMessage (MSG_COLS_CHANGED);
			break;
		}
		case EIGHTYTWENTYFIVE: {
			PrefHandler::Default()->setString(PREF_COLS, "80");
			PrefHandler::Default()->setString(PREF_ROWS, "25");
		   	PostMessage (MSG_COLS_CHANGED);
			break;		
		}
		case EIGHTYFORTY: {
			PrefHandler::Default()->setString(PREF_COLS, "80");
			PrefHandler::Default()->setString(PREF_ROWS, "40");
		   	PostMessage (MSG_COLS_CHANGED);
			break;	
		}
		case ONETHREETWOTWENTYFOUR: {
			PrefHandler::Default()->setString(PREF_COLS, "132");
			PrefHandler::Default()->setString(PREF_ROWS, "24");
		   	PostMessage (MSG_COLS_CHANGED);
			break;	
		}
		case ONETHREETWOTWENTYFIVE: {
			PrefHandler::Default()->setString(PREF_COLS, "132");
			PrefHandler::Default()->setString(PREF_ROWS, "25");
		   	PostMessage (MSG_COLS_CHANGED);
			break;	
		}
		case FULLSCREEN: {
			if (!fSavedFrame.IsValid()) { // go fullscreen
				float mbHeight = fMenubar->Bounds().Height() + 1;
				fSavedFrame = Frame();
				BScreen screen(this);
				_ActiveTermView()->ScrollBar()->Hide();
				fMenubar->Hide();
				_ActiveTermView()->ResizeBy(B_V_SCROLL_BAR_WIDTH, mbHeight);
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
				_ActiveTermView()->ResizeBy(-B_V_SCROLL_BAR_WIDTH, -mbHeight);
				SetLook(fSavedLook);
				fSavedFrame = BRect(0,0,-1,-1);
			}
			break;	
		}
		case MSG_FONT_CHANGED: {
	    		PostMessage(MSG_HALF_FONT_CHANGED);
			break;
		}
		case MSG_COLOR_CHANGED: {
			_SetTermColors();
			_ActiveTermView()->Invalidate();
			break;
		}
		case SAVE_AS_DEFAULT: {
			BPath path;
			if (PrefHandler::GetDefaultPath(path) == B_OK)
				PrefHandler::Default()->SaveAsText(path.Path(), PREFFILE_MIMETYPE);
			break;
		}
		case MENU_PAGE_SETUP: {
			_DoPageSetup();
			break;
		}
		case MENU_PRINT: {
			_DoPrint();
			break;
		}

		case B_ABOUT_REQUESTED: {
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;
		}
		default: {
			BWindow::MessageReceived(message);
			break;
		}
	}
}


void
TermWindow::WindowActivated(bool activated)
{
	BWindow::WindowActivated(activated);
}


bool
TermWindow::QuitRequested()
{
	// TODO: Intercept the B_QUIT_REQUESTED message
	// sent by the TermView, and only close one tab if there
	// are multiple ones ? Or handle the case inside TermView itself ?
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
TermWindow::_SetTermColors()
{
	_ActiveTermView()->SetTextColor(PrefHandler::Default()->getRGB(PREF_TEXT_FORE_COLOR),
				PrefHandler::Default()->getRGB(PREF_TEXT_BACK_COLOR));

	_ActiveTermView()->SetSelectColor(PrefHandler::Default()->getRGB(PREF_SELECT_FORE_COLOR),
				PrefHandler::Default()->getRGB(PREF_SELECT_BACK_COLOR));
	
	_ActiveTermView()->SetCursorColor(PrefHandler::Default()->getRGB(PREF_CURSOR_FORE_COLOR),
				PrefHandler::Default()->getRGB(PREF_CURSOR_BACK_COLOR));
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
				// (new BAlert("Cancel", "Print job cancelled", "OK"))->Go();
			        return;
			}
		}
	}
	
	job.CommitJob(); 
}


void
TermWindow::_NewTab(const char *command)
{
	// Setup font.

	const char *family = PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY);

	BFont halfFont;
	halfFont.SetFamilyAndStyle(family, NULL);
	float size = PrefHandler::Default()->getFloat(PREF_HALF_FONT_SIZE);
	if (size < 6.0f)
		size = 6.0f;
	halfFont.SetSize(size);
	
	family = PrefHandler::Default()->getString(PREF_FULL_FONT_FAMILY);

	BFont fullFont;
	fullFont.SetFamilyAndStyle(family, NULL);
	size = PrefHandler::Default()->getFloat(PREF_FULL_FONT_SIZE);
	if (size < 6.0f)
		size = 6.0f;
	fullFont.SetSize(size);
	fullFont.SetSpacing(B_FIXED_SPACING);

	// Make Terminal text view.
	CustomTermView *view = new CustomTermView(PrefHandler::Default()->getInt32(PREF_ROWS),
					PrefHandler::Default()->getInt32(PREF_COLS),
					command);
	
	BScrollView *scrollView = new BScrollView("scrollView", view, B_FOLLOW_ALL,
					B_WILL_DRAW|B_FRAME_EVENTS, false, true);	
	fTabView->AddTab(scrollView);
	view->SetScrollBar(scrollView->ScrollBar(B_VERTICAL));
	
	// TODO: Resize the vertical scrollbar to take the window gripping handle into account
	// (shouldn't this be done in BScrollView itself ? At least BScrollBar does that).	
	scrollView->ScrollBar(B_VERTICAL)->ResizeBy(0, -13);
	view->SetEncoding(longname2id(PrefHandler::Default()->getString(PREF_TEXT_ENCODING)));
	view->SetTermFont(&halfFont, &fullFont);
	
	_SetTermColors();
	
	// If it's the first time we're called, setup the window
	if (fTabView->CountTabs() == 1) {
		int width, height;
		view->GetFontSize(&width, &height);
		SetSizeLimits(MIN_COLS * width, MAX_COLS * width,
			MIN_COLS * height, MAX_COLS * height);
	
		float fWidth, fHeight;
		view->GetPreferredSize(&fWidth, &fHeight);
		
		// Resize Window
		ResizeTo(fWidth + B_V_SCROLL_BAR_WIDTH, fHeight + fMenubar->Bounds().Height());

		// TODO: If I don't do this, the view won't show up.
		// Bug in BTabView or in my code ?
		fTabView->Select(0);
	}
}


TermView *
TermWindow::_ActiveTermView()
{
	// TODO: BAD HACK:
	// We should probably use the observer api to tell
	// the various "tabs" when settings are changed. Fix this.
	return (TermView *)((BScrollView *)fTabView->ViewForTab(fTabView->Selection()))->Target();
}


// CustomTermView
CustomTermView::CustomTermView(int32 rows, int32 columns, const char *command, int32 historySize)
	:
	TermView(rows, columns, command, historySize)
{
}


void
CustomTermView::NotifyQuit(int32 reason)
{
	if (Window()) {
		BMessage message(kCloseView);
		message.AddInt32("reason", reason);
		Window()->PostMessage(&message);
	}
}


void
CustomTermView::SetTitle(const char *title)
{
	//Window()->SetTitle(title);
}


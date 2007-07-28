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
#include <Dragger.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PrintJob.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <String.h>
#include <TextControl.h>
#include <WindowScreen.h>

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

#if 0
TermWindow::TermWindow(BRect frame, const char* title, const char *command)
	:
	BWindow(frame, title, B_DOCUMENT_WINDOW, B_CURRENT_WORKSPACE|B_QUIT_ON_WINDOW_CLOSE),
	fTabView(NULL),
	fTermView(NULL),	
	fMenubar(NULL),
	fFilemenu(NULL),
	fEditmenu(NULL),
	fEncodingmenu(NULL),
	fHelpmenu(NULL),
	fFontMenu(NULL),
	fWindowSizeMenu(NULL),
	fNewFontMenu(NULL),
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
	fTabView = new SmartTabView(Bounds(), "Tab view");
	AddChild(fTabView);
	
	_NewTab(command);
	_NewTab(NULL);
}
#endif


TermWindow::TermWindow(BRect frame, const char* title, const char *command)
	: BWindow(frame, title, B_DOCUMENT_WINDOW, B_CURRENT_WORKSPACE|B_QUIT_ON_WINDOW_CLOSE),
	fTabView(NULL),
	fTermView(NULL),	
	fMenubar(NULL),
	fFilemenu(NULL),
	fEditmenu(NULL),
	fEncodingmenu(NULL),
	fHelpmenu(NULL),
	fFontMenu(NULL),
	fWindowSizeMenu(NULL),
	fNewFontMenu(NULL),
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

	BRect textframe = Bounds();
	textframe.top = fMenubar->Bounds().bottom + 1.0;

	fTermView = new TermView(textframe, command);
	fTermView->SetEncoding(longname2id(PrefHandler::Default()->getString(PREF_TEXT_ENCODING)));

	// Initialize TermView. (font, size and color)
	fTermView->SetTermFont(&halfFont, &fullFont);
	
	_SetTermColors();
	BRect rect = fTermView->SetTermSize(PrefHandler::Default()->getInt32(PREF_ROWS),
					PrefHandler::Default()->getInt32(PREF_COLS), true);

	int width, height;
	fTermView->GetFontSize(&width, &height);
	SetSizeLimits(MIN_COLS * width, MAX_COLS * width,
		MIN_COLS * height, MAX_COLS * height);
	
	// Add offset to baseview.
	rect.InsetBy(-kViewOffset, -kViewOffset);

	// Resize Window
	ResizeTo(rect.Width()+ B_V_SCROLL_BAR_WIDTH,
		rect.Height() + fMenubar->Bounds().Height());

	fTermView->ResizeTo(rect.Width(), rect.Height());
	
	// Make Scroll Bar.

	BRect scrollRect(0, 0, B_V_SCROLL_BAR_WIDTH,
		rect.Height() - B_H_SCROLL_BAR_HEIGHT + 1);

	scrollRect.OffsetBy(rect.Width() + 1, fMenubar->Bounds().Height());

	BScrollBar *scrollBar = new BScrollBar(scrollRect, "scrollbar",
		fTermView, 0, 0, B_VERTICAL);
	fTermView->SetScrollBar(scrollBar);
	
	AddChild(scrollBar);
	AddChild(fTermView);

	fEditmenu->SetTargetForItems(fTermView);
}


void
TermWindow::MenusBeginning()
{
	// Syncronize Encode Menu Pop-up menu and Preference.
	BMenuItem *item = fEncodingmenu->FindItem(id2longname(fTermView->Encoding()));
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
	fEditmenu->AddSeparatorItem ();
	fEditmenu->AddItem (new BMenuItem ("Select All", new BMessage (B_SELECT_ALL), 'A'));
	fEditmenu->AddItem (new BMenuItem ("Clear All", new BMessage (MENU_CLEAR_ALL), 'L'));
	fEditmenu->AddSeparatorItem ();
	fEditmenu->AddItem (new BMenuItem ("Find" B_UTF8_ELLIPSIS, new BMessage (MENU_FIND_STRING),'F'));
	fFindBackwardMenuItem = new BMenuItem ("Find Backward", new BMessage (MENU_FIND_BACKWARD), '[');
	fEditmenu->AddItem (fFindBackwardMenuItem);
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

 	// Considering we have this in the preferences window, this menu is not
 	// needed and should not be shown if we are to not confuse the user
/*  fNewFontMenu = new BMenu("Font");
	fNewFontMenu->SetRadioMode(true);
		int32 numFamilies1 = count_font_families();
		for ( int32 i = 0; i < numFamilies1; i++ ) {
			font_family family;
			uint32 flags;
			if ( get_font_family(i, &family, &flags) == B_OK ) {
				fNewFontMenu->AddItem(item = new BMenuItem(family, new BMessage(MSG_FONT_CHANGED)));
			//	if (0 ==i) item->SetMarked(true); 
			}
		}
  fNewFontMenu->FindItem (PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY))->SetMarked(true);
*/

	fEncodingmenu = new BMenu("Font Encoding");
	fEncodingmenu->SetRadioMode(true);
	MakeEncodingMenu(fEncodingmenu, true);
	fHelpmenu->AddItem(fWindowSizeMenu);  
	fHelpmenu->AddItem(fEncodingmenu);
//  fHelpmenu->AddItem(fNewFontMenu);
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
		case MENU_SWITCH_TERM: {
			be_app->PostMessage(MENU_SWITCH_TERM);
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
				fTermView->GetSelection(fFindString);

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
			findresult = fTermView->Find(fFindString, fForwardSearch, fMatchCase, fMatchWord);
				
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
			findresult = fTermView->Find(fFindString, true, fMatchCase, fMatchWord);
			if (!findresult) {
				BAlert *alert = new BAlert("find failed", "Not Found.", "Okay", NULL,
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go();
			}
			break;
		}
		case MENU_FIND_BACKWARD: {
			findresult = fTermView->Find(fFindString, false, fMatchCase, fMatchWord);
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
			fTermView->SetEncoding(coding_id);
			break;
		}
		
		// Message from Preference panel.
		case MSG_ROWS_CHANGED:
		case MSG_COLS_CHANGED: {
			r = fTermView->SetTermSize (PrefHandler::Default()->getInt32 (PREF_ROWS),
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
			
			fTermView->SetTermFont (&halfFont, &fullFont);
			r = fTermView->SetTermSize (0, 0, 0);
			
			int width, height;
			
			fTermView->GetFontSize (&width, &height);
			
			SetSizeLimits (MIN_COLS * width, MAX_COLS * width,
							MIN_COLS * height, MAX_COLS * height);
			
			ResizeTo (r.Width()+ B_V_SCROLL_BAR_WIDTH + kViewOffset * 2,
			r.Height()+fMenubar->Bounds().Height() + kViewOffset * 2);
			
			fTermView->Invalidate();
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
				fTermView->ScrollBar()->Hide();
				fMenubar->Hide();
				fTermView->ResizeBy(B_V_SCROLL_BAR_WIDTH, mbHeight);
				fSavedLook = Look();
				// done before ResizeTo to work around a Dano bug (not erasing the decor)
				SetLook(B_NO_BORDER_WINDOW_LOOK);
				ResizeTo(screen.Frame().Width()+1, screen.Frame().Height()+1);
				MoveTo(screen.Frame().left, screen.Frame().top);
			} else { // exit fullscreen
				float mbHeight = fMenubar->Bounds().Height() + 1;
				fMenubar->Show();
				fTermView->ScrollBar()->Show();
				ResizeTo(fSavedFrame.Width(), fSavedFrame.Height());
				MoveTo(fSavedFrame.left, fSavedFrame.top);
				fTermView->ResizeBy(-B_V_SCROLL_BAR_WIDTH, -mbHeight);
				SetLook(fSavedLook);
				fSavedFrame = BRect(0,0,-1,-1);
			}
			break;	
		}
		case MSG_FONT_CHANGED: {
	    		PrefHandler::Default()->setString (PREF_HALF_FONT_FAMILY, fNewFontMenu->FindMarked()->Label());
	    		PostMessage(MSG_HALF_FONT_CHANGED);
			break;
		}
		case MSG_COLOR_CHANGED: {
			_SetTermColors();
			fTermView->Invalidate();
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
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
TermWindow::_SetTermColors()
{
	fTermView->SetTextColor(PrefHandler::Default()->getRGB(PREF_TEXT_FORE_COLOR),
				PrefHandler::Default()->getRGB(PREF_TEXT_BACK_COLOR));

	fTermView->SetSelectColor(PrefHandler::Default()->getRGB(PREF_SELECT_FORE_COLOR),
				PrefHandler::Default()->getRGB(PREF_SELECT_BACK_COLOR));
	
	fTermView->SetCursorColor(PrefHandler::Default()->getRGB(PREF_CURSOR_FORE_COLOR),
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
	fTermView->GetFrameSize(&w, &h);
	int xPages = (int)ceil(w / pWidth);
	int yPages = (int)ceil(h / pHeight);

	job.BeginJob();

	// loop through and draw each page, and write to spool
	for (int x = 0; x < xPages; x++) {
		for (int y = 0; y < yPages; y++) {
			curPageRect.OffsetTo(x * pWidth, y * pHeight);
			job.DrawView(fTermView, curPageRect, B_ORIGIN);
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
	TermView *view = new TermView(Bounds(), command);
	fTabView->AddTab(view);
}




/*
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 *
 * Distributed unter the terms of the MIT license.
 */
#include <Alert.h>
#include <Application.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PrintJob.h>
#include <PropertyInfo.h>
#include <ScrollBar.h>
#include <TextControl.h>
#include <WindowScreen.h>
#include <float.h>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <unistd.h>

#include "CodeConv.h"
#include "ColorWindow.h"
#include "MenuUtil.h"
#include "PrefDlg.h"
#include "PrefView.h"
#include "PrefHandler.h"
#include "TermApp.h"
#include "TermBaseView.h"
#include "TermBuffer.h"
#include "TermParse.h"
#include "TermView.h"
#include "TermWindow.h"
#include "TermConst.h"
#include "spawn.h"


// Global Preference Handler
extern PrefHandler *gTermPref;
//
// help and GPL URL
//
//#define URL_PREFIX  "file:///boot/home/config/settings/MuTerminal/help/"
//#define INDEX_FILE  "/index.html"
//#define GPL_FILE  "/gpl.html"
//#define CHLP_FILE   "file:///boot/beos/documentation/Shell%20Tools/index.html"

extern int gNowCoding;  /* defined TermParce.cpp */

void SetCoding(int);


TermWindow::TermWindow(BRect frame, const char* title)
	: BWindow(frame, title, B_DOCUMENT_WINDOW, B_CURRENT_WORKSPACE)
{
	InitWindow();

	fPrintSettings = NULL;
	fPrefWindow = NULL;
	fFindPanel = NULL;
}


TermWindow::~TermWindow()
{
	delete fWindowUpdate;
}


//	#pragma mark - public methods


/** Initialize Window object. */

void
TermWindow::InitWindow(void)
{
	// make menu bar
	SetupMenu();

	// Setup font.

	const char *family = gTermPref->getString(PREF_HALF_FONT_FAMILY);

	BFont halfFont;
	halfFont.SetFamilyAndStyle(family, NULL);
	halfFont.SetSize(gTermPref->getFloat(PREF_HALF_FONT_SIZE));
	halfFont.SetSpacing(B_FIXED_SPACING);

	family = gTermPref->getString(PREF_FULL_FONT_FAMILY);

	BFont fullFont;
	fullFont.SetFamilyAndStyle(family, NULL);
	fullFont.SetSize(gTermPref->getFloat(PREF_FULL_FONT_SIZE));
	fullFont.SetSpacing(B_FIXED_SPACING);

	// Make Terminal text view.

	BRect textframe = Bounds();
	textframe.top = fMenubar->Bounds().bottom + 1.0;

	fCodeConv = new CodeConv();
	fTermView = new TermView(Bounds(), fCodeConv);

	/*
	 * MuTerm has two views. BaseView is window base view.
	 * TermView is character Terminal view on BaseView. It has paste
	 * on BaseView shift as VIEW_OFFSET.
	 */
	fBaseView = new TermBaseView(textframe, fTermView);

	// Initialize TermView. (font, size and color)

	fTermView->SetTermFont(&halfFont, &fullFont);
	BRect rect = fTermView->SetTermSize(gTermPref->getInt32(PREF_ROWS),
		gTermPref->getInt32(PREF_COLS), 1);

	int width, height;

	fTermView->GetFontSize(&width, &height);
	SetSizeLimits(MIN_COLS * width, MAX_COLS * width,
		MIN_COLS * height, MAX_COLS * height);

	fTermView->SetTermColor();
	fBaseView->SetViewColor(gTermPref->getRGB(PREF_TEXT_BACK_COLOR));

	// Add offset to baseview.
	rect.InsetBy(-VIEW_OFFSET, -VIEW_OFFSET);

	// Resize Window

	ResizeTo(rect.Width()+ B_V_SCROLL_BAR_WIDTH,
		rect.Height() + fMenubar->Bounds().Height());

	fBaseView->ResizeTo(rect.Width(), rect.Height());
	fBaseView->AddChild(fTermView);
	fTermView->MoveBy(VIEW_OFFSET, VIEW_OFFSET);

	// Make Scroll Bar.

	BRect scrollRect(0, 0, B_V_SCROLL_BAR_WIDTH,
		rect.Height() - B_H_SCROLL_BAR_HEIGHT + 1);

	scrollRect.OffsetBy(rect.Width() + 1, fMenubar->Bounds().Height());

	BScrollBar *scrollBar = new BScrollBar(scrollRect, "scrollbar",
		fTermView, 0, 0, B_VERTICAL);
	fTermView->SetScrollBar(scrollBar);
	
	AddChild(scrollBar);
	AddChild(fBaseView);

	// Set fEditmenu's target to fTermView. (Oh!...)
	fEditmenu->SetTargetForItems(fTermView);

	// Initialize TermParse

	gNowCoding = longname2op(gTermPref->getString(PREF_TEXT_ENCODING));
	fTermParse = new TermParse();
	fTermParse->InitPtyReader(this);
	fTermParse->InitTermParse(fTermView, fCodeConv);

	// Set Coding.

	// Initialize MessageRunner.
	fWindowUpdate = new BMessageRunner(BMessenger(this),
		new BMessage (MSGRUN_WINDOW), 500000);
}


void
TermWindow::MenusBeginning(void)
{
	// Syncronize Encode Menu Pop-up menu and Preference.
	(fEncodingmenu->FindItem(op2longname(gNowCoding)))->SetMarked(true);
	BWindow::MenusBeginning();
}


void
TermWindow::SetupMenu(void)
{
  PrefHandler menuText;

  LoadLocaleFile (&menuText);
  
  // Menu bar object.
  fMenubar = new BMenuBar(Bounds(), "mbar");

  /*
   * Make Fiile Menu.
   */
  fFilemenu = new BMenu("Terminal");
  fFilemenu->AddItem(new BMenuItem("Switch Terminals", new BMessage(MENU_SWITCH_TERM),'G'));
  fFilemenu->AddItem(new BMenuItem("Start New Terminal", new BMessage(MENU_NEW_TREM), 'N'));
  fFilemenu->AddSeparatorItem();
  fFilemenu->AddItem(new BMenuItem("Page Setup...", new BMessage(MENU_PAGE_SETUP)));
  fFilemenu->AddItem(new BMenuItem("Print", new BMessage(MENU_PRINT),'P'));
  fFilemenu->AddSeparatorItem();
  fFilemenu->AddItem(new BMenuItem("About Terminal...", new BMessage(B_ABOUT_REQUESTED)));
  fFilemenu->AddSeparatorItem();
  fFilemenu->AddItem(new BMenuItem("Quit", new BMessage(MENU_FILE_QUIT), 'Q'));
  fMenubar->AddItem(fFilemenu);

  /*
   * Make Edit Menu.
   */
  fEditmenu = new BMenu ("Edit");
  fEditmenu->AddItem (new BMenuItem ("Copy", new BMessage (B_COPY),'C'));
  fEditmenu->AddItem (new BMenuItem ("Paste", new BMessage (B_PASTE),'V'));
  fEditmenu->AddSeparatorItem ();
  fEditmenu->AddItem (new BMenuItem ("Select All", new BMessage (B_SELECT_ALL), 'A'));
  fEditmenu->AddItem (new BMenuItem ("Clear All", new BMessage (MENU_CLEAR_ALL), 'L'));

/*
  // TODO: Implement Finding
  fEditmenu->AddSeparatorItem ();
  fEditmenu->AddItem (new BMenuItem ("Find", new BMessage (MENU_FIND_STRING),'F'));
  fEditmenu->AddItem (new BMenuItem ("Find Again", new BMessage (MENU_FIND_AGAIN), ']'));
*/
	fMenubar->AddItem (fEditmenu);
  

  /*
   * Make Help Menu.
   */
	fHelpmenu = new BMenu("Settings");
	fWindowSizeMenu = new BMenu("Window Size");
  	fWindowSizeMenu->AddItem(new BMenuItem("80x24", new BMessage(EIGHTYTWENTYFOUR)));
  	fWindowSizeMenu->AddItem(new BMenuItem("80x25", new BMessage(EIGHTYTWENTYFIVE)));  
   	fWindowSizeMenu->AddItem(new BMenuItem("80x40", new BMessage(EIGHTYFORTY))); 
 	fWindowSizeMenu->AddItem(new BMenuItem("132x24", new BMessage(ONETHREETWOTWENTYFOUR))); 
 	fWindowSizeMenu->AddItem(new BMenuItem("132x25", new BMessage(ONETHREETWOTWENTYFIVE))); 
 	
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
  fNewFontMenu->FindItem (gTermPref->getString(PREF_HALF_FONT_FAMILY))->SetMarked(true);
*/

	fEncodingmenu = new BMenu("Font Encoding");
	fEncodingmenu->SetRadioMode(true);
	MakeEncodingMenu(fEncodingmenu, gNowCoding, true);
	fHelpmenu->AddItem(fWindowSizeMenu);  
	fHelpmenu->AddItem(fEncodingmenu);
//  fHelpmenu->AddItem(fNewFontMenu);
	fHelpmenu->AddSeparatorItem();
	fHelpmenu->AddItem(new BMenuItem("Preferences", new BMessage(MENU_PREF_OPEN)));
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
  
  switch (message->what) {

  case MENU_SWITCH_TERM:
    be_app->PostMessage(MENU_SWITCH_TERM);
    break;

  case MENU_NEW_TREM:
    be_app->PostMessage(MENU_NEW_TREM);
    break;

  case MENU_PREF_OPEN:
    if (!fPrefWindow){
      fPrefWindow = new PrefDlg(this);
    }else{
      fPrefWindow->Activate();
    } 
    break;

  case MSG_PREF_CLOSED:
    fPrefWindow = NULL;
    break;
    
  case MENU_FILE_QUIT:
    be_app->PostMessage(B_QUIT_REQUESTED);
    break;
  case MENU_ENCODING:
    message->FindInt32 ("op", &coding_id);
    gNowCoding = coding_id;
    SetCoding(coding_id);
    break;

    /*
     * Extended B_SET_PROPERTY. Dispatch this message,
     * Set coding ID.
     */
  case B_SET_PROPERTY:
  {
    int32 i;
    BMessage spe;
    message->GetCurrentSpecifier(&i, &spe);
    if (!strcmp("encode", spe.FindString("property", i))){
      message->FindInt32 ("data",  &coding_id);
      gNowCoding = coding_id;
      SetCoding (coding_id);
    
      message->SendReply(B_REPLY);
    }else{
      BWindow::MessageReceived(message);
    }
    break;
  }

    /*
     * Extended B_GET_PROPERTY. Dispatch this message,
     * reply now coding ID.
     */
  case B_GET_PROPERTY:
  {
    int32 i;
    BMessage spe;
    message->GetCurrentSpecifier(&i, &spe);
    if (!strcmp("encode", spe.FindString("property", i))){
      BMessage reply(B_REPLY);
      reply.AddInt32("result", gNowCoding);
      message->SendReply(&reply);
    }else if (!strcmp("tty", spe.FindString("property", i))){
      BMessage reply(B_REPLY);
      reply.AddString("result", &tty_name[8]);
      message->SendReply(&reply);
    }else{
      BWindow::MessageReceived(message);
    }
    break;
  }

  /*
   * Message from Preference panel.
   */
  case MSG_ROWS_CHANGED:
  case MSG_COLS_CHANGED:
    r = fTermView->SetTermSize (gTermPref->getInt32 (PREF_ROWS),
              gTermPref->getInt32 (PREF_COLS),
              0);

    ResizeTo (r.Width()+ B_V_SCROLL_BAR_WIDTH + VIEW_OFFSET * 2,
        r.Height()+fMenubar->Bounds().Height() + VIEW_OFFSET *2);
    break;

  case MSG_HALF_FONT_CHANGED:
  case MSG_FULL_FONT_CHANGED:
  case MSG_HALF_SIZE_CHANGED:
  case MSG_FULL_SIZE_CHANGED:

    halfFont.SetFamilyAndStyle (gTermPref->getString(PREF_HALF_FONT_FAMILY),
        NULL);
    halfFont.SetSize (gTermPref->getFloat(PREF_HALF_FONT_SIZE));
    halfFont.SetSpacing (B_FIXED_SPACING);
  
    fullFont.SetFamilyAndStyle (gTermPref->getString(PREF_FULL_FONT_FAMILY),
        NULL);
    fullFont.SetSize (gTermPref->getFloat(PREF_FULL_FONT_SIZE));
    fullFont.SetSpacing (B_FIXED_SPACING);

    fTermView->SetTermFont (&halfFont, &fullFont);
    r = fTermView->SetTermSize (0, 0, 0);

    int width, height;
  
    fTermView->GetFontSize (&width, &height);
    
    SetSizeLimits (MIN_COLS * width, MAX_COLS * width,
		   MIN_COLS * height, MAX_COLS * height);



    ResizeTo (r.Width()+ B_V_SCROLL_BAR_WIDTH + VIEW_OFFSET * 2,
        r.Height()+fMenubar->Bounds().Height() + VIEW_OFFSET * 2);

    fTermView->Invalidate();
    break;

	case EIGHTYTWENTYFOUR:
		gTermPref->setString(PREF_COLS, "80");
		gTermPref->setString(PREF_ROWS, "24");
	   	this->PostMessage (MSG_ROWS_CHANGED);
		this->PostMessage (MSG_COLS_CHANGED);
	break;

	case EIGHTYTWENTYFIVE:
		gTermPref->setString(PREF_COLS, "80");
		gTermPref->setString(PREF_ROWS, "25");
	   	this->PostMessage (MSG_ROWS_CHANGED);
	   	this->PostMessage (MSG_COLS_CHANGED);
	break;		

	case EIGHTYFORTY:
		gTermPref->setString(PREF_COLS, "80");
		gTermPref->setString(PREF_ROWS, "40");
	   	this->PostMessage (MSG_ROWS_CHANGED);
	   	this->PostMessage (MSG_COLS_CHANGED);
	break;	
	
	case ONETHREETWOTWENTYFOUR:
		gTermPref->setString(PREF_COLS, "132");
		gTermPref->setString(PREF_ROWS, "24");
	   	this->PostMessage (MSG_ROWS_CHANGED);
	   	this->PostMessage (MSG_COLS_CHANGED);
	break;	
	
	case ONETHREETWOTWENTYFIVE:
		gTermPref->setString(PREF_COLS, "132");
		gTermPref->setString(PREF_ROWS, "25");
	   	this->PostMessage (MSG_ROWS_CHANGED);
	   	this->PostMessage (MSG_COLS_CHANGED);
	break;	
	
	case MSG_FONT_CHANGED:
    	gTermPref->setString (PREF_HALF_FONT_FAMILY, fNewFontMenu->FindMarked()->Label());
    	this->PostMessage (MSG_HALF_FONT_CHANGED);
    break;


	case MSG_COLOR_CHANGED:
		fBaseView->SetViewColor (gTermPref->getRGB (PREF_TEXT_BACK_COLOR));
		fTermView->SetTermColor ();
		fBaseView->Invalidate();
		fTermView->Invalidate();
		break;
	case SAVE_AS_DEFAULT:
		{
			BPath path;
			if (PrefHandler::GetDefaultPath(path) == B_OK)
				gTermPref->SaveAsText(path.Path(), PREFFILE_MIMETYPE);
		}
		break;
	case MENU_PAGE_SETUP:
		DoPageSetup ();
		break;
	case MENU_PRINT:
		DoPrint ();
		break;

  case MSGRUN_WINDOW:
    fTermView->UpdateSIGWINCH ();
    break;

	case B_ABOUT_REQUESTED:
		be_app->PostMessage(B_ABOUT_REQUESTED);
		break;

  default:
    BWindow::MessageReceived(message);
    break;
  }
}
////////////////////////////////////////////////////////////////////////////
// WindowActivated (bool)
//  Dispatch Mesasge.
////////////////////////////////////////////////////////////////////////////
void
TermWindow::WindowActivated (bool )
{

}

////////////////////////////////////////////////////////////////////////////
// Quit (void)
//  Quit Application.
////////////////////////////////////////////////////////////////////////////
//void 
//TermWindow::colRequested() {
//	colWindow *colW=new colWindow("Colours for Terminal");
//	colW->Show();
//	}


void
TermWindow::Quit(void)
{
  delete fTermParse;
  delete fCodeConv;
  if(fPrefWindow) fPrefWindow->PostMessage (B_QUIT_REQUESTED);
    be_app->PostMessage (B_QUIT_REQUESTED, be_app);
  BWindow::Quit ();
}


bool
TermWindow::QuitRequested(void)
{
  
  return true;
}
////////////////////////////////////////////////////////////////////////////
// int GetTimeZone (void)
//  Get Machine Timezone.
////////////////////////////////////////////////////////////////////////////
int
TermWindow::GetTimeZone ()
{
  struct timeval tv;
  struct timezone tm;

  gettimeofday (&tv, &tm);

  return -tm.tz_minuteswest / 60;
}


#include "spawn.h"


void
TermWindow::TermWinActivate()
{
	Activate();

#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
	if (focus_follows_mouse()) {
		BPoint aMouseLoc = Frame().LeftTop();
		set_mouse_position(int32(aMouseLoc.x + 16), int32(aMouseLoc.y + 2));
		be_app->SetCursor(B_HAND_CURSOR);
	}
#endif
}


status_t
TermWindow::GetSupportedSuites(BMessage *msg) 
{ 
  static property_info prop_list[] = { 
     { "encode",
       {B_GET_PROPERTY, 0},
       {B_DIRECT_SPECIFIER, 0},
       "get muterminal encode"}, 
     { "encode",
       {B_SET_PROPERTY, 0},
       {B_DIRECT_SPECIFIER, 0},
       "set muterminal encode"}, 
     { "tty",
       {B_GET_PROPERTY, 0},
       {B_DIRECT_SPECIFIER, 0},
       "get tty_name."}, 
     { 0  }
     
  };
  msg->AddString("suites", "suite/vnd.naan-termwindow"); 
  BPropertyInfo prop_info(prop_list); 
  msg->AddFlat("messages", &prop_info); 
  return BWindow::GetSupportedSuites(msg); 
}
////////////////////////////////////////////////////////////////////////////
// ResolveSpecifier
//
////////////////////////////////////////////////////////////////////////////
BHandler*
TermWindow::ResolveSpecifier(BMessage *msg, int32 index,
           BMessage *specifier, int32 form,
           const char *property)
{
  if ( (strcmp(property, "encode") == 0) 
    && ((msg->what == B_SET_PROPERTY) || (msg->what == B_GET_PROPERTY) )) 
      return this;
  else if ( (strcmp(property, "tty") == 0) 
    &&  (msg->what == B_GET_PROPERTY) ) 
      return this;

  return BWindow::ResolveSpecifier(msg, index, specifier, form, property);
}

////////////////////////////////////////////////////////////////////////////
// SetCoding
//  Set coding utility functions.
////////////////////////////////////////////////////////////////////////////
void SetCoding (int coding)
{
  const etable *p = encoding_table;
  p += coding;
  
  gNowCoding = coding;
    
  return;
}
////////////////////////////////////////////////////////////////////////////
// DoPageSetUp ()
//
////////////////////////////////////////////////////////////////////////////
status_t
TermWindow::DoPageSetup() 
{ 
  status_t rv;
  BPrintJob job("PageSetup");

  /* display the page configure panel */
  rv = job.ConfigPage();

  /* save a pointer to the settings */
  fPrintSettings = job.Settings();

  return rv;
}
  
////////////////////////////////////////////////////////////////////////////
// DoPrint ()
//
////////////////////////////////////////////////////////////////////////////
void
TermWindow::DoPrint()
{
//#if B_BEOS_VERSION < 0x0460
  BPrintJob job("Print"); 

  if((! fPrintSettings) || (DoPageSetup() != B_NO_ERROR)) {
    (new BAlert("Cancel", "Print cancelled.", "OK"))->Go();
  return;
  }
  
  job.SetSettings(new BMessage(*fPrintSettings));
 
  BRect pageRect = job.PrintableRect();
  BRect curPageRect = pageRect;

  int pHeight = (int)pageRect.Height();
  int pWidth = (int)pageRect.Width();
  float w,h;
  fTermView->GetFrameSize (&w, &h);
  int xPages = (int)ceil(w / pWidth);
  int yPages = (int)ceil(h / pHeight);

   /* engage the print server */
  job.BeginJob();

  /* loop through and draw each page, and write to spool */
  for(int x = 0; x < xPages; x++)
    for(int y = 0; y < yPages; y++){
      curPageRect.OffsetTo(x * pWidth, y * pHeight);
      job.DrawView(fTermView, curPageRect, BPoint(0, 0));
      job.SpoolPage();
    
      if(!job.CanContinue()){
      	  // It is likely that the only way that the job was cancelled is
      	  // because the user hit 'Cancel' in the page setup window, in which
      	  // case, the user does *not* need to be told that it was cancelled.
      	  // He/she will simply expect that it was done.
//        (new BAlert("Cancel", "Print job cancelled", "OK"))->Go();
        return;
      }
  }

  /* commit the job, send the spool file */
  job.CommitJob();
//#endif 
}

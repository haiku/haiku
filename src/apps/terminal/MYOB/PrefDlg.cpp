/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <Box.h>
#include <Button.h>
#include <TabView.h>
#include <FilePanel.h>
#include <Screen.h>
#include <Alert.h>
#include <storage/Path.h>
#include <stdio.h>

#include "PrefHandler.h"
#include "PrefDlg.h"
#include "TermConst.h"
#include "TermView.h"
#include "TermWindow.h"
#include "MenuUtil.h"

#include "AppearPrefView.h"
#include "ShellPrefView.h"
#include "PrefView.h"
#include "spawn.h"


// Gloval Preference Handler
extern PrefHandler *gTermPref;


//////////////////////////////////////////////////////////////////////////////
// PrefDlg
// 	Constructer
//////////////////////////////////////////////////////////////////////////////
PrefDlg::PrefDlg(TermWindow *inWindow)
  : BWindow(CenteredRect(BRect(0, 0, 350, 250)), "Preference",
	    B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
	    B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{

  fTermWindow = inWindow;
  fPrefTemp = new PrefHandler(gTermPref);
  fDirty = false;
  fSavePanel = NULL;
  
  SetupContent();
  Show();

  AddShortcut ((ulong)'Q', (ulong)B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
  AddShortcut ((ulong)'W', (ulong)B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
}

//////////////////////////////////////////////////////////////////////////////
// PrefDlg
//////////////////////////////////////////////////////////////////////////////
PrefDlg::~PrefDlg (void)
{
}

//////////////////////////////////////////////////////////////////////////////
// SetupContent()
// Set up content of window.
//////////////////////////////////////////////////////////////////////////////
void
PrefDlg::SetupContent()
{
  BRect r;

  Lock();

  BView *top = new BView(Bounds(), "topview", B_FOLLOW_NONE, B_WILL_DRAW);
  top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
  AddChild (top);
  
  //
  // Tab View
  //
  r = top->Bounds();
  r.bottom -= 40;
  r.InsetBy (0, 5);

  fTabView = new BTabView(r, "tab_view", B_WIDTH_FROM_WIDEST); 
  fTabView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

  r = fTabView->Bounds(); 
  r.InsetBy(10,10); 
  r.bottom -= fTabView->TabHeight(); 
  
  fTabView->AddTab(new AppearancePrefView (r, "Appearance", fTermWindow));
  fTabView->AddTab(new ShellPrefView (r, "Shell", fTermWindow));

  //
  // OK, Apply and Cancel button.
  //

	fSaveAsFileButton = new BButton(BRect(50,215,150,20), "okbutton", "Save", new BMessage(MSG_SAVEAS_PRESSED), B_FOLLOW_TOP, B_WILL_DRAW);
	top->AddChild(fSaveAsFileButton);
	fRevertButton = new BButton(BRect(180,215,250,20), "applybutton", "Cancel", new BMessage(MSG_REVERT_PRESSED), B_FOLLOW_TOP, B_WILL_DRAW);
	top->AddChild(fRevertButton); 
	fSaveButton = new BButton(BRect(260,215,330,20), "cancelbutton", "OK", new BMessage(MSG_SAVE_PRESSED), B_FOLLOW_TOP, B_WILL_DRAW);
	top->AddChild(fSaveButton); 		 


  top->AddChild (fTabView);

  Unlock();
}

//////////////////////////////////////////////////////////////////////////////
// if OK button pressed
//
//////////////////////////////////////////////////////////////////////////////
void
PrefDlg::Quit()
{
  fTermWindow->PostMessage(MSG_PREF_CLOSED);
  delete fPrefTemp;
  delete fSavePanel;
  BWindow::Quit();
}
//////////////////////////////////////////////////////////////////////////////
// QuitRequested
// if close box checked and perf is changed..
//////////////////////////////////////////////////////////////////////////////
bool
PrefDlg::QuitRequested ()
{
  if(!fDirty) return true;
  
  BAlert *alert = new BAlert("",
			     "Save changes to this preference panel?",
			     "Cancel", "Don't save",
			     "Save",
			     B_WIDTH_AS_USUAL,
			     B_OFFSET_SPACING,
			     B_WARNING_ALERT); 
  alert->SetShortcut(0, B_ESCAPE); 
  int32 button_index = alert->Go();

  switch (button_index) {

  case 0:	// Cancel 
    return false;
    break;

  case 2:	// Save
    this->doSave();
    break;

  case 1:	// Don't Save
  default:
    break;
  }
  
  return true;
}
//////////////////////////////////////////////////////////////////////////////
// if SaveAs button pressed
//
//////////////////////////////////////////////////////////////////////////////
void
PrefDlg::doSaveAs (void)
{
  for (int32 i = 0; i < fTabView->CountTabs(); i++) {
  //  PrefView *aView = (PrefView *)fTabView->ViewForTab(i);
  //  aView->SaveIfModified ();
  }

  if(!fSavePanel){
    fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this));
  }
 
  fSavePanel->Show();
}
//////////////////////////////////////////////////////////////////////////////
// 
//
//////////////////////////////////////////////////////////////////////////////
void
PrefDlg::SaveRequested(BMessage *msg)
{
  entry_ref dirref;
  const char      *filename;

  msg->FindRef("directory", &dirref);
  msg->FindString("name", &filename);
  BDirectory dir(&dirref);
  BPath path(&dir, filename);
    
  gTermPref->SaveAsText (path.Path(), PREFFILE_MIMETYPE, TERM_SIGNATURE);
}
//////////////////////////////////////////////////////////////////////////////
// if OK button pressed
//
//////////////////////////////////////////////////////////////////////////////
void
PrefDlg::doSave (void)
{
  for (int32 i = 0; i < fTabView->CountTabs(); i++) {
  //  PrefView *aView = (PrefView *)fTabView->ViewForTab(i);
  //  aView->SaveIfModified ();
  }

  delete fPrefTemp;
  fPrefTemp = new PrefHandler (gTermPref);
  
  gTermPref->SaveAsText (TERM_PREF, PREFFILE_MIMETYPE);
  
  fDirty = false;
}
//////////////////////////////////////////////////////////////////////////////
// if cancel button pressed
//
//////////////////////////////////////////////////////////////////////////////
void
PrefDlg::doRevert (void)
{
  BMessenger messenger (fTermWindow);

  delete gTermPref;
  gTermPref = new PrefHandler (fPrefTemp);

  messenger.SendMessage (MSG_HALF_FONT_CHANGED);
  messenger.SendMessage (MSG_COLOR_CHANGED);
  messenger.SendMessage (MSG_ROWS_CHANGED);
  messenger.SendMessage (MSG_INPUT_METHOD_CHANGED);
  
  this->DisableUpdates();
  this->Lock();
  for (int32 i = 0; i < fTabView->CountTabs(); i++) {
  //  PrefView *aView = (PrefView *)fTabView->ViewForTab(i);
 //   aView->Revert ();
  }
  this->Unlock();
  this->EnableUpdates();
 
  fDirty = false;
}
//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////
void
PrefDlg::MessageReceived(BMessage *msg)
{
  switch (msg->what) {  
  case MSG_SAVE_PRESSED:
    doSave();
    Quit();
    break;

  case MSG_SAVEAS_PRESSED:
    doSaveAs();
    break;
    
  case MSG_REVERT_PRESSED:
    doRevert();
    break;
    
  case MSG_PREF_MODIFIED:
    fDirty = true;
    break;

  case B_SAVE_REQUESTED:
    this->SaveRequested(msg);
    break;
    
  default:
    BWindow::MessageReceived(msg);
    break;      
  }
}
//////////////////////////////////////////////////////////////////////////////
// SetupBox
// make Box
//////////////////////////////////////////////////////////////////////////////
BBox *
PrefDlg::SetupBox(BRect r, const char *label, BView *parent)
{
  BBox *b;

  b = new BBox(r);
  b->SetLabel(label);
  parent->AddChild(b);
  return b;
}
//////////////////////////////////////////////////////////////////////////////
// SetupButton
// make Button
//////////////////////////////////////////////////////////////////////////////
//BButton *
//PrefDlg::SetupButton(BRect r, const char *label, ulong msg, BView *parent, bool defaultButton)
//{
//  BButton *b;
//
//  b = new BButton(r, "", label, new BMessage(msg));
//  parent->AddChild(b);
//  
//  if (defaultButton)
//    (parent->Window())->SetDefaultButton(b);
//
//  return b;
//}
//////////////////////////////////////////////////////////////////////////////
//  Ajust Input Rect to locate center of Screen.
//  ( static member function. )
//////////////////////////////////////////////////////////////////////////////
BRect
PrefDlg::CenteredRect(BRect r)
{
  BRect screenRect = BScreen().Frame();

  screenRect.InsetBy(10,10);

  float x = screenRect.left + (screenRect.Width() - r.Width()) / 2;
  float y = screenRect.top + (screenRect.Height() - r.Height()) / 3;

  r.OffsetTo(x, y);
  
  return r;
}

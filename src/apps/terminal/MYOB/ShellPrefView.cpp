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
#include <View.h>
#include <Button.h>
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <stdlib.h>
#include <Beep.h>

#include "ShellPrefView.h"
#include "PrefHandler.h"
#include "TermWindow.h"
#include "TermBuffer.h"
#include "TermConst.h"
#include "MenuUtil.h"
#include "TTextControl.h"

extern PrefHandler *gTermPref;

////////////////////////////////////////////////////////////////////////////
// AparancePrefView ()
//	Constructor.
////////////////////////////////////////////////////////////////////////////
ShellPrefView::ShellPrefView (BRect frame, const char *name,
			      TermWindow *window)
  :PrefView (frame, name)
{
  fTermWindow = window;

	mCols = new TTextControl(BRect(0, 0, 160, 20), "cols", "Columns", "", new BMessage(MSG_COLS_CHANGED));
	AddChild(mCols);
	mCols->SetText(gTermPref->getString(PREF_COLS));

	mRows = new TTextControl(BRect(0, 30, 160, 50), "rows", "Rows", "", new BMessage(MSG_ROWS_CHANGED));
	AddChild(mRows);
	mRows->SetText(gTermPref->getString(PREF_ROWS));

	mHistory = new TTextControl(BRect(0, 60, 160, 80), "history", "History", "", new BMessage(MSG_HISTORY_CHANGED));
	AddChild(mHistory);
	mHistory->SetText (gTermPref->getString (PREF_HISTORY_SIZE));

}
////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////
void
ShellPrefView::Revert (void)
{
  mCols->SetText (gTermPref->getString (PREF_COLS));
  mRows->SetText (gTermPref->getString (PREF_ROWS));
  mHistory->SetText (gTermPref->getString (PREF_HISTORY_SIZE));

}
////////////////////////////////////////////////////////////////////////////
// SaveIfModified (void)
//
////////////////////////////////////////////////////////////////////////////
void
ShellPrefView::SaveIfModified (void)
{
  BMessenger messenger (fTermWindow);

  if (mCols->IsModified()) {
    gTermPref->setString (PREF_COLS, mCols->Text());
    messenger.SendMessage (MSG_COLS_CHANGED);
    mCols->ModifiedText (false);
  }
  if (mRows->IsModified()) {
    gTermPref->setString (PREF_ROWS, mRows->Text());
    messenger.SendMessage (MSG_ROWS_CHANGED);
    mRows->ModifiedText (false);
  }
  //if (mShell->IsModified())
  //  gTermPref->setString (PREF_SHELL, mShell->Text());

  if (mHistory->IsModified()) {
      int size;
      size = atoi(mHistory->Text());
      if (size < 512) mHistory->SetText("512");
      if (size > 1048575) mHistory->SetText("1048575");
      gTermPref->setString (PREF_HISTORY_SIZE, mHistory->Text());
  }

}
//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////
void
ShellPrefView::SetControlLabels (PrefHandler &Labels)
{
  mCols->SetLabel("Columns");
  mRows->SetLabel("Rows");
//  mShell->SetLabel("Shell");
  mHistory->SetLabel("History Size");
}

/*
 *
 * PRIVATE MEMBER FUNCTIONS
 *
 */

////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////
void
ShellPrefView::AttachedToWindow (void)
{
  mCols->SetTarget (this);
  mRows->SetTarget (this);

//  mShell->SetTarget (this);
  mHistory->SetTarget (this);
  
}

////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////
void
ShellPrefView::MessageReceived (BMessage *msg)
{
  bool modified = false;
  int size;

  switch (msg->what) {

    case MSG_COLS_CHANGED:
      size = atoi (mCols->Text());
      if (size >= MAX_COLS || size < MIN_COLS) {
	mCols->SetText (gTermPref->getString (PREF_COLS));
	beep ();
      } else {
	gTermPref->setString (PREF_COLS, mCols->Text());
	modified = true;
      }
      
      break;

  case MSG_ROWS_CHANGED:
    size = atoi (mRows->Text());
    if (size >= MAX_COLS || size < MIN_COLS) {
	mRows->SetText (gTermPref->getString (PREF_ROWS));
	beep ();
      } else {
	gTermPref->setString (PREF_ROWS, mRows->Text());
	modified = true;
      }
      
    break;

//  case MSG_SHELL_CHANGED:
//    gTermPref->setString (PREF_SHELL, mShell->Text());
 //  (this->Window())->PostMessage(MSG_PREF_MODIFIED);
   // break;

  case MSG_HISTORY_CHANGED:
    size = atoi(mHistory->Text());
    
    if (size < 512 || size > 1048575) {
      mHistory->SetText(gTermPref->getString (PREF_HISTORY_SIZE));
      beep ();
    } else {
      gTermPref->setString (PREF_HISTORY_SIZE, mHistory->Text());
      (this->Window())->PostMessage(MSG_PREF_MODIFIED);
    }
    
    break;

  default:
    PrefView::MessageReceived(msg);
    break;      
  }
  
  if(modified){
   fTermWindow->PostMessage (msg);
   (this->Window())->PostMessage(MSG_PREF_MODIFIED);
  }
}

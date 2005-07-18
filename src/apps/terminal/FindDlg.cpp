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

#include <Window.h>
#include <Rect.h>
#include <TextControl.h>
#include <Button.h>
#include <Message.h>
#include <stdio.h>
#include <File.h>
#include <String.h>

#include "FindDlg.h"
#include "TermApp.h"
#include "MenuUtil.h"
#include "PrefHandler.h"

// message define

const uint32 FPANEL_HIDE = 'Fhid';

//////////////////////////////////////////////////////////////////////////////
// FindDlg
// 	Constructer
//////////////////////////////////////////////////////////////////////////////
FindDlg::FindDlg (BRect frame, TermWindow *win)
  : BWindow(frame, "Find",
	    B_FLOATING_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
  PrefHandler title;
  LoadLocaleFile (&title);

  fWindow = win;
  this->SetTitle (title.getString ("Find"));

  AddShortcut ((ulong)'Q', (ulong)B_COMMAND_KEY, new BMessage (FPANEL_HIDE));
  AddShortcut ((ulong)'W', (ulong)B_COMMAND_KEY, new BMessage (FPANEL_HIDE));

  BRect r (10, 10, 120, 20);
  
  BTextControl *text_ctl = new BTextControl (r, "text", NULL, NULL, NULL);
  AddChild (text_ctl);

  r.Set (100, 40, 130, 60);
  BButton *button = new BButton (r, NULL, "Find", new BMessage (MSG_FIND_START));
  AddChild (button);
  
}

FindDlg::~FindDlg (void)
{
  
}

void
FindDlg::Quit (void)
{
  BWindow::Quit ();
}


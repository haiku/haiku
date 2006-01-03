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
#ifndef TERMWIN_H
#define TERMWIN_H

#include <Menu.h>
#include <Window.h>
#include <MessageRunner.h>

#include "PrefHandler.h"

class BFont;
class TermView;
class TermParse;
class CodeConv;
class PrefDlg;
class PrefDlg2;
class FindDlg;


class TermWindow : public BWindow {
	public:
		TermWindow(BRect frame, const char* title);
		~TermWindow();

  void		Quit (void);
  bool		QuitRequested (void);
  void		TermWinActivate (void);
  status_t	GetSupportedSuites (BMessage *msg);
  BHandler*	ResolveSpecifier (BMessage *msg, int32 index,
				  BMessage *specifier, int32 form,
				  const char *property);

private:
  void		InitWindow (void);
  void		SetupMenu (void);
//  void		colRequested (void);
  void		MessageReceived (BMessage *message);
  void		WindowActivated (bool);
  int		GetTimeZone (void);
  void		MenusBeginning(void);
//  void		doShowHelp (uint32 command);
  // Printing
  status_t	DoPageSetup (void);
  void		DoPrint (void);

  /*
   * data member
   */
  TermParse	*fTermParse;
  BMenuBar	*fMenubar;
  BMenu		*fFilemenu, *fEditmenu, *fEncodingmenu, *fHelpmenu, *fFontMenu, *fWindowSizeMenu, *fNewFontMenu; 
  TermView	*fTermView;
  BView		*fBaseView;
  CodeConv	*fCodeConv;
  BMessage	*fPrintSettings;
  PrefDlg	*fPrefWindow;
  FindDlg	*fFindPanel;
  BMessageRunner *fWindowUpdate;
  BMenuItem *item;
};

#endif // TERMWIN_H

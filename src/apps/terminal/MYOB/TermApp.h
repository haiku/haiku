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

#ifndef TERMAPP_H
#define TERMAPP_H

#include <app/Application.h>
#include <String.h>

extern int pfd;
extern char *ptyname;

class TermWindow;
class TermParse;
class BRect;
class AboutDlg;

class TermApp : public BApplication
{
public:
               TermApp (void);
               ~TermApp (void);

private:
  /*
   * Hook functions
   */
  void          ReadyToRun (void);
  void          Quit (void);
  void          MessageReceived (BMessage* msg);
  void          RefsReceived(BMessage *message);
  void          ArgvReceived(int32 argc, char **argv);

  /*
   * Public Member functions.
   */
  void          MakeTermWindow (BRect &frame);
  void          RunNewTerm (void);
  void          SwitchTerm(void);
  void          ActivateTermWindow(team_id id);

  void          Usage(char *name);
  
private:
  bool          IsMinimize (team_id);

  int		fRows, fCols, fXpos, fYpos;
  rgb_color	fFg, fBg, fCurFg, fCurBg, fSelFg, fSelbg;
  rgb_color	fImfg, fImbg, fImSel;
  
  TermWindow    *fTermWindow;
  TermParse     *fTermParse;
  BRect         fTermFrame;
  AboutDlg	*fAboutPanel;

  BString	CommandLine;
  
};

#endif


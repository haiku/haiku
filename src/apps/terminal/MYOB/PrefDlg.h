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
#ifndef PREFDLG_H_INCLUDED
#define PREFDLG_H_INCLUDED

#include <Box.h>
#include <Window.h>
#include "PrefHandler.h"

// local message, so these are in .cpp files....
const ulong MSG_SAVE_PRESSED = 'okok';
const ulong MSG_SAVEAS_PRESSED = 'canl';
const ulong MSG_REVERT_PRESSED = 'revt';
const ulong MSG_PREF_CLOSED = 'mspc';


// Notify PrefDlg closed to TermWindow

class BRect;
class BMessage;
class BTextControl;
class TermWindow;
class PrefView;
class BButton;
class PrefHandler;
class BFilePanel;

class PrefDlg : public BWindow
{
public:
  PrefDlg(TermWindow *inWindow);
  ~PrefDlg ();
  
  void          Quit();

private:
  void		doSave (void);
  void		doSaveAs (void);
  void		doRevert (void);
  void          SaveRequested(BMessage *msg);
  
  bool          QuitRequested ();
  void          MessageReceived (BMessage *msg);

  void          SetupContent();

  BBox*         SetupBox(BRect r, const char *label, BView *parent);
  BButton*      SetupButton(BRect r, const char *label, ulong msg,
			    BView *parent, bool defaultButton=false);

  static BRect	CenteredRect(BRect r);

private:
  TermWindow    *fTermWindow;
  BTabView      *fTabView;
  PrefHandler   *fPrefTemp;
  BFilePanel    *fSavePanel;
  BButton       *fSaveAsFileButton;
  BButton       *fRevertButton;
  BButton       *fSaveButton;
  
  bool          fDirty;
};

#endif //PREFDLG_H_INCLUDED

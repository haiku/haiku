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
#include "PrefView.h"
#include "spawn.h"


// Global Preference Handler
extern PrefHandler *gTermPref;

PrefDlg::PrefDlg(TermWindow *inWindow)
  : BWindow(CenteredRect(BRect(0, 0, 350, 215)), "Preference",
	 	   B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		    B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	fTermWindow = inWindow;
	fPrefTemp = new PrefHandler(gTermPref);
	fDirty = false;
	fSavePanel = NULL;
	
	BRect r;
	
	BView *top = new BView(Bounds(), "topview", B_FOLLOW_NONE, B_WILL_DRAW);
							top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(top);
	
	r=top->Bounds();
	r.bottom *= .75;
	AppearancePrefView *prefView= new AppearancePrefView(r, "Appearance", 
														fTermWindow);
	top->AddChild(prefView);
	
	fSaveAsFileButton = new BButton(BRect(0,0,1,1), "savebutton", 
									"Save to File", 
									new BMessage(MSG_SAVEAS_PRESSED),
									B_FOLLOW_TOP, B_WILL_DRAW);
	fSaveAsFileButton->ResizeToPreferred();
	fSaveAsFileButton->MoveTo(5, top->Bounds().Height() - 5 - 
								fSaveAsFileButton->Bounds().Height());
	top->AddChild(fSaveAsFileButton);
	
	
	fSaveButton = new BButton(BRect(0,0,1,1), "okbutton", "OK", 
								new BMessage(MSG_SAVE_PRESSED), B_FOLLOW_TOP, 
								B_WILL_DRAW);
	fSaveButton->ResizeToPreferred();
	fSaveButton->MoveTo(top->Bounds().Width() - 5 - fSaveButton->Bounds().Width(),
						top->Bounds().Height() - 5 - fSaveButton->Bounds().Height());
	fSaveButton->MakeDefault(true);
	top->AddChild(fSaveButton);
	
	fRevertButton = new BButton(BRect(0,0,1,1), "revertbutton",
								"Cancel", new BMessage(MSG_REVERT_PRESSED), 
								B_FOLLOW_TOP, B_WILL_DRAW);
	fRevertButton->ResizeToPreferred();
	fRevertButton->MoveTo(fSaveButton->Frame().left - 10 - 
							fRevertButton->Bounds().Width(),
							top->Bounds().Height() - 5 - 
							fRevertButton->Bounds().Height());
	top->AddChild(fRevertButton);
	
	AddShortcut ((ulong)'Q', (ulong)B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut ((ulong)'W', (ulong)B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	
	Show();
}

PrefDlg::~PrefDlg (void)
{
}

void
PrefDlg::Quit()
{
	fTermWindow->PostMessage(MSG_PREF_CLOSED);
	delete fPrefTemp;
	delete fSavePanel;
	BWindow::Quit();
}

bool
PrefDlg::QuitRequested()
{
  if(!fDirty)
  	return true;
	
	BAlert *alert = new BAlert("", "Save changes to this preference panel?",
							     "Cancel", "Don't Save", "Save",
							     B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
							     B_WARNING_ALERT); 
	alert->SetShortcut(0, B_ESCAPE); 
	alert->SetShortcut(1, 'd'); 
	alert->SetShortcut(2, 's'); 
	int32 button_index = alert->Go();
	
	if(button_index==0)
		return false;
	else
	if(button_index==2)
		doSave();
	
	return true;
}

void
PrefDlg::doSaveAs (void)
{
	if(!fSavePanel)
		fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this));
	
	fSavePanel->Show();
}

void
PrefDlg::SaveRequested(BMessage *msg)
{
	entry_ref dirref;
	const char *filename;

	msg->FindRef("directory", &dirref);
	msg->FindString("name", &filename);

	BDirectory dir(&dirref);
	BPath path(&dir, filename);

	gTermPref->SaveAsText(path.Path(), PREFFILE_MIMETYPE, TERM_SIGNATURE);
}


void
PrefDlg::doSave()
{
	delete fPrefTemp;
	fPrefTemp = new PrefHandler(gTermPref);

	BPath path;
	if (PrefHandler::GetDefaultPath(path) == B_OK) {
		gTermPref->SaveAsText(path.Path(), PREFFILE_MIMETYPE);
		fDirty = false;
	}
}


void
PrefDlg::doRevert()
{
	BMessenger messenger (fTermWindow);

	delete gTermPref;
	gTermPref = new PrefHandler(fPrefTemp);

	messenger.SendMessage(MSG_HALF_FONT_CHANGED);
	messenger.SendMessage(MSG_COLOR_CHANGED);
	messenger.SendMessage(MSG_ROWS_CHANGED);
	messenger.SendMessage(MSG_INPUT_METHOD_CHANGED);

	fDirty = false;
}

void
PrefDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case MSG_SAVE_PRESSED:
		{
			doSave();
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case MSG_SAVEAS_PRESSED:
		{
			doSaveAs();
			break;
		}
		case MSG_REVERT_PRESSED:
		{
			doRevert();
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case MSG_PREF_MODIFIED:
		{
			fDirty = true;
			break;
		}
		case B_SAVE_REQUESTED:
		{
			SaveRequested(msg);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;      
		}
	}
}

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

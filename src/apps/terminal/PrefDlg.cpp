/*
 * Copyright 2001-2007, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai. 
 * All rights reserved. Distributed under the terms of the MIT license.
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
#include "MenuUtil.h"

#include "AppearPrefView.h"
#include "PrefView.h"
#include "spawn.h"


// Global Preference Handler
extern PrefHandler *gTermPref;

PrefDlg::PrefDlg(BMessenger messenger)
	: BWindow(CenteredRect(BRect(0, 0, 350, 215)), "Terminal Settings",
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fPrefDlgMessenger(messenger)
{
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
		fPrefDlgMessenger);
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


PrefDlg::~PrefDlg()
{
}


void
PrefDlg::Quit()
{
	fPrefDlgMessenger.SendMessage(MSG_PREF_CLOSED);
	delete fPrefTemp;
	delete fSavePanel;
	BWindow::Quit();
}


bool
PrefDlg::QuitRequested()
{
	if (!fDirty)
		return true;

	BAlert *alert = new BAlert("", "Save changes to this preference panel?",
		"Cancel", "Don't Save", "Save",
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
		B_WARNING_ALERT); 
	alert->SetShortcut(0, B_ESCAPE); 
	alert->SetShortcut(1, 'd'); 
	alert->SetShortcut(2, 's'); 

	int32 index = alert->Go();
	if (index == 0)
		return false;

	if (index == 2)
		_Save();

	return true;
}


void
PrefDlg::_SaveAs()
{
	if (!fSavePanel)
		fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this));
	
	fSavePanel->Show();
}


void
PrefDlg::_SaveRequested(BMessage *msg)
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
PrefDlg::_Save()
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
PrefDlg::_Revert()
{
	delete gTermPref;
	gTermPref = new PrefHandler(fPrefTemp);

	fPrefDlgMessenger.SendMessage(MSG_HALF_FONT_CHANGED);
	fPrefDlgMessenger.SendMessage(MSG_COLOR_CHANGED);
	fPrefDlgMessenger.SendMessage(MSG_INPUT_METHOD_CHANGED);

	fDirty = false;
}


void
PrefDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_SAVE_PRESSED:
			_Save();
			PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_SAVEAS_PRESSED:
			_SaveAs();
			break;

		case MSG_REVERT_PRESSED:
			_Revert();
			PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_PREF_MODIFIED:
			fDirty = true;
			break;

		case B_SAVE_REQUESTED:
			_SaveRequested(msg);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
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

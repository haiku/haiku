/*
 * Copyright 2007-2009, Haiku, Inc. All rights reserved.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AppearPrefView.h"
#include "PrefHandler.h"
#include "PrefWindow.h"
#include "TermConst.h"

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <FilePanel.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Path.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal PrefWindow"

PrefWindow::PrefWindow(const BMessenger& messenger)
	:
	BWindow(BRect(0, 0, 375, 185), "Terminal settings",
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fPreviousPref(new PrefHandler(PrefHandler::Default())),
	fSavePanel(NULL),
	fDirty(false),
	fTerminalMessenger(messenger)
{
	BString app = B_TRANSLATE_SYSTEM_NAME("Terminal");
	BString title = B_TRANSLATE_COMMENT("%app% settings", "window title");
	title.ReplaceFirst("%app%", app);
	SetTitle(title);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_VERTICAL)
		.SetInsets(B_USE_WINDOW_SPACING)
			.Add(fAppearanceView = new AppearancePrefView(
				B_TRANSLATE("Appearance"), fTerminalMessenger))
			.AddGroup(B_HORIZONTAL)
				.Add(fDefaultsButton = new BButton("defaultsbutton",
					B_TRANSLATE("Defaults"), new BMessage(MSG_DEFAULTS_PRESSED),
					B_WILL_DRAW))
				.Add(fRevertButton = new BButton("revertbutton",
					B_TRANSLATE("Revert"), new BMessage(MSG_REVERT_PRESSED),
					B_WILL_DRAW))
				.AddGlue()
				.Add(fSaveAsFileButton = new BButton("savebutton",
					B_TRANSLATE("Save to file" B_UTF8_ELLIPSIS),
					new BMessage(MSG_SAVEAS_PRESSED), B_WILL_DRAW))
			.End()
		.End();

	fRevertButton->SetEnabled(fDirty);

	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	CenterOnScreen();
	Show();
}


PrefWindow::~PrefWindow()
{
}


void
PrefWindow::Quit()
{
	fTerminalMessenger.SendMessage(MSG_PREF_CLOSED);
	delete fPreviousPref;
	delete fSavePanel;
	BWindow::Quit();
}


bool
PrefWindow::QuitRequested()
{
	if (fDirty)
		_Save();

	return true;
}


void
PrefWindow::_SaveAs()
{
	if (!fSavePanel) {
		BMessenger messenger(this);
		fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger);
	}

	fSavePanel->Show();
}


void
PrefWindow::_SaveRequested(BMessage *msg)
{
	entry_ref dirref;
	const char* filename;

	msg->FindRef("directory", &dirref);
	msg->FindString("name", &filename);

	BDirectory dir(&dirref);
	BPath path(&dir, filename);

	PrefHandler::Default()->SaveAsText(path.Path(), PREFFILE_MIMETYPE,
		TERM_SIGNATURE);
}


void
PrefWindow::_Save()
{
	delete fPreviousPref;
	fPreviousPref = new PrefHandler(PrefHandler::Default());

	PrefHandler::Default()->SaveDefaultAsText();
	fDirty = false;
}


void
PrefWindow::_Revert()
{
	if (fDirty) {
		PrefHandler::SetDefault(new PrefHandler(fPreviousPref));

		fTerminalMessenger.SendMessage(MSG_HALF_FONT_CHANGED);
		fTerminalMessenger.SendMessage(MSG_COLOR_CHANGED);
		fTerminalMessenger.SendMessage(MSG_TAB_TITLE_SETTING_CHANGED);
		fTerminalMessenger.SendMessage(MSG_WINDOW_TITLE_SETTING_CHANGED);

		fAppearanceView->Revert();

		fDirty = false;
		fRevertButton->SetEnabled(fDirty);
	}
}


void
PrefWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_SAVEAS_PRESSED:
			_SaveAs();
			break;

		case MSG_REVERT_PRESSED:
			_Revert();
			break;

		case MSG_DEFAULTS_PRESSED:
			PrefHandler::SetDefault(new PrefHandler(false));
			fTerminalMessenger.SendMessage(MSG_HALF_FONT_CHANGED);
			fTerminalMessenger.SendMessage(MSG_COLOR_CHANGED);
			fTerminalMessenger.SendMessage(MSG_TAB_TITLE_SETTING_CHANGED);
			fTerminalMessenger.SendMessage(MSG_WINDOW_TITLE_SETTING_CHANGED);
			fAppearanceView->Revert();
			// fallthrough

		case MSG_PREF_MODIFIED:
			fDirty = true;
			fRevertButton->SetEnabled(fDirty);
			break;

		case B_SAVE_REQUESTED:
			_SaveRequested(msg);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

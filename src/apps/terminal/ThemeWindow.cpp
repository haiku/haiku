/*
 * Copyright 2022, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ThemeWindow.h"

#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <Path.h>
#include <SeparatorView.h>

#include "PrefHandler.h"
#include "TermConst.h"
#include "ThemeView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal ThemeWindow"

// local messages
const uint32 MSG_DEFAULTS_PRESSED = 'defl';
const uint32 MSG_SAVEAS_PRESSED = 'canl';
const uint32 MSG_REVERT_PRESSED = 'revt';


ThemeWindow::ThemeWindow(const BMessenger& messenger)
	:
	BWindow(BRect(0, 0, 0, 0), B_TRANSLATE("Colors"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
		fPreviousPref(new PrefHandler(PrefHandler::Default())),
		fSavePanel(NULL),
		fDirty(false),
		fTerminalMessenger(messenger)
{
	fDefaultsButton = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(MSG_DEFAULTS_PRESSED), B_WILL_DRAW);

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(MSG_REVERT_PRESSED), B_WILL_DRAW);

	fSaveAsFileButton = new BButton("savebutton",
		B_TRANSLATE("Save to file" B_UTF8_ELLIPSIS),
		new BMessage(MSG_SAVEAS_PRESSED), B_WILL_DRAW);

	fThemeView = new ThemeView("Theme", fTerminalMessenger);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(fThemeView)
		.AddGroup(B_HORIZONTAL)
			.Add(fDefaultsButton)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fSaveAsFileButton)
			.SetInsets(0, B_USE_DEFAULT_SPACING, 0, 0);

	fRevertButton->SetEnabled(fDirty);

	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	CenterOnScreen();
	Show();
}


void
ThemeWindow::Quit()
{
	fTerminalMessenger.SendMessage(MSG_THEME_CLOSED);
	delete fPreviousPref;
	delete fSavePanel;
	BWindow::Quit();
}


bool
ThemeWindow::QuitRequested()
{
	if (fDirty)
		_Save();

	return true;
}


void
ThemeWindow::_SaveAs()
{
	if (!fSavePanel) {
		BMessenger messenger(this);
		fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger);
	}

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("Terminal/Themes");
		create_directory(path.Path(), 0755);
		fSavePanel->SetPanelDirectory(path.Path());
	}

	fSavePanel->Show();
}


void
ThemeWindow::_SaveRequested(BMessage *msg)
{
	entry_ref dirref;
	const char* filename;

	msg->FindRef("directory", &dirref);
	msg->FindString("name", &filename);

	BDirectory dir(&dirref);
	BPath path(&dir, filename);

	PrefHandler *prefHandler = PrefHandler::Default();

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	char buffer[512];

	for (const char** table = ThemeView::kColorTable; *table != NULL; ++table) {
		int len = snprintf(buffer, sizeof(buffer), "\"%s\" , \"%s\"\n",
			*table, prefHandler->getString(*table));
		file.Write(buffer, len);
	}

	// Name the theme after the filename
	int len = snprintf(buffer, sizeof(buffer), "\"%s\" , \"%s\"\n",
		PREF_THEME_NAME, filename);
	file.Write(buffer, len);

	fThemeView->UpdateMenu();
}


void
ThemeWindow::_Save()
{
	delete fPreviousPref;
	fPreviousPref = new PrefHandler(PrefHandler::Default());

	PrefHandler::Default()->SaveDefaultAsText();
	fDirty = false;
}


void
ThemeWindow::_Revert()
{
	if (fDirty) {
		PrefHandler::SetDefault(new PrefHandler(fPreviousPref));

		fThemeView->Revert();

		fDirty = false;
		fRevertButton->SetEnabled(fDirty);
	}
}


void
ThemeWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {

		case MSG_SAVEAS_PRESSED:
			_SaveAs();
			break;

		case MSG_REVERT_PRESSED:
			_Revert();
			break;

		case MSG_DEFAULTS_PRESSED:
			PrefHandler::SetDefault(new PrefHandler(false));
			fThemeView->SetDefaults();
			// fallthrough

		case MSG_THEME_MODIFIED:
			fDirty = true;
			fRevertButton->SetEnabled(fDirty);
			break;

		case B_SAVE_REQUESTED:
			_SaveRequested(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

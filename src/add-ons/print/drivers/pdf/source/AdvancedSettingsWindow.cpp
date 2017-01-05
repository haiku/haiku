/*

AdvancedSettingsWindow.cpp

Copyright (c) 2002 OpenBeOS.

Author:
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include "AdvancedSettingsWindow.h"
#include "PrintUtils.h"


#include <Box.h>
#include <Button.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <SeparatorView.h>
#include <TextControl.h>


static BMessage* BorderWidthMessage(uint32 what, float width)
{
	BMessage* m = new BMessage(what);
	m->AddFloat("width", width);
	return m;
}


AdvancedSettingsWindow::AdvancedSettingsWindow(BMessage *settings)
	: HWindow(BRect(0, 0, 450, 180), "Advanced settings", B_TITLED_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	 fSettings(settings)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	BString licenseKey;
	if (settings->FindString("pdflib_license_key", &licenseKey) != B_OK)
		licenseKey = "Not supported.";

	if (settings->FindBool("create_web_links", &fCreateLinks) != B_OK)
		fCreateLinks = false;

	if (settings->FindFloat("link_border_width", &fLinkBorderWidth))
		fLinkBorderWidth = 1;

	if (settings->FindBool("create_bookmarks", &fCreateBookmarks) != B_OK)
		fCreateBookmarks = false;

	if (settings->FindString("xrefs_file", &fXRefs) != B_OK)
		fXRefs = "";

	if (settings->FindString("bookmark_definition_file", &fBookmarkDefinition) != B_OK)
		fBookmarkDefinition = "";

	if (settings->FindBool("create_xrefs", &fCreateXRefs) != B_OK)
		fCreateXRefs = false;

	if (settings->FindInt32("close_option", (int32*)&fCloseOption) != B_OK)
		fCloseOption = kNever;

	fLicenseKey = new BTextControl("pdflib_license_key",
		"PDFlib license key: ", "", NULL);
	fLicenseKey->SetEnabled(false);
	fLicenseKey->TextView()->HideTyping(true);
	fLicenseKey->TextView()->SetText(licenseKey.String());

	// Hide the license key field as it is only supported in the commercial
	// version of of pdflib. Leave it here for layout atm.
	fLicenseKey->Hide();
	// border link

	// bounds.OffsetBy(0.0, fLicenseKey->Bounds().Height() + 10.0);
	BPopUpMenu* menuLinkBorder = new BPopUpMenu("Link Border");
	menuLinkBorder->SetRadioMode(true);

	BMenuField* menuFieldLinkBorder = new BMenuField("link_border_width_menu",
		"Link border:", menuLinkBorder);
	menuFieldLinkBorder->Menu()->SetLabelFromMarked(true);

	BMenuItem* item;
	menuLinkBorder->AddItem(item = new BMenuItem("None",
		new BMessage(CREATE_LINKS_MSG)));

	item->SetMarked(!fCreateLinks);
	menuLinkBorder->AddSeparatorItem();

	menuLinkBorder->AddItem(item = new BMenuItem("No border",
		BorderWidthMessage(LINK_BORDER_MSG, 0.0)));

	if (fCreateLinks && fLinkBorderWidth == 0)
		item->SetMarked(true);

	menuLinkBorder->AddItem(item = new BMenuItem("Normal border",
		BorderWidthMessage(LINK_BORDER_MSG, 1.0)));

	if (fCreateLinks && fLinkBorderWidth == 1)
		item->SetMarked(true);

	menuLinkBorder->AddItem(item = new BMenuItem("Bold border",
		BorderWidthMessage(LINK_BORDER_MSG, 2.0)));

	if (fCreateLinks && fLinkBorderWidth == 2)
		item->SetMarked(true);


	// bookmarks

	BPopUpMenu* menuBookmark = new BPopUpMenu("Bookmark Definition File");
	menuBookmark->SetRadioMode(true);

	BMenuField* menuFieldBookmark = new BMenuField("definition_menu",
		"Bookmark definition file:", menuBookmark);
	menuFieldBookmark->Menu()->SetLabelFromMarked(true);

	menuBookmark->AddItem(item = new BMenuItem("None",
		new BMessage(CREATE_BOOKMARKS_MSG)));
	item->SetMarked(!fCreateBookmarks);
	menuBookmark->AddSeparatorItem();

	BDirectory bookmarks(_SetupDirectory("bookmarks"));
	if (bookmarks.InitCheck() == B_OK) {
		BEntry entry;
		while (bookmarks.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) == B_OK) {
				menuBookmark->AddItem(item = new BMenuItem(name, new BMessage(DEFINITION_MSG)));
				if (fCreateBookmarks && strcmp(name, fBookmarkDefinition.String()) == 0)
					item->SetMarked(true);
			}
		}
	}

	// cross references

	BPopUpMenu* menuCrossReferences = new BPopUpMenu("Cross References File");
	menuCrossReferences->SetRadioMode(true);

	BMenuField* menuFieldCrossReferences = new BMenuField("xrefs_menu",
		"Cross references file:", menuCrossReferences);
	menuFieldCrossReferences->Menu()->SetLabelFromMarked(true);

	menuCrossReferences->AddItem(item = new BMenuItem("None",
		new BMessage(CREATE_XREFS_MSG)));
	item->SetMarked(!fCreateXRefs);
	menuCrossReferences->AddSeparatorItem();

	BDirectory xrefs(_SetupDirectory("xrefs"));
	if (xrefs.InitCheck() == B_OK) {
		BEntry entry;
		while (xrefs.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) == B_OK) {
				menuCrossReferences->AddItem(item = new BMenuItem(name,
					 new BMessage(XREFS_MSG)));
				if (fCreateXRefs && strcmp(name, fXRefs.String()) == 0)
					item->SetMarked(true);
			}
		}
	}

	BPopUpMenu* menuStatusWindow =
		new BPopUpMenu("Close status window when done");
	menuStatusWindow->SetRadioMode(true);

	BMenuField* menuFieldStatusWindow = new BMenuField("close_option_menu",
		"Close status window when done:", menuStatusWindow);
	menuFieldStatusWindow->Menu()->SetLabelFromMarked(true);

	_AddMenuItem(menuStatusWindow, "Never", kNever);
	_AddMenuItem(menuStatusWindow, "No errors", kNoErrors);
	_AddMenuItem(menuStatusWindow, "No errors or warnings",
		kNoErrorsOrWarnings);
	_AddMenuItem(menuStatusWindow, "No errors, warnings or info",
		kNoErrorsWarningsOrInfo);
	_AddMenuItem(menuStatusWindow, "Always", kAlways);

	BButton *cancelButton = new BButton("cancel", "Cancel",
		new BMessage(CANCEL_MSG));

	BButton *okButton = new BButton("ok", "OK", new BMessage(OK_MSG));
	okButton->MakeDefault(true);

	BButton *openButton = new BButton("openSettingsFolder",
		"Open settings folder" B_UTF8_ELLIPSIS,
		new BMessage(OPEN_SETTINGS_FOLDER_MSG));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.Add(fLicenseKey->CreateLabelLayoutItem(), 0, 0)
			.Add(fLicenseKey->CreateTextViewLayoutItem(), 1, 0)
			.Add(menuFieldLinkBorder->CreateLabelLayoutItem(), 0, 1)
			.Add(menuFieldLinkBorder->CreateMenuBarLayoutItem(), 1, 1)
			.Add(menuFieldBookmark->CreateLabelLayoutItem(), 0, 2)
			.Add(menuFieldBookmark->CreateMenuBarLayoutItem(), 1, 2)
			.Add(menuFieldCrossReferences->CreateLabelLayoutItem(), 0, 3)
			.Add(menuFieldCrossReferences->CreateMenuBarLayoutItem(), 1, 3)
			.Add(menuFieldStatusWindow->CreateLabelLayoutItem(), 0, 4)
			.Add(menuFieldStatusWindow->CreateMenuBarLayoutItem(), 1, 4)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER))
		.AddGroup(B_HORIZONTAL)
			.Add(openButton)
			.AddGlue()
			.Add(cancelButton)
			.Add(okButton)
		.End();


	BRect winFrame(Frame());
	BRect screenFrame(BScreen().Frame());
	MoveTo((screenFrame.right - winFrame.right) / 2,
		(screenFrame.bottom - winFrame.bottom) / 2);
}


void
AdvancedSettingsWindow::MessageReceived(BMessage *msg)
{
	void  *source;
	BMenuItem *item = NULL;
	if (msg->FindPointer("source", &source) == B_OK)
		item = static_cast<BMenuItem*> (source);

	switch (msg->what){
		case OK_MSG: {
			_UpdateSettings();
			Quit();
		}	break;

		case CANCEL_MSG: {
			Quit();
		}	break;

		case CREATE_LINKS_MSG: {
			fCreateLinks = false;
		}	break;

		case LINK_BORDER_MSG: {
				float w;
			if (msg->FindFloat("width", &w) == B_OK) {
				fLinkBorderWidth = w;
				fCreateLinks = true;
			}
		}	break;

		case CREATE_BOOKMARKS_MSG: {
			fCreateBookmarks = false;
		}	break;

		case DEFINITION_MSG: {
			if (item) {
				fBookmarkDefinition = item->Label();
				fCreateBookmarks = true;
			}
		}	break;

		case CREATE_XREFS_MSG: {
			fCreateXRefs = false;
		}	break;

		case XREFS_MSG: {
			if (item) {
				fXRefs = item->Label();
				fCreateXRefs = true;
			}
		}	break;

		case AUTO_CLOSE_MSG: {
			int32 closeOption;
			if (msg->FindInt32("close_option", &closeOption) == B_OK)
				fCloseOption = (CloseOption)closeOption;
		}	break;

		case OPEN_SETTINGS_FOLDER_MSG: {
			BPath path;
			find_directory(B_USER_SETTINGS_DIRECTORY, &path, false);
			path.Append("PDF Writer");

			entry_ref ref;
			get_ref_for_path(path.Path(), &ref);

			BMessenger tracker("application/x-vnd.Be-TRAK");
			if (tracker.IsValid()) {
				BMessage message(B_REFS_RECEIVED);
				message.AddRef("refs", &ref);
				tracker.SendMessage(&message);
			}
		}	break;

		default:
			inherited::MessageReceived(msg);
			break;
	}
}


void
AdvancedSettingsWindow::_UpdateSettings()
{
#if 0
	SetString(fSettings, "pdflib_license_key", fLicenseKey);
#endif

	SetInt32(fSettings, "close_option", fCloseOption);

	SetBool(fSettings, "create_web_links", fCreateLinks);
	SetFloat(fSettings, "link_border_width", fLinkBorderWidth);

	SetString(fSettings, "xrefs_file", fXRefs);
	SetBool(fSettings, "create_xrefs", fCreateXRefs);

	SetBool(fSettings, "create_bookmarks", fCreateBookmarks);
	SetString(fSettings, "bookmark_definition_file", fBookmarkDefinition);
}


BDirectory
AdvancedSettingsWindow::_SetupDirectory(const char* dirName)
{
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

	BDirectory dir(path.Path());
	if (dir.Contains("PDF Writer")) {
		path.Append("PDF Writer");
		dir.SetTo(path.Path());
	} else {
		dir.CreateDirectory("PDF Writer", &dir);
	}

	if (dir.Contains(dirName)) {
		path.Append(dirName);
		dir.SetTo(path.Path());
	} else {
		dir.CreateDirectory(dirName, &dir);
	}
	return dir;
}


void AdvancedSettingsWindow::_AddMenuItem(BPopUpMenu* menu, const char* label,
	CloseOption option)
{
	BMessage* msg = new BMessage(AUTO_CLOSE_MSG);
	msg->AddInt32("close_option", option);
	BMenuItem* item = new BMenuItem(label, msg);
	menu->AddItem(item);
	if (fCloseOption == option)
		item->SetMarked(true);
}

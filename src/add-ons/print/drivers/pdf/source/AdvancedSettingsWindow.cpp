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
#include "Utils.h"


#include <Box.h>
#include <Button.h>
#include <FindDirectory.h>
#include <MenuField.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <TextControl.h>


static BMessage* BorderWidthMessage(uint32 what, float width)
{
	BMessage* m = new BMessage(what);
	m->AddFloat("width", width);
	return m;
}


AdvancedSettingsWindow::AdvancedSettingsWindow(BMessage *settings)
	: HWindow(BRect(0, 0, 400, 180), "Advanced Settings", B_TITLED_WINDOW_LOOK,
 		B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 		B_NOT_ZOOMABLE),
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

	if (settings->FindInt32("close_option", (int32)&fCloseOption) != B_OK)
		fCloseOption = kNever;

	BRect bounds(Bounds());
	BBox *panel = new BBox(bounds, "background", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(panel);

	bounds.InsetBy(10.0, 10.0);
	float divider = be_plain_font->StringWidth("Close status window when done: ");
	fLicenseKey = new BTextControl(bounds, "pdflib_license_key",
		"PDFlib License Key: ", "", NULL);
	panel->AddChild(fLicenseKey);
	fLicenseKey->SetEnabled(false);
	fLicenseKey->ResizeToPreferred();
	fLicenseKey->SetDivider(divider);
	//fLicenseKey->TextView()->HideTyping(true);
	fLicenseKey->TextView()->SetText(licenseKey.String());

	// border link

	bounds.OffsetBy(0.0, fLicenseKey->Bounds().Height() + 10.0);
	BPopUpMenu* m = new BPopUpMenu("Link Border");
	m->SetRadioMode(true);

	BMenuField *mf = new BMenuField(bounds, "link_border_width_menu",
		"Link Border: ", m);
	panel->AddChild(mf);
	mf->ResizeToPreferred();
	mf->SetDivider(divider);
	mf->Menu()->SetLabelFromMarked(true);

	BMenuItem* item;
	m->AddItem(item = new BMenuItem("None", new BMessage(CREATE_LINKS_MSG)));
	item->SetMarked(!fCreateLinks);
	m->AddSeparatorItem();

	m->AddItem(item = new BMenuItem("No Border", BorderWidthMessage(LINK_BORDER_MSG, 0.0)));
	if (fCreateLinks && fLinkBorderWidth == 0)
		item->SetMarked(true);

	m->AddItem(item = new BMenuItem("Normal Border", BorderWidthMessage(LINK_BORDER_MSG, 1.0)));
	if (fCreateLinks && fLinkBorderWidth == 1)
		item->SetMarked(true);

	m->AddItem(item = new BMenuItem("Bold Border", BorderWidthMessage(LINK_BORDER_MSG, 2.0)));
	if (fCreateLinks && fLinkBorderWidth == 2)
		item->SetMarked(true);


	// bookmarks

	m = new BPopUpMenu("Bookmark Definition File");
	m->SetRadioMode(true);

	bounds.OffsetBy(0.0, mf->Bounds().Height() + 10.0);
	mf = new BMenuField(bounds, "definition_menu", "Bookmark Definition File: ", m);
	panel->AddChild(mf);
	mf->ResizeToPreferred();
	mf->SetDivider(divider);
	mf->Menu()->SetLabelFromMarked(true);

	m->AddItem(item = new BMenuItem("None", new BMessage(CREATE_BOOKMARKS_MSG)));
	item->SetMarked(!fCreateBookmarks);
	m->AddSeparatorItem();

	BDirectory bookmarks(_SetupDirectory("bookmarks"));
	if (bookmarks.InitCheck() == B_OK) {
		BEntry entry;
		while (bookmarks.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) == B_OK) {
				m->AddItem(item = new BMenuItem(name, new BMessage(DEFINITION_MSG)));
				if (fCreateBookmarks && strcmp(name, fBookmarkDefinition.String()) == 0)
					item->SetMarked(true);
			}
		}
	}

	// cross references

	m = new BPopUpMenu("Cross References File");
	m->SetRadioMode(true);

	bounds.OffsetBy(0.0, mf->Bounds().Height() + 10.0);
	mf = new BMenuField(bounds, "xrefs_menu", "Cross References File: ", m);
	panel->AddChild(mf);
	mf->ResizeToPreferred();
	mf->SetDivider(divider);
	mf->Menu()->SetLabelFromMarked(true);

	m->AddItem(item = new BMenuItem("None", new BMessage(CREATE_XREFS_MSG)));
	item->SetMarked(!fCreateXRefs);
	m->AddSeparatorItem();

	BDirectory xrefs(_SetupDirectory("xrefs"));
	if (xrefs.InitCheck() == B_OK) {
		BEntry entry;
		while (xrefs.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) == B_OK) {
				m->AddItem (item = new BMenuItem(name, new BMessage(XREFS_MSG)));
				if (fCreateXRefs && strcmp(name, fXRefs.String()) == 0)
					item->SetMarked(true);
			}
		}
	}

	m = new BPopUpMenu("Close status window when done");
	m->SetRadioMode(true);

	bounds.OffsetBy(0.0, mf->Bounds().Height() + 10.0);
	mf = new BMenuField(bounds, "close_option_menu", "Close status window when done: ", m);
	panel->AddChild(mf);
	mf->ResizeToPreferred();
	mf->SetDivider(divider);
	mf->Menu()->SetLabelFromMarked(true);

	_AddMenuItem(m, "Never", kNever);
	_AddMenuItem(m, "No Errors", kNoErrors);
	_AddMenuItem(m, "No Errors or Warnings", kNoErrorsOrWarnings);
	_AddMenuItem(m, "No Errors, Warnings or Info", kNoErrorsWarningsOrInfo);
	_AddMenuItem(m, "Always", kAlways);

	bounds = Bounds();
	bounds.InsetBy(5.0, 0.0);
	bounds.top = mf->Frame().bottom + 10.0;
	BBox *line = new BBox(BRect(bounds.left, bounds.top, bounds.right,
		bounds.top + 1.0), NULL, B_FOLLOW_LEFT_RIGHT);
	panel->AddChild(line);

	bounds.InsetBy(5.0, 0.0);
	bounds.OffsetBy(0.0, 11.0);
	BButton *cancel = new BButton(bounds, NULL, "Cancel", new BMessage(CANCEL_MSG));
	panel->AddChild(cancel);
	cancel->ResizeToPreferred();

	BButton *ok = new BButton(bounds, NULL, "OK", new BMessage(OK_MSG));
	panel->AddChild(ok, cancel);
	ok->ResizeToPreferred();

	bounds.right = fLicenseKey->Frame().right;
	ok->MoveTo(bounds.right - ok->Bounds().Width(), ok->Frame().top);

	bounds = ok->Frame();
	cancel->MoveTo(bounds.left - cancel->Bounds().Width() - 10.0, bounds.top);

	ok->MakeDefault(true);
	ResizeTo(bounds.right + 10.0, bounds.bottom + 10.0);

	BButton *button = new BButton(bounds, NULL, "Open Settings Folder" B_UTF8_ELLIPSIS,
		new BMessage(OPEN_SETTINGS_FOLDER_MSG));
	panel->AddChild(button);
	button->ResizeToPreferred();
	button->MoveTo(fLicenseKey->Frame().left, bounds.top);

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
	AddString(fSettings, "pdflib_license_key", fLicenseKey);
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

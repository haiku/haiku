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

#include <InterfaceKit.h>
#include <SupportKit.h>
#include <StorageKit.h>
#include "AdvancedSettingsWindow.h"

static BMessage* BorderWidthMessage(uint32 what, float width)
{
	BMessage* m = new BMessage(what);
	m->AddFloat("width", width);
	return m;
}

// --------------------------------------------------
AdvancedSettingsWindow::AdvancedSettingsWindow(BMessage *settings)
	:	HWindow(BRect(0,0,400,220), "Advanced Settings", B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE)
{
	// ---- Ok, build a default page setup user interface
	BRect		r;
	BBox		*panel;
	BButton		*button;
	BCheckBox   *cb;
	BMenuField  *mf;
	float		x, y, w, h;
	fSettings = settings;
	
	// add a *dialog* background
	r = Bounds();
	panel = new BBox(r, "top_panel", B_FOLLOW_ALL, 
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER);

	x = 5; y = 5; w = r.Width(); h = r.Height();

	// web links
	cb = new BCheckBox(BRect(x, y, x+w-10, y+14), "create_links", "Create links for URLs", new BMessage(CREATE_LINKS_MSG));
	panel->AddChild(cb);
	if (settings->FindBool("create_web_links", &fCreateLinks) != B_OK) fCreateLinks = false;
	cb->SetValue(fCreateLinks ? B_CONTROL_ON : B_CONTROL_OFF);
	y += cb->Bounds().Height() + 5;

	BMenuItem* item;
	if (settings->FindFloat("link_border_width", &fLinkBorderWidth)) fLinkBorderWidth = 1;
	BPopUpMenu* m = new BPopUpMenu("link_border_width");
	m->SetRadioMode(true);
	m->AddItem(item = new BMenuItem("no border", BorderWidthMessage(LINK_BORDER_MSG, 0.0)));
	if (fLinkBorderWidth == 0) item->SetMarked(true);
	m->AddItem(item = new BMenuItem("normal",    BorderWidthMessage(LINK_BORDER_MSG, 1.0)));	
	if (fLinkBorderWidth == 1) item->SetMarked(true);
	m->AddItem(item = new BMenuItem("bold",      BorderWidthMessage(LINK_BORDER_MSG, 2.0)));		
	if (fLinkBorderWidth == 2) item->SetMarked(true);

	mf = new BMenuField(BRect(x, y, x+w-10, y+14), "link_border_width_menu", "Link Border Width:", m);
	mf->ResizeToPreferred();
	panel->AddChild(mf);
	y += mf->Bounds().Height() + 15;
	
	// bookmarks
	if (settings->FindBool("create_bookmarks", &fCreateBookmarks) != B_OK) fCreateBookmarks = false;
	cb = new BCheckBox(BRect(x, y, x+w-10, y+14), "create_bookmarks", "Create bookmarks", new BMessage(CREATE_BOOKMARKS_MSG));
	cb->SetValue(fCreateBookmarks ? B_CONTROL_ON : B_CONTROL_OFF);
	panel->AddChild(cb);
	y += cb->Bounds().Height() + 5;

	m = new BPopUpMenu("definition");
	m->SetRadioMode(true);
	mf = new BMenuField(BRect(x, y, x+w-10, y+14), "definition_menu", "Bookmark Definition File:", m);
	panel->AddChild(mf);
	y += mf->Bounds().Height() + 15;

	if (settings->FindString("bookmark_definition_file", &fBookmarkDefinition) != B_OK) fBookmarkDefinition = "";

	BDirectory	Folder;
	BEntry		entry;
	
	// XXX: B_USER_SETTINGS_DIRECTORY
	Folder.SetTo ("/boot/home/config/settings/PDF Writer/bookmarks/");
	if (Folder.InitCheck() == B_OK) {	
		while (Folder.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) == B_NO_ERROR)
				m->AddItem (item = new BMenuItem(name, new BMessage(DEFINITION_MSG)));
				if (strcmp(name, fBookmarkDefinition.String()) == 0) item->SetMarked(true);
		}
	}	

	// cross references
	if (settings->FindBool("create_xrefs", &fCreateXRefs) != B_OK) fCreateXRefs = false;

	cb = new BCheckBox(BRect(x, y, x+w-10, y+14), "create_xrefs", "Create cross references", new BMessage(CREATE_XREFS_MSG));
	cb->SetValue(fCreateXRefs ? B_CONTROL_ON : B_CONTROL_OFF);
	panel->AddChild(cb);
	y += cb->Bounds().Height() + 5;

	m = new BPopUpMenu("cross references");
	m->SetRadioMode(true);
	mf = new BMenuField(BRect(x, y, x+w-10, y+14), "xrefs_menu", "Cross References File:", m);
	panel->AddChild(mf);

	if (settings->FindString("xrefs_file", &fXRefs) != B_OK) fXRefs = "";

	// XXX: B_USER_SETTINGS_DIRECTORY
	Folder.SetTo ("/boot/home/config/settings/PDF Writer/xrefs/");
	if (Folder.InitCheck() == B_OK) {	
		while (Folder.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) == B_NO_ERROR)
				m->AddItem (item = new BMenuItem(name, new BMessage(XREFS_MSG)));
				if (strcmp(name, fXRefs.String()) == 0) item->SetMarked(true);
		}
	}	

	// add a "OK" button, and make it default
	button 	= new BButton(r, NULL, "OK", new BMessage(OK_MSG), 
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->GetPreferredSize(&w, &h);
	x = r.right - w - 8;
	y = r.bottom - h - 8;
	button->MoveTo(x, y);
	panel->AddChild(button);

	// add a "Cancel button	
	button 	= new BButton(r, NULL, "Cancel", new BMessage(CANCEL_MSG), 
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->GetPreferredSize(&w, &h);
	button->ResizeToPreferred();
	button->MoveTo(x - w - 8, y);
	panel->AddChild(button);

	// add a separator line...
	BBox * line = new BBox(BRect(r.left, y - 9, r.right, y - 8), NULL,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM );
	panel->AddChild(line);

	// Finally, add our panel to window
	AddChild(panel);
	
	MoveTo(320, 320);
}

void
AdvancedSettingsWindow::UpdateSettings()
{
	if (fSettings->HasBool("create_web_links")) {
		fSettings->ReplaceBool("create_web_links", fCreateLinks);
	} else {
		fSettings->AddBool("create_web_links", fCreateLinks);
	}

	if (fSettings->HasFloat("link_border_width")) {
		fSettings->ReplaceFloat("link_border_width", fLinkBorderWidth);
	} else {
		fSettings->AddFloat("link_border_width", fLinkBorderWidth);
	}

	if (fSettings->HasBool("create_bookmarks")) {
		fSettings->ReplaceBool("create_bookmarks", fCreateBookmarks);
	} else {
		fSettings->AddBool("create_bookmarks", fCreateBookmarks);
	}

	if (fSettings->HasString("bookmark_definition_file")) {
		fSettings->ReplaceString("bookmark_definition_file", fBookmarkDefinition.String());
	} else {
		fSettings->AddString("bookmark_definition_file", fBookmarkDefinition.String());
	}

	if (fSettings->HasBool("create_xrefs")) {
		fSettings->ReplaceBool("create_xrefs", fCreateXRefs);
	} else {
		fSettings->AddBool("create_xrefs", fCreateXRefs);
	}

	if (fSettings->HasString("xrefs_file")) {
		fSettings->ReplaceString("xrefs_file", fXRefs.String());
	} else {
		fSettings->AddString("xrefs_file", fXRefs.String());
	}
}


// --------------------------------------------------
void 
AdvancedSettingsWindow::MessageReceived(BMessage *msg)
{
	float w;
	void  *source;

	if (msg->FindPointer("source", &source) != B_OK) source = NULL;
	
	switch (msg->what){
		case OK_MSG: UpdateSettings(); Quit();
			break;
		
		case CANCEL_MSG: Quit();
			break;

		case CREATE_LINKS_MSG:
			if (source) {
				BCheckBox* cb = (BCheckBox*)source;
				fCreateLinks = cb->Value() == B_CONTROL_ON;
			}
			break;
			
		case LINK_BORDER_MSG:
			if (msg->FindFloat("width", &w) == B_OK) {
				fLinkBorderWidth = w;				
			}
			break;
		
		case CREATE_BOOKMARKS_MSG:
			if (source) {
				BCheckBox* cb = (BCheckBox*)source;
				fCreateBookmarks = cb->Value() == B_CONTROL_ON;
			}
			break;
		
		case DEFINITION_MSG:
			if (source) {
				BMenuItem* item = (BMenuItem*)source;
				fBookmarkDefinition = item->Label();
			}
			break;
			
		case CREATE_XREFS_MSG:
			if (source) {
				BCheckBox* cb = (BCheckBox*)source;
				fCreateXRefs = cb->Value() == B_CONTROL_ON;
			}
			break;
		
		case XREFS_MSG:
			if (source) {
				BMenuItem* item = (BMenuItem*)source;
				fXRefs = item->Label();
			}
			break;
			
		default:
			inherited::MessageReceived(msg);
			break;
	}
}


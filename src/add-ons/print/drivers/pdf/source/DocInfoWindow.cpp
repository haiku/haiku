/*

DocInfoWindow.cpp

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
#include "DocInfoWindow.h"
#include "InterfaceUtils.h"
#include <ctype.h>

#if HAVE_FULLVERSION_PDF_LIB
// pdflib 5.x supports password protection and permissions in the commercial version only!
static const PermissionLabels gPermissionLabels[] = {
	PermissionLabels("Prevent printing the file.", "noprint"),
	PermissionLabels("Prevent making any changes.", "nomodify"),
	PermissionLabels("Prevent copying or extracting text or graphics.", "nocopy"),
	PermissionLabels("Prevent adding or changing comments or form fields.", "noannots"),
	PermissionLabels("Prevent form field filling.", "noforms"),
	PermissionLabels("Prevent extracting text of graphics.", "noaccessible"),
	PermissionLabels("Prevent inserting, deleting, or rotating pages and creating bookmarks and thumbnails, even if nomodify hasn't been specified", "noassemble"),
	PermissionLabels("Prevent high-resolution printing.", "nohiresprint")
};

// Implementation of Permissions

Permissions::Permissions() {
	fNofPermissions = sizeof(gPermissionLabels)/sizeof(PermissionLabels);
	fPermissions = new Permission[fNofPermissions];
	for (int i = 0; i < fNofPermissions; i ++) {
		fPermissions[i].SetLabels(&gPermissionLabels[i]);
	}	
}

void Permissions::Decode(const char* s) {
	for (int i = 0; i < fNofPermissions; i ++) {
		bool allowed = strstr(s, At(i)->GetPDFName()) == NULL;
		At(i)->SetAllowed(allowed);
	}
}

void Permissions::Encode(BString* s) {
	bool first = true;
	s->Truncate(0);
	for (int i = 0; i < fNofPermissions; i ++) {
		if (!At(i)->IsAllowed()) {
			if (first) {
				first = false;
			} else {
				s->Append(" ");
			}
			s->Append(At(i)->GetPDFName());
		}
	}
}
#endif

// --------------------------------------------------
DocInfoWindow::DocInfoWindow(BMessage *doc_info)
	:	HWindow(BRect(0,0,400,220), "Document Information", B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE)
{
	// ---- Ok, build a default page setup user interface
	BRect		r;
	// BBox		*panel;
	BTabView    *tabView;
	BString     permissions;
	fDocInfo = doc_info;
	
#if HAVE_FULLVERSION_PDF_LIB
	if (DocInfo()->FindString("permissions", &permissions) == B_OK) {
		fPermissions.Decode(permissions.String());
	}
#endif
	
	r = Bounds();
	tabView = new BTabView(r, "tab_view");

	SetupDocInfoView(CreateTabPanel(tabView, "Information"));
#if HAVE_FULLVERSION_PDF_LIB
	SetupPasswordView(CreateTabPanel(tabView, "Password"));
	SetupPermissionsView(CreateTabPanel(tabView, "Permissions"));
#endif
	
	AddChild(tabView);
	MoveTo(320, 320);
	
	if (fTable->ChildAt(0)) fTable->ChildAt(0)->MakeFocus();
}

BBox*
DocInfoWindow::CreateTabPanel(BTabView* tabView, const char* label) {
	BRect r(tabView->Bounds());
	r.bottom -= tabView->TabHeight();
	// create tab panel
	BBox* panel = new BBox(r, "top_panel", B_FOLLOW_ALL, 
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER);;
	// add panel to tab
	BTab* tab = new BTab();
	tabView->AddTab(panel, tab);
	tab->SetLabel(label);
	return panel;
}


void
DocInfoWindow::SetupButtons(BBox* panel) {
	BButton		*button;
	float		x, y, w, h;
	BRect       r(panel->Bounds());
	
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
}

void
DocInfoWindow::SetupDocInfoView(BBox* panel) {
	// BButton		*button;
	// float		x, y, w, h;

	BRect r(panel->Bounds());
	
	// add list of keys
	fKeyList = new BMenu("Delete Key");
	BMenuField *menu = new BMenuField(BRect(0, 0, 90, 10), "delete", "", fKeyList);
	menu->SetDivider(0);
	panel->AddChild(menu);

	// add table for text controls (document info key and value)
	fTable = new Table(BRect(r.left+5, r.top+5, r.right-5-B_V_SCROLL_BAR_WIDTH, r.bottom-5-B_H_SCROLL_BAR_HEIGHT-30-20), "table", B_FOLLOW_ALL, B_WILL_DRAW);
	fTable->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// add table to ScrollView
	fTableScrollView = new BScrollView("scroll_table", fTable, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);
	panel->AddChild(fTableScrollView);

	// position list of keys
	menu->MoveTo(5, fTableScrollView->Frame().bottom+2);

	BMenu* defaultKeys = new BMenu("Default Keys");
	defaultKeys->AddItem(new BMenuItem("Title",        new BMessage(DEFAULT_KEY_MSG)));
	defaultKeys->AddItem(new BMenuItem("Author",       new BMessage(DEFAULT_KEY_MSG)));
	defaultKeys->AddItem(new BMenuItem("Subject",      new BMessage(DEFAULT_KEY_MSG)));
	defaultKeys->AddItem(new BMenuItem("Keywords",     new BMessage(DEFAULT_KEY_MSG)));
	defaultKeys->AddItem(new BMenuItem("Creator",      new BMessage(DEFAULT_KEY_MSG)));
	// PDFlib sets these itselves:
	// defaultKeys->AddItem(new BMenuItem("Producer",     new BMessage(DEFAULT_KEY_MSG)));
	// defaultKeys->AddItem(new BMenuItem("CreationDate", new BMessage(DEFAULT_KEY_MSG)));
	// Not meaningful to set the modification date at creation time!
	// defaultKeys->AddItem(new BMenuItem("ModDate",      new BMessage(DEFAULT_KEY_MSG)));
	BMenuField *keys = new BMenuField(BRect(0, 0, 90, 10), "add", "", defaultKeys);
	keys->SetDivider(0);
	panel->AddChild(keys);
	keys->MoveTo(menu->Frame().right + 5, menu->Frame().top);

	// add add key text control
	BTextControl *add = new BTextControl(BRect(0, 0, 180, 20), "add", "Add Key:", "", new BMessage(ADD_KEY_MSG));
	add->SetDivider(60);
	panel->AddChild(add);
	add->MoveTo(keys->Frame().right + 5, keys->Frame().top);

	// fill table
	BMessage doc_info;
	fDocInfo->FindMessage("doc_info", &doc_info);
	BuildTable(&doc_info);
	SetupButtons(panel);
}


#if HAVE_FULLVERSION_PDF_LIB
BTextControl*
DocInfoWindow::AddPasswordControl(BRect r, BView* panel, const char* name, const char* label) {
	BString s;
	BTextControl* text;
	if (DocInfo()->FindString(name, &s) != B_OK) s = "";
	text = new BTextControl(r, name, label, "", NULL);
	text->TextView()->HideTyping(true);
	text->TextView()->SetText(s.String());
	panel->AddChild(text);
	return text;
}

// --------------------------------------------------
void
DocInfoWindow::SetupPasswordView(BBox* panel) {
	BRect r(panel->Bounds());
	BRect r1(5, 5, r.Width()-10, 25);
	BString label;
	
	fMasterPassword = AddPasswordControl(r1, panel, "master_password", "Master Password:");
	r1.OffsetBy(0, fMasterPassword->Bounds().Height());
	fUserPassword = AddPasswordControl(r1, panel, "user_password", "User Password:");
	
	float w = max_c(panel->StringWidth(fMasterPassword->Label()), panel->StringWidth(fUserPassword->Label()));
	fMasterPassword->SetDivider(w);
	fUserPassword->SetDivider(w);
	
	SetupButtons(panel);
}

void
DocInfoWindow::SetupPermissionsView(BBox* panel) {

	SetupButtons(panel);
}
#endif

// --------------------------------------------------
bool 
DocInfoWindow::QuitRequested()
{
	return true;
}


// --------------------------------------------------
void 
DocInfoWindow::Quit()
{
	EmptyKeyList();
	inherited::Quit();
}


// --------------------------------------------------
void 
DocInfoWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what){
		case OK_MSG: {
				BMessage doc_info;
				ReadFieldsFromTable(&doc_info);
				DocInfo()->RemoveName("doc_info");
				DocInfo()->AddMessage("doc_info", &doc_info);
#if HAVE_FULLVERSION_PDF_LIB
				ReadPasswords();
				ReadPermissions();
#endif
				Quit();
			}
			break;
		
		case CANCEL_MSG: Quit();
			break;
			
		case DEFAULT_KEY_MSG:
		case ADD_KEY_MSG: AddKey(msg, msg->what == ADD_KEY_MSG);
			break;
		
		case REMOVE_KEY_MSG: RemoveKey(msg);
			break;

		default:
			inherited::MessageReceived(msg);
			break;
	}
}

// --------------------------------------------------
void 
DocInfoWindow::BuildTable(BMessage *docInfo) 
{
	BRect r;
	float y;
	float w;
	float rowHeight;
#ifndef B_BEOS_VERSION_DANO	
	char *name;
#else
	const char *name;
#endif		
	uint32 type;
	int32 count;

	EmptyKeyList();
	while (fTable->ChildAt(0)) {
		BView *child = fTable->ChildAt(0);
		fTable->RemoveChild(child);
		delete child;
	}

	fTable->ScrollTo(0, 0);
	
	r = fTable->Bounds();
	y = 5;
	w = r.Width() - 10;

	for (int32 i = 0; docInfo->GetInfo(B_STRING_TYPE, i, &name, &type, &count) == B_OK; i++) {
		if (type == B_STRING_TYPE) {
			BString value;
			if (docInfo->FindString(name, &value) == B_OK) {
				BString s;
				TextControl* v = new TextControl(BRect(0, 0, w, 20), name, name, value.String(), new BMessage(), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
				float w;
				fTable->AddChild(v);
				v->GetPreferredSize(&w, &rowHeight);
				v->MoveTo(5, y);
				y += rowHeight + 2;

				fKeyList->AddItem(new BMenuItem(name, new BMessage(REMOVE_KEY_MSG)));
			}
		}
	}
	
	BScrollBar *sb = fTableScrollView->ScrollBar(B_VERTICAL);
	if (sb) {
		float th = fTable->Bounds().Height()+1;
		float h = y - th;
		if (h > 0) {
			sb->SetProportion(th / (float)y);
			sb->SetRange(0, h);
			sb->SetSteps(rowHeight + 2 + 2, th);
		} else {
			sb->SetRange(0, 0);
		}
	}
}


#if HAVE_FULLVERSION_PDF_LIB
// --------------------------------------------------
void 
DocInfoWindow::ReadPasswords() 
{
	AddString(DocInfo(), "master_password", fMasterPassword->TextView()->Text());
	AddString(DocInfo(), "user_password", fUserPassword->TextView()->Text());
}

// --------------------------------------------------
void 
DocInfoWindow::ReadPermissions() 
{
	BString permissions;
	fPermissions.Encode(&permissions);
	AddString(DocInfo(), "permissions", permissions.String());
}
#endif


// --------------------------------------------------
void 
DocInfoWindow::ReadFieldsFromTable(BMessage* doc_info) 
{
	BView* child;
	BMessage m;
	for (int32 i = 0; (child = fTable->ChildAt(i)) != NULL; i++) {
		TextControl* t = dynamic_cast<TextControl*>(child);
		if (t) {
			m.AddString(t->Label(), t->Text());
		}
	}
	*doc_info = m;
}


// --------------------------------------------------
bool 
DocInfoWindow::IsValidKey(const char* key) 
{
	if (*key == 0) return false;
	while (*key) {
		if (isspace(*key) || iscntrl(*key)) break;
		key ++;
	}
	return *key == 0;
}


// --------------------------------------------------
void 
DocInfoWindow::AddKey(BMessage *msg, bool textControl) 
{
	void         *p;
	BTextControl *text;
	BMenuItem    *item;
	BString      key;
	BMessage     docInfo;
	if (msg->FindPointer("source", &p) != B_OK || p == NULL) return;
	if (textControl) {
		text = reinterpret_cast<BTextControl*>(p);
		key = text->Text();
	} else {
		item = reinterpret_cast<BMenuItem*>(p);
		key = item->Label();
	}
	
	// key is valid and is not in list already
	if (IsValidKey(key.String())) {
		BMessage docInfo;
		ReadFieldsFromTable(&docInfo);
		if (!docInfo.HasString(key.String())) {
			docInfo.AddString(key.String(), "");
			BuildTable(&docInfo);
		}
	}
}


// --------------------------------------------------
void 
DocInfoWindow::RemoveKey(BMessage *msg) 
{
	void *p;
	BMenuItem *item;
	BString key;
	BMessage docInfo;
	if (msg->FindPointer("source", &p) != B_OK) return;
	item = reinterpret_cast<BMenuItem*>(p);
	if (!item) return;
	key = item->Label();
	
	ReadFieldsFromTable(&docInfo);
	if (docInfo.HasString(key.String())) {
		docInfo.RemoveName(key.String());
		BuildTable(&docInfo);
	}
}

// --------------------------------------------------
void 
DocInfoWindow::EmptyKeyList() 
{
	while (fKeyList->ItemAt(0)) {
		BMenuItem *i = fKeyList->ItemAt(0);
		fKeyList->RemoveItem(i);
		delete i;
	}
}

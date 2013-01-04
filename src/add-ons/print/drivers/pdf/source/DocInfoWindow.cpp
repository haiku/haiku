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

#include "DocInfoWindow.h"
#include "PrintUtils.h"


#include <Box.h>
#include <Button.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Message.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TabView.h>
#include <TextControl.h>


#include <ctype.h>


// #pragma mark -- DocInfoWindow


DocInfoWindow::DocInfoWindow(BMessage *docInfo)
	: HWindow(BRect(0, 0, 400, 250), "Document Information", B_TITLED_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL, B_NOT_MINIMIZABLE | B_CLOSE_ON_ESCAPE),
	fDocInfo(docInfo)
{
	BRect bounds(Bounds());
	BView *background = new BView(bounds, "bachground", B_FOLLOW_ALL, B_WILL_DRAW);
	background->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(background);

	bounds.InsetBy(10.0, 10.0);
	BButton *button = new BButton(bounds, "ok", "OK", new BMessage(OK_MSG),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	background->AddChild(button);
	button->ResizeToPreferred();
	button->MoveTo(bounds.right - button->Bounds().Width(),
		bounds.bottom - button->Bounds().Height());

	BRect buttonFrame(button->Frame());
	button = new BButton(buttonFrame, "cancel", "Cancel", new BMessage(CANCEL_MSG),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	background->AddChild(button);
	button->ResizeToPreferred();
	button->MoveTo(buttonFrame.left - (button->Bounds().Width() + 10.0),
		buttonFrame.top);

	bounds.bottom = buttonFrame.top - 10.0;

#if HAVE_FULLVERSION_PDF_LIB
	BString permissions;
	if (_DocInfo()->FindString("permissions", &permissions) == B_OK)
		fPermissions.Decode(permissions.String());

	BTabView *tabView = new BTabView(bounds, "tabView");
	_SetupDocInfoView(_CreateTabPanel(tabView, "Information"));
	_SetupPasswordView(_CreateTabPanel(tabView, "Password"));
	_SetupPermissionsView(_CreateTabPanel(tabView, "Permissions"));

	background->AddChild(tabView);
#else
	BBox* panel = new BBox(bounds, "top_panel", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_NO_BORDER);

	_SetupDocInfoView(panel);
	background->AddChild(panel);
#endif

	if (fTable->ChildAt(0))
		fTable->ChildAt(0)->MakeFocus();

	BRect winFrame(Frame());
	BRect screenFrame(BScreen().Frame());
	MoveTo((screenFrame.right - winFrame.right) / 2,
		(screenFrame.bottom - winFrame.bottom) / 2);

	SetSizeLimits(400.0, 10000.0, 250.0, 10000.0);
}


void
DocInfoWindow::Quit()
{
	_EmptyKeyList();
	inherited::Quit();
}


bool
DocInfoWindow::QuitRequested()
{
	return true;
}


void
DocInfoWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case OK_MSG: {
				BMessage doc_info;
				_ReadFieldsFromTable(doc_info);
				_DocInfo()->RemoveName("doc_info");
				_DocInfo()->AddMessage("doc_info", &doc_info);
#if HAVE_FULLVERSION_PDF_LIB
				_ReadPasswords();
				_ReadPermissions();
#endif
				Quit();
		}	break;

		case CANCEL_MSG: {
			Quit();
		}	break;

		case ADD_KEY_MSG: {
		case DEFAULT_KEY_MSG:
			_AddKey(msg, msg->what == ADD_KEY_MSG);
		}	break;

		case REMOVE_KEY_MSG: {
			_RemoveKey(msg);
		}	break;

		default: {
			inherited::MessageReceived(msg);
		}	break;
	}
}


void
DocInfoWindow::FrameResized(float newWidth, float newHeight)
{
	BTextControl *textControl = dynamic_cast<BTextControl*> (fTable->ChildAt(0));
	if (textControl) {
		float width, height;
		textControl->GetPreferredSize(&width, &height);

		int32 count = fKeyList->CountItems();
		float fieldsHeight = (height * count) + (2 * count);

		_AdjustScrollBar(height, fieldsHeight);

		while (textControl) {
			textControl->SetDivider(textControl->Bounds().Width() / 2.0);
			textControl = dynamic_cast<BTextControl*> (textControl->NextSibling());
		}
	}
}


void
DocInfoWindow::_SetupDocInfoView(BBox* panel)
{
	BRect bounds(panel->Bounds());
#if HAVE_FULLVERSION_PDF_LIB
	bounds.InsetBy(10.0, 10.0);
#endif

	// add list of keys
	fKeyList = new BMenu("Delete Key");
	BMenuField *menu = new BMenuField(bounds, "delete", "", fKeyList,
		B_FOLLOW_BOTTOM);
	panel->AddChild(menu);
	menu->SetDivider(0);

#ifdef __HAIKU__
	menu->ResizeToPreferred();
	menu->MoveTo(bounds.left, bounds.bottom - menu->Bounds().Height());
#else
	menu->ResizeTo(menu->StringWidth("Delete Key") + 15.0, 25.0);
	menu->MoveTo(bounds.left, bounds.bottom - 25.0);
#endif

	const char* title[6] = { "Title", "Author", "Subject", "Keywords", "Creator",
		NULL };	// PDFlib sets these: "Producer", "CreationDate", not "ModDate"
	BMenu* defaultKeys = new BMenu("Default Keys");
	for (int32 i = 0; title[i] != NULL; ++i)
		defaultKeys->AddItem(new BMenuItem(title[i], new BMessage(DEFAULT_KEY_MSG)));

	BRect frame(menu->Frame());
	menu = new BMenuField(frame, "add", "", defaultKeys, B_FOLLOW_BOTTOM);
	panel->AddChild(menu);
	menu->SetDivider(0);

#ifdef __HAIKU__
	menu->ResizeToPreferred();
	menu->MoveBy(frame.Width() + 10.0, 0.0);
#else
	menu->ResizeTo(menu->StringWidth("Default Keys") + 15.0, 25.0);
	menu->MoveBy(menu->Bounds().Width() + 10.0, 0.0);
#endif

	frame = menu->Frame();
	frame.left = frame.right + 10.0;
	frame.right = bounds.right;
	BTextControl *add = new BTextControl(frame, "add", "Add Key:", "",
		new BMessage(ADD_KEY_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	panel->AddChild(add);

	float width, height;
	add->GetPreferredSize(&width, &height);
	add->ResizeTo(bounds.right - frame.left, height);
	add->SetDivider(be_plain_font->StringWidth("Add Key: "));

	bounds.bottom = frame.top - 10.0;
	bounds.right -= B_V_SCROLL_BAR_WIDTH;
	bounds.InsetBy(2.0, 2.0);
	fTable = new BView(bounds, "table", B_FOLLOW_ALL, B_WILL_DRAW);
	fTable->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTableScrollView = new BScrollView("scroll_table", fTable, B_FOLLOW_ALL, 0,
		false, true);
	panel->AddChild(fTableScrollView);

	BMessage docInfo;
	fDocInfo->FindMessage("doc_info", &docInfo);

	// fill table
	_BuildTable(docInfo);
}


void
DocInfoWindow::_BuildTable(const BMessage& docInfo)
{
	_EmptyKeyList();

	while (fTable->ChildAt(0)) {
		BView *child = fTable->ChildAt(0);
		fTable->RemoveChild(child);
		delete child;
	}

	fTable->ScrollTo(0, 0);

#ifdef B_BEOS_VERSION_DANO
	const
#endif
	char *name;
	uint32 type;
	int32 count;

	float rowHeight = 20.0;
	float fieldsHeight = 2.0;
	float width = fTable->Bounds().Width() - 4.0;
	for (int32 i = 0; docInfo.GetInfo(B_STRING_TYPE, i, &name, &type, &count)
		== B_OK; i++) {
		if (type != B_STRING_TYPE)
			continue;

		BString value;
		BTextControl* textControl;
		if (docInfo.FindString(name, &value) == B_OK) {
			BRect rect(2.0, fieldsHeight, width, rowHeight);
			textControl = _AddFieldToTable(rect, name, value.String());

			rowHeight = textControl->Bounds().Height();
			fieldsHeight += rowHeight + 2.0;
		}
	}

	_AdjustScrollBar(rowHeight, fieldsHeight);
}


BTextControl*
DocInfoWindow::_AddFieldToTable(BRect rect, const char* name, const char* value)
{
	BTextControl *textControl = new BTextControl(rect, name, name, value, NULL,
		B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_NAVIGABLE);
	fTable->AddChild(textControl);
	float width, height;
	textControl->GetPreferredSize(&width, &height);

	textControl->ResizeTo(rect.Width(), height);
	textControl->SetDivider(rect.Width() / 2.0);

	fKeyList->AddItem(new BMenuItem(name, new BMessage(REMOVE_KEY_MSG)));

	return textControl;
}


void
DocInfoWindow::_AdjustScrollBar(float controlHeight, float fieldsHeight)
{
	BScrollBar *sb = fTableScrollView->ScrollBar(B_VERTICAL);
	if (!sb)
		return;

	sb->SetRange(0.0, 0.0);
	float tableHeight = fTable->Bounds().Height();
	if ((fieldsHeight - tableHeight) > 0.0) {
		sb->SetProportion(tableHeight / fieldsHeight);
		sb->SetRange(0.0, fieldsHeight - tableHeight);
		sb->SetSteps(controlHeight + 2.0, tableHeight);
	}
}


void
DocInfoWindow::_ReadFieldsFromTable(BMessage& docInfo)
{
	docInfo.MakeEmpty();

	BView* child;
	for (int32 i = 0; (child = fTable->ChildAt(i)) != NULL; i++) {
		BTextControl* textControl = dynamic_cast<BTextControl*>(child);
		if (textControl)
			docInfo.AddString(textControl->Label(), textControl->Text());
	}
}


void
DocInfoWindow::_RemoveKey(BMessage *msg)
{
	void *p;
	if (msg->FindPointer("source", &p) != B_OK)
		return;

	BMenuItem *item = reinterpret_cast<BMenuItem*>(p);
	if (!item)
		return;

	BMessage docInfo;
	_ReadFieldsFromTable(docInfo);

	const char *label = item->Label();
	if (docInfo.HasString(label)) {
		docInfo.RemoveName(label);
		_BuildTable(docInfo);
	}
}


bool
DocInfoWindow::_IsKeyValid(const char* key) const
{
	if (*key == 0)
		return false;

	while (*key) {
		if (isspace(*key) || iscntrl(*key))
			break;
		key++;
	}
	return *key == 0;
}


void
DocInfoWindow::_AddKey(BMessage *msg, bool textControl)
{
	void *p;
	if (msg->FindPointer("source", &p) != B_OK || p == NULL)
		return;

	const char* key = NULL;
	if (textControl) {
		BTextControl *text = reinterpret_cast<BTextControl*>(p);
		key = text->Text();
	} else {
		BMenuItem *item = reinterpret_cast<BMenuItem*>(p);
		key = item->Label();
	}

	if (!_IsKeyValid(key))
		return;

	BMessage docInfo;
	_ReadFieldsFromTable(docInfo);

	if (!docInfo.HasString(key)) {
		// key is valid and is not in list already
		docInfo.AddString(key, "");

		float width = fTable->Bounds().Width() - 4.0;
		BTextControl *textControl =
			_AddFieldToTable(BRect(2.0, 0.0, width, 20.0), key, "");

		float rowHeight = textControl->Bounds().Height();
		int32 count = fKeyList->CountItems();
		float fieldsHeight = (rowHeight * count) + (2 * count);
		textControl->MoveTo(2.0, fieldsHeight - rowHeight);

		_AdjustScrollBar(rowHeight, fieldsHeight);
	}
}


void
DocInfoWindow::_EmptyKeyList()
{
	while (fKeyList->CountItems() > 0L)
		delete fKeyList->RemoveItem((int32)0);
}


#if HAVE_FULLVERSION_PDF_LIB

void
DocInfoWindow::_SetupPasswordView(BBox* panel)
{
	BRect bounds(panel->Bounds().InsetByCopy(10.0, 10.0));

	fMasterPassword = _AddPasswordControl(bounds, panel, "master_password",
		"Master Password:");
	bounds.OffsetBy(0, fMasterPassword->Bounds().Height());
	fUserPassword = _AddPasswordControl(bounds, panel, "user_password",
		"User Password:");

	float divider = max_c(panel->StringWidth("Master Password:"),
		panel->StringWidth("User Password:"));
	fMasterPassword->SetDivider(divider);
	fUserPassword->SetDivider(divider);
}


void
DocInfoWindow::_SetupPermissionsView(BBox* panel)
{
	(void)panel;
}


BBox*
DocInfoWindow::_CreateTabPanel(BTabView* tabView, const char* label)
{
	BRect rect(tabView->Bounds().InsetByCopy(3.0, 15.0));
	rect.OffsetTo(B_ORIGIN);
	BBox* panel = new BBox(rect, "top_panel", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_NO_BORDER);

	BTab* tab = new BTab();
	tabView->AddTab(panel, tab);
	tab->SetLabel(label);

	return panel;
}


BTextControl*
DocInfoWindow::_AddPasswordControl(BRect r, BView* panel, const char* name,
	const char* label)
{
	BString text;
	_DocInfo()->FindString(name, &text);
	BTextControl* textControl = new BTextControl(r, name, label, "", NULL,
		B_FOLLOW_LEFT_RIGHT);
	panel->AddChild(textControl);
	textControl->ResizeToPreferred();
	textControl->TextView()->HideTyping(true);
	textControl->TextView()->SetText(text.String());

	return textControl;
}


void
DocInfoWindow::_ReadPasswords()
{
	SetString(_DocInfo(), "master_password", fMasterPassword->TextView()->Text());
	SetString(_DocInfo(), "user_password", fUserPassword->TextView()->Text());
}


void
DocInfoWindow::_ReadPermissions()
{
	BString permissions;
	fPermissions.Encode(permissions);

	SetString(_DocInfo(), "permissions", permissions.String());
}


// #pragma mark -- Permissions


// pdflib 5.x supports password protection and permissions in the commercial version only!
static const PermissionLabels gPermissionLabels[] = {
	PermissionLabels("Prevent printing the file.", "noprint"),
	PermissionLabels("Prevent making any changes.", "nomodify"),
	PermissionLabels("Prevent copying or extracting text or graphics.", "nocopy"),
	PermissionLabels("Prevent adding or changing comments or form fields.", "noannots"),
	PermissionLabels("Prevent form field filling.", "noforms"),
	PermissionLabels("Prevent extracting text of graphics.", "noaccessible"),
	PermissionLabels("Prevent inserting, deleting, or rotating pages and creating "
		"bookmarks and thumbnails, even if nomodify hasn't been specified", "noassemble"),
	PermissionLabels("Prevent high-resolution printing.", "nohiresprint")
};

Permissions::Permissions()
{
	fNofPermissions = sizeof(gPermissionLabels) / sizeof(PermissionLabels);
	fPermissions = new Permission[fNofPermissions];
	for (int32 i = 0; i < fNofPermissions; i++)
		fPermissions[i].SetLabels(&gPermissionLabels[i]);
}


void Permissions::Decode(const char* s)
{
	for (int32 i = 0; i < fNofPermissions; i++)
		At(i)->SetAllowed((strstr(s, At(i)->GetPDFName()) == NULL));
}


void Permissions::Encode(BString& permissions)
{
	permissions.Truncate(0);
	for (int32 i = 0; i < fNofPermissions; i++) {
		if (!At(i)->IsAllowed())
			permissions.Append(At(i)->GetPDFName()).Append(" ");
	}
}

#endif	// HAVE_FULLVERSION_PDF_LIB

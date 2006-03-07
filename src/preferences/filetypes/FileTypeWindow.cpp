/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypes.h"
#include "FileTypeWindow.h"
#include "IconView.h"
#include "PreferredAppMenu.h"

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <File.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include <stdio.h>


const uint32 kMsgTypeEntered = 'type';
const uint32 kMsgSelectType = 'sltp';
const uint32 kMsgSameTypeAs = 'stpa';
const uint32 kMsgSameTypeAsOpened = 'stpO';

const uint32 kMsgPreferredAppChosen = 'papc';
const uint32 kMsgSelectPreferredApp = 'slpa';
const uint32 kMsgSamePreferredAppAs = 'spaa';
const uint32 kMsgPreferredAppOpened = 'paOp';
const uint32 kMsgSamePreferredAppAsOpened = 'spaO';


FileTypeWindow::FileTypeWindow(BPoint position, const BMessage& refs)
	: BWindow(BRect(0.0f, 0.0f, 200.0f, 200.0f).OffsetBySelf(position),
		"File Type", B_TITLED_WINDOW,
		B_NOT_V_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BRect rect = Bounds();
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	// "File Type" group

	BFont font(be_bold_font);
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	rect.InsetBy(8.0f, 8.0f);
	BBox* box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("File Type");
	topView->AddChild(box);

	rect = box->Bounds();
	rect.InsetBy(8.0f, 4.0f + fontHeight.ascent + fontHeight.descent);
	fTypeControl = new BTextControl(rect, "type", NULL, NULL,
		new BMessage(kMsgTypeEntered), B_FOLLOW_LEFT_RIGHT);
	fTypeControl->SetDivider(0.0f);
	float width, height;
	fTypeControl->GetPreferredSize(&width, &height);
	fTypeControl->ResizeTo(rect.Width(), height);
	box->AddChild(fTypeControl);

	// filter out invalid characters that can't be part of a MIME type name
	BTextView* textView = fTypeControl->TextView();
	const char* disallowedCharacters = "/<>@,;:\"()[]?=";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		textView->DisallowChar(disallowedCharacters[i]);
	}

	rect.OffsetBy(0.0f, fTypeControl->Bounds().Height() + 5.0f);
	fSelectTypeButton = new BButton(rect, "select type", "Select" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSelectType), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fSelectTypeButton->ResizeToPreferred();
	box->AddChild(fSelectTypeButton);

	rect.OffsetBy(fSelectTypeButton->Bounds().Width() + 8.0f, 0.0f);
	fSameTypeAsButton = new BButton(rect, "same type as", "Same As" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSameTypeAs), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fSameTypeAsButton->ResizeToPreferred();
	box->AddChild(fSameTypeAsButton);

	width = font.StringWidth("Icon") + 16.0f;
	if (width < B_LARGE_ICON + 16.0f)
		width = B_LARGE_ICON + 16.0f;

	height = fSelectTypeButton->Frame().bottom + 8.0f;
	if (height < 8.0f + B_LARGE_ICON + fontHeight.ascent + fontHeight.descent)
		height = 8.0f + B_LARGE_ICON + fontHeight.ascent + fontHeight.descent;
	box->ResizeTo(box->Bounds().Width() - width - 8.0f, height);

	// "Icon" group

	rect = box->Frame();
	rect.left = rect.right + 8.0f;
	rect.right += width + 8.0f;
	float iconBoxWidth = rect.Width();
	box = new BBox(rect, NULL, B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	box->SetLabel("Icon");
	topView->AddChild(box);

	rect = BRect(8.0f, 0.0f, 7.0f + B_LARGE_ICON, B_LARGE_ICON - 1.0f);
	rect.OffsetBy(0.0f, (box->Bounds().Height() - rect.Height()) / 2.0f);
	if (rect.top < fontHeight.ascent + fontHeight.descent + 4.0f)
		rect.top = fontHeight.ascent + fontHeight.descent + 4.0f;
	fIconView = new IconView(rect, "icon");
	box->AddChild(fIconView);

	// "Preferred Application" group

	rect.top = box->Frame().bottom + 8.0f;
	rect.bottom = rect.top + box->Bounds().Height();
	rect.left = 8.0f;
	rect.right = Bounds().Width() - 8.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Preferred Application");
	topView->AddChild(box);

	BMenu* menu = new BPopUpMenu("preferred");
	BMenuItem* item;
	menu->AddItem(item = new BMenuItem("Default Application",
		new BMessage(kMsgPreferredAppChosen)));
	item->SetMarked(true);

	rect = fTypeControl->Frame();
	BView* constrainingView = new BView(rect, NULL, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
	constrainingView->SetViewColor(topView->ViewColor());

	fPreferredField = new BMenuField(rect.OffsetToCopy(B_ORIGIN), "preferred",
		NULL, menu);
	fPreferredField->GetPreferredSize(&width, &height);
	fPreferredField->ResizeTo(rect.Width(), height);
	constrainingView->AddChild(fPreferredField);
		// we embed the menu field in another view to make it behave like
		// we want so that it can't obscure other elements with larger
		// labels

	box->AddChild(constrainingView);

	rect.OffsetBy(0.0f, fTypeControl->Bounds().Height() + 5.0f);
	fSelectAppButton = new BButton(rect, "select app", "Select" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSelectPreferredApp), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fSelectAppButton->ResizeToPreferred();
	box->AddChild(fSelectAppButton);

	rect.OffsetBy(fSelectAppButton->Bounds().Width() + 8.0f, 0.0f);
	fSameAppAsButton = new BButton(rect, "same app as", "Same As" B_UTF8_ELLIPSIS,
		new BMessage(kMsgSamePreferredAppAs), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fSameAppAsButton->ResizeToPreferred();
	box->AddChild(fSameAppAsButton);

	ResizeTo(fSameAppAsButton->Frame().right + 100.0f, box->Frame().bottom + 8.0f);
	SetSizeLimits(fSameAppAsButton->Frame().right + iconBoxWidth + 32.0f, 32767.0f,
		Bounds().Height(), Bounds().Height());

	fTypeControl->MakeFocus(true);

	BMimeType::StartWatching(this);
	_SetTo(refs);
}


FileTypeWindow::~FileTypeWindow()
{
	BMimeType::StopWatching(this);
}


BString
FileTypeWindow::_Title(const BMessage& refs)
{
	BString title;
	entry_ref ref;
	if (refs.FindRef("refs", 1, &ref) == B_OK) {
		bool same = false;
		BEntry entry, parent;
		if (entry.SetTo(&ref) == B_OK
			&& entry.GetParent(&parent) == B_OK) {
			same = true;

			// Check if all entries have the same parent directory
			for (int32 i = 0; refs.FindRef("refs", i, &ref) == B_OK; i++) {
				BEntry directory;
				if (entry.SetTo(&ref) == B_OK
					&& entry.GetParent(&directory) == B_OK) {
					if (directory != parent) {
						same = false;
						break;
					}
				}
			}
		}

		char name[B_FILE_NAME_LENGTH];
		if (same && parent.GetName(name) == B_OK) {
			title = "Multiple Files from \"";
			title.Append(name);
			title.Append("\"");
		} else
			title = "[Multiple Files]";
	} else if (refs.FindRef("refs", 0, &ref) == B_OK)
		title = ref.name;

	title.Append(" File Type");
	return title;
}


void
FileTypeWindow::_SetTo(const BMessage& refs)
{
	SetTitle(_Title(refs).String());

	// get common info and icons

	fCommonPreferredApp = "";
	fCommonType = "";
	entry_ref ref;
	for (int32 i = 0; refs.FindRef("refs", i, &ref) == B_OK; i++) {
		BNode node(&ref);
		if (node.InitCheck() != B_OK)
			continue;

		BNodeInfo info(&node);
		if (info.InitCheck() != B_OK)
			continue;

		// TODO: watch entries?

		entry_ref* copiedRef = new entry_ref(ref);
		fEntries.AddItem(copiedRef);

		char type[B_MIME_TYPE_LENGTH];
		if (info.GetType(type) != B_OK)
			type[0] = '\0';

		if (i > 0) {
			if (fCommonType != type)
				fCommonType = "";
		} else
			fCommonType = type;

		char preferredApp[B_MIME_TYPE_LENGTH];
		if (info.GetPreferredApp(preferredApp) != B_OK)
			preferredApp[0] = '\0';

		if (i > 0) {
			if (fCommonPreferredApp != preferredApp)
				fCommonPreferredApp = "";
		} else
			fCommonPreferredApp = preferredApp;

		if (i == 0)
			fIconView->SetTo(&ref);
	}

	fTypeControl->SetText(fCommonType.String());
	_UpdatePreferredApps();

	fIconView->ShowIconHeap(fEntries.CountItems() != 1);
}


void
FileTypeWindow::_AdoptType(BMessage* message)
{
	entry_ref ref;
	if (message == NULL || message->FindRef("refs", &ref) != B_OK)
		return;

	BNode node(&ref);
	status_t status = node.InitCheck();

	char type[B_MIME_TYPE_LENGTH];

	if (status == B_OK) {
			// get type from file
		BNodeInfo nodeInfo(&node);
		status = nodeInfo.InitCheck();
		if (status == B_OK) {
			if (nodeInfo.GetType(type) != B_OK)
				type[0] = '\0';
		}
	}

	if (status != B_OK) {
		error_alert("Could not open file", status);
		return;
	}

	fCommonType = type;
	fTypeControl->SetText(type);
	_AdoptType();
}


void
FileTypeWindow::_AdoptType()
{
	for (int32 i = 0; i < fEntries.CountItems(); i++) {
		BNode node(fEntries.ItemAt(i));
		BNodeInfo info(&node);
		if (node.InitCheck() != B_OK
			|| info.InitCheck() != B_OK)
			continue;

		info.SetType(fCommonType.String());
	}
}


void
FileTypeWindow::_AdoptPreferredApp(BMessage* message, bool sameAs)
{
	if (retrieve_preferred_app(message, sameAs, fCommonType.String(),
			fCommonPreferredApp) == B_OK) {
		_AdoptPreferredApp();
		_UpdatePreferredApps();
	}
}


void
FileTypeWindow::_AdoptPreferredApp()
{
	for (int32 i = 0; i < fEntries.CountItems(); i++) {
		BNode node(fEntries.ItemAt(i));
		if (fCommonPreferredApp.Length() == 0) {
			node.RemoveAttr("BEOS:PREF_APP");
			continue;
		}

		BNodeInfo info(&node);
		if (node.InitCheck() != B_OK
			|| info.InitCheck() != B_OK)
			continue;

		info.SetPreferredApp(fCommonPreferredApp.String());
	}
}


void
FileTypeWindow::_UpdatePreferredApps()
{
	BMimeType type(fCommonType.String());
	update_preferred_app_menu(fPreferredField->Menu(), &type,
		kMsgPreferredAppChosen, fCommonPreferredApp.String());
}


void
FileTypeWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		// File Type group

		case kMsgTypeEntered:
			fCommonType = fTypeControl->Text();
			_AdoptType();
			break;

		case kMsgSameTypeAs:
		{
			BMessage panel(kMsgOpenFilePanel);
			panel.AddString("title", "Select Same Type As");
			panel.AddInt32("message", kMsgSameTypeAsOpened);
			panel.AddMessenger("target", this);

			be_app_messenger.SendMessage(&panel);
			break;
		}
		case kMsgSameTypeAsOpened:
			_AdoptType(message);
			break;

		// Preferred Application group

		case kMsgPreferredAppChosen:
		{
			const char* signature;
			if (message->FindString("signature", &signature) == B_OK)
				fCommonPreferredApp = signature;
			else
				fCommonPreferredApp = "";

			_AdoptPreferredApp();
			break;
		}

		case kMsgSelectPreferredApp:
		{
			BMessage panel(kMsgOpenFilePanel);
			panel.AddString("title", "Select Preferred Application");
			panel.AddInt32("message", kMsgPreferredAppOpened);
			panel.AddMessenger("target", this);

			be_app_messenger.SendMessage(&panel);
			break;
		}
		case kMsgPreferredAppOpened:
			_AdoptPreferredApp(message, false);
			break;

		case kMsgSamePreferredAppAs:
		{
			BMessage panel(kMsgOpenFilePanel);
			panel.AddString("title", "Select Same Preferred Application As");
			panel.AddInt32("message", kMsgSamePreferredAppAsOpened);
			panel.AddMessenger("target", this);

			be_app_messenger.SendMessage(&panel);
			break;
		}
		case kMsgSamePreferredAppAsOpened:
			_AdoptPreferredApp(message, true);
			break;

		// Other

		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			BFile file(&ref, B_READ_ONLY);
			if (is_application(file))
				_AdoptPreferredApp(message, false);
			else
				_AdoptType(message);
			break;
		}

		case B_META_MIME_CHANGED:
			const char* type;
			int32 which;
			if (message->FindString("be:type", &type) != B_OK
				|| message->FindInt32("be:which", &which) != B_OK)
				break;

			if (which == B_MIME_TYPE_DELETED
#ifdef __HAIKU__
				|| which == B_SUPPORTED_TYPES_CHANGED
#endif
				)
				_UpdatePreferredApps();
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


bool
FileTypeWindow::QuitRequested()
{
	be_app->PostMessage(kMsgTypeWindowClosed);
	return true;
}


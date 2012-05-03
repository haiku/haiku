/*
 * Copyright 2006-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypes.h"
#include "FileTypeWindow.h"
#include "IconView.h"
#include "PreferredAppMenu.h"
#include "TypeListWindow.h"

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <TextControl.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FileType Window"


const uint32 kMsgTypeEntered = 'type';
const uint32 kMsgSelectType = 'sltp';
const uint32 kMsgTypeSelected = 'tpsd';
const uint32 kMsgSameTypeAs = 'stpa';
const uint32 kMsgSameTypeAsOpened = 'stpO';

const uint32 kMsgPreferredAppChosen = 'papc';
const uint32 kMsgSelectPreferredApp = 'slpa';
const uint32 kMsgSamePreferredAppAs = 'spaa';
const uint32 kMsgPreferredAppOpened = 'paOp';
const uint32 kMsgSamePreferredAppAsOpened = 'spaO';


FileTypeWindow::FileTypeWindow(BPoint position, const BMessage& refs)
	:
	BWindow(BRect(0.0f, 0.0f, 200.0f, 200.0f).OffsetBySelf(position),
		B_TRANSLATE("File type"), B_TITLED_WINDOW,
		B_NOT_V_RESIZABLE | B_NOT_ZOOMABLE
			| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	float padding = be_control_look->DefaultItemSpacing();

	// "File Type" group
	BBox* fileTypeBox = new BBox("file type BBox");
	fileTypeBox->SetLabel(B_TRANSLATE("File type"));

	fTypeControl = new BTextControl("type", NULL, "Type Control",
		new BMessage(kMsgTypeEntered));

	// filter out invalid characters that can't be part of a MIME type name
	BTextView* textView = fTypeControl->TextView();
	const char* disallowedCharacters = "<>@,;:\"()[]?=";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		textView->DisallowChar(disallowedCharacters[i]);
	}

	fSelectTypeButton = new BButton("select type",
		B_TRANSLATE("Select" B_UTF8_ELLIPSIS), new BMessage(kMsgSelectType));

	fSameTypeAsButton = new BButton("same type as",
		B_TRANSLATE_COMMENT("Same as" B_UTF8_ELLIPSIS,
			"The same TYPE as ..."), new BMessage(kMsgSameTypeAs));

	BLayoutBuilder::Grid<>(fileTypeBox)
		.SetInsets(padding, padding * 2, padding, padding)
		.Add(fTypeControl, 0, 0, 3, 1)
		.Add(fSelectTypeButton, 1, 1)
		.Add(fSameTypeAsButton, 2, 1);

	// "Icon" group

	BBox* iconBox = new BBox("icon BBox");
	iconBox->SetLabel(B_TRANSLATE("Icon"));
	fIconView = new IconView("icon");
	BLayoutBuilder::Group<>(iconBox, B_HORIZONTAL)
		.SetInsets(padding, padding * 2, padding, padding)
		.Add(fIconView);

	// "Preferred Application" group

	BBox* preferredBox = new BBox("preferred BBox");
	preferredBox->SetLabel(B_TRANSLATE("Preferred application"));

	BMenu* menu = new BPopUpMenu("preferred");
	BMenuItem* item;
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Default application"),
		new BMessage(kMsgPreferredAppChosen)));
	item->SetMarked(true);

	fPreferredField = new BMenuField("preferred", NULL, menu);

	fSelectAppButton = new BButton("select app",
		B_TRANSLATE("Select" B_UTF8_ELLIPSIS),
		new BMessage(kMsgSelectPreferredApp));

	fSameAppAsButton = new BButton("same app as",
		B_TRANSLATE_COMMENT("Same as" B_UTF8_ELLIPSIS,
			"The same APPLICATION as ..."),
			new BMessage(kMsgSamePreferredAppAs));

	BLayoutBuilder::Grid<>(preferredBox, padding, padding)
		.SetInsets(padding, padding * 2, padding, padding)
		.Add(fPreferredField, 0, 0, 3, 1)
		.Add(fSelectAppButton, 1, 1)
		.Add(fSameAppAsButton, 2, 1);

	BLayoutBuilder::Grid<>(this)
		.SetInsets(padding)
		.Add(fileTypeBox, 0, 0, 2, 1)
		.Add(iconBox, 0, 1, 1, 1)
		.Add(preferredBox, 1, 1, 1, 1);

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
			char buffer[512];
			snprintf(buffer, sizeof(buffer),
				B_TRANSLATE("Multiple files from \"%s\" file type"), name);
			title = buffer;
		} else
			title = B_TRANSLATE("[Multiple files] file types");
	} else if (refs.FindRef("refs", 0, &ref) == B_OK) {
		char buffer[512];
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("%s file type"), ref.name);
		title = buffer;
	}

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
			fIconView->SetTo(ref);
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
		error_alert(B_TRANSLATE("Could not open file"), status);
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

		case kMsgSelectType:
		{
			BWindow* window = new TypeListWindow(fCommonType.String(),
				kMsgTypeSelected, this);
			window->Show();
			break;
		}
		case kMsgTypeSelected:
		{
			const char* type;
			if (message->FindString("type", &type) == B_OK) {
				fCommonType = type;
				fTypeControl->SetText(type);
				_AdoptType();
			}
			break;
		}

		case kMsgSameTypeAs:
		{
			BMessage panel(kMsgOpenFilePanel);
			panel.AddString("title", B_TRANSLATE("Select same type as"));
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
			panel.AddString("title",
				B_TRANSLATE("Select preferred application"));
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
			panel.AddString("title",
				B_TRANSLATE("Select same preferred application as"));
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
				|| which == B_SUPPORTED_TYPES_CHANGED) {
				_UpdatePreferredApps();
			}
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


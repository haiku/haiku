/*
 * Copyright 2005-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include "PeopleApp.h"

#include <Alert.h>
#include <AutoDeleter.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <fs_index.h>
#include <Locale.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "PersonWindow.h"
#include "PersonIcons.h"

#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "People"


struct DefaultAttribute {
	const char*	attribute;
	int32		width;
	const char*	name;
};

// TODO: Add flags in attribute info message to find these.
static const char* kNameAttribute = "META:name";
static const char* kCategoryAttribute = "META:group";

struct DefaultAttribute sDefaultAttributes[] = {
	{ kNameAttribute, 120, B_TRANSLATE("Contact name") },
	{ "META:nickname", 120, B_TRANSLATE("Nickname") },
	{ "META:company", 120, B_TRANSLATE("Company") },
	{ "META:address", 120, B_TRANSLATE("Address") },
	{ "META:city", 90, B_TRANSLATE("City") },
	{ "META:state", 50, B_TRANSLATE("State") },
	{ "META:zip", 50, B_TRANSLATE("Zip") },
	{ "META:country", 120, B_TRANSLATE("Country") },
	{ "META:hphone", 90, B_TRANSLATE("Home phone") },
	{ "META:wphone", 90, B_TRANSLATE("Work phone") },
	{ "META:fax", 90, B_TRANSLATE("Fax") },
	{ "META:email", 120, B_TRANSLATE("E-mail") },
	{ "META:url", 120, B_TRANSLATE("URL") },
	{ kCategoryAttribute, 120, B_TRANSLATE("Group") },
	{ NULL, 0, NULL }
};


TPeopleApp::TPeopleApp()
	:
	BApplication(APP_SIG),
	fWindowCount(0),
	fAttributes(20, true)
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("People");

	fPosition.Set(6, TITLE_BAR_HEIGHT, 6 + WIND_WIDTH,
		TITLE_BAR_HEIGHT + WIND_HEIGHT);
	BPoint pos = fPosition.LeftTop();

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

	BDirectory dir(path.Path());
	BEntry entry;
	if (dir.FindEntry("People_data", &entry) == B_OK) {
		fPrefs = new BFile(&entry, B_READ_WRITE);
		if (fPrefs->InitCheck() == B_NO_ERROR) {
			fPrefs->Read(&pos, sizeof(BPoint));
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(pos))
				fPosition.OffsetTo(pos);
		}
	} else {
		fPrefs = new BFile();
		if (dir.CreateFile("People_data", fPrefs) != B_OK) {
			delete fPrefs;
			fPrefs = NULL;
		}
	}

	// Read attributes from person mime type. If it does not exist,
	// or if it contains no attribute definitions, install a "clean"
	// person mime type from the hard-coded default attributes.

	bool valid = false;
	BMimeType mime(B_PERSON_MIMETYPE);
	if (mime.IsInstalled()) {
		BMessage info;
		if (mime.GetAttrInfo(&info) == B_NO_ERROR) {
			int32 index = 0;
			while (true) {
				int32 type;
				if (info.FindInt32("attr:type", index, &type) != B_OK)
					break;
				bool editable;
				if (info.FindBool("attr:editable", index, &editable) != B_OK)
					break;

				// TODO: Support other types besides string attributes.
				if (type != B_STRING_TYPE || !editable)
					break;

				Attribute* attribute = new Attribute();
				ObjectDeleter<Attribute> deleter(attribute);
				if (info.FindString("attr:public_name", index,
						&attribute->name) != B_OK) {
					break;
				}
				if (info.FindString("attr:name", index,
						&attribute->attribute) != B_OK) {
					break;
				}

				if (!fAttributes.AddItem(attribute))
					break;

				deleter.Detach();
				index++;
			}
		}
		if (fAttributes.CountItems() == 0) {
			valid = false;
			mime.Delete();
		} else
			valid = true;
	}
	if (!valid) {
		mime.Install();
		mime.SetShortDescription(B_TRANSLATE_CONTEXT("Person",
			"Short mimetype description"));
		mime.SetLongDescription(B_TRANSLATE_CONTEXT(
			"Contact information for a person.",
			"Long mimetype description"));
		mime.SetIcon(kPersonIcon, sizeof(kPersonIcon));
		mime.SetPreferredApp(APP_SIG);

		// add default person fields to meta-mime type
		BMessage fields;
		for (int32 i = 0; sDefaultAttributes[i].attribute; i++) {
			fields.AddString("attr:public_name", sDefaultAttributes[i].name);
			fields.AddString("attr:name", sDefaultAttributes[i].attribute);
			fields.AddInt32("attr:type", B_STRING_TYPE);
			fields.AddBool("attr:viewable", true);
			fields.AddBool("attr:editable", true);
			fields.AddInt32("attr:width", sDefaultAttributes[i].width);
			fields.AddInt32("attr:alignment", B_ALIGN_LEFT);
			fields.AddBool("attr:extra", false);

			// Add the default attribute to the attribute list, too.
			Attribute* attribute = new Attribute();
			attribute->name = sDefaultAttributes[i].name;
			attribute->attribute = sDefaultAttributes[i].attribute;
			if (!fAttributes.AddItem(attribute))
				delete attribute;
		}

		mime.SetAttrInfo(&fields);
	}

	// create indices on all volumes for the found attributes.

	int32 count = fAttributes.CountItems();
	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		for (int32 i = 0; i < count; i++) {
			Attribute* attribute = fAttributes.ItemAt(i);
			fs_create_index(volume.Device(), attribute->attribute,
				B_STRING_TYPE, 0);
		}
	}

}


TPeopleApp::~TPeopleApp()
{
	delete fPrefs;
}


void
TPeopleApp::ArgvReceived(int32 argc, char** argv)
{
	BMessage message(B_REFS_RECEIVED);

	for (int32 i = 1; i < argc; i++) {
		BEntry entry(argv[i]);
		entry_ref ref;
		if (entry.Exists() && entry.GetRef(&ref) == B_OK)
			message.AddRef("refs", &ref);
	}

	RefsReceived(&message);
}


void
TPeopleApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_NEW:
		case B_SILENT_RELAUNCH:
			_NewWindow();
			break;

		case M_WINDOW_QUITS:
			_SavePreferences(message);
			fWindowCount--;
			if (fWindowCount < 1)
				PostMessage(B_QUIT_REQUESTED);
			break;

		case M_CONFIGURE_ATTRIBUTES:
		{
			const char* arguments[] = { "-type", B_PERSON_MIMETYPE, 0 };
			status_t ret = be_roster->Launch(
				"application/x-vnd.Haiku-FileTypes",
				sizeof(arguments) / sizeof(const char*) - 1,
				const_cast<char**>(arguments));
			if (ret != B_OK && ret != B_ALREADY_RUNNING) {
				BString errorMsg(B_TRANSLATE("Launching the FileTypes "
					"preflet to configure Person attributes has failed."
					"\n\nError: "));
				errorMsg << strerror(ret);
				BAlert* alert = new BAlert(B_TRANSLATE("Error"),
					errorMsg.String(), B_TRANSLATE("OK"), NULL, NULL,
					B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->Go(NULL);
			}
			break;
		}

		default:
			BApplication::MessageReceived(message);
	}
}


void
TPeopleApp::RefsReceived(BMessage* message)
{
	int32 index = 0;
	while (message->HasRef("refs", index)) {
		entry_ref ref;
		message->FindRef("refs", index++, &ref);

		PersonWindow* window = _FindWindow(ref);
		if (window != NULL)
			window->Activate(true);
		else {
			BFile file(&ref, B_READ_ONLY);
			if (file.InitCheck() == B_OK)
				_NewWindow(&ref);
		}
	}
}


void
TPeopleApp::ReadyToRun()
{
	if (fWindowCount < 1)
		_NewWindow();
}


// #pragma mark -


PersonWindow*
TPeopleApp::_NewWindow(entry_ref* ref)
{
	PersonWindow* window = new PersonWindow(fPosition,
		B_TRANSLATE("New person"), kNameAttribute,
		kCategoryAttribute, ref);

	_AddAttributes(window);

	window->Show();

	fWindowCount++;

	// Offset the position for the next window which will be opened and
	// reset it if it would open outside the screen bounds.
	fPosition.OffsetBy(20, 20);
	BScreen screen(window);
	if (fPosition.bottom > screen.Frame().bottom)
		fPosition.OffsetTo(fPosition.left, TITLE_BAR_HEIGHT);
	if (fPosition.right > screen.Frame().right)
		fPosition.OffsetTo(6, fPosition.top);

	return window;
}


void
TPeopleApp::_AddAttributes(PersonWindow* window) const
{
	int32 count = fAttributes.CountItems();
	for (int32 i = 0; i < count; i++) {
		Attribute* attribute = fAttributes.ItemAt(i);
		const char* label = attribute->name;
		if (attribute->attribute == kNameAttribute)
			label = B_TRANSLATE("Name");

		window->AddAttribute(label, attribute->attribute);
	}
}


PersonWindow*
TPeopleApp::_FindWindow(const entry_ref& ref) const
{
	for (int32 i = 0; BWindow* window = WindowAt(i); i++) {
		PersonWindow* personWindow = dynamic_cast<PersonWindow*>(window);
		if (personWindow == NULL)
			continue;
		if (personWindow->RefersPersonFile(ref))
			return personWindow;
	}
	return NULL;
}


void
TPeopleApp::_SavePreferences(BMessage* message) const
{
	BRect frame;
	if (message->FindRect("frame", &frame) != B_OK)
		return;

	BPoint leftTop = frame.LeftTop();

	if (fPrefs != NULL) {
		fPrefs->Seek(0, 0);
		fPrefs->Write(&leftTop, sizeof(BPoint));
	}
}


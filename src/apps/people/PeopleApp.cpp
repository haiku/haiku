/*
 * Copyright 2005-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Axel Dörfler, axeld@pinc-software.de
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include <Alert.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <fs_index.h>
#include <Locale.h>
#include <Path.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "PeopleApp.h"
#include "PeopleWindow.h"
#include "PersonIcons.h"

#include <string.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "People"


struct people_field gFields[] = {
	{ "META:name", 120, B_TRANSLATE("Contact name") },
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
	{ "META:group", 120, B_TRANSLATE("Group") },
	{ NULL, 0, NULL }
};


TPeopleApp::TPeopleApp(void)
	: BApplication(APP_SIG),
	fWindowCount(0)
{
	fPosition.Set(6, TITLE_BAR_HEIGHT, 6 + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);
	BPoint pos = fPosition.LeftTop();

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

	BDirectory dir(path.Path());
	BEntry entry;
	if (dir.FindEntry("People_data", &entry) == B_NO_ERROR) {
		fPrefs = new BFile(&entry, B_READ_WRITE);
		if (fPrefs->InitCheck() == B_NO_ERROR) {
			fPrefs->Read(&pos, sizeof(BPoint));
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(pos))
				fPosition.OffsetTo(pos);
		}
	} else {
		fPrefs = new BFile();
		if (dir.CreateFile("People_data", fPrefs) != B_NO_ERROR) {
			delete fPrefs;
			fPrefs = NULL;
		}
	}

	// create indices on all volumes

	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		for (int32 i = 0; gFields[i].attribute; i++) {
			fs_create_index(volume.Device(), gFields[i].attribute, B_STRING_TYPE, 0);
		}
	}

	// install person mime type

	bool valid = false;
	BMimeType mime;
	mime.SetType(B_PERSON_MIMETYPE);

	if (mime.IsInstalled()) {
		BMessage info;
		if (mime.GetAttrInfo(&info) == B_NO_ERROR) {
			const char *string;
			int32 index = 0;
			while (info.FindString("attr:name", index++, &string) == B_OK) {
				if (!strcmp(string, gFields[0].attribute)) {
					valid = true;
					break;
				}
			}
			if (!valid)
				mime.Delete();
		}
	}
	if (!valid) {
		mime.Install();
		mime.SetShortDescription(B_TRANSLATE_WITH_CONTEXT("Person", 
			"Short mimetype description"));
		mime.SetLongDescription(B_TRANSLATE_WITH_CONTEXT(
			"Contact information for a person.", 
			"Long mimetype description"));
		mime.SetIcon(kPersonIcon, sizeof(kPersonIcon));
		mime.SetPreferredApp(APP_SIG);

		// add relevant person fields to meta-mime type

		BMessage fields;
		for (int32 i = 0; gFields[i].attribute; i++) {
			fields.AddString("attr:public_name", gFields[i].name);
			fields.AddString("attr:name", gFields[i].attribute);
			fields.AddInt32("attr:type", B_STRING_TYPE);
			fields.AddBool("attr:viewable", true);
			fields.AddBool("attr:editable", true);
			fields.AddInt32("attr:width", gFields[i].width);
			fields.AddInt32("attr:alignment", B_ALIGN_LEFT);
			fields.AddBool("attr:extra", false);
		}

		mime.SetAttrInfo(&fields);
	}
}


TPeopleApp::~TPeopleApp(void)
{
	delete fPrefs;
}


void
TPeopleApp::AboutRequested(void)
{
	(new BAlert("", "...by Robert Polic", B_TRANSLATE("OK")))->Go();
}


void
TPeopleApp::ArgvReceived(int32 argc, char **argv)
{
	TPeopleWindow* window = NULL;

	for (int32 loop = 1; loop < argc; loop++) {
		char* arg = argv[loop];

		int32 index;
		for (index = 0; gFields[index].attribute; index++) {
			if (!strncmp(gFields[index].attribute, arg, strlen(gFields[index].attribute)))
				break;
		}

		if (gFields[index].attribute != NULL) {
			if (!window)
				window = NewWindow();

			while (arg[0] && arg[0] != ' ' && arg[0] != '=')
				arg++;

			if (arg[0]) {
				arg++;
				window->SetField(index, arg);
			}
		}
	}
}


void
TPeopleApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_NEW:
		case B_SILENT_RELAUNCH:
			NewWindow();
			break;

		case M_WINDOW_QUITS:
			SavePreferences(msg);
			fWindowCount--;
			if (fWindowCount < 1)
				PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}


void
TPeopleApp::RefsReceived(BMessage *message)
{
	int32 index = 0;

	while (message->HasRef("refs", index)) {
		entry_ref ref;
		message->FindRef("refs", index++, &ref);

		TPeopleWindow* window = FindWindow(ref);
		if (window != NULL)
			window->Activate(true);
		else {
			BFile file(&ref, B_READ_ONLY);
			if (file.InitCheck() == B_OK)
				NewWindow(&ref);
		}
	}
}


void
TPeopleApp::ReadyToRun(void)
{
	if (fWindowCount < 1)
		NewWindow();
}


TPeopleWindow*
TPeopleApp::NewWindow(entry_ref *ref)
{
	TPeopleWindow *window;

	window = new TPeopleWindow(fPosition, B_TRANSLATE("New person"), ref);
	window->Show();
	fWindowCount++;
	fPosition.OffsetBy(20, 20);

	if (fPosition.bottom > BScreen(B_MAIN_SCREEN_ID).Frame().bottom)
		fPosition.OffsetTo(fPosition.left, TITLE_BAR_HEIGHT);
	if (fPosition.right > BScreen(B_MAIN_SCREEN_ID).Frame().right)
		fPosition.OffsetTo(6, fPosition.top);

	return window;
}


TPeopleWindow*
TPeopleApp::FindWindow(entry_ref ref)
{
	TPeopleWindow* window;
	int32 index = 0;

	while ((window = (TPeopleWindow *)WindowAt(index++))) {
		if (window->FindView("PeopleView") != NULL && window->fRef && *window->fRef == ref)
			return window;
	}
	return NULL;
}


void
TPeopleApp::SavePreferences(BMessage* message)
{
	BRect frame;
	if (message->FindRect("frame", &frame) != B_OK)
		return;

	BPoint leftTop = frame.LeftTop();

	if (fPrefs) {
		fPrefs->Seek(0, 0);	
		fPrefs->Write(&leftTop, sizeof(BPoint));
	}
}


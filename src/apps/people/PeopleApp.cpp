/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Axel Dörfler, axeld@pinc-software.de
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include <Bitmap.h>
#include <Directory.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Screen.h>
#include <Alert.h>
#include <fs_index.h>

#include "PeopleApp.h"
#include "PeopleWindow.h"
#include "PersonIcons.h"

#include <string.h>


struct people_field gFields[] = {
	{ "META:name", 120, "Contact Name" },
	{ "META:nickname", 120, "Nickname" },
	{ "META:company", 120, "Company" },
	{ "META:address", 120, "Address" },
	{ "META:city", 90, "City" },
	{ "META:state", 50, "State" },
	{ "META:zip", 50, "Zip" },
	{ "META:country", 120, "Country" },
	{ "META:hphone", 90, "Home Phone" },
	{ "META:wphone", 90, "Work Phone" },
	{ "META:fax", 90, "Fax" },
	{ "META:email", 120, "E-mail" },
	{ "META:url", 120, "URL" },
	{ "META:group", 120, "Group" },
	{ NULL, NULL }
};


TPeopleApp::TPeopleApp(void)
	: BApplication(APP_SIG),
	fHaveWindow(false)
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
		BBitmap largeIcon(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_CMAP8);
		BBitmap miniIcon(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);

		mime.Install();
		largeIcon.SetBits(kLargePersonIcon, largeIcon.BitsLength(), 0, B_CMAP8);
		miniIcon.SetBits(kSmallPersonIcon, miniIcon.BitsLength(), 0, B_CMAP8);
		mime.SetShortDescription("Person");
		mime.SetLongDescription("Contact information for a person.");
		mime.SetIcon(&largeIcon, B_LARGE_ICON);
		mime.SetIcon(&miniIcon, B_MINI_ICON);
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
	(new BAlert("", "...by Robert Polic", "Big Deal"))->Go();
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
			NewWindow();
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
	if (!fHaveWindow)
		NewWindow();
}


TPeopleWindow*
TPeopleApp::NewWindow(entry_ref *ref)
{
	TPeopleWindow *window;

	window = new TPeopleWindow(fPosition, "New Person", ref);
	window->Show();
	fHaveWindow = true;
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

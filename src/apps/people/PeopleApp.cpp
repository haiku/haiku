//--------------------------------------------------------------------
//	
//	PeopleApp.cpp
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Bitmap.h>
#include <Directory.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Screen.h>
#include <fs_index.h>
#include <string.h>
#include <Alert.h>


#include "PeopleApp.h"
#include "PeopleWindow.h"
#include "PersonIcons.h"

//====================================================================

TPeopleApp::TPeopleApp(void)
		  :BApplication(APP_SIG)
{
	bool			valid = FALSE;
	const char		*str;
	int32			index = 0;
	BBitmap			large_icon(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_COLOR_8_BIT);
	BBitmap			mini_icon(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_COLOR_8_BIT);
	BDirectory		dir;
	BEntry			entry;
	BMessage		msg;
	BMessage		info;
	BMimeType		mime;
	BPath			path;
	BPoint			pos;
	BVolume			vol;
	BVolumeRoster	roster;
//	TPeopleWindow	*window;

	fHaveWindow = FALSE;

	fPosition.Set(6, TITLE_BAR_HEIGHT, 6 + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);
	pos = fPosition.LeftTop();

	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	dir.SetTo(path.Path());
	if (dir.FindEntry("People_data", &entry) == B_NO_ERROR) {
		fPrefs = new BFile(&entry, O_RDWR);
		if (fPrefs->InitCheck() == B_NO_ERROR) {
			fPrefs->Read(&pos, sizeof(BPoint));
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(pos))
				fPosition.OffsetTo(pos);
		}
	}
	else {
		fPrefs = new BFile();
		if (dir.CreateFile("People_data", fPrefs) != B_NO_ERROR) {
			delete fPrefs;
			fPrefs = NULL;
		}
	}

	roster.GetBootVolume(&vol);
	fs_create_index(vol.Device(), P_NAME, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_COMPANY, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_ADDRESS, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_CITY, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_STATE, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_ZIP, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_COUNTRY, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_HPHONE, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_WPHONE, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_FAX, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_EMAIL, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_URL, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_GROUP, B_STRING_TYPE, 0);
	fs_create_index(vol.Device(), P_NICKNAME, B_STRING_TYPE, 0);

	// install person mime type
	mime.SetType(B_PERSON_MIMETYPE);
	if (mime.IsInstalled()) {
		if (mime.GetAttrInfo(&info) == B_NO_ERROR) {
			while (info.FindString("attr:name", index++, &str) == B_NO_ERROR) {
				if (!strcmp(str, P_NAME)) {
					valid = TRUE;
					break;
				}
			}
			if (!valid)
				mime.Delete();
		}
	}
	if (!valid) {
		mime.Install();
		large_icon.SetBits(kLargePersonIcon, large_icon.BitsLength(), 0, B_COLOR_8_BIT);
		mini_icon.SetBits(kSmallPersonIcon, mini_icon.BitsLength(), 0, B_COLOR_8_BIT);
		mime.SetShortDescription("Person");
		mime.SetLongDescription("Contact information for a person.");
		mime.SetIcon(&large_icon, B_LARGE_ICON);
		mime.SetIcon(&mini_icon, B_MINI_ICON);
		mime.SetPreferredApp(APP_SIG);

		// add relevant person fields to meta-mime type
		msg.AddString("attr:public_name", "Contact Name"); 
		msg.AddString("attr:name", P_NAME); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Company"); 
		msg.AddString("attr:name", P_COMPANY); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Address"); 
		msg.AddString("attr:name", P_ADDRESS); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "City"); 
		msg.AddString("attr:name", P_CITY); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 90); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "State"); 
		msg.AddString("attr:name", P_STATE); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 50); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Zip"); 
		msg.AddString("attr:name", P_ZIP); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 50); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Country"); 
		msg.AddString("attr:name", P_COUNTRY); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Home Phone"); 
		msg.AddString("attr:name", P_HPHONE); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 90); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Work Phone"); 
		msg.AddString("attr:name", P_WPHONE); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 90); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Fax"); 
		msg.AddString("attr:name", P_FAX); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 90); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "E-mail"); 
		msg.AddString("attr:name", P_EMAIL); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "URL"); 
		msg.AddString("attr:name", P_URL); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Group"); 
		msg.AddString("attr:name", P_GROUP); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Nickname"); 
		msg.AddString("attr:name", P_NICKNAME); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		mime.SetAttrInfo(&msg);
	}
}

//--------------------------------------------------------------------

TPeopleApp::~TPeopleApp(void)
{
	if (fPrefs)
		delete fPrefs;
}

//--------------------------------------------------------------------

void TPeopleApp::AboutRequested(void)
{
	(new BAlert("", "...by Robert Polic", "Big Deal"))->Go();
}

//--------------------------------------------------------------------

void TPeopleApp::ArgvReceived(int32 argc, char **argv)
{
	char			*arg;
	int32			index;
	int32			loop;
	TPeopleWindow	*window = NULL;

	for (loop = 1; loop < argc; loop++) {
		arg = argv[loop];
		if (!strncmp(P_NAME, arg, strlen(P_NAME)))
			index = F_NAME;
		else if (!strncmp(P_COMPANY, arg, strlen(P_COMPANY)))
			index = F_COMPANY;
		else if (!strncmp(P_ADDRESS, arg, strlen(P_ADDRESS)))
			index = F_ADDRESS;
		else if (!strncmp(P_CITY, arg, strlen(P_CITY)))
			index = F_CITY;
		else if (!strncmp(P_STATE, arg, strlen(P_STATE)))
			index = F_STATE;
		else if (!strncmp(P_ZIP, arg, strlen(P_ZIP)))
			index = F_ZIP;
		else if (!strncmp(P_COUNTRY, arg, strlen(P_COUNTRY)))
			index = F_COUNTRY;
		else if (!strncmp(P_HPHONE, arg, strlen(P_HPHONE)))
			index = F_HPHONE;
		else if (!strncmp(P_WPHONE, arg, strlen(P_WPHONE)))
			index = F_WPHONE;
		else if (!strncmp(P_FAX, arg, strlen(P_FAX)))
			index = F_FAX;
		else if (!strncmp(P_EMAIL, arg, strlen(P_EMAIL)))
			index = F_EMAIL;
		else if (!strncmp(P_URL, arg, strlen(P_URL)))
			index = F_URL;
		else if (!strncmp(P_GROUP, arg, strlen(P_GROUP)))
			index = F_GROUP;
		else if (!strncmp(P_NICKNAME, arg, strlen(P_NICKNAME)))
			index = F_NICKNAME;
		else
			index = F_END;

		if (index != F_END) {
			if (!window)
				window = NewWindow();
			while(*arg != ' ')
				arg++;
			arg++;
			window->SetField(index, arg);
		}
	}
}

//--------------------------------------------------------------------

void TPeopleApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_NEW:
			NewWindow();
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void TPeopleApp::RefsReceived(BMessage *msg)
{
	int32			item = 0;
	BFile			file;
	entry_ref		ref;
	TPeopleWindow	*window;

	while (msg->HasRef("refs", item)) {
		msg->FindRef("refs", item++, &ref);
		if ((window = FindWindow(ref)))
			window->Activate(TRUE);
		else {
			file.SetTo(&ref, O_RDONLY);
			if (file.InitCheck() == B_NO_ERROR)
				NewWindow(&ref);
		}
	}
}

//--------------------------------------------------------------------

void TPeopleApp::ReadyToRun(void)
{
	if (!fHaveWindow)
		NewWindow();
}

//--------------------------------------------------------------------

TPeopleWindow* TPeopleApp::NewWindow(entry_ref *ref)
{
	TPeopleWindow	*window;

	window = new TPeopleWindow(fPosition, "New Person", ref);
	window->Show();
	fHaveWindow = TRUE;
	fPosition.OffsetBy(20, 20);

	if (fPosition.bottom > BScreen(B_MAIN_SCREEN_ID).Frame().bottom)
		fPosition.OffsetTo(fPosition.left, TITLE_BAR_HEIGHT);
	if (fPosition.right > BScreen(B_MAIN_SCREEN_ID).Frame().right)
		fPosition.OffsetTo(6, fPosition.top);

	return window;
}

//--------------------------------------------------------------------

TPeopleWindow* TPeopleApp::FindWindow(entry_ref ref)
{
	int32			index = 0;
	TPeopleWindow	*window;

	while ((window = (TPeopleWindow *)WindowAt(index++))) {
		if ((window->FindView("PeopleView")) && (window->fRef) && (*(window->fRef) == ref))
			return window;
	}
	return NULL;
}

/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <Message.h>
#include <PropertyInfo.h>

#include "Tracker.h"
#include "FSUtils.h"

#define kPropertyTrash "Trash"
#define kPropertyFolder "Folder"
#define kPropertyPreferences "Preferences"

#if 0

doo Tracker delete Trash
doo Tracker create Folder to '/boot/home/Desktop/hello'

ToDo:
Create file: on a "Tracker" "File" "B_CREATE_PROPERTY" "name"
Create query: on a "Tracker" "Query" "B_CREATE_PROPERTY" "name"
Open a folder: Tracker Execute "Folder" bla
Find a window for a path

#endif


#if _SUPPORTS_FEATURE_SCRIPTING

const property_info kTrackerPropertyList[] = {
	{	kPropertyTrash,
		{ B_DELETE_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"delete Trash # Empties the Trash",
		0,
		{},
		{},
		{}
	},
	{	kPropertyFolder,
		{ B_CREATE_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"create Folder to path # creates a new folder",
		0,
		{ B_REF_TYPE },
		{},
		{}
	},
	{	kPropertyPreferences,
		{ B_EXECUTE_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"shows Tracker preferences",
		0,
		{},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};


status_t
TTracker::GetSupportedSuites(BMessage* data)
{
	data->AddString("suites", kTrackerSuites);
	BPropertyInfo propertyInfo(const_cast<property_info*>
		(kTrackerPropertyList));
	data->AddFlat("messages", &propertyInfo);

	return _inherited::GetSupportedSuites(data);
}


BHandler*
TTracker::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	BPropertyInfo propertyInfo(const_cast<property_info*>
		(kTrackerPropertyList));

	int32 result = propertyInfo.FindMatch(message, index, specifier, form,
		property);
	if (result < 0) {
		//PRINT(("FindMatch result %d %s\n", result, strerror(result)));
		return _inherited::ResolveSpecifier(message, index, specifier,
			form, property);
	}

	return this;
}


bool
TTracker::HandleScriptingMessage(BMessage* message)
{
	if (message->what != B_GET_PROPERTY
		&& message->what != B_SET_PROPERTY
		&& message->what != B_CREATE_PROPERTY
		&& message->what != B_COUNT_PROPERTIES
		&& message->what != B_DELETE_PROPERTY
		&& message->what != B_EXECUTE_PROPERTY)
		return false;

	// dispatch scripting messages
	BMessage reply(B_REPLY);
	const char* property = 0;
	bool handled = false;

	int32 index = 0;
	int32 form = 0;
	BMessage specifier;

	status_t result = message->GetCurrentSpecifier(&index, &specifier,
		&form, &property);

	if (result != B_OK || index == -1)
		return false;

	ASSERT(property);

	switch (message->what) {
		case B_CREATE_PROPERTY:
			handled = CreateProperty(message, &specifier, form, property,
				&reply);
			break;

		case B_GET_PROPERTY:
			handled = GetProperty(&specifier, form, property, &reply);
			break;

		case B_SET_PROPERTY:
			handled = SetProperty(message, &specifier, form, property,
				&reply);
			break;

		case B_COUNT_PROPERTIES:
			handled = CountProperty(&specifier, form, property, &reply);
			break;

		case B_DELETE_PROPERTY:
			handled = DeleteProperty(&specifier, form, property, &reply);
			break;

		case B_EXECUTE_PROPERTY:
			handled = ExecuteProperty(&specifier, form, property, &reply);
			break;
	}

	if (handled) {
		// done handling message, send a reply
		message->SendReply(&reply);
	}

	return handled;
}


bool
TTracker::CreateProperty(BMessage* message, BMessage*, int32 form,
	const char* property, BMessage* reply)
{
	bool handled = false;
	status_t error = B_OK;
	if (strcmp(property, kPropertyFolder) == 0) {
		if (form != B_DIRECT_SPECIFIER)
			return false;

		// create new empty folders
		entry_ref ref;
		for (int32 index = 0;
			message->FindRef("data", index, &ref) == B_OK; index++) {

			BEntry entry(&ref);
			if (!entry.Exists())
				error = FSCreateNewFolder(&ref);

			if (error != B_OK)
				break;
		}

		handled = true;
	}

	if (error != B_OK)
		reply->AddInt32("error", error);

	return handled;
}


bool
TTracker::DeleteProperty(BMessage* /*specifier*/, int32 form,
	const char* property, BMessage* /*reply*/)
{
	if (strcmp(property, kPropertyTrash) == 0) {
		// deleting on a selection is handled as removing a part of the
		// selection not to be confused with deleting a selected item

		if (form != B_DIRECT_SPECIFIER)
			// only support direct specifier
			return false;
		
		// empty the trash
		FSEmptyTrash();
		return true;

	}
	return false;
}

#else	// _SUPPORTS_FEATURE_SCRIPTING

status_t
TTracker::GetSupportedSuites(BMessage* /*data*/)
{
	return B_UNSUPPORTED;
}


BHandler*
TTracker::ResolveSpecifier(BMessage* /*message*/,
	int32 /*index*/, BMessage* /*specifier*/,
	int32 /*form*/, const char* /*property*/)
{
	return NULL;
}


bool
TTracker::HandleScriptingMessage(BMessage* /*message*/)
{
	return false;
}


bool
TTracker::CreateProperty(BMessage* /*message*/, BMessage*, int32 /*form*/,
	const char* /*property*/, BMessage* /*reply*/)
{
	return false;
}


bool
TTracker::DeleteProperty(BMessage* /*specifier*/, int32 /*form*/,
	const char* /*property*/, BMessage*)
{
	return false;
}

#endif	// _SUPPORTS_FEATURE_SCRIPTING


bool
TTracker::ExecuteProperty(BMessage*, int32 form, const char* property, BMessage*)
{
	if (strcmp(property, kPropertyPreferences) == 0) {
		
		if (form != B_DIRECT_SPECIFIER)
			// only support direct specifier
			return false;
		
		ShowSettingsWindow();
		return true;

	}
	return false;
}


bool
TTracker::CountProperty(BMessage*, int32, const char*, BMessage*)
{
	return false;
}


bool
TTracker::GetProperty(BMessage*, int32, const char*, BMessage*)
{
	return false;
}


bool
TTracker::SetProperty(BMessage*, BMessage*, int32, const char*, BMessage*)
{
	return false;
}

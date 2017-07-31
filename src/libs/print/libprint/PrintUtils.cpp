/*

Preview printer driver.

Copyright (c) 2001, 2002 OpenBeOS.
Copyright (c) 2005 - 2008 Haiku.

Authors:
	Philippe Houdoin
	Simon Gauvin
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

#include "PrintUtils.h"


#include <Message.h>
#include <Window.h>


BRect
ScaleRect(const BRect& rect, float scale)
{
	BRect scaleRect(rect);

	scaleRect.left *= scale;
	scaleRect.right *= scale;
	scaleRect.top *= scale;
	scaleRect.bottom *= scale;

	return scaleRect;
}


void
SetBool(BMessage* msg, const char* name, bool value)
{
	if (msg->HasBool(name)) {
		msg->ReplaceBool(name, value);
	} else {
		msg->AddBool(name, value);
	}
}


void
SetFloat(BMessage* msg, const char* name, float value)
{
	if (msg->HasFloat(name)) {
		msg->ReplaceFloat(name, value);
	} else {
		msg->AddFloat(name, value);
	}
}


void
SetInt32(BMessage* msg, const char* name, int32 value)
{
	if (msg->HasInt32(name)) {
		msg->ReplaceInt32(name, value);
	} else {
		msg->AddInt32(name, value);
	}
}


void
SetString(BMessage* msg, const char* name, const char* value)
{
	if (msg->HasString(name, 0)) {
		msg->ReplaceString(name, value);
	} else {
		msg->AddString(name, value);
	}
}


void
SetRect(BMessage* msg, const char* name, const BRect& rect)
{
	if (msg->HasRect(name)) {
		msg->ReplaceRect(name, rect);
	} else {
		msg->AddRect(name, rect);
	}
}


void
SetString(BMessage* msg, const char* name, const BString& value)
{
	SetString(msg, name, value.String());
}


static
bool InList(const char* list[], const char* name)
{
	for (int i = 0; list[i] != NULL; ++i) {
		if (strcmp(list[i], name) == 0)
			return true;
	}
	return false;
}


void
AddFields(BMessage* to, const BMessage* from, const char* excludeList[],
	const char* includeList[], bool overwrite)
{
	if (to == from)
		return;
	char* name;
	type_code type;
	int32 count;
	for (int32 i = 0; from->GetInfo(B_ANY_TYPE, i, &name, &type, &count)
		== B_OK; ++i) {
		if (excludeList && InList(excludeList, name))
			continue;

		if (includeList && !InList(includeList, name))
			continue;

		ssize_t size;
		const void* data;
		if (!overwrite && to->FindData(name, type, 0, &data, &size) == B_OK)
			continue;

		// replace existing data
		to->RemoveName(name);

		for (int32 j = 0; j < count; ++j) {
			if (from->FindData(name, type, j, &data, &size) == B_OK) {
				if (type == B_STRING_TYPE) {
					to->AddString(name, (const char*)data);
				} else if (type == B_MESSAGE_TYPE) {
					BMessage m;
					from->FindMessage(name, j, &m);
					to->AddMessage(name, &m);
				} else {
					to->AddData(name, type, data, size);
				}
			}
		}
	}
}

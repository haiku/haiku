/*

PDF Writer printer driver.

Copyright (c) 2001-2003 OpenBeOS. 

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

#include "Utils.h"

// --------------------------------------------------
EscapeMessageFilter::EscapeMessageFilter(BWindow *window, int32 what) 
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, '_KYD')
	, fWindow(window), 
	fWhat(what) 
{ 
}


// --------------------------------------------------
filter_result 
EscapeMessageFilter::Filter(BMessage *msg, BHandler **target) 
{
	int32 key;
	// notify window with message fWhat if Escape key is hit
	if (B_OK == msg->FindInt32("key", &key) && key == 1) {
		fWindow->PostMessage(fWhat);
		return B_SKIP_MESSAGE;
	}
	return B_DISPATCH_MESSAGE;
}

// --------------------------------------------------
// copied from BeUtils.cpp
static bool InList(const char* list[], const char* name) {
	for (int i = 0; list[i] != NULL; i ++) {
		if (strcmp(list[i], name) == 0) return true;
	}
	return false;
} 

// --------------------------------------------------
// copied from BeUtils.cpp
void AddFields(BMessage* to, const BMessage* from, bool overwrite = true, const char* excludeList[], const char* includeList[]) {
	if (to == from) return;
	char* name;
	type_code type;
	int32 count;
	for (int32 i = 0; from->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i ++) {
		const void* data;
		ssize_t size;
		if (excludeList && InList(excludeList, name)) continue;
		if (includeList && !InList(includeList, name)) continue;

		if (!overwrite && to->FindData(name, type, 0, &data, &size) == B_OK) {
			continue;
		}
		// replace existing data
		to->RemoveName(name);
		
		for (int32 j = 0; j < count; j ++) {
			if (from->FindData(name, type, j, &data, &size) == B_OK) {
				// WTF why works AddData not for B_STRING_TYPE in R5.0.3?
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

void AddString(BMessage* m, const char* name, const char* value) {
	if (m->HasString(name, 0)) {
		m->ReplaceString(name, value);
	} else {
		m->AddString(name, value);
	}
}



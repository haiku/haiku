/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

// NOTE: This code is the same in DiskUsge. Maybe it could be made
// into some kind of common base class?

#include "GrepApp.h"

#include <AppFileInfo.h>
#include <Entry.h>
#include <Roster.h>
#include <TrackerAddOn.h>

#include "GlobalDefs.h"


int
main() 
{
	GrepApp app;
	app.Run();
	return 0;
}


void
process_refs(entry_ref dirRef, BMessage *message, void* /*reserved*/)
{
	// Tracker calls this function when the user invokes the add-on. 
	// "dir_ref" contains the entry_ref of the current directory. 
	// "message" is a standard B_REFS_RECEIVED BMessage whose "refs" 
	// array contains the entry_refs of the selected files. The last
	// argument, "reserved", is currently unused.
	
	// This version of TextSearch is a Tracker add-on, but primarily
	// it is a stand-alone application. The add-on launches the app
	// on the set of files you had selected in Tracker. That way you
	// get the benefits of the Tracker add-on with the benefits of the
	// stand-alone application.

	message->AddRef("dir_ref", &dirRef);

	// get the path of the Tracker add-on
	image_info image;
	int32 cookie = 0;
	status_t status = get_next_image_info(0, &cookie, &image);

	while (status == B_OK) {
		if (((char*)process_refs >= (char*)image.text
				&& (char*)process_refs <= (char*)image.text + image.text_size)
			|| ((char*)process_refs >= (char*)image.data
				&& (char*)process_refs <= (char*)image.data + image.data_size))
			break;
		
		status = get_next_image_info(0, &cookie, &image);
	}

	entry_ref addonRef;

	if (get_ref_for_path(image.name, &addonRef) == B_OK) {
		// It's better to launch the application by its entry
		// than by its application signature. There may be 
		// multiple instances and we would not be certain
		// that the desired one is launched - the one which was
		// loaded into Tracker and whose process_refs() was called.
		be_roster->Launch(&addonRef, message);
	} else
		be_roster->Launch(APP_SIGNATURE, message);
}

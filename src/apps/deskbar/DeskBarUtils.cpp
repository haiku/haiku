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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Debug.h>
#include <Window.h>
#include <AppFileInfo.h>
#include <Directory.h>
#include <FilePanel.h>
#include <List.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Screen.h>
#include <SymLink.h>

#include "BarMenuBar.h"
#include "DeskBarUtils.h"
#include "ExpandoMenuBar.h"

const char* const kBePath = "/boot/home/config/be";

BFilePanel*
AskForRefLocation(BMessenger* target)
{
	entry_ref startRef;
	get_ref_for_path(kBePath, &startRef);
	
	BFilePanel* fp = new BFilePanel(B_OPEN_PANEL, target, &startRef,
		B_DIRECTORY_NODE | B_FILE_NODE,
		false, new BMessage(msg_be_container));
	fp->Show();
		
	return fp;		
}

void
AddRefsToBeMenu(const BMessage* m, entry_ref* subdirectory)
{
	if (m) {
		int32 count = 0;
		uint32 type = 0;
		entry_ref ref;
		
		m->GetInfo("refs", &type, &count);
		if (count <= 0)
			return;
			
		BPath path;
		BSymLink link;
		BDirectory dir;
		if (subdirectory) {
			ref = *subdirectory;
			BEntry entry(&ref);
			if (entry.Exists()) {
				//	if the ref is a file
				//	get the parent and convert it to a ref
				if (entry.IsFile()) {
					BEntry parent;
					entry.GetParent(&parent);
					parent.GetRef(&ref);
				}
			} else
				return;
				
			dir.SetTo(&ref);
		} else
			dir.SetTo(kBePath);
			
		for (long i = 0; i < count; i++) {
			if (m->FindRef("refs", i, &ref) == B_NO_ERROR) {

				BEntry entry(&ref);
				entry.GetPath(&path);
				
				dir.CreateSymLink(ref.name, path.Path(), &link);
			}
		}
	}
}

bool
SignatureForRef(const entry_ref* ref, char* signature)
{
	BEntry entry(ref, true);
	if (entry.InitCheck()==B_OK && entry.Exists()) {
		if (entry.IsFile()) {
			BFile file(&entry, O_RDWR);
			BAppFileInfo finfo(&file);
			
			finfo.GetSignature(signature);
			
			return true;
		}	
	}
	return false;
}

void
CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}

float
FontHeight(const BFont* font, bool full)
{
	font_height finfo;		
	font->GetHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

#ifdef LOG
const char* const kLogPath = "/boot/home/";
const char* const kLogFile = "deskbar_log";
void
AddToLog(const char* str)
{
	if (!str)
		return;
		
//	BFile file;
//	BDirectory dir(kLogPath);
//	if (!dir.Contains(kLogFile))
//		dir.CreateFile(kLogFile, &file);
//	else {
//		BEntry entry;
//		dir.FindEntry(kLogFile, &entry);
//		file.SetTo(&entry, O_RDWR);
//	}
//	
//	if (file.InitCheck() != B_OK)
//		printf("Deskbar Log: can't add to log\n");
//		
//	off_t size;
//	file.GetSize(&size);	
//	file.WriteAt(size, str, strlen(str));
	
	char dstr[B_PATH_NAME_LENGTH+1024];
	sprintf(dstr, "Deskbar: %s\n", str);
//	printf("%s\n", str);
	SERIAL_PRINT((dstr));
}
#endif

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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/


#include "DeskbarUtils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AppFileInfo.h>
#include <Debug.h>
#include <Directory.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <List.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Screen.h>
#include <SymLink.h>
#include <Window.h>

#include "BarMenuBar.h"
#include "ExpandoMenuBar.h"

void
AddRefsToDeskbarMenu(const BMessage* m, entry_ref* subdirectory)
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
				// if the ref is a file get the parent and convert it to a ref
				if (entry.IsFile()) {
					BEntry parent;
					entry.GetParent(&parent);
					parent.GetRef(&ref);
				}
			} else
				return;

			dir.SetTo(&ref);
		} else {
			if (find_directory(B_USER_DESKBAR_DIRECTORY, &path) == B_OK)
				dir.SetTo(path.Path());
			else
				return;
		}

		for (long i = 0; i < count; i++) {
			if (m->FindRef("refs", i, &ref) == B_NO_ERROR) {
				BEntry entry(&ref);
				entry.GetPath(&path);

				dir.CreateSymLink(ref.name, path.Path(), &link);
			}
		}
	}
}

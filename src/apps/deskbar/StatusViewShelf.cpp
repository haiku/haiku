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


#include "StatusViewShelf.h"

#include <stdio.h>
#include <string.h>

#include <Debug.h>
#include <InterfaceDefs.h>

#include "StatusView.h"


TReplicantShelf::TReplicantShelf(TReplicantTray* parent)
	: BShelf(parent, false, "DeskbarShelf")
{
	fParent = parent;
	SetAllowsZombies(false);
}


TReplicantShelf::~TReplicantShelf()
{
}


void
TReplicantShelf::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_DELETE_PROPERTY:
		{
			BMessage repspec;
			int32 index = 0;

			// this should only occur via scripting
			// since we can't use ReplicantDeleted
			// catch the message and find the id or name specifier
			// then delete the rep vi the api,

			// this will fix the problem of realigning the reps
			// after a remove when done through scripting

			// note: if specified by index its the index not the id!

			while (message->FindMessage("specifiers", index++, &repspec)
				== B_OK) {
				const char* str;

				if (repspec.FindString("property", &str) == B_OK) {
					if (strcmp(str, "Replicant") == 0) {
						int32 index;
						if (repspec.FindInt32("index", &index) == B_OK) {
							int32 id;
							const char* name;
							if (fParent->ItemInfo(index, &name, &id) == B_OK)
								fParent->RemoveIcon(id);
							break;
						} else if (repspec.FindString("name", &str) == B_OK) {
							fParent->RemoveIcon(str);
							break;
						}
					}
				}
			}
			break;
		}

		default:
			BShelf::MessageReceived(message);
			break;
	}
}


bool
TReplicantShelf::CanAcceptReplicantView(BRect frame, BView* view,
	BMessage* message) const
{
	if (view->ResizingMode() != B_FOLLOW_NONE)
		view->SetResizingMode(B_FOLLOW_NONE);

	// determine placement and whether the new replicant will 'fit'
	return fParent->AcceptAddon(frame, message);
}


BPoint
TReplicantShelf::AdjustReplicantBy(BRect frame, BMessage* message) const
{
	// added in AcceptAddon, from TReplicantTray
	BPoint point;
	message->FindPoint("_pjp_loc", &point);
	message->RemoveName("_pjp_loc");

	return point - frame.LeftTop();
}


// the virtual BShelf::ReplicantDeleted is called before the
// replicant is actually removed from BShelf's internal list
// thus, this returns the wrong number of replicants.

void
TReplicantShelf::ReplicantDeleted(int32, const BMessage*, const BView*)
{
}

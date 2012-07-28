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

//	PoseView scripting interface

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>
#include <Debug.h>
#include <Message.h>
#include <PropertyInfo.h>

#include "Tracker.h"
#include "PoseView.h"

#define kPosesSuites "suite/vnd.Be-TrackerPoses"

#define kPropertyPath "Path"

#ifndef _SCRIPTING_ONLY
	#if _SUPPORTS_FEATURE_SCRIPTING
		#define _SCRIPTING_ONLY(x) x
	#else
		#define _SCRIPTING_ONLY(x)
	#endif
#endif

// notes on PoseView scripting interface:
// Indices and entry_refs are used to specify poses; In the case of indices
// and previous/next specifiers the current PoseView sort order is used.
// If PoseView is not in list view mode, the order in which poses are indexed
// is arbitrary.
// Both of these specifiers, but indices more so, are likely to be accurate only
// till a next change to the PoseView (a change may be adding, removing a pose, changing
// an attribute or stat resulting in a sort ordering change, changing the sort ordering
// rule. When getting a selected item, there is no guarantee that the item will still
// be selected after the operation. The client must be able to deal with these
// inaccuracies.
// Specifying an index/entry_ref that no longer exists will be handled well.

#if 0
doo Tracker get Suites of Poses of Window test
doo Tracker get Path of Poses of Window test
doo Tracker count Entry of Poses of Window test
doo Tracker get Entry of Poses of Window test
doo Tracker get Entry 2 of Poses of Window test
doo Tracker count Selection of Poses of Window test
doo Tracker get Selection of Poses of Window test
doo Tracker delete Entry 'test/6L6' of Poses of Window test
doo Tracker execute Entry 'test/6L6' of Poses of Window test
doo Tracker execute Entry 2 of Poses of Window test
doo Tracker set Selection of Poses of Window test to [0,2]
doo Tracker set Selection of Poses of Window test to 'test/KT55'
doo Tracker create Selection of Poses of Window test to 'test/EL34'
doo Tracker delete Selection 'test/EL34' of Poses of Window test
#endif

// ToDo:
//	access list view column state
//	access poses
//				- pose location
//				- pose text widgets


#if _SUPPORTS_FEATURE_SCRIPTING

const property_info kPosesPropertyList[] = {
	{	kPropertyPath,
		{ B_GET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"get Path of ... # returns the path of a Tracker window, "
			"error if no path associated",
		0,
		{ B_REF_TYPE },
		{},
		{}
	},
	{	kPropertyEntry,
		{ B_COUNT_PROPERTIES },
		{ B_DIRECT_SPECIFIER },
		"count Entry of ... # count entries in a PoseView",
		0,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{	kPropertyEntry,
		{ B_DELETE_PROPERTY },
		{ B_ENTRY_SPECIFIER, B_INDEX_SPECIFIER },
		"delete Entry {path|index} # deletes specified entries in a PoseView",
		0,
		{},
		{},
		{}
	},
	{	kPropertyEntry,
		{ B_GET_PROPERTY },
		{ B_DIRECT_SPECIFIER, B_INDEX_SPECIFIER, kPreviousSpecifier, kNextSpecifier },
		"get Entry [next|previous|index] # returns specified entries",
		0,
		{ B_REF_TYPE },
		{},
		{}
	},
	{	kPropertyEntry,
		{ B_EXECUTE_PROPERTY },
		{ B_ENTRY_SPECIFIER, B_INDEX_SPECIFIER },
		"execute Entry {path|index}	# opens specified entries",
		0,
		{ B_REF_TYPE },
		{},
		{}
	},
	{	kPropertySelection,
		{ B_GET_PROPERTY },
		{ B_DIRECT_SPECIFIER, kPreviousSpecifier, kNextSpecifier },
		"get Selection [next|previous] # returns the selected entries",
		0,
		{ B_REF_TYPE },
		{},
		{}
	},
	{	kPropertySelection,
		{ B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER, kPreviousSpecifier, kNextSpecifier },
		"set Selection of ... to {next|previous|entry} # selects specified entries",
		0,
		{},
		{},
		{}
	},
	{	kPropertySelection,
		{ B_COUNT_PROPERTIES },
		{ B_DIRECT_SPECIFIER },
		"count Selection of ... # counts selected items",
		0,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{	kPropertySelection,
		{ B_CREATE_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"create selection of ... to {entry|index} "
		"# adds specified items to a selection in a PoseView",
		0,
		{},
		{},
		{}
	},
	{	kPropertySelection,
		{ B_DELETE_PROPERTY },
		{ B_ENTRY_SPECIFIER, B_INDEX_SPECIFIER },
		"delete selection {path|index} of ... "
		"# removes specified items from a selection in a PoseView",
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

#endif


status_t
BPoseView::GetSupportedSuites(BMessage* _SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", kPosesSuites);
	BPropertyInfo propertyInfo(const_cast<property_info*>(kPosesPropertyList));
	data->AddFlat("messages", &propertyInfo);
	
	return _inherited::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}


bool
BPoseView::HandleScriptingMessage(BMessage* _SCRIPTING_ONLY(message))
{
#if _SUPPORTS_FEATURE_SCRIPTING
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
			handled = CreateProperty(message, &specifier, form, property, &reply);
			break;

		case B_GET_PROPERTY:
			handled = GetProperty(&specifier, form, property, &reply);
			break;
		
		case B_SET_PROPERTY:
			handled = SetProperty(message, &specifier, form, property, &reply);
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
#else
	return false;
#endif
}


bool
BPoseView::ExecuteProperty(BMessage* _SCRIPTING_ONLY(specifier),
	int32 _SCRIPTING_ONLY(form), const char* _SCRIPTING_ONLY(property),
	BMessage* _SCRIPTING_ONLY(reply))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t error = B_OK;
	bool handled = false;
	if (strcmp(property, kPropertyEntry) == 0) {
		BMessage launchMessage(B_REFS_RECEIVED);
		
		if (form == (int32)B_ENTRY_SPECIFIER) {
			// move all poses specified by entry_ref to Trash
			entry_ref ref;
			for (int32 index = 0; specifier->FindRef("refs", index, &ref)
				== B_OK; index++)
				launchMessage.AddRef("refs", &ref);

		} else if (form == (int32)B_INDEX_SPECIFIER) {
			// move all poses specified by index to Trash
			int32 specifyingIndex;
			for (int32 index = 0; specifier->FindInt32("index", index,
				&specifyingIndex) == B_OK; index++) {
				BPose* pose = PoseAtIndex(specifyingIndex);

				if (!pose) {
					error = B_ENTRY_NOT_FOUND;
					break;
				}

				launchMessage.AddRef("refs", pose->TargetModel()->EntryRef());
			}
		} else
			return false;

		if (error == B_OK) {
			// add a messenger to the launch message that will be used to
			// dispatch scripting calls from apps to the PoseView
			launchMessage.AddMessenger("TrackerViewToken", BMessenger(this, 0, 0));
			if (fSelectionHandler)
				fSelectionHandler->PostMessage(&launchMessage);
		}
		handled = true;
	}

	if (error != B_OK)
		reply->AddInt32("error", error);

	return handled;
#else
	return false;
#endif
}


bool
BPoseView::CreateProperty(BMessage* _SCRIPTING_ONLY(specifier), BMessage*,
	int32 _SCRIPTING_ONLY(form), const char* _SCRIPTING_ONLY(property),
	BMessage* _SCRIPTING_ONLY(reply))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t error = B_OK;
	bool handled = false;
	if (strcmp(property, kPropertySelection) == 0) {
		// creating on a selection expands the current selection

		if (form != B_DIRECT_SPECIFIER)
			// only support direct specifier
			return false;

		// items to add to a selection may be passed as refs or as indices
		if (specifier->HasRef("data")) {
			entry_ref ref;
			// select poses specified by entries
			for (int32 index = 0; specifier->FindRef("data", index, &ref)
				== B_OK; index++) {

				int32 poseIndex;
				BPose* pose = FindPose(&ref, form, &poseIndex);

				if (!pose) {
					error = B_ENTRY_NOT_FOUND;
					handled = true;
					break;
				}

				AddPoseToSelection(pose, poseIndex);
			}
			handled = true;
		} else {
			// select poses specified by indices
			int32 specifyingIndex;
			for (int32 index = 0; specifier->FindInt32("data", index,
				&specifyingIndex) == B_OK; index++) {

				BPose* pose = PoseAtIndex(specifyingIndex);
				if (!pose) {
					error = B_BAD_INDEX;
					handled = true;
					break;
				}

				AddPoseToSelection(pose, specifyingIndex);
			}
			handled = true;
		}
	}

	if (error != B_OK)
		reply->AddInt32("error", error);

	return handled;
#else
	return false;
#endif
}


bool
BPoseView::DeleteProperty(BMessage* _SCRIPTING_ONLY(specifier),
	int32 _SCRIPTING_ONLY(form), const char* _SCRIPTING_ONLY(property),
	BMessage* _SCRIPTING_ONLY(reply))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t error = B_OK;
	bool handled = false;

	if (strcmp(property, kPropertySelection) == 0) {
		// deleting on a selection is handled as removing a part of the selection
		// not to be confused with deleting a selected item

		if (form == (int32)B_ENTRY_SPECIFIER) {
			entry_ref ref;
			// select poses specified by entries
			for (int32 index = 0; specifier->FindRef("refs", index, &ref)
				== B_OK; index++) {

				int32 poseIndex;
				BPose* pose = FindPose(&ref, form, &poseIndex);

				if (!pose) {
					error = B_ENTRY_NOT_FOUND;
					break;
				}

				RemovePoseFromSelection(pose);
			}
			handled = true;

		} else if (form == B_INDEX_SPECIFIER) {
			// move all poses specified by index to Trash
			int32 specifyingIndex;
			for (int32 index = 0; specifier->FindInt32("index", index,
				&specifyingIndex) == B_OK; index++) {
				BPose* pose = PoseAtIndex(specifyingIndex);

				if (!pose) {
					error = B_BAD_INDEX;
					break;
				}

				RemovePoseFromSelection(pose);
			}
			handled = true;
		} else
			return false;

	} else if (strcmp(property, kPropertyEntry) == 0) {
		// deleting entries is handled by moving entries to trash
	
		// build a list of entries, specified by the specifier
		BObjectList<entry_ref>* entryList = new BObjectList<entry_ref>();
			// list will be deleted for us by the trashing thread

		if (form == (int32)B_ENTRY_SPECIFIER) {
			// move all poses specified by entry_ref to Trash
			entry_ref ref;
			for (int32 index = 0; specifier->FindRef("refs", index, &ref)
				== B_OK; index++)
				entryList->AddItem(new entry_ref(ref));

		} else if (form == (int32)B_INDEX_SPECIFIER) {
			// move all poses specified by index to Trash
			int32 specifyingIndex;
			for (int32 index = 0; specifier->FindInt32("index", index, &specifyingIndex)
				== B_OK; index++) {
				BPose* pose = PoseAtIndex(specifyingIndex);

				if (!pose) {
					error = B_BAD_INDEX;
					break;
				}

				entryList->AddItem(new entry_ref(*pose->TargetModel()->EntryRef()));
			}
		} else
			return false;

		if (error == B_OK) {
			TrackerSettings settings;
			if (!settings.DontMoveFilesToTrash()) {
				// move the list we build into trash, don't make the trashing task
				// select the next item
				MoveListToTrash(entryList, false, false);
			} else
				Delete(entryList, false, settings.AskBeforeDeleteFile());
		}

		handled = true;
	}

	if (error != B_OK)
		reply->AddInt32("error", error);

	return handled;
#else
	return false;
#endif
}


bool
BPoseView::CountProperty(BMessage*, int32, const char* _SCRIPTING_ONLY(property),
	BMessage* _SCRIPTING_ONLY(reply))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	bool handled = false;
//	PRINT(("BPoseView::CountProperty, %s\n", property));

	// just return the respecitve counts
	if (strcmp(property, kPropertySelection) == 0) {
		reply->AddInt32("result", fSelectionList->CountItems());
		handled = true;
	} else if (strcmp(property, kPropertyEntry) == 0) {
		reply->AddInt32("result", fPoseList->CountItems());
		handled = true;
	}
	return handled;
#else
	return false;
#endif
}


bool
BPoseView::GetProperty(BMessage* _SCRIPTING_ONLY(specifier),
	int32 _SCRIPTING_ONLY(form), const char* _SCRIPTING_ONLY(property),
	BMessage* _SCRIPTING_ONLY(reply))
{
#if _SUPPORTS_FEATURE_SCRIPTING
//	PRINT(("GetProperty %s\n", property));
	bool handled = false;
	status_t error = B_OK;

	if (strcmp(property, kPropertyPath) == 0) {
		if (form == B_DIRECT_SPECIFIER) {
			handled = true;
			if (!TargetModel())
				error = B_NOT_A_DIRECTORY;
			else
				reply->AddRef("result", TargetModel()->EntryRef());
		}
	} else if (strcmp(property, kPropertySelection) == 0) {
		int32 count = fSelectionList->CountItems();
		switch (form) {
			case B_DIRECT_SPECIFIER:
				// return entries of all poses in selection
				for (int32 index = 0; index < count; index++)
					reply->AddRef("result", fSelectionList->ItemAt(index)->
						TargetModel()->EntryRef());

				handled = true;
				break;

			case kPreviousSpecifier:
			case kNextSpecifier:
				{
					// return entry and index of selected pose before or after
					// specified pose
					entry_ref ref;
					if (specifier->FindRef("data", &ref) != B_OK)
						break;
					
					int32 poseIndex;
					BPose* pose = FindPose(&ref, &poseIndex);
					
					for (;;) {
						if (form == (int32)kPreviousSpecifier)
							pose = PoseAtIndex(--poseIndex);
						else if (form == (int32)kNextSpecifier)
							pose = PoseAtIndex(++poseIndex);
						
						if (!pose) {
							error = B_ENTRY_NOT_FOUND;
							break;
						}
						
						if (pose->IsSelected()) {
							reply->AddRef("result", pose->TargetModel()->EntryRef());
							reply->AddInt32("index", IndexOfPose(pose));
							break;
						}
					}
			
					handled = true;
					break;
				}
		}
	} else if (strcmp(property, kPropertyEntry) == 0) {
		int32 count = fPoseList->CountItems();
		switch (form) {
			case B_DIRECT_SPECIFIER:
			{
				// return all entries of all poses in PoseView
				for (int32 index = 0; index < count; index++)
					reply->AddRef("result", PoseAtIndex(index)->TargetModel()->EntryRef());

				handled = true;
				break;
			}

			case B_INDEX_SPECIFIER:
			{
				// return entry at index
				int32 index;
				if (specifier->FindInt32("index", &index) != B_OK)
					break;
				
				if (!PoseAtIndex(index)) {
					error = B_BAD_INDEX;
					handled = true;
					break;
				}
				reply->AddRef("result", PoseAtIndex(index)->TargetModel()->EntryRef());

				handled = true;
				break;
			}

			case kPreviousSpecifier:
			case kNextSpecifier:
			{
				// return entry and index of pose before or after specified pose
				entry_ref ref;
				if (specifier->FindRef("data", &ref) != B_OK)
					break;
				
				int32 tmp;
				BPose* pose = FindPose(&ref, form, &tmp);

				if (!pose) {
					error = B_ENTRY_NOT_FOUND;
					handled = true;
					break;
				}
				
				reply->AddRef("result", pose->TargetModel()->EntryRef());
				reply->AddInt32("index", IndexOfPose(pose));

				handled = true;
				break;
			}
		}
	}
	
	if (error != B_OK)
		reply->AddInt32("error", error);

	return handled;
#else
	return false;
#endif
}


bool
BPoseView::SetProperty(BMessage* _SCRIPTING_ONLY(message), BMessage*,
	int32 _SCRIPTING_ONLY(form), const char* _SCRIPTING_ONLY(property),
	BMessage* _SCRIPTING_ONLY(reply))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t error = B_OK;
	bool handled = false;

	if (strcmp(property, kPropertySelection) == 0) {
		entry_ref ref;

		switch (form) {
			case B_DIRECT_SPECIFIER:
			{
				int32 selStart;
				int32 selEnd;
				if (message->FindInt32("data", 0, &selStart) == B_OK
					&& message->FindInt32("data", 1, &selEnd) == B_OK) {

					if (selStart < 0 || selStart >= fPoseList->CountItems()
						|| selEnd < 0 || selEnd >= fPoseList->CountItems()) {
						error = B_BAD_INDEX;
						handled = true;
						break;
					}

					SelectPoses(selStart, selEnd);
					handled = true;
					break;
				}
			} // fall thru
			case kPreviousSpecifier:
			case kNextSpecifier:
			{
				// PRINT(("SetProperty direct/previous/next %s\n", property));
				// select/unselect poses specified by entries
				bool clearSelection = true;
				for (int32 index = 0; message->FindRef("data", index, &ref)
					== B_OK; index++) {

					int32 poseIndex;
					BPose* pose = FindPose(&ref, form, &poseIndex);
					
					if (!pose) {
						error = B_ENTRY_NOT_FOUND;
						handled = true;
						break;
					}
						
					if (clearSelection) {
						// first selected item must call SelectPose so the selection
						// gets cleared first
						SelectPose(pose, poseIndex);
						clearSelection = false;
					} else
						AddPoseToSelection(pose, poseIndex);

					handled = true;
				}
				break;
			}
		}
	}

	if (error != B_OK)
		reply->AddInt32("error", error);

	return handled;
#else
	return false;
#endif
}


BHandler*
BPoseView::ResolveSpecifier(BMessage* _SCRIPTING_ONLY(message),
	int32 _SCRIPTING_ONLY(index), BMessage* _SCRIPTING_ONLY(specifier),
	int32 _SCRIPTING_ONLY(form), const char* _SCRIPTING_ONLY(property))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	BPropertyInfo propertyInfo(const_cast<property_info*>(kPosesPropertyList));

	int32 result = propertyInfo.FindMatch(message, index, specifier, form, property);
	if (result < 0) {
// 		PRINT(("FindMatch result %d \n"));
		return _inherited::ResolveSpecifier(message, index, specifier,
			form, property);
	}

	return this;
#else
	return NULL;
#endif
}


BPose*
BPoseView::FindPose(const entry_ref* _SCRIPTING_ONLY(ref),
	int32 _SCRIPTING_ONLY(specifierForm), int32* _SCRIPTING_ONLY(index)) const
{
#if _SUPPORTS_FEATURE_SCRIPTING
	// flavor of FindPose, used by previous/next specifiers

	BPose* pose = FindPose(ref, index);
	
	if (specifierForm == (int32)kPreviousSpecifier)
		return PoseAtIndex(--*index);
	else if (specifierForm == (int32)kNextSpecifier)
		return PoseAtIndex(++*index);
	else
		return pose;
#else
	return NULL;
#endif
}

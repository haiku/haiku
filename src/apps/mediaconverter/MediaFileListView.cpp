// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#include "MediaFileListView.h"

#include <Application.h>
#include <MediaFile.h>
#include <Messenger.h>

#include "MediaConverterWindow.h"
#include "MessageConstants.h"


// #pragma mark - MediaFileListItem


MediaFileListItem::MediaFileListItem(BMediaFile* file, const entry_ref& ref)
	: BStringItem(ref.name),
	  fRef(ref),
	  fMediaFile(file)
{
}


MediaFileListItem::~MediaFileListItem()
{
	delete fMediaFile;
}


// #pragma mark - MediaFileListView


MediaFileListView::MediaFileListView(BRect frame, uint32 resizingMode)
	: BListView(frame, "MediaFileListView", B_SINGLE_SELECTION_LIST, resizingMode,
				B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
{
	fEnabled = true;
}


MediaFileListView::~MediaFileListView()
{
	BListItem *item;
	while ((item = RemoveItem((int32)0)) != NULL) {
		delete item;
	}
}


void 
MediaFileListView::SetEnabled(bool enabled)
{
	if (enabled == fEnabled)
		return;

	fEnabled = enabled;
	// TODO: visual indication of enabled status?
}


bool 
MediaFileListView::IsEnabled() const
{
	return fEnabled;
}


void
MediaFileListView::AddItem(BMediaFile* file, const entry_ref& ref)
{
	BListView::AddItem(new MediaFileListItem(file, ref));
	be_app_messenger.SendMessage(FILE_LIST_CHANGE_MESSAGE);
}


void 
MediaFileListView::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_DELETE:
			if (IsEnabled()) {
				int32 selection = CurrentSelection();
				if (selection >= 0) {
					delete RemoveItem(selection);
					// select the previous item
					int32 count = CountItems();
					if (selection >= count)
						selection = count - 1;
					Select(selection);
					be_app_messenger.SendMessage(FILE_LIST_CHANGE_MESSAGE);
				}		
			}
			break;
		default:
			BListView::KeyDown(bytes, numBytes);
	}
}


void 
MediaFileListView::SelectionChanged()
{
	MediaConverterWindow* win = dynamic_cast<MediaConverterWindow *>(Window());
	if (win != NULL)
		win->SourceFileSelectionChanged();
}


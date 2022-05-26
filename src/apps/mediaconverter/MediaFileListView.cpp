// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, 2009 Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.


#include "MediaFileListView.h"

#include <new>

#include <Application.h>
#include <MediaFile.h>
#include <Messenger.h>

#include "MediaConverterWindow.h"
#include "MessageConstants.h"


// #pragma mark - MediaFileListItem


MediaFileListItem::MediaFileListItem(BMediaFile* file, const entry_ref& ref)
	:
	BStringItem(ref.name),
	fRef(ref),
	fMediaFile(file)
{
}


MediaFileListItem::~MediaFileListItem()
{
	delete fMediaFile;
}


// #pragma mark - MediaFileListView


MediaFileListView::MediaFileListView()
	:
	BListView("MediaFileListView", B_SINGLE_SELECTION_LIST, B_WILL_DRAW
			| B_NAVIGABLE | B_FRAME_EVENTS)
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


bool
MediaFileListView::AddMediaItem(BMediaFile* file, const entry_ref& ref)
{
	MediaFileListItem* item = new(std::nothrow) MediaFileListItem(file, ref);
	if (item == NULL || !AddItem(item)) {
		delete item;
		return false;
	}
	be_app_messenger.SendMessage(FILE_LIST_CHANGE_MESSAGE);
	return true;
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
			break;
	}
}


void
MediaFileListView::SelectionChanged()
{
	MediaConverterWindow* win = dynamic_cast<MediaConverterWindow*>(Window());
	if (win != NULL)
		win->SourceFileSelectionChanged();
}

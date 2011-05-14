/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef MUSIC_FILE_LIST_VIEW_H
#define MUSIC_FILE_LIST_VIEW_H


#include <Bitmap.h>
#include <ListItem.h>
#include <OutlineListView.h>
#include <Roster.h>

#include "QueryMonitor.h"


class FileListItem : public BStringItem {
public:
	FileListItem(const char* text, WatchedFile* file = NULL)
		:
		BStringItem(text),
		fFile(file)
	{
	}


	WatchedFile*
	File()
	{
		return fFile;
	}

private:
			WatchedFile*		fFile;
};


class MusicFileListView : public BOutlineListView {
public:

	MusicFileListView(const char *name)
		:		
		BOutlineListView(name)
	{
	}


	bool
	InitiateDrag(BPoint where, int32 index, bool wasSelected)
	{
		int32 itemIndex = IndexOf(where);
		FileListItem* item = (FileListItem*)ItemAt(itemIndex);
		if (item == NULL)
			return false;

		const char* text = item->Text();

		BRect rect(0, 0, 200, 50);
		BBitmap* bitmap = new BBitmap(rect, B_RGB32, true);
		BView* bitmapView = new BView(rect, "bitmap", B_FOLLOW_NONE,
			B_WILL_DRAW);

		bitmap->Lock();
		bitmap->AddChild(bitmapView);

		bitmapView->SetLowColor(255, 255, 255, 0);	//	transparent
		bitmapView->SetHighColor(0, 0, 0, 100);
		bitmapView->SetDrawingMode(B_OP_COPY);
		bitmapView->FillRect(bitmapView->Bounds(), B_SOLID_LOW);

		bitmapView->SetDrawingMode(B_OP_OVER);
		font_height height;
		bitmapView->GetFontHeight(&height);
		float fontHeight = height.ascent + height.descent;
		BRect latchRect = LatchRect(BRect(0, 0, item->Width(), item->Height()),
			item->OutlineLevel());
		bitmapView->DrawString(text, BPoint(latchRect.Width(), fontHeight));

		bitmapView->Sync();
		bitmap->Unlock();

		BMessage dragMessage(B_SIMPLE_DATA); 
		dragMessage.AddPoint("click_location", where); 

		_RecursiveAddRefs(dragMessage, item);

		BRect itemFrame(ItemFrame(itemIndex));
		BPoint pt(where.x + itemFrame.left, where.y - itemFrame.top);
		DragMessage(&dragMessage, bitmap, B_OP_ALPHA, pt, this);

		return true;
	}


	void
	Launch(BMessage* message)
	{
		int32 index;
		for (int32 i = 0; ; i++) {
			if (message->FindInt32("index", i, &index) != B_OK)
				break;
			FileListItem* item = (FileListItem*)ItemAt(index);

			BMessage refs(B_REFS_RECEIVED);
			_RecursiveAddRefs(refs, item);
			be_roster->Launch("application/x-vnd.Haiku-MediaPlayer", &refs);
		}
	};

private:

	void
	_RecursiveAddRefs(BMessage& message, FileListItem* item)
	{
		WatchedFile* file = item->File();
		if (file != NULL) {
			message.AddRef("refs", &(file->entry));
		} else {
			for (int32 i = 0; i < CountItemsUnder(item, true); i++) {
				_RecursiveAddRefs(message, (FileListItem*)ItemUnderAt(
					item, true, i));
			}
		}
	}
};

#endif // MUSIC_FILE_LIST_VIEW_H

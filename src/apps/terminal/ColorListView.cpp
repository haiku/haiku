/*
 * Copyright 2016-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "ColorListView.h"

#include <algorithm>

#include <math.h>
#include <stdio.h>

#include <Bitmap.h>
#include <String.h>

#include "ColorItem.h"
#include "ThemeView.h"


// golden ratio
#ifdef M_PHI
#	undef M_PHI
#endif
#define M_PHI 1.61803398874989484820


//	#pragma mark - ColorListView


ColorListView::ColorListView(const char* name,
	list_view_type type, uint32 flags)
	:
	BListView(name, type, flags)
{
}


ColorListView::~ColorListView()
{
}


bool
ColorListView::InitiateDrag(BPoint where, int32 index, bool wasSelected)
{
	ColorItem* colorItem
		= dynamic_cast<ColorItem*>(ItemAt(index));
	if (colorItem == NULL)
		return false;

	rgb_color color = colorItem->Color();

	BString hexStr;
	hexStr.SetToFormat("#%.2X%.2X%.2X", color.red, color.green, color.blue);

	BMessage message(B_PASTE);
	message.AddData("text/plain", B_MIME_TYPE, hexStr.String(),
		hexStr.Length());
	message.AddData(kRGBColor, B_RGB_COLOR_TYPE, &color, sizeof(color));

	float itemHeight = colorItem->Height() - 5;
	BRect rect(0.0f, 0.0f, roundf(itemHeight * M_PHI) - 1, itemHeight - 1);

	BBitmap* bitmap = new BBitmap(rect, B_RGB32, true);
	if (bitmap->Lock()) {
		BView* view = new BView(rect, "", B_FOLLOW_NONE, B_WILL_DRAW);
		bitmap->AddChild(view);

		view->SetHighColor(B_TRANSPARENT_COLOR);
		view->FillRect(view->Bounds());

		++rect.top;
		++rect.left;

		view->SetHighColor(0, 0, 0, 100);
		view->FillRect(rect);
		rect.OffsetBy(-1.0f, -1.0f);

		view->SetHighColor(std::min(255, (int)(1.2 * color.red + 40)),
			std::min(255, (int)(1.2 * color.green + 40)),
			std::min(255, (int)(1.2 * color.blue + 40)));
		view->StrokeRect(rect);

		++rect.left;
		++rect.top;

		view->SetHighColor((int32)(0.8 * color.red),
			(int32)(0.8 * color.green),
			(int32)(0.8 * color.blue));
		view->StrokeRect(rect);

		--rect.right;
		--rect.bottom;

		view->SetHighColor(color.red, color.green, color.blue);
		view->FillRect(rect);
		view->Sync();

		bitmap->Unlock();
	}

	DragMessage(&message, bitmap, B_OP_ALPHA, BPoint(14.0f, 14.0f));

	return true;
}


void
ColorListView::MessageReceived(BMessage* message)
{
	// if we received a dropped message, see if it contains color data
	if (message->WasDropped()) {
		BPoint dropPoint = message->DropPoint();
		ConvertFromScreen(&dropPoint);
		int32 index = IndexOf(dropPoint);
		ColorItem* item = dynamic_cast<ColorItem*>(ItemAt(index));
		rgb_color* color;
		ssize_t size;
		if (item != NULL && message->FindData(kRGBColor, B_RGB_COLOR_TYPE,
				(const void**)&color, &size) == B_OK) {
			// build message to send to ThemeView
			uint32 command = index == CurrentSelection()
				? MSG_SET_CURRENT_COLOR : MSG_SET_COLOR;
			BMessage setColorMessage = BMessage(command);
			setColorMessage.AddData(kRGBColor, B_RGB_COLOR_TYPE, color, size);
			// if setting different color, add which color to set
			if (command == MSG_SET_COLOR)
				setColorMessage.AddString(kName, item->Text());

			// build messenger and send message
			BMessenger messenger = BMessenger(Parent());
			if (messenger.IsValid()) {
				messenger.SendMessage(&setColorMessage);
				if (command == MSG_SET_COLOR) {
					// redraw item, MSG_SET_CURRENT_COLOR does this for us
					item->SetColor(*color);
					InvalidateItem(index);
				}
			}
		}
	}

	BListView::MessageReceived(message);
}

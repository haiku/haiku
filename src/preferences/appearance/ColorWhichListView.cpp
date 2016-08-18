/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "ColorWhichListView.h"

#include <algorithm>

#include <math.h>
#include <stdio.h>

#include <Bitmap.h>

#include "ColorWhichItem.h"


// golden ratio
#ifdef M_PHI
#	undef M_PHI
#endif
#define M_PHI 1.61803398874989484820


//	#pragma mark - ColorWhichListView


ColorWhichListView::ColorWhichListView(const char* name,
	list_view_type type, uint32 flags)
	:
	BListView(name, type, flags)
{
}


ColorWhichListView::~ColorWhichListView()
{
}


bool
ColorWhichListView::InitiateDrag(BPoint where, int32 index, bool wasSelected)
{
	ColorWhichItem* colorWhichItem
		= dynamic_cast<ColorWhichItem*>(ItemAt(index));
	if (colorWhichItem == NULL)
		return false;

	rgb_color color = colorWhichItem->Color();

	char hexString[7];
	sprintf(hexString, "#%.2X%.2X%.2X", color.red, color.green, color.blue);

	BMessage message(B_PASTE);
	message.AddData("text/plain", B_MIME_TYPE, &hexString, sizeof(hexString));
	message.AddData("RGBColor", B_RGB_COLOR_TYPE, &color, sizeof(color));

	float itemHeight = colorWhichItem->Height() - 5;
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

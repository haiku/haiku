/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TableCellValueRendererUtils.h"

#include <Font.h>
#include <String.h>
#include <View.h>


static const float kTextMargin = 8;


/*static*/ void
TableCellValueRendererUtils::DrawString(BView* view, BRect rect,
	const char* string, enum alignment alignment, bool truncate)
{
	// get font height info
	font_height	fontHeight;
	view->GetFontHeight(&fontHeight);

	// truncate, if requested
	BString truncatedString;
	if (truncate) {
		truncatedString = string;
		view->TruncateString(&truncatedString, B_TRUNCATE_END,
			rect.Width() - 2 * kTextMargin + 2);
		string = truncatedString.String();
	}

	// compute horizontal position according to alignment
	float x;
	switch (alignment) {
		default:
		case B_ALIGN_LEFT:
			x = rect.left + kTextMargin;
			break;

		case B_ALIGN_CENTER:
			x = rect.left + (rect.Width() - view->StringWidth(string)) / 2;
			break;

		case B_ALIGN_RIGHT:
			x = rect.right - kTextMargin - view->StringWidth(string);
			break;
	}

	// compute vertical position (base line)
	float y = rect.top
		+ (rect.Height() - (fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading)) / 2
		+ (fontHeight.ascent + fontHeight.descent) - 2;
		// TODO: This is the computation BColumnListView (respectively
		// BTitledColumn) is using, which I find somewhat weird.

	view->DrawString(string, BPoint(x, y));
}


/*static*/ float
TableCellValueRendererUtils::PreferredStringWidth(BView* view,
	const char* string)
{
	return view->StringWidth(string) + 2 * kTextMargin;
}

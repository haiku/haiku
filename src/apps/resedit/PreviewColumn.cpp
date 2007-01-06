/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#include "PreviewColumn.h"
#include <stdio.h>

PreviewColumn::PreviewColumn(const char *title, float width,
						 float minWidth, float maxWidth)
  :	BTitledColumn(title, width, minWidth, maxWidth)
{
}

void
PreviewColumn::DrawField(BField* field, BRect rect, BView* parent)
{
	BIntegerField *intField = dynamic_cast<BIntegerField*>(field);
	BStringField *strField = dynamic_cast<BStringField*>(field);
	BBitmapField *bmpField = dynamic_cast<BBitmapField*>(field);
	
	if (intField) {
		float width = rect.Width() - 16;
		BString	string;
		string << intField->Value();
		parent->TruncateString(&string, B_TRUNCATE_MIDDLE, width + 2);
		DrawString(string.String(), parent, rect);
	} else if (strField) {
		float width = rect.Width() - 16;
		if (width != strField->Width()) {
			BString out_string(strField->String());
	
			parent->TruncateString(&out_string, B_TRUNCATE_END, width + 2);
			strField->SetClippedString(out_string.String());
			strField->SetWidth(width);
		}
		DrawString(strField->ClippedString(), parent, rect);
	} else if (bmpField) {
		const BBitmap *bitmap = bmpField->Bitmap();
	
		if (bitmap != NULL) {
			// Scale the bitmap down to completely fit within the field's height
			BRect drawrect(bitmap->Bounds().OffsetToCopy(rect.LeftTop()));
			if (drawrect.Height() > rect.Height()) {
				drawrect = rect;
				drawrect.right = drawrect.left + 
								(bitmap->Bounds().Width() * 
								(rect.Height() / bitmap->Bounds().Height()));
			}
			
			parent->SetDrawingMode(B_OP_ALPHA);
			parent->DrawBitmap(bitmap, bitmap->Bounds(), drawrect);
			parent->SetDrawingMode(B_OP_COPY);
			
			BString out;
			out << bitmap->Bounds().IntegerWidth() << " x " 
				<< bitmap->Bounds().IntegerHeight()	<< " x " 
				<< (int32(bitmap->BytesPerRow() / bitmap->Bounds().Width()) * 8);
			
			BRect stringrect = rect;
			stringrect.right -= 10;
			stringrect.left = stringrect.right - parent->StringWidth(out.String());
			if (stringrect.left < drawrect.right + 5)
				stringrect.left = drawrect.right + 5;
			
			parent->TruncateString(&out, B_TRUNCATE_END, stringrect.Width());
			DrawString(out.String(), parent, stringrect);
		}
	}
}

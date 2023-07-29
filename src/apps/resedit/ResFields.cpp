/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "ResFields.h"
#include "ResourceData.h"

#include <DataIO.h>
#include <TranslationUtils.h>

TypeCodeField::TypeCodeField(const type_code &code, ResourceData *data)
  :	BStringField(MakeTypeString(code).String()),
	fTypeCode(code),
	fData(data)
{
}


PreviewField::PreviewField(void)
{
}


PreviewField::~PreviewField(void)
{
}


BitmapPreviewField::BitmapPreviewField(BBitmap *bitmap)
{
	fBitmap = bitmap;
}


BitmapPreviewField::~BitmapPreviewField(void)
{
	delete fBitmap;
}


void
BitmapPreviewField::DrawField(BRect rect, BView *parent)
{
	if (fBitmap) {
		// Scale the fBitmap down to completely fit within the field's height
		BRect drawrect(fBitmap->Bounds().OffsetToCopy(rect.LeftTop()));
		if (drawrect.Height() > rect.Height()) {
			drawrect = rect;
			drawrect.right = drawrect.left +
							(fBitmap->Bounds().Width() *
							(rect.Height() / fBitmap->Bounds().Height()));
		}

		parent->SetDrawingMode(B_OP_ALPHA);
		parent->DrawBitmap(fBitmap, fBitmap->Bounds(), drawrect);
		parent->SetDrawingMode(B_OP_COPY);

		BString out;
		out.SetToFormat("%" B_PRId32" × %" B_PRId32" × %" B_PRId32,
			fBitmap->Bounds().IntegerWidth(), fBitmap->Bounds().IntegerHeight(),
			(int32(fBitmap->BytesPerRow() / fBitmap->Bounds().Width()) * 8));

		BRect stringrect = rect;
		stringrect.right -= 10;
		stringrect.left = stringrect.right - parent->StringWidth(out.String());
		if (stringrect.left < drawrect.right + 5)
			stringrect.left = drawrect.right + 5;

		parent->TruncateString(&out, B_TRUNCATE_END, stringrect.Width());
		parent->DrawString(out.String(), stringrect.LeftTop());
	}
}


void
BitmapPreviewField::SetData(char *data, const size_t &length)
{
	BMemoryIO memio(data, length);
	fBitmap = BTranslationUtils::GetBitmap(&memio);
}


IntegerPreviewField::IntegerPreviewField(const int64 &value)
{
	fValue = value;
}


IntegerPreviewField::~IntegerPreviewField(void)
{
}


void
IntegerPreviewField::DrawField(BRect rect, BView *parent)
{
	BPoint point(rect.LeftBottom());
	point.y -= 2;
	
	BString	string;
	string << fValue;
	parent->TruncateString(&string, B_TRUNCATE_MIDDLE, rect.Width() - 14);
	parent->DrawString(string.String(), point);
}


void
IntegerPreviewField::SetData(char *data, const size_t &length)
{
	switch(length) {
		case 8: {
			fValue = *((int8*)data);
			break;
		}
		case 16: {
			fValue = *((int16*)data);
			break;
		}
		case 32: {
			fValue = *((int32*)data);
			break;
		}
		case 64: {
			fValue = *((int32*)data);
			break;
		}
		default: {
			break;
		}
	}
}


StringPreviewField::StringPreviewField(const char *string)
{
	fClipped = fString = string;
}


StringPreviewField::~StringPreviewField(void)
{
}


void
StringPreviewField::DrawField(BRect rect, BView *parent)
{
	BPoint point(rect.LeftBottom());
	point.y -= 2;
	
	fClipped = fString;
	parent->TruncateString(&fClipped, B_TRUNCATE_END, rect.Width() - 14);
	parent->DrawString(fClipped.String(), point);
}


void
StringPreviewField::SetData(char *data, const size_t &length)
{
	char *temp = fString.LockBuffer(length + 2);
	if (temp)
		memcpy(temp, data, length);
	temp[length] = '\0';
	fString.UnlockBuffer();
	fClipped = fString;
}


BString
MakeTypeString(int32 type)
{
	char typestring[7];
	char *typeptr = (char *) &type;
	typestring[0] = '\'';
	typestring[1] = typeptr[3];
	typestring[2] = typeptr[2];
	typestring[3] = typeptr[1];
	typestring[4] = typeptr[0];
	typestring[5] = '\'';
	typestring[6] = '\0';
	return BString(typestring);
}

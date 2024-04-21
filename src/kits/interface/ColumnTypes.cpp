/*******************************************************************************
/
/	File:			ColumnTypes.h
/
/   Description:    Experimental classes that implement particular column/field
/					data types for use in BColumnListView.
/
/	Copyright 2000+, Be Incorporated, All Rights Reserved
/	Copyright 2024, Haiku, Inc. All Rights Reserved
/
*******************************************************************************/


#include "ColumnTypes.h"

#include <StringFormat.h>
#include <SystemCatalog.h>
#include <View.h>

#include <stdio.h>


using BPrivate::gSystemCatalog;

#undef B_TRANSLATE_COMMENT
#define B_TRANSLATE_COMMENT(str, comment) \
	gSystemCatalog.GetString(B_TRANSLATE_MARK_COMMENT(str, comment), \
		B_TRANSLATION_CONTEXT, (comment))


#define kTEXT_MARGIN	8


BTitledColumn::BTitledColumn(const char* title, float width, float minWidth,
	float maxWidth, alignment align)
	:
	BColumn(width, minWidth, maxWidth, align),
	fTitle(title)
{
	font_height fh;

	be_plain_font->GetHeight(&fh);
	fFontHeight = fh.descent + fh.leading;
}


void
BTitledColumn::DrawTitle(BRect rect, BView* parent)
{
	float width = rect.Width() - (2 * kTEXT_MARGIN);
	BString out_string(fTitle);

	parent->TruncateString(&out_string, B_TRUNCATE_END, width + 2);
	DrawString(out_string.String(), parent, rect);
}


void
BTitledColumn::GetColumnName(BString* into) const
{
	*into = fTitle;
}


void
BTitledColumn::DrawString(const char* string, BView* parent, BRect rect)
{
	float width = rect.Width() - (2 * kTEXT_MARGIN);
	float y;
	BFont font;
	font_height	finfo;

	parent->GetFont(&font);
	font.GetHeight(&finfo);
	y = rect.top + finfo.ascent
		+ (rect.Height() - ceilf(finfo.ascent + finfo.descent)) / 2.0f;

	switch (Alignment()) {
		default:
		case B_ALIGN_LEFT:
			parent->MovePenTo(rect.left + kTEXT_MARGIN, y);
			break;

		case B_ALIGN_CENTER:
			parent->MovePenTo(rect.left + kTEXT_MARGIN
				+ ((width - font.StringWidth(string)) / 2), y);
			break;

		case B_ALIGN_RIGHT:
			parent->MovePenTo(rect.right - kTEXT_MARGIN
				- font.StringWidth(string), y);
			break;
	}

	parent->DrawString(string);
}


void
BTitledColumn::SetTitle(const char* title)
{
	fTitle.SetTo(title);
}


void
BTitledColumn::Title(BString* forTitle) const
{
	if (forTitle)
		forTitle->SetTo(fTitle.String());
}


float
BTitledColumn::FontHeight() const
{
	return fFontHeight;
}


float
BTitledColumn::GetPreferredWidth(BField *_field, BView* parent) const
{
	return parent->StringWidth(fTitle.String()) + 2 * kTEXT_MARGIN;
}


// #pragma mark - BStringField


BStringField::BStringField(const char* string)
	:
	fWidth(0),
	fString(string),
	fClippedString(string)
{
}


void
BStringField::SetString(const char* val)
{
	fString = val;
	fClippedString = "";
	fWidth = 0;
}


const char*
BStringField::String() const
{
	return fString.String();
}


void
BStringField::SetWidth(float width)
{
	fWidth = width;
}


float
BStringField::Width()
{
	return fWidth;
}


void
BStringField::SetClippedString(const char* val)
{
	fClippedString = val;
}


bool
BStringField::HasClippedString() const
{
	return !fClippedString.IsEmpty();
}


const char*
BStringField::ClippedString()
{
	return fClippedString.String();
}


// #pragma mark - BStringColumn


BStringColumn::BStringColumn(const char* title, float width, float minWidth,
	float maxWidth, uint32 truncate, alignment align)
	:
	BTitledColumn(title, width, minWidth, maxWidth, align),
	fTruncate(truncate)
{
}


void
BStringColumn::DrawField(BField* _field, BRect rect, BView* parent)
{
	float width = rect.Width() - (2 * kTEXT_MARGIN);
	BStringField* field = static_cast<BStringField*>(_field);
	float fieldWidth = field->Width();
	bool updateNeeded = width != fieldWidth;

	if (updateNeeded) {
		BString out_string(field->String());
		float preferredWidth = parent->StringWidth(out_string.String());
		if (width < preferredWidth) {
			parent->TruncateString(&out_string, fTruncate, width + 2);
			field->SetClippedString(out_string.String());
		} else
			field->SetClippedString("");
		field->SetWidth(width);
	}

	DrawString(field->HasClippedString()
		? field->ClippedString()
		: field->String(), parent, rect);
}


float
BStringColumn::GetPreferredWidth(BField *_field, BView* parent) const
{
	BStringField* field = static_cast<BStringField*>(_field);
	return parent->StringWidth(field->String()) + 2 * kTEXT_MARGIN;
}


int
BStringColumn::CompareFields(BField* field1, BField* field2)
{
	return ICompare(((BStringField*)field1)->String(),
		(((BStringField*)field2)->String()));
}


bool
BStringColumn::AcceptsField(const BField *field) const
{
	return static_cast<bool>(dynamic_cast<const BStringField*>(field));
}


// #pragma mark - BDateField


BDateField::BDateField(time_t* time)
	:
	fTime(*localtime(time)),
	fUnixTime(*time),
	fSeconds(0),
	fClippedString(""),
	fWidth(0)
{
	fSeconds = mktime(&fTime);
}


void
BDateField::SetWidth(float width)
{
	fWidth = width;
}


float
BDateField::Width()
{
	return fWidth;
}


void
BDateField::SetClippedString(const char* string)
{
	fClippedString = string;
}


const char*
BDateField::ClippedString()
{
	return fClippedString.String();
}


time_t
BDateField::Seconds()
{
	return fSeconds;
}


time_t
BDateField::UnixTime()
{
	return fUnixTime;
}


// #pragma mark - BDateColumn


BDateColumn::BDateColumn(const char* title, float width, float minWidth,
	float maxWidth, alignment align)
	:
	BTitledColumn(title, width, minWidth, maxWidth, align),
	fTitle(title)
{
}


void
BDateColumn::DrawField(BField* _field, BRect rect, BView* parent)
{
	float width = rect.Width() - (2 * kTEXT_MARGIN);
	BDateField* field = (BDateField*)_field;

	if (field->Width() != rect.Width()) {
		char dateString[256];
		time_t currentTime = field->UnixTime();
		tm time_data;
		BFont font;

		parent->GetFont(&font);
		localtime_r(&currentTime, &time_data);

		// dateStyles[] and timeStyles[] must be the same length
		const BDateFormatStyle dateStyles[] = {
			B_FULL_DATE_FORMAT, B_FULL_DATE_FORMAT, B_LONG_DATE_FORMAT, B_LONG_DATE_FORMAT,
			B_MEDIUM_DATE_FORMAT, B_SHORT_DATE_FORMAT,
		};

		const BTimeFormatStyle timeStyles[] = {
			B_MEDIUM_TIME_FORMAT, B_SHORT_TIME_FORMAT, B_MEDIUM_TIME_FORMAT, B_SHORT_TIME_FORMAT,
			B_SHORT_TIME_FORMAT, B_SHORT_TIME_FORMAT,
		};

		size_t index;
		for (index = 0; index < B_COUNT_OF(dateStyles); index++) {
			ssize_t output = fDateTimeFormat.Format(dateString, sizeof(dateString), currentTime,
				dateStyles[index], timeStyles[index]);
			if (output >= 0 && font.StringWidth(dateString) <= width)
				break;
		}

		if (index == B_COUNT_OF(dateStyles))
			fDateFormat.Format(dateString, sizeof(dateString), currentTime, B_SHORT_DATE_FORMAT);

		if (font.StringWidth(dateString) > width) {
			BString out_string(dateString);

			parent->TruncateString(&out_string, B_TRUNCATE_MIDDLE, width + 2);
			strcpy(dateString, out_string.String());
		}
		field->SetClippedString(dateString);
		field->SetWidth(width);
	}

	DrawString(field->ClippedString(), parent, rect);
}


int
BDateColumn::CompareFields(BField* field1, BField* field2)
{
	return((BDateField*)field1)->Seconds() - ((BDateField*)field2)->Seconds();
}


// #pragma mark - BSizeField


BSizeField::BSizeField(off_t size)
	:
	fSize(size)
{
}


void
BSizeField::SetSize(off_t size)
{
	fSize = size;
}


off_t
BSizeField::Size()
{
	return fSize;
}


// #pragma mark - BSizeColumn


BSizeColumn::BSizeColumn(const char* title, float width, float minWidth,
	float maxWidth, alignment align)
	:
	BTitledColumn(title, width, minWidth, maxWidth, align)
{
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForSize"


void
BSizeColumn::DrawField(BField* _field, BRect rect, BView* parent)
{
	BFont font;
	BString printedSize;
	BString string;

	float width = rect.Width() - (2 * kTEXT_MARGIN);

	double value = ((BSizeField*)_field)->Size();
	parent->GetFont(&font);

	// we cannot use string_for_size due to the precision/cell width logic
	const char* kFormats[] = {
		B_TRANSLATE_MARK_COMMENT("{0, plural, one{%s byte} other{%s bytes}}", "size unit"),
		B_TRANSLATE_MARK_COMMENT("%s KiB", "size unit"),
		B_TRANSLATE_MARK_COMMENT("%s MiB", "size unit"),
		B_TRANSLATE_MARK_COMMENT("%s GiB", "size unit"),
		B_TRANSLATE_MARK_COMMENT("%s TiB", "size unit")
	};

	size_t index = 0;
	while (index < B_COUNT_OF(kFormats) - 1 && value >= 1024.0) {
		value /= 1024.0;
		index++;
	}

	BString format;
	BStringFormat formatter(
		gSystemCatalog.GetString(kFormats[index], B_TRANSLATION_CONTEXT, "size unit"));
	formatter.Format(format, value);

	if (index == 0) {
		fNumberFormat.SetPrecision(0);
		fNumberFormat.Format(printedSize, value);
		string.SetToFormat(format.String(), printedSize.String());

		if (font.StringWidth(string) > width) {
			BStringFormat formatter(B_TRANSLATE_COMMENT("%s B", "size unit, narrow space"));
			format.Truncate(0);
			formatter.Format(format, value);
			string.SetToFormat(format.String(), printedSize.String());
		}
	} else {
		int precision = 2;
		while (precision >= 0) {
			fNumberFormat.SetPrecision(precision);
			fNumberFormat.Format(printedSize, value);
			string.SetToFormat(format.String(), printedSize.String());
			if (font.StringWidth(string) <= width)
				break;

			precision--;
		}
	}

	parent->TruncateString(&string, B_TRUNCATE_MIDDLE, width + 2);
	DrawString(string.String(), parent, rect);
}

#undef B_TRANSLATION_CONTEXT


int
BSizeColumn::CompareFields(BField* field1, BField* field2)
{
	off_t diff = ((BSizeField*)field1)->Size() - ((BSizeField*)field2)->Size();
	if (diff > 0)
		return 1;
	if (diff < 0)
		return -1;
	return 0;
}


// #pragma mark - BIntegerField


BIntegerField::BIntegerField(int32 number)
	:
	fInteger(number)
{
}


void
BIntegerField::SetValue(int32 value)
{
	fInteger = value;
}


int32
BIntegerField::Value()
{
	return fInteger;
}


// #pragma mark - BIntegerColumn


BIntegerColumn::BIntegerColumn(const char* title, float width, float minWidth,
	float maxWidth, alignment align)
	:
	BTitledColumn(title, width, minWidth, maxWidth, align)
{
}


void
BIntegerColumn::DrawField(BField *field, BRect rect, BView* parent)
{
	BString string;

	fNumberFormat.Format(string, (int32)((BIntegerField*)field)->Value());
	float width = rect.Width() - (2 * kTEXT_MARGIN);
	parent->TruncateString(&string, B_TRUNCATE_MIDDLE, width + 2);
	DrawString(string.String(), parent, rect);
}


int
BIntegerColumn::CompareFields(BField *field1, BField *field2)
{
	return (((BIntegerField*)field1)->Value() - ((BIntegerField*)field2)->Value());
}


// #pragma mark - GraphColumn


GraphColumn::GraphColumn(const char* name, float width, float minWidth,
	float maxWidth, alignment align)
	:
	BIntegerColumn(name, width, minWidth, maxWidth, align)
{
}


void
GraphColumn::DrawField(BField* field, BRect rect, BView* parent)
{
	double fieldValue = ((BIntegerField*)field)->Value();
	double percentValue = fieldValue / 100.0;

	if (percentValue > 1.0)
		percentValue = 1.0;
	else if (percentValue < 0.0)
		percentValue = 0.0;

	BRect graphRect(rect);
	graphRect.InsetBy(5, 3);
	parent->StrokeRoundRect(graphRect, 2.5, 2.5);

	if (percentValue > 0.0) {
		graphRect.InsetBy(1, 1);
		double value = graphRect.Width() * percentValue;
		graphRect.right = graphRect.left + value;
		parent->SetHighUIColor(B_NAVIGATION_BASE_COLOR);
		parent->FillRect(graphRect);
	}

	parent->SetDrawingMode(B_OP_INVERT);
	parent->SetHighColor(128, 128, 128);

	BString percentString;
	fNumberFormat.FormatPercent(percentString, percentValue);
	float width = be_plain_font->StringWidth(percentString);

	parent->MovePenTo(rect.left + rect.Width() / 2 - width / 2, rect.bottom - FontHeight());
	parent->DrawString(percentString.String());
}


// #pragma mark - BBitmapField


BBitmapField::BBitmapField(BBitmap* bitmap)
	:
	fBitmap(bitmap)
{
}


const BBitmap*
BBitmapField::Bitmap()
{
	return fBitmap;
}


void
BBitmapField::SetBitmap(BBitmap* bitmap)
{
	fBitmap = bitmap;
}


// #pragma mark - BBitmapColumn


BBitmapColumn::BBitmapColumn(const char* title, float width, float minWidth,
	float maxWidth, alignment align)
	:
	BTitledColumn(title, width, minWidth, maxWidth, align)
{
}


void
BBitmapColumn::DrawField(BField* field, BRect rect, BView* parent)
{
	BBitmapField* bitmapField = static_cast<BBitmapField*>(field);
	const BBitmap* bitmap = bitmapField->Bitmap();

	if (bitmap != NULL) {
		float x = 0.0;
		BRect r = bitmap->Bounds();
		float y = rect.top + ((rect.Height() - r.Height()) / 2);

		switch (Alignment()) {
			default:
			case B_ALIGN_LEFT:
				x = rect.left + kTEXT_MARGIN;
				break;

			case B_ALIGN_CENTER:
				x = rect.left + ((rect.Width() - r.Width()) / 2);
				break;

			case B_ALIGN_RIGHT:
				x = rect.right - kTEXT_MARGIN - r.Width();
				break;
		}
		// setup drawing mode according to bitmap color space,
		// restore previous mode after drawing
		drawing_mode oldMode = parent->DrawingMode();
		if (bitmap->ColorSpace() == B_RGBA32
			|| bitmap->ColorSpace() == B_RGBA32_BIG) {
			parent->SetDrawingMode(B_OP_ALPHA);
			parent->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		} else {
			parent->SetDrawingMode(B_OP_OVER);
		}

		parent->DrawBitmap(bitmap, BPoint(x, y));

		parent->SetDrawingMode(oldMode);
	}
}


int
BBitmapColumn::CompareFields(BField* /*field1*/, BField* /*field2*/)
{
	// Comparing bitmaps doesn't really make sense...
	return 0;
}


bool
BBitmapColumn::AcceptsField(const BField *field) const
{
	return static_cast<bool>(dynamic_cast<const BBitmapField*>(field));
}

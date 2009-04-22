/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TableColumns.h"

#include <stdio.h>


// #pragma mark - DelagateBasedTableColumn


DelagateBasedTableColumn::DelagateBasedTableColumn(BColumn* columnDelegate,
	int32 modelIndex, float width, float minWidth, float maxWidth,
	alignment align)
	:
	TableColumn(modelIndex, width, minWidth, maxWidth, align),
	fColumnDelegate(columnDelegate)
{
}


DelagateBasedTableColumn::~DelagateBasedTableColumn()
{
}


void
DelagateBasedTableColumn::DrawTitle(BRect rect, BView* targetView)
{
	fColumnDelegate->DrawTitle(rect, targetView);
}


void
DelagateBasedTableColumn::GetColumnName(BString* into) const
{
	fColumnDelegate->GetColumnName(into);
}


void
DelagateBasedTableColumn::DrawValue(const Variant& value, BRect rect,
	BView* targetView)
{
	fColumnDelegate->DrawField(PrepareField(value), rect, targetView);
}


float
DelagateBasedTableColumn::GetPreferredValueWidth(const Variant& value,
	BView* parent) const
{
	return fColumnDelegate->GetPreferredWidth(PrepareField(value), parent);
}


// #pragma mark - StringTableColumn


StringTableColumn::StringTableColumn(int32 modelIndex, const char* title,
	float width, float minWidth, float maxWidth, uint32 truncate,
	alignment align)
	:
	DelagateBasedTableColumn(&fColumn, modelIndex, width, minWidth, maxWidth,
		align),
	fColumn(title, width, minWidth, maxWidth, truncate, align),
	fField("")
{
}


StringTableColumn::~StringTableColumn()
{
}


BField*
StringTableColumn::PrepareField(const Variant& value) const
{
	fField.SetString(value.ToString());
	fField.SetWidth(Width());
	return &fField;
}


int
StringTableColumn::CompareValues(const Variant& a, const Variant& b)
{
	return strcasecmp(a.ToString(), b.ToString());
}


// #pragma mark - Int32TableColumn


Int32TableColumn::Int32TableColumn(int32 modelIndex, const char* title,
	float width, float minWidth, float maxWidth, uint32 truncate,
	alignment align)
	:
	StringTableColumn(modelIndex, title, width, minWidth, maxWidth, truncate,
		align)
{
}


BField*
Int32TableColumn::PrepareField(const Variant& value) const
{
	char buffer[16];
	snprintf(buffer, sizeof(buffer), "%ld", value.ToInt32());
	return StringTableColumn::PrepareField(
		Variant(buffer, VARIANT_DONT_COPY_DATA));
}


int
Int32TableColumn::CompareValues(const Variant& a, const Variant& b)
{
	return a.ToInt32() - b.ToInt32();
}


// #pragma mark - BigtimeTableColumn


BigtimeTableColumn::BigtimeTableColumn(int32 modelIndex, const char* title,
	float width, float minWidth, float maxWidth, bool invalidFirst,
	uint32 truncate, alignment align)
	:
	StringTableColumn(modelIndex, title, width, minWidth, maxWidth, truncate,
		align),
	fInvalidFirst(invalidFirst)
{
}


BField*
BigtimeTableColumn::PrepareField(const Variant& value) const
{
	bigtime_t time = value.ToInt64();
	if (time < 0) {
		return StringTableColumn::PrepareField(
			Variant("-", VARIANT_DONT_COPY_DATA));
	}

	int micros = int(time % 1000000);
	time /= 1000000;
	int seconds = int(time % 60);
	time /= 60;
	int minutes = int(time % 60);
	time /= 60;

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%02lld:%02d:%02d:%06d", time, minutes,
		seconds, micros);
	return StringTableColumn::PrepareField(
		Variant(buffer, VARIANT_DONT_COPY_DATA));
}


int
BigtimeTableColumn::CompareValues(const Variant& _a, const Variant& _b)
{
	bigtime_t a = _a.ToInt64();
	bigtime_t b = _b.ToInt64();

	if (a == b)
		return 0;
	
	if (a < 0)
		return fInvalidFirst ? -1 : 1;
	if (b < 0)
		return fInvalidFirst ? 1 : -1;

	return a - b < 0 ? -1 : 1;
}

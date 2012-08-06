/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "table/TableColumns.h"

#include <stdio.h>

#include "util/TimeUtils.h"


// #pragma mark - DelegateBasedTableColumn


DelegateBasedTableColumn::DelegateBasedTableColumn(BColumn* columnDelegate,
	int32 modelIndex, float width, float minWidth, float maxWidth,
	alignment align)
	:
	TableColumn(modelIndex, width, minWidth, maxWidth, align),
	fColumnDelegate(columnDelegate)
{
}


DelegateBasedTableColumn::~DelegateBasedTableColumn()
{
}


void
DelegateBasedTableColumn::DrawTitle(BRect rect, BView* targetView)
{
	fColumnDelegate->DrawTitle(rect, targetView);
}


void
DelegateBasedTableColumn::GetColumnName(BString* into) const
{
	fColumnDelegate->GetColumnName(into);
}


void
DelegateBasedTableColumn::DrawValue(const BVariant& value, BRect rect,
	BView* targetView)
{
	fColumnDelegate->DrawField(PrepareField(value), rect, targetView);
}


float
DelegateBasedTableColumn::GetPreferredWidth(const BVariant& value,
	BView* parent) const
{
	return fColumnDelegate->GetPreferredWidth(PrepareField(value), parent);
}


// #pragma mark - StringTableColumn


StringTableColumn::StringTableColumn(int32 modelIndex, const char* title,
	float width, float minWidth, float maxWidth, uint32 truncate,
	alignment align)
	:
	DelegateBasedTableColumn(&fColumn, modelIndex, width, minWidth, maxWidth,
		align),
	fColumn(title, width, minWidth, maxWidth, truncate, align),
	fField("")
{
}


StringTableColumn::~StringTableColumn()
{
}


BField*
StringTableColumn::PrepareField(const BVariant& value) const
{
	fField.SetString(value.ToString());
	fField.SetWidth(Width());
	return &fField;
}


int
StringTableColumn::CompareValues(const BVariant& a, const BVariant& b)
{
	return strcasecmp(a.ToString(), b.ToString());
}


// #pragma mark - BoolStringTableColumn


BoolStringTableColumn::BoolStringTableColumn(int32 modelIndex, const char* title,
	float width, float minWidth, float maxWidth, const BString& trueString,
	const BString& falseString, uint32 truncate, alignment align)
	:
	StringTableColumn(modelIndex, title, width, minWidth, maxWidth, truncate,
		align),
	fTrueString(trueString),
	fFalseString(falseString)
{
}


BField*
BoolStringTableColumn::PrepareField(const BVariant& value) const
{
	return StringTableColumn::PrepareField(
		BVariant(value.ToBool() ? fTrueString : fFalseString,
			B_VARIANT_DONT_COPY_DATA));
}


int
BoolStringTableColumn::CompareValues(const BVariant& a, const BVariant& b)
{
	bool aValue = a.ToBool();
	return aValue == b.ToBool() ? 0 : (aValue ? 1 : -1);
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
Int32TableColumn::PrepareField(const BVariant& value) const
{
	char buffer[16];
	snprintf(buffer, sizeof(buffer), "%" B_PRId32, value.ToInt32());
	return StringTableColumn::PrepareField(
		BVariant(buffer, B_VARIANT_DONT_COPY_DATA));
}


int
Int32TableColumn::CompareValues(const BVariant& a, const BVariant& b)
{
	return a.ToInt32() - b.ToInt32();
}


// #pragma mark - Int64TableColumn


Int64TableColumn::Int64TableColumn(int32 modelIndex, const char* title,
	float width, float minWidth, float maxWidth, uint32 truncate,
	alignment align)
	:
	StringTableColumn(modelIndex, title, width, minWidth, maxWidth, truncate,
		align)
{
}


BField*
Int64TableColumn::PrepareField(const BVariant& value) const
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%" B_PRId64, value.ToInt64());
	return StringTableColumn::PrepareField(
		BVariant(buffer, B_VARIANT_DONT_COPY_DATA));
}


int
Int64TableColumn::CompareValues(const BVariant& a, const BVariant& b)
{
	int64 diff = a.ToInt64() - b.ToInt64();
	if (diff == 0)
		return 0;
	return diff < 0 ? -1 : 1;
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
BigtimeTableColumn::PrepareField(const BVariant& value) const
{
	bigtime_t time = value.ToInt64();
	if (time < 0) {
		return StringTableColumn::PrepareField(
			BVariant("-", B_VARIANT_DONT_COPY_DATA));
	}

	char buffer[64];
	format_bigtime(time, buffer, sizeof(buffer));
	return StringTableColumn::PrepareField(
		BVariant(buffer, B_VARIANT_DONT_COPY_DATA));
}


int
BigtimeTableColumn::CompareValues(const BVariant& _a, const BVariant& _b)
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


// #pragma mark - NanotimeTableColumn


NanotimeTableColumn::NanotimeTableColumn(int32 modelIndex, const char* title,
	float width, float minWidth, float maxWidth, bool invalidFirst,
	uint32 truncate, alignment align)
	:
	StringTableColumn(modelIndex, title, width, minWidth, maxWidth, truncate,
		align),
	fInvalidFirst(invalidFirst)
{
}


BField*
NanotimeTableColumn::PrepareField(const BVariant& value) const
{
	nanotime_t time = value.ToInt64();
	if (time < 0) {
		return StringTableColumn::PrepareField(
			BVariant("-", B_VARIANT_DONT_COPY_DATA));
	}

	char buffer[64];
	format_nanotime(time, buffer, sizeof(buffer));
	return StringTableColumn::PrepareField(
		BVariant(buffer, B_VARIANT_DONT_COPY_DATA));
}


int
NanotimeTableColumn::CompareValues(const BVariant& _a, const BVariant& _b)
{
	nanotime_t a = _a.ToInt64();
	nanotime_t b = _b.ToInt64();

	if (a == b)
		return 0;

	if (a < 0)
		return fInvalidFirst ? -1 : 1;
	if (b < 0)
		return fInvalidFirst ? 1 : -1;

	return a - b < 0 ? -1 : 1;
}

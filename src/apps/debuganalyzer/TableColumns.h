/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_COLUMNS_H
#define TABLE_COLUMNS_H

#include <ColumnTypes.h>

#include "Table.h"


class DelagateBasedTableColumn : public TableColumn {
public:
								DelagateBasedTableColumn(
									BColumn* columnDelegate,
									int32 modelIndex, float width,
									float minWidth, float maxWidth,
									alignment align);
	virtual						~DelagateBasedTableColumn();

protected:
	virtual	void				DrawTitle(BRect rect, BView* targetView);
	virtual	void				GetColumnName(BString* into) const;

	virtual	void				DrawValue(const Variant& value, BRect rect,
									BView* targetView);
	virtual	float				GetPreferredValueWidth(const Variant& value,
									BView* parent) const;

	virtual	BField*				PrepareField(const Variant& value) const = 0;

protected:
			BColumn*			fColumnDelegate;
};


class StringTableColumn : public DelagateBasedTableColumn {
public:
								StringTableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									uint32 truncate,
									alignment align = B_ALIGN_LEFT);
	virtual						~StringTableColumn();

protected:
	virtual	BField*				PrepareField(const Variant& value) const;

	virtual	int					CompareValues(const Variant& a,
									const Variant& b);

private:
			BStringColumn		fColumn;
	mutable	BStringField		fField;
};


class Int32TableColumn : public StringTableColumn {
public:
								Int32TableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									uint32 truncate = B_TRUNCATE_MIDDLE,
									alignment align = B_ALIGN_RIGHT);

protected:
	virtual	BField*				PrepareField(const Variant& value) const;

	virtual	int					CompareValues(const Variant& a,
									const Variant& b);
};


class Int64TableColumn : public StringTableColumn {
public:
								Int64TableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									uint32 truncate = B_TRUNCATE_MIDDLE,
									alignment align = B_ALIGN_RIGHT);

protected:
	virtual	BField*				PrepareField(const Variant& value) const;

	virtual	int					CompareValues(const Variant& a,
									const Variant& b);
};


class BigtimeTableColumn : public StringTableColumn {
public:
								BigtimeTableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									bool invalidFirst,
									uint32 truncate = B_TRUNCATE_MIDDLE,
									alignment align = B_ALIGN_RIGHT);

protected:
	virtual	BField*				PrepareField(const Variant& value) const;

	virtual	int					CompareValues(const Variant& a,
									const Variant& b);

private:
			bool				fInvalidFirst;
};


#endif	// TABLE_COLUMNS_H

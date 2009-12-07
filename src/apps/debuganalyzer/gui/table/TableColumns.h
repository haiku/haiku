/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_COLUMNS_H
#define TABLE_COLUMNS_H


#include <ColumnTypes.h>

#include "table/TableColumn.h"


class DelegateBasedTableColumn : public TableColumn {
public:
								DelegateBasedTableColumn(
									BColumn* columnDelegate,
									int32 modelIndex, float width,
									float minWidth, float maxWidth,
									alignment align);
	virtual						~DelegateBasedTableColumn();

protected:
	virtual	void				DrawTitle(BRect rect, BView* targetView);
	virtual	void				GetColumnName(BString* into) const;

	virtual	void				DrawValue(const BVariant& value, BRect rect,
									BView* targetView);
	virtual	float				GetPreferredWidth(const BVariant& value,
									BView* parent) const;

	virtual	BField*				PrepareField(const BVariant& value) const = 0;

protected:
			BColumn*			fColumnDelegate;
};


class StringTableColumn : public DelegateBasedTableColumn {
public:
								StringTableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									uint32 truncate,
									alignment align = B_ALIGN_LEFT);
	virtual						~StringTableColumn();

protected:
	virtual	BField*				PrepareField(const BVariant& value) const;

	virtual	int					CompareValues(const BVariant& a,
									const BVariant& b);

protected:
			BStringColumn		fColumn;
	mutable	BStringField		fField;
};


class BoolStringTableColumn : public StringTableColumn {
public:
								BoolStringTableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									const BString& trueString = "true",
									const BString& falseString = "false",
									uint32 truncate = B_TRUNCATE_MIDDLE,
									alignment align = B_ALIGN_LEFT);

protected:
	virtual	BField*				PrepareField(const BVariant& value) const;

	virtual	int					CompareValues(const BVariant& a,
									const BVariant& b);

private:
			BString				fTrueString;
			BString				fFalseString;
};


class Int32TableColumn : public StringTableColumn {
public:
								Int32TableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									uint32 truncate = B_TRUNCATE_MIDDLE,
									alignment align = B_ALIGN_RIGHT);

protected:
	virtual	BField*				PrepareField(const BVariant& value) const;

	virtual	int					CompareValues(const BVariant& a,
									const BVariant& b);
};


class Int64TableColumn : public StringTableColumn {
public:
								Int64TableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									uint32 truncate = B_TRUNCATE_MIDDLE,
									alignment align = B_ALIGN_RIGHT);

protected:
	virtual	BField*				PrepareField(const BVariant& value) const;

	virtual	int					CompareValues(const BVariant& a,
									const BVariant& b);
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
	virtual	BField*				PrepareField(const BVariant& value) const;

	virtual	int					CompareValues(const BVariant& a,
									const BVariant& b);

private:
			bool				fInvalidFirst;
};


class NanotimeTableColumn : public StringTableColumn {
public:
								NanotimeTableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									bool invalidFirst,
									uint32 truncate = B_TRUNCATE_MIDDLE,
									alignment align = B_ALIGN_RIGHT);

protected:
	virtual	BField*				PrepareField(const BVariant& value) const;

	virtual	int					CompareValues(const BVariant& a,
									const BVariant& b);

private:
			bool				fInvalidFirst;
};


#endif	// TABLE_COLUMNS_H

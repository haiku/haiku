/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_COLUMN_H
#define TABLE_COLUMN_H

#include <InterfaceDefs.h>
#include <Rect.h>

#include <Variant.h>


class BString;
class BView;


class TableColumn {
public:
								TableColumn(int32 modelIndex, float width,
									float minWidth, float maxWidth,
									alignment align);
	virtual						~TableColumn();

			int32				ModelIndex() const	{ return fModelIndex; }

			float				Width() const		{ return fWidth; }
			float				MinWidth() const	{ return fMinWidth; }
			float				MaxWidth() const	{ return fMaxWidth; }

			alignment			Alignment() const	{ return fAlignment; }

	virtual	void				GetColumnName(BString* into) const;

	virtual	void				DrawTitle(BRect rect, BView* targetView);
	virtual	void				DrawValue(const BVariant& value, BRect rect,
									BView* targetView);
	virtual	int					CompareValues(const BVariant& a,
									const BVariant& b);
	virtual	float				GetPreferredWidth(const BVariant& value,
									BView* parent) const;

private:
			int32				fModelIndex;
			float				fWidth;
			float				fMinWidth;
			float				fMaxWidth;
			alignment			fAlignment;
};


#endif	// TABLE_COLUMN_H

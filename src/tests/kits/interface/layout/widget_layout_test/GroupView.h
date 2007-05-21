/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_GROUP_VIEW_H
#define WIDGET_LAYOUT_TEST_GROUP_VIEW_H


#include "View.h"


// GroupView
class GroupView : public View {
public:
								GroupView(enum orientation orientation,
									int32 lineCount = 1);
	virtual						~GroupView();

			void				SetSpacing(float horizontal, float vertical);
			void				SetInsets(float left, float top, float right,
									float bottom);

	virtual	BSize 				MinSize();
	virtual	BSize 				MaxSize();
	virtual	BSize 				PreferredSize();
	virtual	BAlignment			Alignment();

	virtual	void				InvalidateLayout();
	virtual	void				Layout();

private:
	struct LayoutInfo;

private:
			void				_ValidateMinMax();
			void				_LayoutLine(int32 size, LayoutInfo* infos,
									int32 infoCount);

			BSize				_AddInsetsAndSpacing(BSize size);
			BSize				_SubtractInsetsAndSpacing(BSize size);

			int32				_RowCount() const;
			int32				_ColumnCount() const;
			View*				_ChildAt(int32 column, int32 row) const;

private:
			enum orientation	fOrientation;
			int32				fLineCount;
			float				fColumnSpacing;
			float				fRowSpacing;
			BRect				fInsets;
			bool				fMinMaxValid;
			int32				fMinWidth;
			int32				fMinHeight;
			int32				fMaxWidth;
			int32				fMaxHeight;
			int32				fPreferredWidth;
			int32				fPreferredHeight;
			LayoutInfo*			fColumnInfos;
			LayoutInfo*			fRowInfos;
			int32				fColumnCount;
			int32				fRowCount;
};


// Glue
class Glue : public View {
public:
								Glue();
};


// Strut
class Strut : public View {
public:
								Strut(float pixelWidth, float pixelHeight);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

private:
			BSize				fSize;
};


// HStrut
class HStrut : public Strut {
public:
								HStrut(float width);
};


// VStrut
class VStrut : public Strut {
public:
								VStrut(float height);
};


#endif	// WIDGET_LAYOUT_TEST_GROUP_VIEW_H

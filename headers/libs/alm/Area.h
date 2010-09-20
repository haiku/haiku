/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	AREA_H
#define	AREA_H

#include <Alignment.h>
#include <List.h>
#include <Size.h>
#include <SupportDefs.h>
#include <View.h>

#include "Column.h"
#include "LinearSpec.h"
#include "Row.h"
#include "XTab.h"
#include "YTab.h"


class Constraint;


namespace BALM {


/**
 * Rectangular area in the GUI, defined by a tab on each side.
 */
class Area {
public:
								~Area();

			XTab*				Left() const;
			void				SetLeft(XTab* left);
			XTab*				Right() const;
			void				SetRight(XTab* right);
			YTab*				Top() const;
			void				SetTop(YTab* top);
			YTab*				Bottom() const;
			void				SetBottom(YTab* bottom);

			Row*				GetRow() const;
			void				SetRow(Row* row);
			Column*				GetColumn() const;
			void				SetColumn(Column* column);

			BView*				Content() const;
			void				SetContent(BView* content);
			XTab*				ContentLeft() const;
			YTab*				ContentTop() const;
			XTab*				ContentRight() const;
			YTab*				ContentBottom() const;
			BSize				MinContentSize() const;
			void				SetMinContentSize(BSize min);
			BSize				MaxContentSize() const;
			void				SetMaxContentSize(BSize max);
			BSize				PreferredContentSize() const;
			void				SetPreferredContentSize(BSize preferred);
			double				ContentAspectRatio() const;
			void				SetContentAspectRatio(double ratio);

			BSize				ShrinkPenalties() const;
			void				SetShrinkPenalties(BSize shrink);
			BSize				GrowPenalties() const;
			void				SetGrowPenalties(BSize grow);

			BAlignment			Alignment() const;
			void				SetAlignment(BAlignment alignment);
			void				SetHorizontalAlignment(alignment horizontal);
			void				SetVerticalAlignment(
									vertical_alignment vertical);

			int32				LeftInset() const;
			void				SetLeftInset(int32 left);
			int32				TopInset() const;
			void				SetTopInset(int32 top);
			int32				RightInset() const;
			void				SetRightInset(int32 right);
			int32				BottomInset() const;
			void				SetBottomInset(int32 bottom);

			void				SetDefaultBehavior();
			bool				AutoPreferredContentSize() const;
			void				SetAutoPreferredContentSize(bool value);

								operator BString() const;
			void				GetString(BString& string) const;

			Constraint*			HasSameWidthAs(Area* area);
			Constraint*			HasSameHeightAs(Area* area);
			BList*				HasSameSizeAs(Area* area);

protected:
								Area(BALMLayout* layout,
									LinearSpec* ls, XTab* left, YTab* top,
									XTab* right, YTab* bottom, BView* content,
									BSize minContentSize);
								Area(BALMLayout* layout,
									LinearSpec* ls, Row* row, Column* column,
									BView* content, BSize minContentSize);
			void				DoLayout();

private:
			void				InitChildArea();
			void				UpdateHorizontal();
			void				UpdateVertical();
			void				Init(BALMLayout* layout,
									LinearSpec* ls, XTab* left, YTab* top,
									XTab* right, YTab* bottom, BView* content,
									BSize minContentSize);

public:
			static BSize		kMaxSize;
			static BSize		kMinSize;
			static BSize		kUndefinedSize;

protected:
			BView*				fContent;
			BList*				fConstraints;

private:
			// TODO remove the layout pointer when making Area a LayoutItem
			BALMLayout*			fALMLayout;

			LinearSpec*			fLS;
			XTab*				fLeft;
			XTab*				fRight;
			YTab*				fTop;
			YTab*				fBottom;
			Row*				fRow;
			Column*				fColumn;
			BSize				fMinContentSize;
			BSize				fMaxContentSize;
			Constraint*			fMinContentWidth;
			Constraint*			fMaxContentWidth;
			Constraint*			fMinContentHeight;
			Constraint*			fMaxContentHeight;
			BSize				fPreferredContentSize;
			BSize				fShrinkPenalties;
			BSize				fGrowPenalties;
			double				fContentAspectRatio;
			Constraint*			fContentAspectRatioC;
			bool				fAutoPreferredContentSize;
			Constraint*			fPreferredContentWidth;
			Constraint*			fPreferredContentHeight;
			Area*				fChildArea;
			BAlignment			fAlignment;
			int32				fLeftInset;
			int32				fTopInset;
			int32				fRightInset;
			int32				fBottomInset;
			Constraint*			fLeftConstraint;
			Constraint*			fTopConstraint;
			Constraint*			fRightConstraint;
			Constraint*			fBottomConstraint;

public:
	friend class		BALMLayout;

};

}	// namespace BALM

using BALM::Area;

#endif	// AREA_H


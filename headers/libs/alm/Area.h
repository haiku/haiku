/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
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

			BView*				View();

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

			XTab*				ContentLeft() const;
			YTab*				ContentTop() const;
			XTab*				ContentRight() const;
			YTab*				ContentBottom() const;
			BSize				MinContentSize() const;

			double				ContentAspectRatio() const;
			void				SetContentAspectRatio(double ratio);

			BSize				ShrinkPenalties() const;
			void				SetShrinkPenalties(BSize shrink);
			BSize				GrowPenalties() const;
			void				SetGrowPenalties(BSize grow);

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

			void				InvalidateSizeConstraints();

protected:
								Area(BLayoutItem* item);

			void				Init(LinearSpec* ls, XTab* left, YTab* top,
									XTab* right, YTab* bottom, BView* content,
									BSize minContentSize);
			void				Init(LinearSpec* ls, Row* row, Column* column,
									BView* content, BSize minContentSize);

			void				DoLayout();

private:
			void				_UpdateMinSizeConstraint(BSize min);
			void				_UpdateMaxSizeConstraint(BSize max);
			void				_UpdatePreferredConstraint(BSize preferred);

protected:
			BList				fConstraints;

private:
			BLayoutItem*		fLayoutItem;

			LinearSpec*			fLS;

			XTab*				fLeft;
			XTab*				fRight;
			YTab*				fTop;
			YTab*				fBottom;
			Constraint*			fLeftConstraint;
			Constraint*			fTopConstraint;
			Constraint*			fRightConstraint;
			Constraint*			fBottomConstraint;

			Constraint*			fMinContentWidth;
			Constraint*			fMaxContentWidth;
			Constraint*			fMinContentHeight;
			Constraint*			fMaxContentHeight;
			bool				fAutoPreferredContentSize;
			Constraint*			fPreferredContentWidth;
			Constraint*			fPreferredContentHeight;

			double				fContentAspectRatio;
			Constraint*			fContentAspectRatioC;

			Row*				fRow;
			Column*				fColumn;
			
			BSize				fShrinkPenalties;
			BSize				fGrowPenalties;

			int32				fLeftInset;
			int32				fTopInset;
			int32				fRightInset;
			int32				fBottomInset;

public:
	friend class		BALMLayout;

};

}	// namespace BALM

using BALM::Area;

#endif	// AREA_H


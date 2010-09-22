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
			XTab*				Right() const;
			YTab*				Top() const;
			YTab*				Bottom() const;
			void				SetLeft(XTab* left);
			void				SetRight(XTab* right);
			void				SetTop(YTab* top);
			void				SetBottom(YTab* bottom);

			Row*				GetRow() const;
			Column*				GetColumn() const;
			void				SetRow(Row* row);
			void				SetColumn(Column* column);

			double				ContentAspectRatio() const;
			void				SetContentAspectRatio(double ratio);

			BSize				ShrinkPenalties() const;
			BSize				GrowPenalties() const;
			void				SetShrinkPenalties(BSize shrink);
			void				SetGrowPenalties(BSize grow);

			int32				LeftInset() const;
			int32				TopInset() const;
			int32				RightInset() const;
			int32				BottomInset() const;
			void				SetLeftInset(int32 left);
			void				SetTopInset(int32 top);
			void				SetRightInset(int32 right);
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

private:
								Area(BLayoutItem* item);

			void				_Init(LinearSpec* ls, XTab* left, YTab* top,
									XTab* right, YTab* bottom, BView* content,
									BSize minContentSize);
			void				_Init(LinearSpec* ls, Row* row, Column* column,
									BView* content, BSize minContentSize);

			void				_DoLayout();

			void				_UpdateMinSizeConstraint(BSize min);
			void				_UpdateMaxSizeConstraint(BSize max);
			void				_UpdatePreferredConstraint(BSize preferred);

private:
			BLayoutItem*		fLayoutItem;
			LinearSpec*			fLS;
			BList				fConstraints;

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


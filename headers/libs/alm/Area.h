/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	AREA_H
#define	AREA_H


#include <ObjectList.h>
#include <Referenceable.h>
#include <Rect.h>
#include <Size.h>
#include <String.h>


class BLayoutItem;
class BView;


namespace LinearProgramming {
	class LinearSpec;
	class Constraint;
};


namespace BALM {


class Column;
class Row;
class XTab;
class YTab;




class RowColumnManager;


/**
 * Rectangular area in the GUI, defined by a tab on each side.
 */
class Area {
public:
								~Area();

			int32				ID() const;
			void				SetID(int32 id);

			BLayoutItem*		Item();

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

			double				ContentAspectRatio() const;
			void				SetContentAspectRatio(double ratio);

			BSize				ShrinkPenalties() const;
			BSize				GrowPenalties() const;
			void				SetShrinkPenalties(BSize shrink);
			void				SetGrowPenalties(BSize grow);

			void				GetInsets(float* left, float* top, float* right,
									float* bottom) const;
			float				LeftInset() const;
			float				TopInset() const;
			float				RightInset() const;
			float				BottomInset() const;

			void				SetInsets(float insets);
			void				SetInsets(float horizontal, float vertical);
			void				SetInsets(float left, float top, float right,
									float bottom);
			void				SetLeftInset(float left);
			void				SetTopInset(float top);
			void				SetRightInset(float right);
			void				SetBottomInset(float bottom);

			BString				ToString() const;

			LinearProgramming::Constraint*
								SetWidthAs(Area* area, float factor = 1.0f);

			LinearProgramming::Constraint*
								SetHeightAs(Area* area, float factor = 1.0f);

			void				InvalidateSizeConstraints();

			BRect				Frame() const;

private:
								Area(BLayoutItem* item);

			void				_Init(LinearProgramming::LinearSpec* ls,
									XTab* left, YTab* top, XTab* right,
									YTab* bottom, RowColumnManager* manager);
			void				_Init(LinearProgramming::LinearSpec* ls,
									Row* row, Column* column,
									RowColumnManager* manager);

			void				_DoLayout();

			void				_UpdateMinSizeConstraint(BSize min);
			void				_UpdateMaxSizeConstraint(BSize max);
private:
			BLayoutItem*		fLayoutItem;
			int32				fID;

			LinearProgramming::LinearSpec* fLS;

			BReference<XTab>	fLeft;
			BReference<XTab>	fRight;
			BReference<YTab>	fTop;
			BReference<YTab>	fBottom;

			Row*				fRow;
			Column*				fColumn;
			
			BSize				fShrinkPenalties;
			BSize				fGrowPenalties;

			BSize				fTopLeftInset;
			BSize				fRightBottomInset;

			double				fContentAspectRatio;
			RowColumnManager*	fRowColumnManager;

			BObjectList<LinearProgramming::Constraint>	fConstraints;
			LinearProgramming::Constraint* fMinContentWidth;
			LinearProgramming::Constraint* fMaxContentWidth;
			LinearProgramming::Constraint* fMinContentHeight;
			LinearProgramming::Constraint* fMaxContentHeight;
			LinearProgramming::Constraint* fContentAspectRatioC;

public:
	friend class BALMLayout;
	friend class RowColumnManager;

};

}	// namespace BALM

using BALM::Area;

#endif	// AREA_H


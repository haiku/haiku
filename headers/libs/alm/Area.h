/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	AREA_H
#define	AREA_H


#include <vector>

#include <Alignment.h>
#include <List.h>
#include <Size.h>
#include <SupportDefs.h>
#include <View.h>

#include "Column.h"
#include "LinearSpec.h"
#include "Row.h"
#include "Tab.h"


class Constraint;


namespace BALM {


class GroupItem {
public:
								GroupItem(BLayoutItem* item);
								GroupItem(BView* view);

			BLayoutItem*		LayoutItem();
			BView*				View();

	const	std::vector<GroupItem>&	GroupItems();
			enum orientation	Orientation();

			GroupItem& 			operator|(const GroupItem& right);
			GroupItem& 			operator/(const GroupItem& bottom);
private:
								GroupItem();

			void				_Init(BLayoutItem* item, BView* view,
									  enum orientation orien = B_HORIZONTAL);
			GroupItem& 			_AddItem(const GroupItem& item,
									enum orientation orien);

			BLayoutItem*		fLayoutItem;
			BView*				fView;

			std::vector<GroupItem>	fGroupItems;
			enum orientation	fOrientation;
};


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

			float				LeftInset() const;
			float				TopInset() const;
			float				RightInset() const;
			float				BottomInset() const;
			void				SetLeftInset(float left);
			void				SetTopInset(float top);
			void				SetRightInset(float right);
			void				SetBottomInset(float bottom);

			BString				ToString() const;

			Constraint*			SetWidthAs(Area* area, float factor = 1.0f);
			Constraint*			SetHeightAs(Area* area, float factor = 1.0f);

			void				InvalidateSizeConstraints();

			BRect				Frame() const;

private:
								Area(BLayoutItem* item);

			void				_Init(LinearSpec* ls, XTab* left, YTab* top,
									XTab* right, YTab* bottom,
									RowColumnManager* manager);
			void				_Init(LinearSpec* ls, Row* row, Column* column,
									RowColumnManager* manager);

			void				_DoLayout();

			void				_UpdateMinSizeConstraint(BSize min);
			void				_UpdateMaxSizeConstraint(BSize max);
private:
			BLayoutItem*		fLayoutItem;
			int32				fID;

			LinearSpec*			fLS;

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

			BObjectList<Constraint>	fConstraints;
			Constraint*			fMinContentWidth;
			Constraint*			fMaxContentWidth;
			Constraint*			fMinContentHeight;
			Constraint*			fMaxContentHeight;
			double				fContentAspectRatio;
			Constraint*			fContentAspectRatioC;

			RowColumnManager*	fRowColumnManager;
public:
	friend class BALMLayout;
	friend class RowColumnManager;

};

}	// namespace BALM

using BALM::Area;
using BALM::GroupItem;

#endif	// AREA_H


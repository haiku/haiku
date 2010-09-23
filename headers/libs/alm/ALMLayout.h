/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	ALM_LAYOUT_H
#define	ALM_LAYOUT_H


#include <AbstractLayout.h>
#include <File.h>
#include <List.h>
#include <Size.h>
#include <SupportDefs.h>
#include <View.h>

#include "Area.h"
#include "Column.h"
#include "LinearSpec.h"
#include "Row.h"
#include "XTab.h"
#include "YTab.h"


namespace BALM {

/*!
 * A GUI layout engine using the Auckland Layout Model (ALM).
 */
class BALMLayout : public BAbstractLayout {
public:
								BALMLayout(float spacing = 0.0f);
	virtual						~BALMLayout();

			XTab*				AddXTab();
			YTab*				AddYTab();
			Row*				AddRow();
			Row*				AddRow(YTab* top, YTab* bottom);
			Column*				AddColumn();
			Column*				AddColumn(XTab* left, XTab* right);

			XTab*				Left() const;
			XTab*				Right() const;
			YTab*				Top() const;
			YTab*				Bottom() const;

			char*				PerformancePath() const;
			void				SetPerformancePath(char* path);

			LinearSpec*			Solver();

			void				SetInset(float inset);
			float				Inset();

			void				SetSpacing(float spacing);
			float				Spacing();

	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);
	virtual	Area*				AddView(BView* view, XTab* left, YTab* top,
									XTab* right, YTab* bottom);
	virtual	Area*				AddView(BView* view, Row* row, Column* column);
	virtual	Area*				AddViewToRight(BView* view, Area* leftArea,
									XTab* right = NULL, YTab* top = NULL,
									YTab* bottom = NULL);
	virtual	Area*				AddViewToLeft(BView* view, Area* rightArea,
									XTab* left = NULL, YTab* top = NULL,
									YTab* bottom = NULL);
	virtual	Area*				AddViewToTop(BView* view, Area* bottomArea,
									YTab* top = NULL, XTab* left = NULL,
									XTab* right = NULL);
	virtual	Area*				AddViewToBottom(BView* view, Area* topArea,
									YTab* bottom = NULL, XTab* left = NULL,
									XTab* right = NULL);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);
	virtual	Area*				AddItem(BLayoutItem* item, XTab* left,
									YTab* top, XTab* right, YTab* bottom);
	virtual	Area*				AddItem(BLayoutItem* item, Row* row,
									Column* column);
	virtual	Area*				AddItemToRight(BLayoutItem* item,
									Area* leftArea, XTab* right = NULL,
									YTab* top = NULL, YTab* bottom = NULL);
	virtual	Area*				AddItemToLeft(BLayoutItem* item,
									Area* rightArea, XTab* left = NULL,
									YTab* top = NULL, YTab* bottom = NULL);
	virtual	Area*				AddItemToTop(BLayoutItem* item,
									Area* bottomArea, YTab* top = NULL,
									XTab* left = NULL, XTab* right = NULL);
	virtual	Area*				AddItemToBottom(BLayoutItem* item,
									Area* topArea, YTab* bottom = NULL,
									XTab* left = NULL, XTab* right = NULL);

	virtual	Area*				AreaOf(BView* control);

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual	void				InvalidateLayout(bool children = false);

	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);
	virtual	void				DerivedLayoutItems();

private:
			/*! Add a view without initialize the Area. */
			BLayoutItem*		_CreateLayoutItem(BView* view);

			void				_SolveLayout();

			Area*				_AreaForItem(BLayoutItem* item) const;
			void				_UpdateAreaConstraints();

			BSize				_CalculateMinSize();
			BSize				_CalculateMaxSize();
			BSize				_CalculatePreferredSize();


			LinearSpec			fSolver;

			XTab*				fLeft;
			XTab*				fRight;
			YTab*				fTop;
			YTab*				fBottom;
			BSize				fMinSize;
			BSize				fMaxSize;
			BSize				fPreferredSize;
			char*				fPerformancePath;

			float				fInset;
			float				fSpacing;
};

}	// namespace BALM

using BALM::BALMLayout;

#endif	// ALM_LAYOUT_H

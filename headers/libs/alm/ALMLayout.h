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

			LinearSpec*			Solver() const;

			void				SetInset(float inset);
			float				Inset() const;

			void				SetSpacing(float spacing);
			float				Spacing() const;

			Area*				AreaFor(const BView* control) const;
			Area*				AreaFor(const BLayoutItem* item) const;
			Area*				CurrentArea() const;
			void				SetCurrentArea(const Area* area);
			void				SetCurrentArea(const BView* control);
			void				SetCurrentArea(const BLayoutItem* item);
	
	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);
	virtual	Area*				AddView(BView* view, XTab* left, YTab* top,
									XTab* right, YTab* bottom);
	virtual	Area*				AddView(BView* view, Row* row, Column* column);
	virtual	Area*				AddViewToRight(BView* view, XTab* right = NULL,
									YTab* top = NULL, YTab* bottom = NULL);
	virtual	Area*				AddViewToLeft(BView* view, XTab* left = NULL,
									YTab* top = NULL, YTab* bottom = NULL);
	virtual	Area*				AddViewToTop(BView* view, YTab* top = NULL,
									XTab* left = NULL, XTab* right = NULL);
	virtual	Area*				AddViewToBottom(BView* view,
									YTab* bottom = NULL, XTab* left = NULL,
									XTab* right = NULL);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);
	virtual	Area*				AddItem(BLayoutItem* item, XTab* left,
									YTab* top, XTab* right, YTab* bottom);
	virtual	Area*				AddItem(BLayoutItem* item, Row* row,
									Column* column);
	virtual	Area*				AddItemToRight(BLayoutItem* item,
									XTab* right = NULL, YTab* top = NULL,
									YTab* bottom = NULL);
	virtual	Area*				AddItemToLeft(BLayoutItem* item,
									XTab* left = NULL, YTab* top = NULL,
									YTab* bottom = NULL);
	virtual	Area*				AddItemToTop(BLayoutItem* item,
									YTab* top = NULL, XTab* left = NULL,
									XTab* right = NULL);
	virtual	Area*				AddItemToBottom(BLayoutItem* item,
									YTab* bottom = NULL, XTab* left = NULL,
									XTab* right = NULL);

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

			Area*				fCurrentArea;
};

}	// namespace BALM

using BALM::BALMLayout;

#endif	// ALM_LAYOUT_H

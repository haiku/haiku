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
#include "Tab.h"


namespace BALM {


class RowColumnManager;


/*!
 * A GUI layout engine using the Auckland Layout Model (ALM).
 */
class BALMLayout : public BAbstractLayout {
public:
								BALMLayout(float spacing = 0.0f,
									BALMLayout* friendLayout = NULL);
	virtual						~BALMLayout();

			BReference<XTab>	AddXTab();
			BReference<YTab>	AddYTab();

			int32				CountXTabs() const;
			int32				CountYTabs() const;
			XTab*				XTabAt(int32 index) const;
			YTab*				YTabAt(int32 index) const;
			/*! Order the tab list and return a reference to the list. */
	const	XTabList&			OrderedXTabs();
	const	YTabList&			OrderedYTabs();

			Row*				AddRow(YTab* top, YTab* bottom);
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

			Area*				AreaFor(int32 id) const;
			Area*				AreaFor(const BView* view) const;
			Area*				AreaFor(const BLayoutItem* item) const;
			int32				CountAreas() const;
			Area*				AreaAt(int32 index) const;
			Area*				CurrentArea() const;
			bool				SetCurrentArea(const Area* area);
			bool				SetCurrentArea(const BView* view);
			bool				SetCurrentArea(const BLayoutItem* item);
	
			XTab*				LeftOf(const BView* view) const;
			XTab*				LeftOf(const BLayoutItem* item) const;
			XTab*				RightOf(const BView* view) const;
			XTab*				RightOf(const BLayoutItem* item) const;
			YTab*				TopOf(const BView* view) const;
			YTab*				TopOf(const BLayoutItem* item) const;
			YTab*				BottomOf(const BView* view) const;
			YTab*				BottomOf(const BLayoutItem* item) const;

			void				BuildLayout(GroupItem& item, XTab* left = NULL,
									YTab* top = NULL, XTab* right = NULL,
									YTab* bottom = NULL);

	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);
	virtual	Area*				AddView(BView* view, XTab* left, YTab* top,
									XTab* right = NULL, YTab* bottom = NULL);
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
									YTab* top, XTab* right = NULL,
									YTab* bottom = NULL);
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

			bool				SaveLayout(BMessage* archive) const;
			bool				RestoreLayout(const BMessage* archive);

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

protected:
	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);

	virtual	void				LayoutInvalidated(bool children);
	virtual	void				DoLayout();

private:
	friend class XTab;
	friend class YTab;

			/*! Add a view without initialize the Area. */
			BLayoutItem*		_CreateLayoutItem(BView* view);

			void				_UpdateAreaConstraints();

			BSize				_CalculateMinSize();
			BSize				_CalculateMaxSize();
			BSize				_CalculatePreferredSize();

			void				_ParseGroupItem(GroupItem& item,
									BReference<XTab> left, BReference<YTab> top,
									BReference<XTab> right,
									BReference<YTab> bottom);

			LinearSpec*			fSolver;
			LinearSpec			fOwnSolver;

			BReference<XTab>	fLeft;
			BReference<XTab>	fRight;
			BReference<YTab>	fTop;
			BReference<YTab>	fBottom;
			BSize				fMinSize;
			BSize				fMaxSize;
			BSize				fPreferredSize;
			char*				fPerformancePath;

			float				fInset;
			float				fSpacing;

			Area*				fCurrentArea;

			XTabList			fXTabList;
			YTabList			fYTabList;

			RowColumnManager*	fRowColumnManager;
};

}	// namespace BALM

using BALM::BALMLayout;

#endif	// ALM_LAYOUT_H

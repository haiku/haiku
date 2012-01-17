/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	ALM_LAYOUT_H
#define	ALM_LAYOUT_H


#include <AbstractLayout.h>
#include <Size.h>

#include "Area.h"
#include "LinearSpec.h"
#include "Tab.h"


class BView;
class BLayoutItem;


namespace BALM {


class Column;
class GroupItem;
class Row;
class RowColumnManager;


/*!
 * A GUI layout engine using the Auckland Layout Model (ALM).
 */
class BALMLayout : public BAbstractLayout {
public:
								BALMLayout(float hSpacing = 0.0f,
									float vSpacing = 0.0f,
									BALMLayout* friendLayout = NULL);
	virtual						~BALMLayout();

			BReference<XTab>	AddXTab();
			void				AddXTabs(BReference<XTab>* tabs, uint32 count);
			BReference<YTab>	AddYTab();
			void				AddYTabs(BReference<YTab>* tabs, uint32 count);

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

			LinearProgramming::LinearSpec* Solver() const;

			void				SetInsets(float insets);
			void				SetInsets(float x, float y);
			void				SetInsets(float left, float top, float right,
									float bottom);
			void				GetInsets(float* left, float* top, float* right,
									float* bottom) const;

			void				SetSpacing(float hSpacing, float vSpacing);
			void				GetSpacing(float* _hSpacing,
									float* _vSpacing) const;

			Area*				AreaFor(int32 id) const;
			Area*				AreaFor(const BView* view) const;
			Area*				AreaFor(const BLayoutItem* item) const;
			int32				CountAreas() const;
			Area*				AreaAt(int32 index) const;
	
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

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);
	virtual	Area*				AddItem(BLayoutItem* item, XTab* left,
									YTab* top, XTab* right = NULL,
									YTab* bottom = NULL);
	virtual	Area*				AddItem(BLayoutItem* item, Row* row,
									Column* column);
									
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
	friend class Area;

			float				InsetForTab(XTab* tab);
			float				InsetForTab(YTab* tab);

			BLayoutItem*		_LayoutItemToAdd(BView* view);

			void				_UpdateAreaConstraints();

			BSize				_CalculateMinSize();
			BSize				_CalculateMaxSize();
			BSize				_CalculatePreferredSize();

			void				_ParseGroupItem(GroupItem& item,
									BReference<XTab> left, BReference<YTab> top,
									BReference<XTab> right,
									BReference<YTab> bottom);

			LinearProgramming::LinearSpec*	fSolver;
			LinearProgramming::LinearSpec	fOwnSolver;

			BReference<XTab>	fLeft;
			BReference<XTab>	fRight;
			BReference<YTab>	fTop;
			BReference<YTab>	fBottom;
			BSize				fMinSize;
			BSize				fMaxSize;
			BSize				fPreferredSize;

			float				fLeftInset;
			float				fRightInset;
			float				fTopInset;
			float				fBottomInset;

			float				fHSpacing;
			float				fVSpacing;

			XTabList			fXTabList;
			YTabList			fYTabList;

			RowColumnManager*	fRowColumnManager;
};

}	// namespace BALM


using BALM::BALMLayout;


#endif	// ALM_LAYOUT_H

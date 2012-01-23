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

namespace BPrivate {
	class SharedSolver;
};


namespace BALM {


class Column;
class ALMGroup;
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
	struct BadLayoutPolicy;

			void				SetBadLayoutPolicy(BadLayoutPolicy* policy);
			BadLayoutPolicy*	GetBadLayoutPolicy() const;

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);

	virtual	void				LayoutInvalidated(bool children);
	virtual	void				DoLayout();

public:
	struct BadLayoutPolicy {
		virtual ~BadLayoutPolicy();
		/* return false to abandon layout, true to use layout */
		virtual bool OnBadLayout(BALMLayout* layout) = 0;
	};

	struct DefaultPolicy : public BadLayoutPolicy {
		virtual ~DefaultPolicy();
		virtual bool OnBadLayout(BALMLayout* layout);
	};

private:

	// FBC padding
	virtual	void				_ReservedALMLayout1();
	virtual	void				_ReservedALMLayout2();
	virtual	void				_ReservedALMLayout3();
	virtual	void				_ReservedALMLayout4();
	virtual	void				_ReservedALMLayout5();
	virtual	void				_ReservedALMLayout6();
	virtual	void				_ReservedALMLayout7();
	virtual	void				_ReservedALMLayout8();
	virtual	void				_ReservedALMLayout9();
	virtual	void				_ReservedALMLayout10();

	// forbidden methods
								BALMLayout(const BALMLayout&);
			void				operator =(const BALMLayout&);

private:
	template <class T>
	struct TabAddTransaction;

	template <class T>
	friend class TabAddTransaction;
	friend class BPrivate::SharedSolver;

	friend class XTab;
	friend class YTab;
	friend class Area;

			float				InsetForTab(XTab* tab) const;
			float				InsetForTab(YTab* tab) const;

			void				UpdateConstraints(BLayoutContext* context);

			void				_RemoveSelfFromTab(XTab* tab);
			void				_RemoveSelfFromTab(YTab* tab);
			bool				_HasTabInLayout(XTab* tab);
			bool				_HasTabInLayout(YTab* tab);
			bool				_AddedTab(XTab* tab);
			bool				_AddedTab(YTab* tab);

			BLayoutItem*		_LayoutItemToAdd(BView* view);

			BPrivate::SharedSolver* fSolver;

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

			BadLayoutPolicy*	fBadLayoutPolicy;
			uint32				_reserved[5];
};

}	// namespace BALM


using BALM::BALMLayout;


#endif	// ALM_LAYOUT_H

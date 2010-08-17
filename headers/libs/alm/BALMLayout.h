/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	BALM_LAYOUT_H
#define	BALM_LAYOUT_H

#include <AbstractLayout.h>
#include <File.h>
#include <List.h>
#include <Size.h>
#include <SupportDefs.h>
#include <View.h>

#include "LayoutStyleType.h"
#include "XTab.h"
#include "YTab.h"
#include "Area.h"
#include "Row.h"
#include "Column.h"
#include "Constraint.h"
#include "LinearSpec.h"


namespace BALM {
	
/**
 * A GUI layout engine using the ALM.
 */
class BALMLayout : public BAbstractLayout, public LinearSpec {
		
public:
						BALMLayout();
	void				SolveLayout();

	XTab*				AddXTab();
	YTab*				AddYTab();
	Row*				AddRow();
	Row*				AddRow(YTab* top, YTab* bottom);
	Column*				AddColumn();
	Column*				AddColumn(XTab* left, XTab* right);
	
	Area*				AddArea(XTab* left, YTab* top, XTab* right, YTab* bottom,
								BView* content, BSize minContentSize);
	Area*				AddArea(Row* row, Column* column, BView* content,
								BSize minContentSize);
	Area*				AddArea(XTab* left, YTab* top, XTab* right, YTab* bottom,
								BView* content);
	Area*				AddArea(Row* row, Column* column, BView* content);
	Area*				AreaOf(BView* control);
	BList*				Areas() const;

	XTab*				Left() const;
	XTab*				Right() const;
	YTab*				Top() const;
	YTab*				Bottom() const;
	
	void				RecoverLayout(BView* parent);
	
	LayoutStyleType		LayoutStyle() const;
	void				SetLayoutStyle(LayoutStyleType style);

	BLayoutItem*		AddView(BView* child);
	BLayoutItem*		AddView(int32 index, BView* child);
	bool				AddItem(BLayoutItem* item);
	bool				AddItem(int32 index, BLayoutItem* item);
	bool				RemoveView(BView* child);
	bool				RemoveItem(BLayoutItem* item);
	BLayoutItem*		RemoveItem(int32 index);

	BSize				BaseMinSize();
	BSize				BaseMaxSize();
	BSize				BasePreferredSize();
	BAlignment			BaseAlignment();
	bool				HasHeightForWidth();
	void				GetHeightForWidth(float width, float* min,
								float* max, float* preferred);
	void				InvalidateLayout(bool children = false);
	virtual void		DerivedLayoutItems();
	
	char*				PerformancePath() const;
	void				SetPerformancePath(char* path);

private:
	BSize				CalculateMinSize();
	BSize				CalculateMaxSize();
	BSize				CalculatePreferredSize();

private:
	LayoutStyleType		fLayoutStyle;
	bool				fActivated;

	BList*				fAreas;
	XTab*				fLeft;
	XTab*				fRight;
	YTab*				fTop;
	YTab*				fBottom;
	BSize				fMinSize;
	BSize				fMaxSize;
	BSize				fPreferredSize;
	char*				fPerformancePath;
};

}	// namespace BALM

using BALM::BALMLayout;

#endif	// BALM_LAYOUT_H

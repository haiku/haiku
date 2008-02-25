/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	AREA_H
#define	AREA_H

#include "Constraint.h"

#include <Alignment.h>
#include <List.h>
#include <Size.h>
#include <SupportDefs.h>
#include <View.h>


namespace BALM {

class Column;
class BALMLayout;
class Row;
class XTab;
class YTab;

/**
 * Rectangular area in the GUI, defined by a tab on each side.
 */
class Area {
	
public:
	bool				AutoPrefContentSize() const;
	void				SetAutoPrefContentSize(bool value);
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
	BView*				Content() const;
	void				SetContent(BView* content);
	XTab*				ContentLeft() const;
	YTab*				ContentTop() const;
	XTab*				ContentRight() const;
	YTab*				ContentBottom() const;
	BSize				MinContentSize() const;
	void				SetMinContentSize(BSize min);
	BSize				MaxContentSize() const;
	void				SetMaxContentSize(BSize max);
	BSize				PrefContentSize() const;
	void				SetPrefContentSize(BSize pref);
	BSize				ShrinkRigidity() const;
	void				SetShrinkRigidity(BSize shrink);
	BSize				ExpandRigidity() const;
	void				SetExpandRigidity(BSize expand);
	double				ContentAspectRatio() const;
	void				SetContentAspectRatio(double ratio);
	BAlignment			Alignment() const;
	void				SetAlignment(BAlignment alignment);
	void				SetHAlignment(alignment horizontal);
	void				SetVAlignment(vertical_alignment vertical);
	int32				LeftInset() const;
	void				SetLeftInset(int32 left);
	int32				TopInset() const;
	void				SetTopInset(int32 top);
	int32				RightInset() const;
	void				SetRightInset(int32 right);
	int32				BottomInset() const;
	void				SetBottomInset(int32 bottom);
	void				SetDefaultPrefContentSize();
	//~ string			ToString();
	Constraint*			HasSameWidthAs(Area* area);
	Constraint*			HasSameHeightAs(Area* area);
	BList*				HasSameSizetAs(Area* area);
						~Area();
	
protected:
						Area(BALMLayout* ls, XTab* left, YTab* top,
								XTab* right, YTab* bottom, 
								BView* content, 
								BSize minContentSize);
						Area(BALMLayout* ls, Row* row, Column* column, 
								BView* content,
								BSize minContentSize);
	void				DoLayout();
	
private:
	void					InitChildArea();
	void					UpdateHorizontal();
	void					UpdateVertical();
	void					Init(BALMLayout* ls, XTab* left, YTab* top, 
								XTab* right, YTab* bottom, 
								BView* content, 
								BSize minContentSize);

public:
	static BSize			kMaxSize;
	static BSize			kMinSize;
	static BSize			kUndefinedSize;

protected:
	BView*				fContent;
	BList*				fConstraints;

private:
	BALMLayout*			fLS;
	XTab*				fLeft;
	XTab*				fRight;
	YTab*				fTop;
	YTab*				fBottom;
	Row*				fRow;
	Column*				fColumn;
	BSize				fMinContentSize;
	BSize				fMaxContentSize;
	Constraint*			fMinContentWidth;
	Constraint*			fMaxContentWidth;
	Constraint*			fMinContentHeight;
	Constraint*			fMaxContentHeight;
	BSize				fPrefContentSize;
	BSize				fShrinkRigidity;
	BSize				fExpandRigidity;
	double				fContentAspectRatio;
	Constraint*			fContentAspectRatioC;
	bool				fAutoPrefContentSize;
	Constraint*			fPrefContentWidth;
	Constraint*			fPrefContentHeight;
	Area*				fChildArea;
	BAlignment			fAlignment;
	int32				fLeftInset;
	int32				fTopInset;
	int32				fRightInset;
	int32				fBottomInset;
	Constraint*			fLeftConstraint;
	Constraint*			fTopConstraint;
	Constraint*			fRightConstraint;
	Constraint*			fBottomConstraint;
	
public:
	friend class			BALMLayout;

};

}	// namespace BALM

using BALM::Area;

#endif	// AREA_H

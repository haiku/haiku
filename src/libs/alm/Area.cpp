/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "Area.h"

#include <algorithm>	// for max

#include <Button.h>
#include <CheckBox.h>
#include <PictureButton.h>
#include <RadioButton.h>
#include <StatusBar.h>
#include <StringView.h>

#include "ALMLayout.h"
#include "RowColumnManager.h"


using namespace LinearProgramming;
using namespace std;


GroupItem::GroupItem(BLayoutItem* item)
{
	_Init(item, NULL);
}


GroupItem::GroupItem(BView* view)
{
	_Init(NULL, view);
}


BLayoutItem*
GroupItem::LayoutItem()
{
	return fLayoutItem;
}


BView*
GroupItem::View()
{
	return fView;
}


const std::vector<GroupItem>&
GroupItem::GroupItems()
{
	return fGroupItems;
}


enum orientation
GroupItem::Orientation()
{
	return fOrientation;
}


GroupItem&
GroupItem::operator|(const GroupItem& right)
{
	return _AddItem(right, B_HORIZONTAL);
}


GroupItem&
GroupItem::operator/(const GroupItem& bottom)
{
	return _AddItem(bottom, B_VERTICAL);
}


GroupItem::GroupItem()
{
	_Init(NULL, NULL);
}


void
GroupItem::_Init(BLayoutItem* item, BView* view, enum orientation orien)
{
	fLayoutItem = item;
	fView = view;
	fOrientation = orien;
}


GroupItem&
GroupItem::_AddItem(const GroupItem& item, enum orientation orien)
{
	if (fGroupItems.size() == 0)
		fGroupItems.push_back(*this);
	else if (fOrientation != orien) {
		GroupItem clone = *this;
		fGroupItems.clear();
		_Init(NULL, NULL, orien);
		fGroupItems.push_back(clone);
	}

	_Init(NULL, NULL, orien);
	fGroupItems.push_back(item);
	return *this;
}


BLayoutItem*
Area::Item()
{
	return fLayoutItem;
}


/**
 * Gets the left tab of the area.
 *
 * @return the left tab of the area
 */
XTab*
Area::Left() const
{
	return fLeft;
}


/**
 * Gets the right tab of the area.
 *
 * @return the right tab of the area
 */
XTab*
Area::Right() const
{
	return fRight;
}


/**
 * Gets the top tab of the area.
 */
YTab*
Area::Top() const
{
	return fTop;
}


/**
 * Gets the bottom tab of the area.
 */
YTab*
Area::Bottom() const
{
	return fBottom;
}


/**
 * Sets the left tab of the area.
 *
 * @param left	the left tab of the area
 */
void
Area::SetLeft(XTab* left)
{
	fLeft = left;

	fMinContentWidth->SetLeftSide(-1.0, fLeft, 1.0, fRight);
	if (fMaxContentWidth != NULL)
		fMaxContentWidth->SetLeftSide(-1.0, fLeft, 1.0, fRight);
	fRowColumnManager->TabsChanged(this);

	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets the right tab of the area.
 *
 * @param right	the right tab of the area
 */
void
Area::SetRight(XTab* right)
{
	fRight = right;

	fMinContentWidth->SetLeftSide(-1.0, fLeft, 1.0, fRight);
	if (fMaxContentWidth != NULL)
		fMaxContentWidth->SetLeftSide(-1.0, fLeft, 1.0, fRight);
	fRowColumnManager->TabsChanged(this);

	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets the top tab of the area.
 */
void
Area::SetTop(YTab* top)
{
	fTop = top;

	fMinContentHeight->SetLeftSide(-1.0, fTop, 1.0, fBottom);
	if (fMaxContentHeight != NULL)
		fMaxContentHeight->SetLeftSide(-1.0, fTop, 1.0, fBottom);
	fRowColumnManager->TabsChanged(this);

	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets the bottom tab of the area.
 */
void
Area::SetBottom(YTab* bottom)
{
	fBottom = bottom;

	fMinContentHeight->SetLeftSide(-1.0, fTop, 1.0, fBottom);
	if (fMaxContentHeight != NULL)
		fMaxContentHeight->SetLeftSide(-1.0, fTop, 1.0, fBottom);
	fRowColumnManager->TabsChanged(this);

	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Gets the row that defines the top and bottom tabs.
 */
Row*
Area::GetRow() const
{
	return fRow;
}


/**
 * Gets the column that defines the left and right tabs.
 */
Column*
Area::GetColumn() const
{
	return fColumn;
}


/**
 * The reluctance with which the area's content shrinks below its preferred size.
 * The bigger the less likely is such shrinking.
 */
BSize
Area::ShrinkPenalties() const
{
	return fShrinkPenalties;
}


/**
 * The reluctance with which the area's content grows over its preferred size.
 * The bigger the less likely is such growth.
 */
BSize
Area::GrowPenalties() const
{
	return fGrowPenalties;
}


void
Area::SetShrinkPenalties(BSize shrink) {
	fShrinkPenalties = shrink;

	fLayoutItem->Layout()->InvalidateLayout();
}


void
Area::SetGrowPenalties(BSize grow)
{
	fGrowPenalties = grow;

	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Gets aspect ratio of the area's content.
 */
double
Area::ContentAspectRatio() const
{
	return fContentAspectRatio;
}


/**
 * Sets aspect ratio of the area's content.
 * May be different from the aspect ratio of the area.
 */
void
Area::SetContentAspectRatio(double ratio)
{
	fContentAspectRatio = ratio;
	if (fContentAspectRatio <= 0) {
		delete fContentAspectRatioC;
		fContentAspectRatioC = NULL;
	} else if (fContentAspectRatioC == NULL) {
		fContentAspectRatioC = fLS->AddConstraint(-1.0, fLeft, 1.0, fRight,
			ratio, fTop, -ratio, fBottom, kEQ, 0.0);
		fConstraints.AddItem(fContentAspectRatioC);
	} else {
		fContentAspectRatioC->SetLeftSide(-1.0, fLeft, 1.0, fRight, ratio,
			fTop, -ratio, fBottom);
	}
	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Gets left inset between area and its content.
 */
float
Area::LeftInset() const
{
	if (fTopLeftInset.IsWidthSet())
		return fTopLeftInset.Width();

	BALMLayout* layout = static_cast<BALMLayout*>(fLayoutItem->Layout());
	return layout->InsetForTab(fLeft.Get());
}


/**
 * Gets top inset between area and its content.
 */
float
Area::TopInset() const
{
	if (fTopLeftInset.IsHeightSet())
		return fTopLeftInset.Height();

	BALMLayout* layout = static_cast<BALMLayout*>(fLayoutItem->Layout());
	return layout->InsetForTab(fTop.Get());
}


/**
 * Gets right inset between area and its content.
 */
float
Area::RightInset() const
{
	if (fRightBottomInset.IsWidthSet())
		return fRightBottomInset.Width();

	BALMLayout* layout = static_cast<BALMLayout*>(fLayoutItem->Layout());
	return layout->InsetForTab(fRight.Get());
}


/**
 * Gets bottom inset between area and its content.
 */
float
Area::BottomInset() const
{
	if (fRightBottomInset.IsHeightSet())
		return fRightBottomInset.Height();

	BALMLayout* layout = static_cast<BALMLayout*>(fLayoutItem->Layout());
	return layout->InsetForTab(fBottom.Get());
}


/**
 * Sets left inset between area and its content.
 */
void
Area::SetLeftInset(float left)
{
	fTopLeftInset.width = left;
	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets top inset between area and its content.
 */
void
Area::SetTopInset(float top)
{
	fTopLeftInset.height = top;
	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets right inset between area and its content.
 */
void
Area::SetRightInset(float right)
{
	fRightBottomInset.width = right;
	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets bottom inset between area and its content.
 */
void
Area::SetBottomInset(float bottom)
{
	fRightBottomInset.height = bottom;
	fLayoutItem->Layout()->InvalidateLayout();
}


BString
Area::ToString() const
{
	BString string = "Area(";
	string += fLeft->ToString();
	string << ", ";
	string += fTop->ToString();
	string << ", ";
	string += fRight->ToString();
	string << ", ";
	string += fBottom->ToString();
	string << ")";
	return string;
}


/*!
 * Sets the width of the area to be the same as the width of the given area
 * times factor.
 *
 * @param area	the area that should have the same width
 * @return the same-width constraint
 */
Constraint*
Area::SetWidthAs(Area* area, float factor)
{
	return fLS->AddConstraint(-1.0, fLeft, 1.0, fRight, factor, area->Left(),
		-factor, area->Right(), kEQ, 0.0);
}


/*!
 * Sets the height of the area to be the same as the height of the given area
 * times factor.
 *
 * @param area	the area that should have the same height
 * @return the same-height constraint
 */
Constraint*
Area::SetHeightAs(Area* area, float factor)
{
	return fLS->AddConstraint(-1.0, fTop, 1.0, fBottom, factor, area->Top(),
		-factor, area->Bottom(), kEQ, 0.0);
}


void
Area::InvalidateSizeConstraints()
{
	// check if if we are initialized
	if (!fLeft)
		return;

	BSize minSize = fLayoutItem->MinSize();
	BSize maxSize = fLayoutItem->MaxSize();

	_UpdateMinSizeConstraint(minSize);
	_UpdateMaxSizeConstraint(maxSize);
}


BRect
Area::Frame() const
{
	return BRect(fLeft->Value(), fTop->Value(), fRight->Value(),
		fBottom->Value());
}


BRect
Area::ItemFrame() const
{
	return fLayoutItem->Frame();
}


/**
 * Destructor.
 * Removes the area from its specification.
 */
Area::~Area()
{
	for (int32 i = 0; i < fConstraints.CountItems(); i++)
		delete fConstraints.ItemAt(i);
}


static int32 sAreaID = 0;

static int32
new_area_id()
{
	return sAreaID++;
}


/**
 * Constructor.
 * Uses XTabs and YTabs.
 */
Area::Area(BLayoutItem* item)
	:
	fLayoutItem(item),

	fLS(NULL),
	fLeft(NULL),
	fRight(NULL),
	fTop(NULL),
	fBottom(NULL),

	fRow(NULL),
	fColumn(NULL),

	fShrinkPenalties(5, 5),
	fGrowPenalties(5, 5),

	fMinContentWidth(NULL),
	fMaxContentWidth(NULL),
	fMinContentHeight(NULL),
	fMaxContentHeight(NULL),

	fContentAspectRatio(-1),
	fContentAspectRatioC(NULL)
{
	fID = new_area_id();
}


int32
Area::ID() const
{
	return fID;
}


void
Area::SetID(int32 id)
{
	fID = id;
}


/**
 * Initialize variables.
 */
void
Area::_Init(LinearSpec* ls, XTab* left, YTab* top, XTab* right, YTab* bottom,
	RowColumnManager* manager)
{
	fLS = ls;
	fLeft = left;
	fRight = right;
	fTop = top;
	fBottom = bottom;

	fRowColumnManager = manager;

	// adds the two essential constraints of the area that make sure that the
	// left x-tab is really to the left of the right x-tab, and the top y-tab
	// really above the bottom y-tab
	fMinContentWidth = ls->AddConstraint(-1.0, fLeft, 1.0, fRight, kGE, 0);
	fMinContentHeight = ls->AddConstraint(-1.0, fTop, 1.0, fBottom, kGE, 0);

	fConstraints.AddItem(fMinContentWidth);
	fConstraints.AddItem(fMinContentHeight);
}


void
Area::_Init(LinearSpec* ls, Row* row, Column* column, RowColumnManager* manager)
{
	_Init(ls, column->Left(), row->Top(), column->Right(), row->Bottom(), manager);

	fRow = row;
	fColumn = column;
}


/**
 * Perform layout on the area.
 */
void
Area::_DoLayout()
{
	// check if if we are initialized
	if (!fLeft)
		return;

	BRect areaFrame(round(fLeft->Value()), round(fTop->Value()),
		round(fRight->Value()), round(fBottom->Value()));
	areaFrame.left += LeftInset();
	areaFrame.right -= RightInset();
	areaFrame.top += TopInset();
	areaFrame.bottom -= BottomInset();

	fLayoutItem->AlignInFrame(areaFrame);
}


void
Area::_UpdateMinSizeConstraint(BSize min)
{
	float width = 0.;
	float height = 0.;
	if (min.width > 0)
		width = min.Width() + LeftInset() + RightInset();
	if (min.height > 0)
		height = min.Height() + TopInset() + BottomInset();

	fMinContentWidth->SetRightSide(width);
	fMinContentHeight->SetRightSide(height);
}


void
Area::_UpdateMaxSizeConstraint(BSize max)
{
	max.width += LeftInset() + RightInset();
	max.height += TopInset() + BottomInset();

	// we only need max constraints if the alignment is full height/width
	// otherwise we can just align the item in the free space
	BAlignment alignment = fLayoutItem->Alignment();
	if (alignment.Vertical() == B_ALIGN_USE_FULL_HEIGHT) {
		if (fMaxContentHeight == NULL) {
			fMaxContentHeight = fLS->AddConstraint(-1.0, fTop, 1.0, fBottom,
				kLE, max.Height());
			fConstraints.AddItem(fMaxContentHeight);
		} else
			fMaxContentHeight->SetRightSide(max.Height());
	}
	else {
		fConstraints.RemoveItem(fMaxContentHeight);
		delete fMaxContentHeight;
		fMaxContentHeight = NULL;
	}

	if (alignment.Horizontal() == B_ALIGN_USE_FULL_WIDTH) {
		if (fMaxContentWidth == NULL) {
			fMaxContentWidth = fLS->AddConstraint(-1.0, fLeft, 1.0, fRight, kLE,
				max.Width());
			fConstraints.AddItem(fMaxContentWidth);
		} else
			fMaxContentWidth->SetRightSide(max.Width());
	}
	else {
		fConstraints.RemoveItem(fMaxContentWidth);
		delete fMaxContentWidth;
		fMaxContentWidth = NULL;
	}
}

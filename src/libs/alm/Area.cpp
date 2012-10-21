/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "Area.h"

#include <Alignment.h>
#include <ControlLook.h>
#include <View.h>

#include "ALMLayout.h"
#include "RowColumnManager.h"
#include "Row.h"
#include "Column.h"


using namespace LinearProgramming;


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
Area::SetLeft(BReference<XTab> left)
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
Area::SetRight(BReference<XTab> right)
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
Area::SetTop(BReference<YTab> top)
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
Area::SetBottom(BReference<YTab> bottom)
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
	} else {
		fContentAspectRatioC->SetLeftSide(-1.0, fLeft, 1.0, fRight, ratio,
			fTop, -ratio, fBottom);
	}
	/* called during BALMLayout::ItemUnarchived */
	if (BLayout* layout = fLayoutItem->Layout())
		layout->InvalidateLayout();
}


void
Area::GetInsets(float* left, float* top, float* right, float* bottom) const
{
	if (left)
		*left = fLeftTopInset.Width();
	if (top)
		*top = fLeftTopInset.Height();
	if (right)
		*right = fRightBottomInset.Width();
	if (bottom)
		*bottom = fRightBottomInset.Height();
}


/**
 * Gets left inset between area and its content.
 */
float
Area::LeftInset() const
{
	if (fLeftTopInset.IsWidthSet())
		return fLeftTopInset.Width();

	BALMLayout* layout = static_cast<BALMLayout*>(fLayoutItem->Layout());
	return layout->InsetForTab(fLeft.Get());
}


/**
 * Gets top inset between area and its content.
 */
float
Area::TopInset() const
{
	if (fLeftTopInset.IsHeightSet())
		return fLeftTopInset.Height();

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


void
Area::SetInsets(float insets)
{
	if (insets != B_SIZE_UNSET)
		insets = BControlLook::ComposeSpacing(insets);

	fLeftTopInset.Set(insets, insets);
	fRightBottomInset.Set(insets, insets);
	fLayoutItem->Layout()->InvalidateLayout();
}


void
Area::SetInsets(float horizontal, float vertical)
{
	if (horizontal != B_SIZE_UNSET)
		horizontal = BControlLook::ComposeSpacing(horizontal);
	if (vertical != B_SIZE_UNSET)
		vertical = BControlLook::ComposeSpacing(vertical);

	fLeftTopInset.Set(horizontal, horizontal);
	fRightBottomInset.Set(vertical, vertical);
	fLayoutItem->Layout()->InvalidateLayout();
}


void
Area::SetInsets(float left, float top, float right, float bottom)
{
	if (left != B_SIZE_UNSET)
		left = BControlLook::ComposeSpacing(left);
	if (right != B_SIZE_UNSET)
		right = BControlLook::ComposeSpacing(right);
	if (top != B_SIZE_UNSET)
		top = BControlLook::ComposeSpacing(top);
	if (bottom != B_SIZE_UNSET)
		bottom = BControlLook::ComposeSpacing(bottom);

	fLeftTopInset.Set(left, top);
	fRightBottomInset.Set(right, bottom);
	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets left inset between area and its content.
 */
void
Area::SetLeftInset(float left)
{
	fLeftTopInset.width = left;
	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets top inset between area and its content.
 */
void
Area::SetTopInset(float top)
{
	fLeftTopInset.height = top;
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
	return BRect(round(fLeft->Value()), round(fTop->Value()),
		round(fRight->Value()), round(fBottom->Value()));
}


/**
 * Destructor.
 * Removes the area from its specification.
 */
Area::~Area()
{
	delete fMinContentWidth;
	delete fMaxContentWidth;
	delete fMinContentHeight;
	delete fMaxContentHeight;
	delete fContentAspectRatioC;
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
	fContentAspectRatio(-1),
	fRowColumnManager(NULL),
	fMinContentWidth(NULL),
	fMaxContentWidth(NULL),
	fMinContentHeight(NULL),
	fMaxContentHeight(NULL),
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

	InvalidateSizeConstraints();
}


void
Area::_Init(LinearSpec* ls, Row* row, Column* column, RowColumnManager* manager)
{
	_Init(ls, column->Left(), row->Top(), column->Right(),
		row->Bottom(), manager);

	fRow = row;
	fColumn = column;
}


/**
 * Perform layout on the area.
 */
void
Area::_DoLayout(const BPoint& offset)
{
	// check if if we are initialized
	if (!fLeft)
		return;

	if (!fLayoutItem->IsVisible())
		fLayoutItem->AlignInFrame(BRect(0, 0, -1, -1));

	BRect areaFrame(Frame());
	areaFrame.left += LeftInset();
	areaFrame.right -= RightInset();
	areaFrame.top += TopInset();
	areaFrame.bottom -= BottomInset();

	fLayoutItem->AlignInFrame(areaFrame.OffsetBySelf(offset));
}


void
Area::_UpdateMinSizeConstraint(BSize min)
{
	if (!fLayoutItem->IsVisible()) {
		fMinContentHeight->SetRightSide(-1);
		fMinContentWidth->SetRightSide(-1);
		return;
	}

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
	if (!fLayoutItem->IsVisible()) {
		fMaxContentHeight->SetRightSide(B_SIZE_UNLIMITED);
		fMaxContentWidth->SetRightSide(B_SIZE_UNLIMITED);
		return;
	}

	max.width += LeftInset() + RightInset();
	max.height += TopInset() + BottomInset();

	const double kPriority = 100;
	// we only need max constraints if the alignment is full height/width
	// otherwise we can just align the item in the free space
	BAlignment alignment = fLayoutItem->Alignment();
	double priority = kPriority;
	if (alignment.Vertical() == B_ALIGN_USE_FULL_HEIGHT)
		priority = -1;

	if (max.Height() < 20000) {
		if (fMaxContentHeight == NULL) {
			fMaxContentHeight = fLS->AddConstraint(-1.0, fTop, 1.0, fBottom,
				kLE, max.Height(), priority, priority);
		} else {
			fMaxContentHeight->SetRightSide(max.Height());
			fMaxContentHeight->SetPenaltyNeg(priority);
			fMaxContentHeight->SetPenaltyPos(priority);
		}
	} else {
		delete fMaxContentHeight;
		fMaxContentHeight = NULL;
	}

	priority = kPriority;
	if (alignment.Horizontal() == B_ALIGN_USE_FULL_WIDTH)
		priority = -1;

	if (max.Width() < 20000) {
		if (fMaxContentWidth == NULL) {
			fMaxContentWidth = fLS->AddConstraint(-1.0, fLeft, 1.0, fRight, kLE,
				max.Width(), priority, priority);
		} else {
			fMaxContentWidth->SetRightSide(max.Width());
			fMaxContentWidth->SetPenaltyNeg(priority);
			fMaxContentWidth->SetPenaltyPos(priority);
		}
	} else {
		delete fMaxContentWidth;
		fMaxContentWidth = NULL;
	}
}

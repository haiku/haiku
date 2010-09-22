/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
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


using namespace std;


BView*
Area::View()
{
	return fLayoutItem->View();
}


/**
 * Gets the auto preferred content size.
 *
 * @return the auto preferred content size
 */
bool
Area::AutoPreferredContentSize() const
{
	return fAutoPreferredContentSize;
}


/**
 * Sets the auto preferred content size true or false.
 *
 * @param value	the auto preferred content size
 */
void
Area::SetAutoPreferredContentSize(bool value)
{
	fAutoPreferredContentSize = value;
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

	fColumn = NULL;

	fMinContentWidth->SetLeftSide(-1.0, fLeft, 1.0, fRight);

	if (fMaxContentWidth != NULL)
		fMaxContentWidth->SetLeftSide(-1.0, fLeft, 1.0, fRight);

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

	fColumn = NULL;

	fMinContentWidth->SetLeftSide(-1.0, fLeft, 1.0, fRight);

	if (fMaxContentWidth != NULL)
		fMaxContentWidth->SetLeftSide(-1.0, fLeft, 1.0, fRight);

	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets the top tab of the area.
 */
void
Area::SetTop(YTab* top)
{
	fTop = top;

	fRow = NULL;

	fMinContentHeight->SetLeftSide(-1.0, fTop, 1.0, fBottom);

	if (fMaxContentHeight != NULL)
		fMaxContentHeight->SetLeftSide(-1.0, fTop, 1.0, fBottom);

	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets the bottom tab of the area.
 */
void
Area::SetBottom(YTab* bottom)
{
	fBottom = bottom;

	fRow = NULL;

	fMinContentHeight->SetLeftSide(-1.0, fTop, 1.0, fBottom);

	if (fMaxContentHeight != NULL)
		fMaxContentHeight->SetLeftSide(-1.0, fTop, 1.0, fBottom);

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
 * Sets the row that defines the top and bottom tabs.
 * May be null.
 */
void
Area::SetRow(Row* row)
{
	SetTop(row->Top());
	SetBottom(row->Bottom());
	fRow = row;
	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets the column that defines the left and right tabs.
 * May be null.
 */
void
Area::SetColumn(Column* column)
{
	SetLeft(column->Left());
	SetRight(column->Right());
	fColumn = column;
	fLayoutItem->Layout()->InvalidateLayout();
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


void Area::SetShrinkPenalties(BSize shrink) {
	fShrinkPenalties = shrink;
	if (fPreferredContentWidth != NULL) {
		fPreferredContentWidth->SetPenaltyNeg(shrink.Width());
		fPreferredContentHeight->SetPenaltyNeg(shrink.Height());
	}
	fLayoutItem->Layout()->InvalidateLayout();
}


void
Area::SetGrowPenalties(BSize grow)
{
	fGrowPenalties = grow;
	if (fPreferredContentWidth != NULL) {
		fPreferredContentWidth->SetPenaltyPos(grow.Width());
		fPreferredContentHeight->SetPenaltyPos(grow.Height());
	}
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
	if (fContentAspectRatioC == NULL) {
		fContentAspectRatioC = fLS->AddConstraint(-1.0, fLeft, 1.0, fRight,
			ratio, fTop, -ratio, fBottom, OperatorType(EQ), 0.0);
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
int32
Area::LeftInset() const
{
	return fLeftInset;
}


/**
 * Gets top inset between area and its content.
 */
int32
Area::TopInset() const
{
	return fTopInset;
}


/**
 * Gets right inset between area and its content.
 */
int32
Area::RightInset() const
{
	return fRightInset;
}


/**
 * Gets bottom inset between area and its content.
 */
int32
Area::BottomInset() const
{
	return fBottomInset;
}


/**
 * Sets left inset between area and its content.
 */
void
Area::SetLeftInset(int32 left)
{
	fLeftInset = left;
	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets top inset between area and its content.
 */
void
Area::SetTopInset(int32 top)
{
	fTopInset = top;
	fLayoutItem->Layout()->InvalidateLayout();
}


/**
 * Sets right inset between area and its content.
 */
void
Area::SetRightInset(int32 right)
{
	fRightInset = right;
}


/**
 * Sets bottom inset between area and its content.
 */
void
Area::SetBottomInset(int32 bottom)
{
	fBottomInset = bottom;
}


/**
 * Sets the preferred size according to the content's PreferredSize method,
 * and the penalties according to heuristics.
 */
void
Area::SetDefaultBehavior()
{
	if (View() == NULL) {
		SetShrinkPenalties(BSize(0, 0));
		SetGrowPenalties(BSize(0, 0));
		return;
	}

	if (dynamic_cast<BButton*>(View()) != NULL
		|| dynamic_cast<BRadioButton*>(View()) != NULL
		|| dynamic_cast<BCheckBox*>(View()) != NULL
		|| dynamic_cast<BStringView*>(View()) != NULL
		|| dynamic_cast<BPictureButton*>(View()) != NULL
		|| dynamic_cast<BStatusBar*>(View()) != NULL) {
		fShrinkPenalties = BSize(4, 4);
		fGrowPenalties = BSize(3, 3);
	} else {
		fShrinkPenalties = BSize(2, 2);
		fGrowPenalties = BSize(1, 1);
	}
}


Area::operator BString() const
{
	BString string;
	GetString(string);
	return string;
}


void
Area::GetString(BString& string) const
{
	string << "Area(";
	fLeft->GetString(string);
	string << ", ";
	fTop->GetString(string);
	string << ", ";
	fRight->GetString(string);
	string << ", ";
	fBottom->GetString(string);
	string << ")";
}


/**
 * Sets the width of the area to be the same as the width of the given area.
 *
 * @param area	the area that should have the same width
 * @return the same-width constraint
 */
Constraint*
Area::HasSameWidthAs(Area* area)
{
	return fLS->AddConstraint(
		-1.0, fLeft, 1.0, fRight, 1.0, area->fLeft, -1.0, area->fRight,
		OperatorType(EQ), 0.0);
}


/**
 * Sets the height of the area to be the same as the height of the given area.
 *
 * @param area	the area that should have the same height
 * @return the same-height constraint
 */
Constraint*
Area::HasSameHeightAs(Area* area)
{
	return fLS->AddConstraint(
		-1.0, fTop, 1.0, fBottom, 1.0, area->fTop, -1.0, area->fBottom,
		OperatorType(EQ), 0.0);
}


/**
 * Sets the size of the area to be the same as the size of the given area.
 *
 * @param area	the area that should have the same size
 * @return a list containing a same-width and same-height constraint
 */
BList*
Area::HasSameSizeAs(Area* area)
{
	BList* constraints = new BList(2);
	constraints->AddItem(this->HasSameWidthAs(area));
	constraints->AddItem(this->HasSameHeightAs(area));
	return constraints;
}


void
Area::InvalidateSizeConstraints()
{
	BSize minSize = fLayoutItem->MinSize();
	BSize maxSize = fLayoutItem->MaxSize();
	BSize prefSize = fLayoutItem->PreferredSize();

	_UpdateMinSizeConstraint(minSize);
	_UpdateMaxSizeConstraint(maxSize);
	_UpdatePreferredConstraint(prefSize);
}


/**
 * Destructor.
 * Removes the area from its specification.
 */
Area::~Area()
{
	for (int32 i = 0; i < fConstraints.CountItems(); i++)
		delete (Constraint*)fConstraints.ItemAt(i);
}


/**
 * Constructor.
 * Uses XTabs and YTabs.
 */
Area::Area(BLayoutItem* item)
	:
	fLayoutItem(item)
{

}


/**
 * Initialize variables.
 */
void
Area::_Init(LinearSpec* ls, XTab* left, YTab* top, XTab* right, YTab* bottom,
	BView* content)
{
	fMaxContentWidth = NULL;
	fMaxContentHeight = NULL;

	fShrinkPenalties = BSize(2, 2);
	fGrowPenalties = BSize(1, 1);
	fContentAspectRatio = 0;
	fContentAspectRatioC = NULL;

	fAutoPreferredContentSize = false;

	fPreferredContentWidth = NULL;
	fPreferredContentHeight = NULL;

	fLeftInset = 0;
	fTopInset = 0;
	fRightInset = 0;
	fBottomInset = 0;

	fLS = ls;
	fLeft = left;
	fRight = right;
	fTop = top;
	fBottom = bottom;

	// adds the two essential constraints of the area that make sure that the
	// left x-tab is really to the left of the right x-tab, and the top y-tab
	// really above the bottom y-tab
	fMinContentWidth = ls->AddConstraint(-1.0, left, 1.0, right,
		OperatorType(GE), 0);
	fMinContentHeight = ls->AddConstraint(-1.0, top, 1.0, bottom,
		OperatorType(GE), 0);

	fConstraints.AddItem(fMinContentWidth);
	fConstraints.AddItem(fMinContentHeight);
}


void
Area::_Init(LinearSpec* ls, Row* row, Column* column, BView* content)
{
	_Init(ls, column->Left(), row->Top(), column->Right(), row->Bottom(),
		  content);
	fRow = row;
	fColumn = column;
}


/**
 * Perform layout on the area.
 */
void Area::_DoLayout()
{
	BRect areaFrame(round(Left()->Value()), round(Top()->Value()),
		round(Right()->Value()), round(Bottom()->Value()));
	areaFrame.left += fLeftInset;
	areaFrame.right -= fRightInset;
	areaFrame.top += fTopInset;
	areaFrame.bottom -= fBottomInset;

	fLayoutItem->AlignInFrame(areaFrame);
}


void
Area::_UpdateMinSizeConstraint(BSize min)
{
	fMinContentWidth->SetRightSide(min.Width() + fLeftInset + fRightInset);
	fMinContentHeight->SetRightSide(min.Height() + fTopInset + fBottomInset);
}


void
Area::_UpdateMaxSizeConstraint(BSize max)
{
	max.width += fLeftInset + fRightInset;
	max.height += fTopInset + fBottomInset;

	// we only need max constraints if the alignment is full height/width
	// otherwise we can just align the item in the free space
	BAlignment alignment = fLayoutItem->Alignment();
	if (alignment.Vertical() == B_ALIGN_USE_FULL_HEIGHT) {
		if (fMaxContentHeight == NULL) {
			fMaxContentHeight = fLS->AddConstraint(-1.0, fTop, 1.0, fBottom,
				OperatorType(LE), max.Height());
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
			fMaxContentWidth = fLS->AddConstraint(-1.0, fLeft, 1.0, fRight,
				OperatorType(LE), max.Width());
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


/**
 * Sets Preferred size of the area's content.
 * May be different from the preferred size of the area.
 * Manual changes of PreferredContentSize are ignored unless
 * autoPreferredContentSize is set to false.
 */
void
Area::_UpdatePreferredConstraint(BSize preferred)
{
	preferred.width += fLeftInset + fRightInset;
	preferred.height += fTopInset + fBottomInset;
	if (fPreferredContentWidth == NULL) {
		fPreferredContentWidth = fLS->AddConstraint(-1.0, fLeft, 1.0,
			fRight, OperatorType(EQ), preferred.Width(),
			fShrinkPenalties.Width(), fGrowPenalties.Width());
		fConstraints.AddItem(fPreferredContentWidth);

		fPreferredContentHeight = fLS->AddConstraint(-1.0, fTop, 1.0,
			fBottom, OperatorType(EQ), preferred.Height(),
			fShrinkPenalties.Height(), fGrowPenalties.Height());
		fConstraints.AddItem(fPreferredContentHeight);
	} else {
		fPreferredContentWidth->SetRightSide(preferred.Width());
		fPreferredContentHeight->SetRightSide(preferred.Height());
	}
}

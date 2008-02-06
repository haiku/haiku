#include "Area.h"
#include "Column.h"
#include "BALMLayout.h"
#include "OperatorType.h"
#include "Row.h"
#include "XTab.h"
#include "YTab.h"

#include <Button.h>
#include <RadioButton.h>
#include <CheckBox.h>
#include <StringView.h>
#include <PictureButton.h>
#include <StatusBar.h>

#include <algorithm>	// for max


BSize Area::kMaxSize(INT_MAX, INT_MAX);
BSize Area::kMinSize(0, 0);
BSize Area::kUndefinedSize(-1, -1);


/**
 * Gets the auto preferred content size.
 *
 * @return the auto preferred content size
 */
bool
Area::AutoPrefContentSize() const
{
	return fAutoPrefContentSize;
}


/**
 * Sets the auto preferred content size true or false.
 *
 * @param value	the auto preferred content size
 */
void
Area::SetAutoPrefContentSize(bool value)
{
	fAutoPrefContentSize = value;
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
 * Sets the left tab of the area.
 *
 * @param left	the left tab of the area
 */
void
Area::SetLeft(XTab* left)
{
	fLeft = left;
	
	fColumn = NULL;
	
	if (fChildArea == NULL) {
		BList* coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		BList* vars = new BList(2);
		vars->AddItem(fLeft);
		vars->AddItem(fRight);
		fMinContentWidth->ChangeLeftSide(coeffs, vars);
	
		if (fMaxContentWidth != NULL) {
			BList* coeffs2 = new BList(2);
			coeffs2->AddItem(new double(-1.0));
			coeffs2->AddItem(new double(1.0));
			BList* vars2 = new BList(2);
			vars2->AddItem(fLeft);
			vars2->AddItem(fRight);
			fMaxContentWidth->ChangeLeftSide(coeffs2, vars2);
		}
	} else
		UpdateHorizontal();
	fLS->InvalidateLayout();
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
 * Sets the right tab of the area.
 *
 * @param right	the right tab of the area
 */
void
Area::SetRight(XTab* right)
{
	fRight = right;
	
	fColumn = NULL;
	
	if (fChildArea == NULL) {
		BList* coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		BList* vars = new BList(2);
		vars->AddItem(fLeft);
		vars->AddItem(fRight);
		fMinContentWidth->ChangeLeftSide(coeffs, vars);
	
		if (fMaxContentWidth != NULL) {
			BList* coeffs2 = new BList(2);
			coeffs2->AddItem(new double(-1.0));
			coeffs2->AddItem(new double(1.0));
			BList* vars2 = new BList(2);
			vars2->AddItem(fLeft);
			vars2->AddItem(fRight);
			fMaxContentWidth->ChangeLeftSide(coeffs2, vars2);
		}
	} else
		UpdateHorizontal();
	fLS->InvalidateLayout();
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
 * Sets the top tab of the area.
 */
void
Area::SetTop(YTab* top)
{
	fTop = top;
	
	fRow = NULL;
	
	if (fChildArea == NULL) {
		BList* coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		BList* vars = new BList(2);
		vars->AddItem(fTop);
		vars->AddItem(fBottom);
		fMinContentHeight->ChangeLeftSide(coeffs, vars);
		
		if (fMaxContentHeight != NULL) {
			BList* coeffs2 = new BList(2);
			coeffs2->AddItem(new double(-1.0));
			coeffs2->AddItem(new double(1.0));
			BList* vars2 = new BList(2);
			vars2->AddItem(fTop);
			vars2->AddItem(fBottom);
			fMaxContentHeight->ChangeLeftSide(coeffs2, vars2);
		}
	} else
		UpdateVertical();
	fLS->InvalidateLayout();
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
 * Sets the bottom tab of the area.
 */
void
Area::SetBottom(YTab* bottom)
{
	fBottom = bottom;
	
	fRow = NULL;
	
	if (fChildArea == NULL) {
		BList* coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		BList* vars = new BList(2);
		vars->AddItem(fTop);
		vars->AddItem(fBottom);
		fMinContentHeight->ChangeLeftSide(coeffs, vars);
		
		if (fMaxContentHeight != NULL) {
			BList* coeffs2 = new BList(2);
			coeffs2->AddItem(new double(-1.0));
			coeffs2->AddItem(new double(1.0));
			BList* vars2 = new BList(2);
			vars2->AddItem(fTop);
			vars2->AddItem(fBottom);
			fMaxContentHeight->ChangeLeftSide(coeffs2, vars2);
		}
	} else
		UpdateVertical();
	fLS->InvalidateLayout();
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
 * Sets the row that defines the top and bottom tabs.
 * May be null.
 */
void
Area::SetRow(Row* row)
{
	SetTop(row->Top());
	SetBottom(row->Bottom());
	fRow = row;
	fLS->InvalidateLayout();
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
 * Sets the column that defines the left and right tabs.
 * May be null.
 */
void
Area::SetColumn(Column* column)
{
	SetLeft(column->Left());
	SetRight(column->Right());
	fColumn = column;
	fLS->InvalidateLayout();
}


/**
 * Gets the control that is the content of the area.
 */
BView*
Area::Content() const
{
	return (fChildArea == NULL) ? fContent : fChildArea->Content();
}


/**
 * Sets the control that is the content of the area.
 */
void
Area::SetContent(BView* content)
{
	if (fChildArea == NULL) fContent = content;
	else fChildArea->fContent = content;
	fLS->InvalidateLayout();
}


/**
 * Left tab of the area's content. May be different from the left tab of the area.
 */
XTab*
Area::ContentLeft() const
{
	return (fChildArea == NULL) ? fLeft : fChildArea->fLeft;
}


/**
 * Top tab of the area's content. May be different from the top tab of the area.
 */
YTab*
Area::ContentTop() const
{
	return (fChildArea == NULL) ? fTop : fChildArea->fTop;
}


/**
 * Right tab of the area's content. May be different from the right tab of the area.
 */
XTab*
Area::ContentRight() const
{
	return (fChildArea == NULL) ? fRight : fChildArea->fRight;
}


/**
 * Bottom tab of the area's content. May be different from the bottom tab of the area.
 */
YTab*
Area::ContentBottom() const
{
	return (fChildArea == NULL) ? fBottom : fChildArea->fBottom;
}


/**
 * Gets minimum size of the area's content.
 */
BSize
Area::MinContentSize() const
{
	return (fChildArea == NULL) ? fMinContentSize : fChildArea->fMinContentSize;
}


/**
 * Sets minimum size of the area's content.
 * May be different from the minimum size of the area.
 */
void
Area::SetMinContentSize(BSize min)
{
	if (fChildArea == NULL) {
		fMinContentSize = min;
		fMinContentWidth->SetRightSide(fMinContentSize.Width());
		fMinContentHeight->SetRightSide(fMinContentSize.Height());
	} else 
		fChildArea->SetMinContentSize(min);
	fLS->InvalidateLayout();
}


/**
 * Gets maximum size of the area's content.
 */
BSize Area::MaxContentSize() const {
	return (fChildArea == NULL) ? fMaxContentSize : fChildArea->fMaxContentSize;
}


/**
 * Sets maximum size of the area's content.
 * May be different from the maximum size of the area.
 */
void
Area::SetMaxContentSize(BSize max)
{
	if (fChildArea == NULL) {
		fMaxContentSize = max;
		if (fMaxContentWidth == NULL) {
			BList* coeffs = new BList(2);
			coeffs->AddItem(new double(-1.0));
			coeffs->AddItem(new double(1.0));
			BList* vars = new BList(2);
			vars->AddItem(fLeft);
			vars->AddItem(fRight);
			fMaxContentWidth = fLS->AddConstraint(coeffs, vars, OperatorType(LE),
					fMaxContentSize.Width());
			fConstraints->AddItem(fMaxContentWidth);
			
			BList* coeffs2 = new BList(2);
			coeffs2->AddItem(new double(-1.0));
			coeffs2->AddItem(new double(1.0));
			BList* vars2 = new BList(2);
			vars2->AddItem(fTop);
			vars2->AddItem(fBottom);
			fMaxContentHeight = fLS->AddConstraint(coeffs2, vars2, OperatorType(LE),
					fMaxContentSize.Height());
			fConstraints->AddItem(fMaxContentHeight);
		} else {
			fMaxContentWidth->SetRightSide(fMaxContentSize.Width());
			fMaxContentHeight->SetRightSide(fMaxContentSize.Height());
		}
	} else
		fChildArea->SetMaxContentSize(max);
	fLS->InvalidateLayout();
}


/**
 * Gets Preferred size of the area's content.
 */
BSize
Area::PrefContentSize() const
{
	return (fChildArea == NULL) ? fPrefContentSize : fChildArea->fPrefContentSize;
}


/**
 * Sets Preferred size of the area's content.
 * May be different from the preferred size of the area.
 * Manual changes of PrefContentSize are ignored unless autoPrefContentSize is set to false.
 */
void
Area::SetPrefContentSize(BSize pref)
{
	if (fChildArea == NULL) {
		fPrefContentSize = pref;
		if (fPrefContentWidth == NULL) {
			BList* coeffs = new BList(2);
			coeffs->AddItem(new double(-1.0));
			coeffs->AddItem(new double(1.0));
			BList* vars = new BList(2);
			vars->AddItem(fLeft);
			vars->AddItem(fRight);
			fPrefContentWidth = fLS->AddSoftConstraint(coeffs, vars, OperatorType(EQ),
					fPrefContentSize.Width(), fShrinkRigidity.Width(),
					fExpandRigidity.Width());
			fConstraints->AddItem(fPrefContentWidth);
			
			BList* coeffs2 = new BList(2);
			coeffs2->AddItem(new double(-1.0));
			coeffs2->AddItem(new double(1.0));
			BList* vars2 = new BList(2);
			vars2->AddItem(fTop);
			vars2->AddItem(fBottom);
			fPrefContentHeight = fLS->AddSoftConstraint(coeffs2, vars2, OperatorType(EQ),
					fPrefContentSize.Height(), fShrinkRigidity.Height(),
					fExpandRigidity.Height());
			fConstraints->AddItem(fPrefContentHeight);
		} else {
			fPrefContentWidth->SetRightSide(pref.Width());
			fPrefContentHeight->SetRightSide(pref.Height());
		}
	} else
		fChildArea->SetPrefContentSize(pref);
	fLS->InvalidateLayout();
}


/**
 * The reluctance with which the area's content shrinks below its preferred size.
 * The bigger the less likely is such shrinking.
 */
BSize
Area::ShrinkRigidity() const
{
	return (fChildArea == NULL) ? fShrinkRigidity : fChildArea->fShrinkRigidity;
}


void Area::SetShrinkRigidity(BSize shrink) {
	if (fChildArea == NULL) {
		fShrinkRigidity = shrink;
		if (fPrefContentWidth != NULL) {
			fPrefContentWidth->SetPenaltyNeg(shrink.Width());
			fPrefContentHeight->SetPenaltyNeg(shrink.Height());
		}
	} else 
		fChildArea->SetShrinkRigidity(shrink);
	fLS->InvalidateLayout();
}


/**
 * The reluctance with which the area's content expands over its preferred size.
 * The bigger the less likely is such expansion.
 */
BSize
Area::ExpandRigidity() const
{
	return (fChildArea == NULL) ? fExpandRigidity : fChildArea->fExpandRigidity;
}


void
Area::SetExpandRigidity(BSize expand)
{
	if (fChildArea == NULL) {
		fExpandRigidity = expand;
		if (fPrefContentWidth != NULL) {
			fPrefContentWidth->SetPenaltyPos(expand.Width());
			fPrefContentHeight->SetPenaltyPos(expand.Height());
		}
	} else 
		fChildArea->SetExpandRigidity(expand);
	fLS->InvalidateLayout();
}


/**
 * Gets aspect ratio of the area's content.
 */
double
Area::ContentAspectRatio() const
{
	return (fChildArea == NULL) ? fContentAspectRatio : fChildArea->fContentAspectRatio;
}


/**
 * Sets aspect ratio of the area's content.
 * May be different from the aspect ratio of the area.
 */
void
Area::SetContentAspectRatio(double ratio)
{
	if (fChildArea == NULL) {
		fContentAspectRatio = ratio;
		if (fContentAspectRatioC == NULL) {
			BList* coeffs = new BList(4);
			coeffs->AddItem(new double(-1.0));
			coeffs->AddItem(new double(1.0));
			coeffs->AddItem(new double(ratio));
			coeffs->AddItem(new double(-ratio));
			BList* vars = new BList(4);
			vars->AddItem(fLeft);
			vars->AddItem(fRight);
			vars->AddItem(fTop);
			vars->AddItem(fBottom);
			fContentAspectRatioC = fLS->AddConstraint(coeffs, vars,
					OperatorType(EQ), 0.0);
			fConstraints->AddItem(fContentAspectRatioC);
		} else {
			BList* coeffs2 = new BList(4);
			coeffs2->AddItem(new double(-1.0));
			coeffs2->AddItem(new double(1.0));
			coeffs2->AddItem(new double(ratio));
			coeffs2->AddItem(new double(-ratio));
			BList* vars2 = new BList(4);
			vars2->AddItem(fLeft);
			vars2->AddItem(fRight);
			vars2->AddItem(fTop);
			vars2->AddItem(fBottom);
			fContentAspectRatioC->ChangeLeftSide(coeffs2, vars2);
		}
	} else
		fChildArea->SetContentAspectRatio(ratio);
	fLS->InvalidateLayout();
}


/**
 * Gets alignment of the content in its area.
 */
BAlignment
Area::Alignment() const
{
	return fAlignment;
}


/**
 * Sets alignment of the content in its area.
 */
void
Area::SetAlignment(BAlignment alignment)
{
	fAlignment = alignment;
	UpdateHorizontal();
	fLS->InvalidateLayout();
	UpdateVertical();
	fLS->InvalidateLayout();
}


/**
 * Sets horizontal alignment of the content in its area.
 */
void Area::SetHAlignment(alignment horizontal) {
	fAlignment.SetHorizontal(horizontal);
	UpdateHorizontal();
	fLS->InvalidateLayout();
}


/**
 * Sets vertical alignment of the content in its area.
 */
void
Area::SetVAlignment(vertical_alignment vertical)
{
	fAlignment.SetVertical(vertical);
	UpdateVertical();
	fLS->InvalidateLayout();
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
 * Sets left inset between area and its content.
 */
void
Area::SetLeftInset(int32 left)
{
	fLeftInset = left;
	UpdateHorizontal();
	fLS->InvalidateLayout();
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
 * Sets top inset between area and its content.
 */
void
Area::SetTopInset(int32 top)
{
	fTopInset = top;
	UpdateVertical();
	fLS->InvalidateLayout();
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
 * Sets right inset between area and its content.
 */
void
Area::SetRightInset(int32 right)
{
	fRightInset = right;
	UpdateHorizontal();
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
 * Sets bottom inset between area and its content.
 */
void
Area::SetBottomInset(int32 bottom)
{
	fBottomInset = bottom;
	UpdateVertical();
}


/**
 * Sets the preferred size according to the content's PreferredSize method, 
 * and the rigidities according to heuristics.
 */
void
Area::SetDefaultPrefContentSize()
{
	if (Content() == NULL) {
		SetPrefContentSize(BSize(0, 0));
		SetShrinkRigidity(BSize(0, 0));
		SetExpandRigidity(BSize(0, 0));
		return;
	}
	
	if (PrefContentSize() != Content()->PreferredSize()){
		SetPrefContentSize(Content()->PreferredSize());
		fLS->InvalidateLayout();
	}
	
	if (dynamic_cast<BButton*>(Content()) != NULL
		|| dynamic_cast<BRadioButton*>(Content()) != NULL
		|| dynamic_cast<BCheckBox*>(Content()) != NULL
		|| dynamic_cast<BStringView*>(Content()) != NULL
		|| dynamic_cast<BPictureButton*>(Content()) != NULL
		|| dynamic_cast<BStatusBar*>(Content()) != NULL) {
		//~ || Content is LinkLabel
		//~ || Content is NumericUpDown) {
		fShrinkRigidity = BSize(4, 4);
		fExpandRigidity = BSize(3, 3);
	} else {
		fShrinkRigidity = BSize(2, 2);
		fExpandRigidity = BSize(1, 1);
	}
}


//~ string Area::ToString() {
	//~ return "Area(" + fLeft->ToString() + "," + fTop->ToString() + ","
			//~ + fRight->ToString() + "," + fBottom->ToString() + ")";
//~ }


/**
 * Sets the width of the area to be the same as the width of the given area.
 * 
 * @param area	the area that should have the same width
 * @return the same-width constraint
 */
Constraint*
Area::HasSameWidthAs(Area* area)
{
	BList* coeffs = new BList(4);
	coeffs->AddItem(new double(-1.0));
	coeffs->AddItem(new double(1.0));
	coeffs->AddItem(new double(1.0));
	coeffs->AddItem(new double(-1.0));
	BList* vars = new BList(4);
	vars->AddItem(fLeft);
	vars->AddItem(fRight);
	vars->AddItem(area->fLeft);
	vars->AddItem(area->fRight);
	return fLS->AddConstraint(coeffs, vars, OperatorType(EQ), 0.0);
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
	BList* coeffs = new BList(4);
	coeffs->AddItem(new double(-1.0));
	coeffs->AddItem(new double(1.0));
	coeffs->AddItem(new double(1.0));
	coeffs->AddItem(new double(-1.0));
	BList* vars = new BList(4);
	vars->AddItem(fTop);
	vars->AddItem(fBottom);
	vars->AddItem(area->fTop);
	vars->AddItem(area->fBottom);
	return fLS->AddConstraint(coeffs, vars, OperatorType(EQ), 0.0);
}


/**
 * Sets the size of the area to be the same as the size of the given area.
 * 
 * @param area	the area that should have the same size
 * @return a list containing a same-width and same-height constraint
 */
BList*
Area::HasSameSizetAs(Area* area)
{
	BList* constraints = new BList(2);
	constraints->AddItem(this->HasSameWidthAs(area));
	constraints->AddItem(this->HasSameHeightAs(area));
	return constraints;
}


/**
 * Destructor.
 * Removes the area from its specification.
 */
Area::~Area()
{
	if (fChildArea != NULL) delete fChildArea;
	for (int32 i = 0; i < fConstraints->CountItems(); i++)
		delete (Constraint*)fConstraints->ItemAt(i);
	fLS->Areas()->RemoveItem(this);
}


/**
 * Constructor.
 * Uses XTabs and YTabs.
 */
Area::Area(BALMLayout* ls, XTab* left, YTab* top, XTab* right, YTab* bottom, 
	BView* content, BSize minContentSize)
{			
	Init(ls, left, top, right, bottom, content, minContentSize);
}


/**
 * Constructor.
 * Uses Rows and Columns.
 */
Area::Area(BALMLayout* ls, Row* row, Column* column, BView* content,
	BSize minContentSize)
{
	
	Init(ls, column->Left(), row->Top(), column->Right(), row->Bottom(), 
			content, minContentSize);
	fRow = row;
	fColumn = column;
}


/**
 * Initialize variables.
 */
void
Area::Init(BALMLayout* ls, XTab* left, YTab* top, XTab* right, YTab* bottom, 
	BView* content, BSize minContentSize)
{
			
	fConstraints = new BList(2);
	fMaxContentSize = kMaxSize;
			
	fMaxContentWidth = NULL;
	fMaxContentHeight = NULL;
			
	fPrefContentSize = kUndefinedSize;
	fShrinkRigidity = BSize(2, 2);
	fExpandRigidity = BSize(1, 1);
	fContentAspectRatio = NULL;
	fContentAspectRatioC = NULL;
			
	fAutoPrefContentSize = false;
			
	fPrefContentWidth = NULL;
	fPrefContentHeight = NULL;
	
	fChildArea = NULL;
	
	fAlignment = BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
	fLeftInset = 0;
	fTopInset = 0;
	fRightInset = 0;
	fBottomInset = 0;
	
	fLeftConstraint = NULL;
	fTopConstraint = NULL;
	fRightConstraint = NULL;
	fBottomConstraint = NULL;
	
	fLS = ls;
	fLeft = left;
	fRight = right;
	fTop = top;
	fBottom = bottom;
	SetContent(content);
	fMinContentSize = minContentSize;
	
	// adds the two essential constraints of the area that make sure that the left x-tab is 
	// really to the left of the right x-tab, and the top y-tab really above the bottom y-tab
	BList* coeffs = new BList(2);
	coeffs->AddItem(new double(-1.0));
	coeffs->AddItem(new double(1.0));
	BList* vars = new BList(2);
	vars->AddItem(left);
	vars->AddItem(right);
	fMinContentWidth = ls->AddConstraint(coeffs, vars, OperatorType(GE),
			minContentSize.Width());
	fConstraints->AddItem(fMinContentWidth);
	
	BList* coeffs2 = new BList(2);
	coeffs2->AddItem(new double(-1.0));
	coeffs2->AddItem(new double(1.0));
	BList* vars2 = new BList(2);
	vars2->AddItem(top);
	vars2->AddItem(bottom);
	fMinContentHeight = ls->AddConstraint(coeffs2, vars2, OperatorType(GE),
			minContentSize.Height());
	fConstraints->AddItem(fMinContentHeight);
}


/**
 * Perform layout on the area.
 */
void Area::DoLayout()
{
	if (Content() == NULL) 
		return; // empty areas need no layout
		
	// if there is a childArea, then it is the childArea that actually contains the content
	Area* area = (fChildArea != NULL) ? fChildArea : this;
	
	// set content location and size
	area->Content()->MoveTo(floor(area->Left()->Value() + 0.5), 
			floor(area->Top()->Value() + 0.5));
	int32 width = (int32)floor(area->Right()->Value() - area->Left()->Value() + 0.5);
	int32 height = (int32)floor(area->Bottom()->Value() - area->Top()->Value() + 0.5);
	area->Content()->ResizeTo(width, height);
}


/**
 * Adds a childArea to this area, together with constraints that specify the relative location 
 * of the childArea within this area. It is called when such a childArea becomes necessary, 
 * i.e. when the user requests insets or special alignment.
 */
void
Area::InitChildArea()
{
	// add a child area with new tabs,
	// and add constraints that set its tabs to be equal to the 
	// coresponding tabs of this area (for a start)
	fChildArea = new Area(fLS, new XTab(fLS), new YTab(fLS), new XTab(fLS),
			new YTab(fLS), fContent, BSize(0, 0));
	fLeftConstraint = Left()->IsEqual(fChildArea->Left());
	fConstraints->AddItem(fLeftConstraint);
	fTopConstraint = Top()->IsEqual(fChildArea->Top());
	fConstraints->AddItem(fTopConstraint);
	fRightConstraint = Right()->IsEqual(fChildArea->Right());
	fConstraints->AddItem(fRightConstraint);
	fBottomConstraint = Bottom()->IsEqual(fChildArea->Bottom());
	fConstraints->AddItem(fBottomConstraint);
	
	// remove the minimum content size constraints from this area
	// and copy the minimum content size setting to the childArea
	fConstraints->RemoveItem(fMinContentWidth);
	delete fMinContentWidth;
	fMinContentWidth = fChildArea->fMinContentWidth;
	fConstraints->RemoveItem(fMinContentHeight);
	delete fMinContentHeight;
	fMinContentHeight = fChildArea->fMinContentHeight;
	fChildArea->SetMinContentSize(fMinContentSize);

	// if there are maximum content size constraints on this area, 
	// change them so that they refer to the tabs of the childArea 
	// and copy the minimum content size settings to the childArea
	if (fMaxContentWidth != NULL) {
		fChildArea->fMaxContentSize = fMaxContentSize;
		
		fChildArea->fMaxContentWidth = fMaxContentWidth;
		BList* coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		BList* vars = new BList(2);
		vars->AddItem(fChildArea->Left());
		vars->AddItem(fChildArea->Right());
		fMaxContentWidth->ChangeLeftSide(coeffs, vars);
		
		fChildArea->fMaxContentHeight = fMaxContentHeight;
		BList* coeffs2 = new BList(2);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		BList* vars2 = new BList(2);
		vars2->AddItem(fChildArea->Top());
		vars2->AddItem(fChildArea->Bottom());
		fMaxContentHeight->ChangeLeftSide(coeffs2, vars2);
	}
	
	// if there are preferred content size constraints on this area, 
	// change them so that they refer to the tabs of the childArea 
	// and copy the preferred content size settings to the childArea
	if (fPrefContentHeight != NULL) {
		fChildArea->fPrefContentSize = fPrefContentSize;
		fChildArea->fShrinkRigidity = fShrinkRigidity;
		fChildArea->fExpandRigidity = fExpandRigidity;
		
		fChildArea->fPrefContentWidth = fPrefContentWidth;
		BList* coeffs3 = new BList(2);
		coeffs3->AddItem(new double(-1.0));
		coeffs3->AddItem(new double(1.0));
		BList* vars3 = new BList(2);
		vars3->AddItem(fChildArea->Left());
		vars3->AddItem(fChildArea->Right());
		fPrefContentWidth->ChangeLeftSide(coeffs3, vars3);
		
		fChildArea->fPrefContentHeight = fPrefContentHeight;
		BList* coeffs4 = new BList(2);
		coeffs4->AddItem(new double(-1.0));
		coeffs4->AddItem(new double(1.0));
		BList* vars4 = new BList(2);
		vars4->AddItem(fChildArea->Top());
		vars4->AddItem(fChildArea->Bottom());
		fPrefContentHeight->ChangeLeftSide(coeffs4, vars4);
	}
}


/**
 * Update the constraints for horizontal insets and alignment.
 */
void
Area::UpdateHorizontal()
{
	// if the area does not have a childAdrea yet, this is the time to add it
	if (fChildArea == NULL)
		InitChildArea();
	
	BList* coeffs;
	BList* coeffs2;
	BList* vars;
	BList* vars2;
	
	// change the constraints leftConstraint and rightConstraint so that the horizontal 
	// alignment and insets of the childArea within this area are as specified by the user
	if (fAlignment.Horizontal() == B_ALIGN_LEFT) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Left());
		vars->AddItem(fChildArea->Left());
		fLeftConstraint->ChangeLeftSide(coeffs, vars);
		fLeftConstraint->SetOp(OperatorType(EQ));
		fLeftConstraint->SetRightSide(fLeftInset);
		
		coeffs2 = new BList(2);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		vars2 = new BList(2);
		vars2->AddItem(fChildArea->Right());
		vars2->AddItem(Right());
		fRightConstraint->ChangeLeftSide(coeffs2, vars2);
		fRightConstraint->SetOp(OperatorType(GE));
		fRightConstraint->SetRightSide(fRightInset);
	} else if (fAlignment.Horizontal() == B_ALIGN_RIGHT) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Left());
		vars->AddItem(fChildArea->Left());
		fLeftConstraint->ChangeLeftSide(coeffs, vars);
		fLeftConstraint->SetOp(OperatorType(GE));
		fLeftConstraint->SetRightSide(fLeftInset);
		
		coeffs2 = new BList(2);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		vars2 = new BList(2);
		vars2->AddItem(fChildArea->Right());
		vars2->AddItem(Right());
		fRightConstraint->ChangeLeftSide(coeffs2, vars2);
		fRightConstraint->SetOp(OperatorType(EQ));
		fRightConstraint->SetRightSide(fRightInset);
	} else if (fAlignment.Horizontal() == B_ALIGN_HORIZONTAL_CENTER) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Left());
		vars->AddItem(fChildArea->Left());
		fLeftConstraint->ChangeLeftSide(coeffs, vars);
		fLeftConstraint->SetOp(OperatorType(GE));
		fLeftConstraint->SetRightSide(max(fLeftInset, fRightInset));
		
		coeffs2 = new BList(4);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		coeffs2->AddItem(new double(1.0));
		coeffs2->AddItem(new double(-1.0));
		vars2 = new BList(4);
		vars2->AddItem(Left());
		vars2->AddItem(fChildArea->Left());
		vars2->AddItem(fChildArea->Right());
		vars2->AddItem(Right());
		fRightConstraint->ChangeLeftSide(coeffs2, vars2);
		fRightConstraint->SetOp(OperatorType(EQ));
		fRightConstraint->SetRightSide(0);
	} else if (fAlignment.Horizontal() == B_ALIGN_USE_FULL_WIDTH) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Left());
		vars->AddItem(fChildArea->Left());
		fLeftConstraint->ChangeLeftSide(coeffs, vars);
		fLeftConstraint->SetOp(OperatorType(EQ));
		fLeftConstraint->SetRightSide(fLeftInset);
		
		coeffs2 = new BList(2);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		vars2 = new BList(2);
		vars2->AddItem(fChildArea->Right());
		vars2->AddItem(Right());
		fRightConstraint->ChangeLeftSide(coeffs2, vars2);
		fRightConstraint->SetOp(OperatorType(EQ));
		fRightConstraint->SetRightSide(fRightInset);
	} else if (fAlignment.Horizontal() == B_ALIGN_HORIZONTAL_UNSET) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Left());
		vars->AddItem(fChildArea->Left());
		fLeftConstraint->ChangeLeftSide(coeffs, vars);
		fLeftConstraint->SetOp(OperatorType(GE));
		fLeftConstraint->SetRightSide(fLeftInset);
		
		coeffs2 = new BList(2);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		vars2 = new BList(2);
		vars2->AddItem(fChildArea->Right());
		vars2->AddItem(Right());
		fRightConstraint->ChangeLeftSide(coeffs2, vars2);
		fRightConstraint->SetOp(OperatorType(GE));
		fRightConstraint->SetRightSide(fRightInset);
	}
}


/**
 * Update the constraints for vertical insets and alignment.
 */
void Area::UpdateVertical() {
	// if the area does not have a childAdrea yet, this is the time to add it
	if (fChildArea == NULL) 
		InitChildArea();
	
	BList* coeffs;
	BList* coeffs2;
	BList* vars;
	BList* vars2;
	
	// change the constraints topConstraint and bottomConstraint so that the vertical 
	// alignment and insets of the childArea within this area are as specified by the user
	if (fAlignment.Vertical() == B_ALIGN_TOP) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Top());
		vars->AddItem(fChildArea->Top());
		fTopConstraint->ChangeLeftSide(coeffs, vars);
		fTopConstraint->SetOp(OperatorType(EQ));
		fTopConstraint->SetRightSide(fTopInset);
		
		coeffs2 = new BList(2);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		vars2 = new BList(2);
		vars2->AddItem(fChildArea->Bottom());
		vars2->AddItem(Bottom());
		fBottomConstraint->ChangeLeftSide(coeffs2, vars2);
		fBottomConstraint->SetOp(OperatorType(GE));
		fBottomConstraint->SetRightSide(fBottomInset);
	} else if (fAlignment.Vertical() == B_ALIGN_BOTTOM) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Top());
		vars->AddItem(fChildArea->Top());
		fTopConstraint->ChangeLeftSide(coeffs, vars);
		fTopConstraint->SetOp(OperatorType(GE));
		fTopConstraint->SetRightSide(fTopInset);
		
		coeffs2 = new BList(2);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		vars2 = new BList(2);
		vars2->AddItem(fChildArea->Bottom());
		vars2->AddItem(Bottom());
		fBottomConstraint->ChangeLeftSide(coeffs2, vars2);
		fBottomConstraint->SetOp(OperatorType(EQ));
		fBottomConstraint->SetRightSide(fBottomInset);
	} else if (fAlignment.Vertical() == B_ALIGN_VERTICAL_CENTER) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Top());
		vars->AddItem(fChildArea->Top());
		fTopConstraint->ChangeLeftSide(coeffs, vars);
		fTopConstraint->SetOp(OperatorType(GE));
		fTopConstraint->SetRightSide(max(fTopInset, fBottomInset));
		
		coeffs2 = new BList(4);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		coeffs2->AddItem(new double(1.0));
		coeffs2->AddItem(new double(-1.0));
		vars2 = new BList(4);
		vars2->AddItem(Top());
		vars2->AddItem(fChildArea->Top());
		vars2->AddItem(fChildArea->Bottom());
		vars2->AddItem(Bottom());
		fBottomConstraint->ChangeLeftSide(coeffs2, vars2);
		fBottomConstraint->SetOp(OperatorType(EQ));
		fBottomConstraint->SetRightSide(0);
	} else if (fAlignment.Vertical() == B_ALIGN_USE_FULL_HEIGHT) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Top());
		vars->AddItem(fChildArea->Top());
		fTopConstraint->ChangeLeftSide(coeffs, vars);
		fTopConstraint->SetOp(OperatorType(EQ));
		fTopConstraint->SetRightSide(fTopInset);
		
		coeffs2 = new BList(2);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		vars2 = new BList(2);
		vars2->AddItem(fChildArea->Bottom());
		vars2->AddItem(Bottom());
		fBottomConstraint->ChangeLeftSide(coeffs2, vars2);
		fBottomConstraint->SetOp(OperatorType(EQ));
		fBottomConstraint->SetRightSide(fBottomInset);
	} else if (fAlignment.Vertical() == B_ALIGN_VERTICAL_UNSET) {
		coeffs = new BList(2);
		coeffs->AddItem(new double(-1.0));
		coeffs->AddItem(new double(1.0));
		vars = new BList(2);
		vars->AddItem(Top());
		vars->AddItem(fChildArea->Top());
		fTopConstraint->ChangeLeftSide(coeffs, vars);
		fTopConstraint->SetOp(OperatorType(GE));
		fTopConstraint->SetRightSide(fTopInset);
		
		coeffs2 = new BList(2);
		coeffs2->AddItem(new double(-1.0));
		coeffs2->AddItem(new double(1.0));
		vars2 = new BList(2);
		vars2->AddItem(fChildArea->Bottom());
		vars2->AddItem(Bottom());
		fBottomConstraint->ChangeLeftSide(coeffs2, vars2);
		fBottomConstraint->SetOp(OperatorType(GE));
		fBottomConstraint->SetRightSide(fBottomInset);
	}
}


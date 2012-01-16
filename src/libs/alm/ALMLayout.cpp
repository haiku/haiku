/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#include "ALMLayout.h"

#include <math.h>		// for floor
#include <new>
#include <iostream>
#include <vector>

#include <ControlLook.h>

#include "RowColumnManager.h"
#include "ViewLayoutItem.h"


using namespace LinearProgramming;


const BSize kUnsetSize(B_SIZE_UNSET, B_SIZE_UNSET);


/*!
 * Constructor.
 * Creates new layout engine.
 *
 * If friendLayout is not NULL the solver of the friend layout is used.
 */
BALMLayout::BALMLayout(float hSpacing, float vSpacing, BALMLayout* friendLayout)
	:
	fLeftInset(0),
	fRightInset(0),
	fTopInset(0),
	fBottomInset(0),
	fHSpacing(hSpacing),
	fVSpacing(vSpacing)
{
	fSolver = friendLayout ? friendLayout->Solver() : &fOwnSolver;
	fRowColumnManager = new RowColumnManager(fSolver);

	fLeft = AddXTab();
	fRight = AddXTab();
	fTop = AddYTab();
	fBottom = AddYTab();

	// the Left tab is always at x-position 0, and the Top tab is always at
	// y-position 0
	fLeft->SetRange(0, 0);
	fTop->SetRange(0, 0);

	// cached layout values
	// need to be invalidated whenever the layout specification is changed
	fMinSize = kUnsetSize;
	fMaxSize = kUnsetSize;
	fPreferredSize = kUnsetSize;
}


BALMLayout::~BALMLayout()
{
	delete fRowColumnManager;
}


/**
 * Adds a new x-tab to the specification.
 *
 * @return the new x-tab
 */
BReference<XTab>
BALMLayout::AddXTab()
{
	BReference<XTab> tab(new(std::nothrow) XTab(this), true);
	if (!tab)
		return NULL;
	if (!fSolver->AddVariable(tab))
		return NULL;

	fXTabList.AddItem(tab);
	return tab;
}


void
BALMLayout::AddXTabs(BReference<XTab>* tabs, uint32 count)
{
	for (uint32 i = 0; i < count; i++)
		tabs[i] = AddXTab();
}


void
BALMLayout::AddYTabs(BReference<YTab>* tabs, uint32 count)
{
	for (uint32 i = 0; i < count; i++)
		tabs[i] = AddYTab();
}


/**
 * Adds a new y-tab to the specification.
 *
 * @return the new y-tab
 */
BReference<YTab>
BALMLayout::AddYTab()
{
	BReference<YTab> tab(new(std::nothrow) YTab(this), true);
	if (tab.Get() == NULL)
		return NULL;
	if (!fSolver->AddVariable(tab))
		return NULL;

	fYTabList.AddItem(tab);
	return tab;
}


int32
BALMLayout::CountXTabs() const
{
	return fXTabList.CountItems();
}


int32
BALMLayout::CountYTabs() const
{
	return fYTabList.CountItems();
}


XTab*
BALMLayout::XTabAt(int32 index) const
{
	return fXTabList.ItemAt(index);
}


YTab*
BALMLayout::YTabAt(int32 index) const
{
	return fYTabList.ItemAt(index);
}


static int
compare_x_tab_func(const XTab* tab1, const XTab* tab2)
{
	if (tab1->Value() < tab2->Value())
		return -1;
	else if (tab1->Value() == tab2->Value())
		return 0;
	return 1;
}


static int
compare_y_tab_func(const YTab* tab1, const YTab* tab2)
{
	if (tab1->Value() < tab2->Value())
		return -1;
	else if (tab1->Value() == tab2->Value())
		return 0;
	return 1;
}


const XTabList&
BALMLayout::OrderedXTabs()
{
	fXTabList.SortItems(compare_x_tab_func);
	return fXTabList;
}


const YTabList&
BALMLayout::OrderedYTabs()
{
	fYTabList.SortItems(compare_y_tab_func);
	return fYTabList;
}


/**
 * Adds a new row to the specification that is glued to the given y-tabs.
 *
 * @param top
 * @param bottom
 * @return the new row
 */
Row*
BALMLayout::AddRow(YTab* _top, YTab* _bottom)
{
	BReference<YTab> top = _top;
	BReference<YTab> bottom = _bottom;
	if (_top == NULL)
		top = AddYTab();
	if (_bottom == NULL)
		bottom = AddYTab();
	return new(std::nothrow) Row(fSolver, top, bottom);
}


/**
 * Adds a new column to the specification that is glued to the given x-tabs.
 *
 * @param left
 * @param right
 * @return the new column
 */
Column*
BALMLayout::AddColumn(XTab* _left, XTab* _right)
{
	BReference<XTab> left = _left;
	BReference<XTab> right = _right;
	if (_left == NULL)
		left = AddXTab();
	if (_right == NULL)
		right = AddXTab();
	return new(std::nothrow) Column(fSolver, left, right);
}


Area*
BALMLayout::AreaFor(int32 id) const
{
	int32 areaCount = CountAreas();
	for (int32 i = 0; i < areaCount; i++) {
		Area* area = AreaAt(i);
		if (area->ID() == id)
			return area;
	}
	return NULL;
}


/**
 * Finds the area that contains the given control.
 *
 * @param control	the control to look for
 * @return the area that contains the control
 */
Area*
BALMLayout::AreaFor(const BView* control) const
{
	return AreaFor(ItemAt(IndexOfView(const_cast<BView*>(control))));
}


Area*
BALMLayout::AreaFor(const BLayoutItem* item) const
{
	if (!item)
		return NULL;
	return static_cast<Area*>(item->LayoutData());
}


int32
BALMLayout::CountAreas() const
{
	return CountItems();
}


Area*
BALMLayout::AreaAt(int32 index) const
{
	return AreaFor(ItemAt(index));
}


XTab*
BALMLayout::LeftOf(const BView* view) const
{
	Area* area = AreaFor(view);
	if (!area)
		return NULL;
	return area->Left();
}


XTab*
BALMLayout::LeftOf(const BLayoutItem* item) const
{
	Area* area = AreaFor(item);
	if (!area)
		return NULL;
	return area->Left();
}


XTab*
BALMLayout::RightOf(const BView* view) const
{
	Area* area = AreaFor(view);
	if (!area)
		return NULL;
	return area->Right();
}


XTab*
BALMLayout::RightOf(const BLayoutItem* item) const
{
	Area* area = AreaFor(item);
	if (!area)
		return NULL;
	return area->Right();
}


YTab*
BALMLayout::TopOf(const BView* view) const
{
	Area* area = AreaFor(view);
	if (!area)
		return NULL;
	return area->Top();
}


YTab*
BALMLayout::TopOf(const BLayoutItem* item) const
{
	Area* area = AreaFor(item);
	if (!area)
		return NULL;
	return area->Top();
}


YTab*
BALMLayout::BottomOf(const BView* view) const
{
	Area* area = AreaFor(view);
	if (!area)
		return NULL;
	return area->Bottom();
}


YTab*
BALMLayout::BottomOf(const BLayoutItem* item) const
{
	Area* area = AreaFor(item);
	if (!area)
		return NULL;
	return area->Bottom();
}


void
BALMLayout::BuildLayout(GroupItem& item, XTab* left, YTab* top, XTab* right,
	YTab* bottom)
{
	if (left == NULL)
		left = Left();
	if (top == NULL)
		top = Top();
	if (right == NULL)
		right = Right();
	if (bottom == NULL)
		bottom = Bottom();

	_ParseGroupItem(item, left, top, right, bottom);
}


void
BALMLayout::_ParseGroupItem(GroupItem& item, BReference<XTab> left,
	BReference<YTab> top, BReference<XTab> right, BReference<YTab> bottom)
{
	if (item.LayoutItem())
		AddItem(item.LayoutItem(), left, top, right, bottom);
	else if (item.View()) {
		AddView(item.View(), left, top, right, bottom);
	}
	else {
		for (unsigned int i = 0; i < item.GroupItems().size(); i++) {
			GroupItem& current = const_cast<GroupItem&>(
				item.GroupItems()[i]);
			if (item.Orientation() == B_HORIZONTAL) {
				BReference<XTab> r = (i == item.GroupItems().size() - 1) ? right
					: AddXTab();
				_ParseGroupItem(current, left, top, r, bottom);
				left = r;
			}
			else {
				BReference<YTab> b = (i == item.GroupItems().size() - 1)
					? bottom : AddYTab();
				_ParseGroupItem(current, left, top, right, b);
				top = b;
			}
		}
	}
}


BLayoutItem*
BALMLayout::AddView(BView* child)
{
	return AddView(-1, child);
}


BLayoutItem*
BALMLayout::AddView(int32 index, BView* child)
{
	return BAbstractLayout::AddView(index, child);
}


/**
 * Adds a new area to the specification, automatically setting preferred size constraints.
 *
 * @param left			left border
 * @param top			top border
 * @param right		right border
 * @param bottom		bottom border
 * @param content		the control which is the area content
 * @return the new area
 */
Area*
BALMLayout::AddView(BView* view, XTab* left, YTab* top, XTab* right,
	YTab* bottom)
{
	BLayoutItem* item = _LayoutItemToAdd(view);
	Area* area = AddItem(item, left, top, right, bottom);
	if (!area) {
		if (item != view->GetLayout())
			delete item;
		return NULL;
	}
	return area;
}


/**
 * Adds a new area to the specification, automatically setting preferred size constraints.
 *
 * @param row			the row that defines the top and bottom border
 * @param column		the column that defines the left and right border
 * @param content		the control which is the area content
 * @return the new area
 */
Area*
BALMLayout::AddView(BView* view, Row* row, Column* column)
{
	BLayoutItem* item = _LayoutItemToAdd(view);
	Area* area = AddItem(item, row, column);
	if (!area) {
		if (item != view->GetLayout())
			delete item;
		return NULL;
	}
	return area;
}


bool
BALMLayout::AddItem(BLayoutItem* item)
{
	return AddItem(-1, item);
}


bool
BALMLayout::AddItem(int32 index, BLayoutItem* item)
{
	if (!item)
		return false;

	// simply add the item at the upper right corner of the previous item
	// TODO maybe find a more elegant solution
	XTab* left = Left();
	YTab* top = Top();

	// check range
	if (index < 0 || index > CountItems())
		index = CountItems();

	// for index = 0 we already have set the right tabs
	if (index != 0) {
		BLayoutItem* prevItem = ItemAt(index - 1);
		Area* area = AreaFor(prevItem);
		if (area) {
			left = area->Right();
			top = area->Top();
		}
	}
	Area* area = AddItem(item, left, top);
	return area ? true : false;
}


Area*
BALMLayout::AddItem(BLayoutItem* item, XTab* left, YTab* top, XTab* _right,
	YTab* _bottom)
{
	BReference<XTab> right = _right;
	if (right.Get() == NULL)
		right = AddXTab();
	BReference<YTab> bottom = _bottom;
	if (bottom.Get() == NULL)
		bottom = AddYTab();

	// Area is added int ItemAdded
	if (!BAbstractLayout::AddItem(-1, item))
		return NULL;
	Area* area = AreaFor(item);
	if (!area)
		return NULL;

	area->_Init(fSolver, left, top, right, bottom, fRowColumnManager);

	fRowColumnManager->AddArea(area);
	return area;
}


Area*
BALMLayout::AddItem(BLayoutItem* item, Row* row, Column* column)
{
	if (!BAbstractLayout::AddItem(-1, item))
		return NULL;
	Area* area = AreaFor(item);
	if (!area)
		return NULL;

	area->_Init(fSolver, row, column, fRowColumnManager);

	fRowColumnManager->AddArea(area);
	return area;
}


enum {
	kLeftBorderIndex = -2,
	kTopBorderIndex = -3,
	kRightBorderIndex = -4,
	kBottomBorderIndex = -5,
};


bool
BALMLayout::SaveLayout(BMessage* archive) const
{
	archive->MakeEmpty();

	archive->AddInt32("nXTabs", CountXTabs());
	archive->AddInt32("nYTabs", CountYTabs());

	XTabList xTabs = fXTabList;
	xTabs.RemoveItem(fLeft);
	xTabs.RemoveItem(fRight);
	YTabList yTabs = fYTabList;
	yTabs.RemoveItem(fTop);
	yTabs.RemoveItem(fBottom);
	
	int32 nAreas = CountAreas();
	for (int32 i = 0; i < nAreas; i++) {
		Area* area = AreaAt(i);
		if (area->Left() == fLeft)
			archive->AddInt32("left", kLeftBorderIndex);
		else
			archive->AddInt32("left", xTabs.IndexOf(area->Left()));
		if (area->Top() == fTop)
			archive->AddInt32("top", kTopBorderIndex);
		else
			archive->AddInt32("top", yTabs.IndexOf(area->Top()));
		if (area->Right() == fRight)
			archive->AddInt32("right", kRightBorderIndex);
		else
			archive->AddInt32("right", xTabs.IndexOf(area->Right()));
		if (area->Bottom() == fBottom)
			archive->AddInt32("bottom", kBottomBorderIndex);
		else
			archive->AddInt32("bottom", yTabs.IndexOf(area->Bottom()));
	}
	return true;
}


bool
BALMLayout::RestoreLayout(const BMessage* archive)
{
	int32 neededXTabs;
	int32 neededYTabs;
	if (archive->FindInt32("nXTabs", &neededXTabs) != B_OK)
		return false;
	if (archive->FindInt32("nYTabs", &neededYTabs) != B_OK)
		return false;
	// First store a reference to all needed tabs otherwise they might get lost
	// while editing the layout
	std::vector<BReference<XTab> > newXTabs;
	std::vector<BReference<YTab> > newYTabs;
	int32 existingXTabs = fXTabList.CountItems();
	for (int32 i = 0; i < neededXTabs; i++) {
		if (i < existingXTabs)
			newXTabs.push_back(BReference<XTab>(fXTabList.ItemAt(i)));
		else
			newXTabs.push_back(AddXTab());
	}
	int32 existingYTabs = fYTabList.CountItems();
	for (int32 i = 0; i < neededYTabs; i++) {
		if (i < existingYTabs)
			newYTabs.push_back(BReference<YTab>(fYTabList.ItemAt(i)));
		else
			newYTabs.push_back(AddYTab());
	}

	XTabList xTabs = fXTabList;
	xTabs.RemoveItem(fLeft);
	xTabs.RemoveItem(fRight);
	YTabList yTabs = fYTabList;
	yTabs.RemoveItem(fTop);
	yTabs.RemoveItem(fBottom);

	int32 nAreas = CountAreas();
	for (int32 i = 0; i < nAreas; i++) {
		Area* area = AreaAt(i);
		if (area == NULL)
			return false;
		int32 left = -1;
		if (archive->FindInt32("left", i, &left) != B_OK)
			break;
		int32 top = archive->FindInt32("top", i);
		int32 right = archive->FindInt32("right", i);
		int32 bottom = archive->FindInt32("bottom", i);

		XTab* leftTab = NULL;
		YTab* topTab = NULL;
		XTab* rightTab = NULL;
		YTab* bottomTab = NULL;

		if (left == kLeftBorderIndex)
			leftTab = fLeft;
		else
			leftTab = xTabs.ItemAt(left);
		if (top == kTopBorderIndex)
			topTab = fTop;
		else
			topTab = yTabs.ItemAt(top);
		if (right == kRightBorderIndex)
			rightTab = fRight;
		else
			rightTab = xTabs.ItemAt(right);
		if (bottom == kBottomBorderIndex)
			bottomTab = fBottom;
		else
			bottomTab = yTabs.ItemAt(bottom);
		if (leftTab == NULL || topTab == NULL || rightTab == NULL
			|| bottomTab == NULL)
			return false;

		area->SetLeft(leftTab);
		area->SetTop(topTab);
		area->SetRight(rightTab);
		area->SetBottom(bottomTab);
	}
	return true;
}


/**
 * Gets the left variable.
 */
XTab*
BALMLayout::Left() const
{
	return fLeft;
}


/**
 * Gets the right variable.
 */
XTab*
BALMLayout::Right() const
{
	return fRight;
}


/**
 * Gets the top variable.
 */
YTab*
BALMLayout::Top() const
{
	return fTop;
}


/**
 * Gets the bottom variable.
 */
YTab*
BALMLayout::Bottom() const
{
	return fBottom;
}


/**
 * Gets minimum size.
 */
BSize
BALMLayout::BaseMinSize() {
	if (fMinSize == kUnsetSize)
		fMinSize = _CalculateMinSize();
	return fMinSize;
}


/**
 * Gets maximum size.
 */
BSize
BALMLayout::BaseMaxSize()
{
	if (fMaxSize == kUnsetSize)
		fMaxSize = _CalculateMaxSize();
	return fMaxSize;
}


/**
 * Gets preferred size.
 */
BSize
BALMLayout::BasePreferredSize()
{
	if (fPreferredSize == kUnsetSize)
		fPreferredSize = _CalculatePreferredSize();
	return fPreferredSize;
}


/**
 * Gets the alignment.
 */
BAlignment
BALMLayout::BaseAlignment()
{
	BAlignment alignment;
	alignment.SetHorizontal(B_ALIGN_HORIZONTAL_CENTER);
	alignment.SetVertical(B_ALIGN_VERTICAL_CENTER);
	return alignment;
}


/**
 * Invalidates the layout.
 * Resets minimum/maximum/preferred size.
 */
void
BALMLayout::LayoutInvalidated(bool children)
{
	fMinSize = kUnsetSize;
	fMaxSize = kUnsetSize;
	fPreferredSize = kUnsetSize;
}


bool
BALMLayout::ItemAdded(BLayoutItem* item, int32 atIndex)
{
	item->SetLayoutData(new(std::nothrow) Area(item));
	return item->LayoutData() != NULL;
}


void
BALMLayout::ItemRemoved(BLayoutItem* item, int32 fromIndex)
{
	if (Area* area = AreaFor(item)) {
		fRowColumnManager->RemoveArea(area);
		item->SetLayoutData(NULL);
		delete area;
	}
}


/**
 * Calculate and set the layout.
 * If no layout specification is given, a specification is reverse engineered automatically.
 */
void
BALMLayout::DoLayout()
{
	_UpdateAreaConstraints();

	// Enforced absolute positions of Right and Bottom
	BRect area(LayoutArea());
	Right()->SetRange(area.right, area.right);
	Bottom()->SetRange(area.bottom, area.bottom);

	fSolver->Solve();

	// if new layout is infeasible, use previous layout
	if (fSolver->Result() == kInfeasible)
		return;

	if (fSolver->Result() != kOptimal) {
		fSolver->Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). ",
			fSolver->Result());
		printf("Saved specification in file failed-layout.txt\n");
	}

	// set the calculated positions and sizes for every area
	for (int32 i = 0; i < CountItems(); i++)
		AreaFor(ItemAt(i))->_DoLayout();
}


LinearSpec*
BALMLayout::Solver() const
{
	return const_cast<LinearSpec*>(fSolver);
}


void
BALMLayout::SetInsets(float left, float top, float right,
	float bottom)
{
	fLeftInset = BControlLook::ComposeSpacing(left);
	fTopInset = BControlLook::ComposeSpacing(top);
	fRightInset = BControlLook::ComposeSpacing(right);
	fBottomInset = BControlLook::ComposeSpacing(bottom);

	InvalidateLayout();
}


void
BALMLayout::SetInsets(float horizontal, float vertical)
{
	fLeftInset = BControlLook::ComposeSpacing(horizontal);
	fRightInset = fLeftInset;

	fTopInset = BControlLook::ComposeSpacing(vertical);
	fBottomInset = fTopInset;

	InvalidateLayout();
}


void
BALMLayout::SetInsets(float insets)
{
	fLeftInset = BControlLook::ComposeSpacing(insets);
	fRightInset = fLeftInset;
	fTopInset = fLeftInset;
	fBottomInset = fLeftInset;

	InvalidateLayout();
}


void
BALMLayout::GetInsets(float* left, float* top, float* right,
	float* bottom) const
{
	if (left)
		*left = fLeftInset;
	if (top)
		*top = fTopInset;
	if (right)
		*right = fRightInset;
	if (bottom)
		*bottom = fBottomInset;
}


void
BALMLayout::SetSpacing(float hSpacing, float vSpacing)
{
	fHSpacing = BControlLook::ComposeSpacing(hSpacing);
	fVSpacing = BControlLook::ComposeSpacing(vSpacing);
}


void
BALMLayout::GetSpacing(float *_hSpacing, float *_vSpacing) const
{
	if (_hSpacing)
		*_hSpacing = fHSpacing;
	if (_vSpacing)
		*_vSpacing = fVSpacing;
}


float
BALMLayout::InsetForTab(XTab* tab)
{
	if (tab == fLeft.Get())
		return fLeftInset;
	if (tab == fRight.Get())
		return fRightInset;
	return fHSpacing / 2;
}


float
BALMLayout::InsetForTab(YTab* tab)
{
	if (tab == fTop.Get())
		return fTopInset;
	if (tab == fBottom.Get())
		return fBottomInset;
	return fVSpacing / 2;
}


BLayoutItem*
BALMLayout::_LayoutItemToAdd(BView* view)
{
	if (view->GetLayout())
		return view->GetLayout();
	return new(std::nothrow) BViewLayoutItem(view);
}


/**
 * Caculates the miminum size.
 */
BSize
BALMLayout::_CalculateMinSize()
{
	_UpdateAreaConstraints();

	Right()->SetRange(0, 20000);
	Bottom()->SetRange(0, 20000);

	return fSolver->MinSize(Right(), Bottom());
}


/**
 * Caculates the maximum size.
 */
BSize
BALMLayout::_CalculateMaxSize()
{
	_UpdateAreaConstraints();

	Right()->SetRange(0, 20000);
	Bottom()->SetRange(0, 20000);

	return fSolver->MaxSize(Right(), Bottom());
}


/**
 * Caculates the preferred size.
 */
BSize
BALMLayout::_CalculatePreferredSize()
{
	_UpdateAreaConstraints();

	Right()->SetRange(0, 20000);
	Bottom()->SetRange(0, 20000);

	fSolver->Solve();
	if (fSolver->Result() != kOptimal) {
		fSolver->Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). "
			"Saved specification in file failed-layout.txt", fSolver->Result());
	}

	return BSize(Right()->Value() - Left()->Value(),
		Bottom()->Value() - Top()->Value());
}


void
BALMLayout::_UpdateAreaConstraints()
{
	for (int i = 0; i < CountItems(); i++)
		AreaFor(ItemAt(i))->InvalidateSizeConstraints();
	fRowColumnManager->UpdateConstraints();
}

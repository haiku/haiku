/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "ALMLayout.h"


#include <stdio.h>
#include <vector>

#include <AutoDeleter.h>
#include <ControlLook.h>

#include "RowColumnManager.h"
#include "SharedSolver.h"
#include "ViewLayoutItem.h"


using BPrivate::AutoDeleter;
using namespace LinearProgramming;


const BSize kUnsetSize(B_SIZE_UNSET, B_SIZE_UNSET);


namespace {

const char* kSolverField = "BALMLayout:solver";
const char* kBadLayoutPolicyField = "BALMLayout:policy";
const char* kXTabsField = "BALMLayout:xtabs";
const char* kYTabsField = "BALMLayout:ytabs";
const char* kMyTabsField = "BALMLayout:tabs";
const char* kInsetsField = "BALMLayout:insets";
const char* kSpacingField = "BALMLayout:spacing";

const char* kTabsField = "BALMLayout:item:tabs";
const char* kItemAspectRatio = "BALMLayout:item:aspect";
const char* kItemPenalties = "BALMLayout:item:penalties";
const char* kItemInsets = "BALMLayout:item:insets";

int CompareXTabFunc(const XTab* tab1, const XTab* tab2);
int CompareYTabFunc(const YTab* tab1, const YTab* tab2);

};


namespace BALM {


template <class T>
struct BALMLayout::TabAddTransaction {
	~TabAddTransaction()
	{
		if (fTab)
			fLayout->_RemoveSelfFromTab(fTab);
		if (fIndex > 0)
			_TabList()->RemoveItemAt(fIndex);
	}

	TabAddTransaction(BALMLayout* layout)
		:
		fTab(NULL),
		fLayout(layout),
		fIndex(-1)
	{
	}

	bool AttempAdd(T* tab)
	{
		if (fLayout->_HasTabInLayout(tab))
			return true;
		if (!fLayout->_AddedTab(tab))
			return false;
		fTab = tab;

		BObjectList<T>* tabList = _TabList();
		int32 index = tabList->CountItems();
		if (!tabList->AddItem(tab, index))
			return false;
		fIndex = index;
		return true;
	}

	void Commit()
	{
		fTab = NULL;
		fIndex = -1;
	}

private:
	BObjectList<T>* _TabList();

	T*				fTab;
	BALMLayout*		fLayout;
	int32			fIndex;
};


template <>
BObjectList<XTab>*
BALMLayout::TabAddTransaction<XTab>::_TabList()
{
	return &fLayout->fXTabList;
}


template <>
BObjectList<YTab>*
BALMLayout::TabAddTransaction<YTab>::_TabList()
{
	return &fLayout->fYTabList;
}


}; // end namespace BALM


BALM::BALMLayout::BadLayoutPolicy::BadLayoutPolicy()
{
}


BALM::BALMLayout::BadLayoutPolicy::BadLayoutPolicy(BMessage* archive)
	:
	BArchivable(archive)
{
}


BALM::BALMLayout::BadLayoutPolicy::~BadLayoutPolicy()
{
}


BALM::BALMLayout::DefaultPolicy::DefaultPolicy()
{
}


BALM::BALMLayout::DefaultPolicy::DefaultPolicy(BMessage* archive)
	:
	BadLayoutPolicy(archive)
{
}


BALM::BALMLayout::DefaultPolicy::~DefaultPolicy()
{
}


bool
BALM::BALMLayout::DefaultPolicy::OnBadLayout(BALMLayout* layout,
	ResultType result, BLayoutContext* context)
{
	if (!context)
		return true;

	if (result == kInfeasible) {
		printf("BALMLayout failed to solve your layout!\n");
		return false;
	} else
		return true;
}


status_t
BALM::BALMLayout::DefaultPolicy::Archive(BMessage* archive, bool deep) const
{
	return BadLayoutPolicy::Archive(archive, deep);
}


BArchivable*
BALM::BALMLayout::DefaultPolicy::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BALM::BALMLayout::DefaultPolicy"))
		return new DefaultPolicy(archive);
	return NULL;
}


class BALMLayout::BALMLayoutSpecListener 
	: public LinearProgramming::SpecificationListener {
public:
	BALMLayoutSpecListener(BALMLayout* layout)
		:
		fLayout(layout)
	{		
	}

	void ConstraintRemoved(Constraint* constraint)
	{
		fLayout->fConstraints.RemoveItem(constraint);
	}

private:
			BALMLayout*			fLayout;
};


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
	fHSpacing(BControlLook::ComposeSpacing(hSpacing)),
	fVSpacing(BControlLook::ComposeSpacing(vSpacing)),
	fXTabsSorted(false),
	fYTabsSorted(false),
	fBadLayoutPolicy(new DefaultPolicy())
{
	_SetSolver(friendLayout ? friendLayout->fSolver : new SharedSolver());

	fSpecListener = new BALMLayoutSpecListener(this);
	Solver()->AddListener(fSpecListener);

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


BALMLayout::BALMLayout(BMessage* archive)
	:
	BAbstractLayout(BUnarchiver::PrepareArchive(archive)),
	fSolver(NULL),
	fLeft(NULL),
	fRight(NULL),
	fTop(NULL),
	fBottom(NULL),
	fMinSize(kUnsetSize),
	fMaxSize(kUnsetSize),
	fPreferredSize(kUnsetSize),
	fXTabsSorted(false),
	fYTabsSorted(false),
	fBadLayoutPolicy(new DefaultPolicy())
{
	BUnarchiver unarchiver(archive);

	BRect insets;
	status_t err = archive->FindRect(kInsetsField, &insets);
	if (err != B_OK) {
		unarchiver.Finish(err);
		return;
	}

	fLeftInset = insets.left;
	fRightInset = insets.right;
	fTopInset = insets.top;
	fBottomInset = insets.bottom;


	BSize spacing;
	err = archive->FindSize(kSpacingField, &spacing);
	if (err != B_OK) {
		unarchiver.Finish(err);
		return;
	}

	fHSpacing = spacing.width;
	fVSpacing = spacing.height;

	int32 tabCount = 0;
	archive->GetInfo(kXTabsField, NULL, &tabCount);
	for (int32 i = 0; i < tabCount && err == B_OK; i++)
		err = unarchiver.EnsureUnarchived(kXTabsField, i);

	archive->GetInfo(kYTabsField, NULL, &tabCount);
	for (int32 i = 0; i < tabCount && err == B_OK; i++)
		err = unarchiver.EnsureUnarchived(kYTabsField, i);

	if (err == B_OK && archive->GetInfo(kBadLayoutPolicyField, NULL) == B_OK)
		err = unarchiver.EnsureUnarchived(kBadLayoutPolicyField);

	if (err == B_OK)
		err = unarchiver.EnsureUnarchived(kSolverField);

	unarchiver.Finish(err);

	fSpecListener = new BALMLayoutSpecListener(this);
	Solver()->AddListener(fSpecListener);
}


BALMLayout::~BALMLayout()
{
	Solver()->RemoveListener(fSpecListener);
	delete fSpecListener;

	delete fRowColumnManager;
	delete fBadLayoutPolicy;

	for (int32 i = 0; i < fConstraints.CountItems(); i++)
		Solver()->RemoveConstraint(fConstraints.ItemAt(i), true);

	if (fSolver) {
		fSolver->LayoutLeaving(this);
		fSolver->ReleaseReference();
	}
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
	if (!Solver()->AddVariable(tab))
		return NULL;

	fXTabList.AddItem(tab);
	if (!tab->AddedToLayout(this)) {
		fXTabList.RemoveItem(tab);
		return NULL;
	}
	fXTabsSorted = false;
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
	if (!Solver()->AddVariable(tab))
		return NULL;

	fYTabList.AddItem(tab);
	if (!tab->AddedToLayout(this)) {
		fYTabList.RemoveItem(tab);
		return NULL;
	}
	fYTabsSorted = false;
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
BALMLayout::XTabAt(int32 index, bool ordered)
{
	if (ordered && !fXTabsSorted) {
		Layout();
		fXTabList.SortItems(CompareXTabFunc);
		fXTabsSorted = true;
	}
	return fXTabList.ItemAt(index);
}


XTab*
BALMLayout::XTabAt(int32 index) const
{
	return fXTabList.ItemAt(index);
}


YTab*
BALMLayout::YTabAt(int32 index, bool ordered)
{
	if (ordered && !fYTabsSorted) {
		Layout();
		fYTabList.SortItems(CompareYTabFunc);
		fYTabsSorted = true;
	}
	return fYTabList.ItemAt(index);
}


YTab*
BALMLayout::YTabAt(int32 index) const 
{
	return fYTabList.ItemAt(index);
}


const XTabList
BALMLayout::GetXTabs() const
{
	return fXTabList;
}


const YTabList
BALMLayout::GetYTabs() const
{
	return fYTabList;
}


int32
BALMLayout::IndexOf(XTab* tab, bool ordered)
{
	if (ordered && !fXTabsSorted) {
		Layout();
		fXTabList.SortItems(CompareXTabFunc);
		fXTabsSorted = true;
	}
	return fXTabList.IndexOf(tab);
}


int32
BALMLayout::IndexOf(YTab* tab, bool ordered)
{
	if (ordered && !fYTabsSorted) {
		Layout();
		fYTabList.SortItems(CompareYTabFunc);
		fYTabsSorted = true;
	}
	return fYTabList.IndexOf(tab);
}


int32
BALMLayout::CountConstraints() const
{
	return fConstraints.CountItems();
}


Constraint*
BALMLayout::ConstraintAt(int32 index) const
{
	return fConstraints.ItemAt(index);
}


bool
BALMLayout::AddConstraint(Constraint* constraint)
{
	fConstraints.AddItem(constraint);
	return Solver()->AddConstraint(constraint);
}


bool
BALMLayout::RemoveConstraint(Constraint* constraint,
	bool deleteConstraint)
{
	if (!fConstraints.RemoveItem(constraint))
		return false;
	return Solver()->RemoveConstraint(constraint, deleteConstraint);
}


Constraint*
BALMLayout::AddConstraint(double coeff1, Variable* var1, OperatorType op,
	double rightSide, double penaltyNeg, double penaltyPos)
{
	Constraint* constraint = Solver()->AddConstraint(coeff1, var1, op,
		rightSide, penaltyNeg, penaltyPos);
	fConstraints.AddItem(constraint);
	return constraint;
}


Constraint*
BALMLayout::AddConstraint(double coeff1, Variable* var1, double coeff2,
	Variable* var2,	OperatorType op, double rightSide, double penaltyNeg,
	double penaltyPos)
{
	Constraint* constraint = Solver()->AddConstraint(coeff1, var1, coeff2, var2,
		op, rightSide, penaltyNeg, penaltyPos);
	fConstraints.AddItem(constraint);
	return constraint;
}

Constraint*
BALMLayout::AddConstraint(double coeff1, Variable* var1, double coeff2,
	Variable* var2, double coeff3, Variable* var3, OperatorType op,
	double rightSide, double penaltyNeg, double penaltyPos)
{
	Constraint* constraint = Solver()->AddConstraint(coeff1, var1, coeff2, var2,
		coeff3, var3, op, rightSide, penaltyNeg, penaltyPos);
	fConstraints.AddItem(constraint);
	return constraint;
}


Constraint*
BALMLayout::AddConstraint(double coeff1, Variable* var1, double coeff2,
	Variable* var2, double coeff3, Variable* var3, double coeff4,
	Variable* var4, OperatorType op, double rightSide, double penaltyNeg,
	double penaltyPos)
{
	Constraint* constraint = Solver()->AddConstraint(coeff1, var1, coeff2, var2,
		coeff3, var3, coeff4, var4, op, rightSide, penaltyNeg, penaltyPos);
	fConstraints.AddItem(constraint);
	return constraint;
}


namespace {


int
CompareXTabFunc(const XTab* tab1, const XTab* tab2)
{
	if (tab1->Value() < tab2->Value())
		return -1;
	else if (tab1->Value() == tab2->Value())
		return 0;
	return 1;
}


int
CompareYTabFunc(const YTab* tab1, const YTab* tab2)
{
	if (tab1->Value() < tab2->Value())
		return -1;
	else if (tab1->Value() == tab2->Value())
		return 0;
	return 1;
}


}; // end anonymous namespace


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
	return new(std::nothrow) Row(Solver(), top, bottom);
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
	return new(std::nothrow) Column(Solver(), left, right);
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
BALMLayout::AddItem(BLayoutItem* item, XTab* _left, YTab* _top, XTab* _right,
	YTab* _bottom)
{
	if ((_left && !_left->IsSuitableFor(this))
			|| (_top && !_top->IsSuitableFor(this))
			|| (_right && !_right->IsSuitableFor(this))
			|| (_bottom && !_bottom->IsSuitableFor(this)))
		debugger("Tab added to unfriendly layout!");

	BReference<XTab> right = _right;
	if (right.Get() == NULL)
		right = AddXTab();
	BReference<YTab> bottom = _bottom;
	if (bottom.Get() == NULL)
		bottom = AddYTab();
	BReference<XTab> left = _left;
	if (left.Get() == NULL)
		left = AddXTab();
	BReference<YTab> top = _top;
	if (top.Get() == NULL)
		top = AddYTab();

	TabAddTransaction<XTab> leftTabAdd(this);
	if (!leftTabAdd.AttempAdd(left))
		return NULL;

	TabAddTransaction<YTab> topTabAdd(this);
	if (!topTabAdd.AttempAdd(top))
		return NULL;

	TabAddTransaction<XTab> rightTabAdd(this);
	if (!rightTabAdd.AttempAdd(right))
		return NULL;

	TabAddTransaction<YTab> bottomTabAdd(this);
	if (!bottomTabAdd.AttempAdd(bottom))
		return NULL;

	// Area is added in ItemAdded
	if (!BAbstractLayout::AddItem(-1, item))
		return NULL;
	Area* area = AreaFor(item);
	if (!area) {
		RemoveItem(item);
		return NULL;
	}

	fSolver->Invalidate(true);
	area->_Init(Solver(), left, top, right, bottom, fRowColumnManager);
	fRowColumnManager->AddArea(area);

	leftTabAdd.Commit();
	topTabAdd.Commit();
	rightTabAdd.Commit();
	bottomTabAdd.Commit();
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

	fSolver->Invalidate(true);
	area->_Init(Solver(), row, column, fRowColumnManager);

	fRowColumnManager->AddArea(area);
	return area;
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


void
BALMLayout::SetBadLayoutPolicy(BadLayoutPolicy* policy)
{
	if (fBadLayoutPolicy != policy)
		delete fBadLayoutPolicy;
	if (policy == NULL)
		policy = new DefaultPolicy();
	fBadLayoutPolicy = policy;
}


struct BALMLayout::BadLayoutPolicy*
BALMLayout::GetBadLayoutPolicy() const
{
	return fBadLayoutPolicy;
}


/**
 * Gets minimum size.
 */
BSize
BALMLayout::BaseMinSize()
{
	ResultType result = fSolver->ValidateMinSize();
	if (result != kOptimal && result != kUnbounded)
		fBadLayoutPolicy->OnBadLayout(this, result, NULL);
	return fMinSize;
}


/**
 * Gets maximum size.
 */
BSize
BALMLayout::BaseMaxSize()
{
	ResultType result = fSolver->ValidateMaxSize();
	if (result != kOptimal && result != kUnbounded)
		fBadLayoutPolicy->OnBadLayout(this, result, NULL);
	return fMaxSize;
}


/**
 * Gets preferred size.
 */
BSize
BALMLayout::BasePreferredSize()
{
	ResultType result = fSolver->ValidatePreferredSize();
	if (result != kOptimal)
		fBadLayoutPolicy->OnBadLayout(this, result, NULL);

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


status_t
BALMLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BAbstractLayout::Archive(into, deep);
	if (err != B_OK)
		return archiver.Finish(err);

	BRect insets(fLeftInset, fTopInset, fRightInset, fBottomInset);
	err = into->AddRect(kInsetsField, insets);
	if (err != B_OK)
		return archiver.Finish(err);

	BSize spacing(fHSpacing, fVSpacing);
	err = into->AddSize(kSpacingField, spacing);
	if (err != B_OK)
		return archiver.Finish(err);

	if (deep) {
		for (int32 i = CountXTabs() - 1; i >= 0 && err == B_OK; i--)
			err = archiver.AddArchivable(kXTabsField, XTabAt(i));

		for (int32 i = CountYTabs() - 1; i >= 0 && err == B_OK; i--)
			err = archiver.AddArchivable(kYTabsField, YTabAt(i));

		err = archiver.AddArchivable(kBadLayoutPolicyField, fBadLayoutPolicy);
	}

	if (err == B_OK)
		err = archiver.AddArchivable(kSolverField, fSolver);

	if (err == B_OK)
		err = archiver.AddArchivable(kMyTabsField, fLeft);
	if (err == B_OK)
		err = archiver.AddArchivable(kMyTabsField, fTop);
	if (err == B_OK)
		err = archiver.AddArchivable(kMyTabsField, fRight);
	if (err == B_OK)
		err = archiver.AddArchivable(kMyTabsField, fBottom);

	return archiver.Finish(err);
}


BArchivable*
BALMLayout::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BALM::BALMLayout"))
		return new BALMLayout(from);
	return NULL;
}


status_t
BALMLayout::ItemArchived(BMessage* into, BLayoutItem* item, int32 index) const
{
	BArchiver archiver(into);
	status_t err = BAbstractLayout::ItemArchived(into, item, index);
	if (err != B_OK)
		return err;

	Area* area = AreaFor(item);
	err = into->AddSize(kItemPenalties, area->fShrinkPenalties);
	if (err == B_OK)
		err = into->AddSize(kItemPenalties, area->fGrowPenalties);
	if (err == B_OK)
		err = into->AddSize(kItemInsets, area->fLeftTopInset);
	if (err == B_OK)
		err = into->AddSize(kItemInsets, area->fRightBottomInset);
	if (err == B_OK)
		err = into->AddDouble(kItemAspectRatio, area->fContentAspectRatio);

	err = archiver.AddArchivable(kTabsField, area->Left());
	if (err == B_OK)
		archiver.AddArchivable(kTabsField, area->Top());
	if (err == B_OK)
		archiver.AddArchivable(kTabsField, area->Right());
	if (err == B_OK)
		archiver.AddArchivable(kTabsField, area->Bottom());

	return err;
}


status_t
BALMLayout::ItemUnarchived(const BMessage* from, BLayoutItem* item,
	int32 index)
{
	BUnarchiver unarchiver(from);
	status_t err = BAbstractLayout::ItemUnarchived(from, item, index);
	if (err != B_OK)
		return err;

	Area* area = AreaFor(item);
	XTab* left;
	XTab* right;
	YTab* bottom;
	YTab* top;
	err = unarchiver.FindObject(kTabsField, index * 4, left);
	if (err == B_OK)
		err = unarchiver.FindObject(kTabsField, index * 4 + 1, top);
	if (err == B_OK)
		err = unarchiver.FindObject(kTabsField, index * 4 + 2, right);
	if (err == B_OK)
		err = unarchiver.FindObject(kTabsField, index * 4 + 3, bottom);
	
	if (err != B_OK)
		return err;

	area->_Init(Solver(), left, top, right, bottom, fRowColumnManager);
	fRowColumnManager->AddArea(area);

	err = from->FindSize(kItemPenalties, index * 2, &area->fShrinkPenalties);
	if (err != B_OK)
		return err;

	err = from->FindSize(kItemPenalties, index * 2 + 1, &area->fGrowPenalties);
	if (err != B_OK)
		return err;

	err = from->FindSize(kItemInsets, index * 2, &area->fLeftTopInset);
	if (err != B_OK)
		return err;

	err = from->FindSize(kItemInsets, index * 2 + 1, &area->fRightBottomInset);

	if (err == B_OK) {
		double contentAspectRatio;
		err = from->FindDouble(kItemAspectRatio, index, &contentAspectRatio);
		if (err == B_OK)
			area->SetContentAspectRatio(contentAspectRatio);
	}

	return err;
}


status_t
BALMLayout::AllUnarchived(const BMessage* archive)
{
	BUnarchiver unarchiver(archive);

	SharedSolver* solver;
	status_t err = unarchiver.FindObject(kSolverField, solver);

	if (err != B_OK)
		return err;

	_SetSolver(solver);

	if (archive->GetInfo(kBadLayoutPolicyField, NULL) == B_OK) {
		BadLayoutPolicy* policy;
		err = unarchiver.FindObject(kBadLayoutPolicyField, policy);
		if (err == B_OK)
			SetBadLayoutPolicy(policy);
	}

	LinearSpec* spec = Solver();
	int32 tabCount = 0;
	archive->GetInfo(kXTabsField, NULL, &tabCount);
	for (int32 i = 0; i < tabCount && err == B_OK; i++) {
		XTab* tab;
		err = unarchiver.FindObject(kXTabsField, i,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, tab);
		spec->AddVariable(tab);
		TabAddTransaction<XTab> adder(this);
		if (adder.AttempAdd(tab))
			adder.Commit();
		else
			err = B_NO_MEMORY;
	}

	archive->GetInfo(kYTabsField, NULL, &tabCount);
	for (int32 i = 0; i < tabCount; i++) {
		YTab* tab;
		unarchiver.FindObject(kYTabsField, i,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, tab);
		spec->AddVariable(tab);
		TabAddTransaction<YTab> adder(this);
		if (adder.AttempAdd(tab))
			adder.Commit();
		else
			err = B_NO_MEMORY;
	}


	if (err == B_OK) {
		XTab* leftTab = NULL;
		err = unarchiver.FindObject(kMyTabsField, 0, leftTab);
		fLeft = leftTab;
	}

	if (err == B_OK) {
		YTab* topTab = NULL;
		err = unarchiver.FindObject(kMyTabsField, 1, topTab);
		fTop = topTab;
	}

	if (err == B_OK) {
		XTab* rightTab = NULL;
		err = unarchiver.FindObject(kMyTabsField, 2, rightTab);
		fRight = rightTab;
	}

	if (err == B_OK) {
		YTab* bottomTab = NULL;
		err = unarchiver.FindObject(kMyTabsField, 3, bottomTab);
		fBottom = bottomTab;
	}

	if (err == B_OK) {
		fLeft->SetRange(0, 0);
		fTop->SetRange(0, 0);

   		err = BAbstractLayout::AllUnarchived(archive);
	}
	return err;
}


status_t
BALMLayout::AllArchived(BMessage* archive) const
{
	status_t err = BAbstractLayout::AllArchived(archive);

	return err;
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
	fXTabsSorted = false;
	fYTabsSorted = false;

	if (fSolver)
		fSolver->Invalidate(children);
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
	BLayoutContext* context = LayoutContext();
	ResultType result = fSolver->ValidateLayout(context);
	if (result != kOptimal
			&& !fBadLayoutPolicy->OnBadLayout(this, result, context)) {
		return;
	}

	// set the calculated positions and sizes for every area
	for (int32 i = 0; i < CountItems(); i++)
		AreaFor(ItemAt(i))->_DoLayout(LayoutArea().LeftTop());

	fXTabsSorted = false;
	fYTabsSorted = false;
}


LinearSpec*
BALMLayout::Solver() const
{
	return fSolver->Solver();
}


ResultType
BALMLayout::ValidateLayout()
{
	// we explicitly recaluclate the layout so set the invalidate flag first
	fSolver->Invalidate(true);

	BLayoutContext* context = LayoutContext();
	return fSolver->ValidateLayout(context);
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
BALMLayout::InsetForTab(XTab* tab) const
{
	if (tab == fLeft.Get())
		return fLeftInset;
	if (tab == fRight.Get())
		return fRightInset;
	return fHSpacing / 2;
}


float
BALMLayout::InsetForTab(YTab* tab) const
{
	if (tab == fTop.Get())
		return fTopInset;
	if (tab == fBottom.Get())
		return fBottomInset;
	return fVSpacing / 2;
}


void
BALMLayout::UpdateConstraints(BLayoutContext* context)
{
	for (int i = 0; i < CountItems(); i++)
		AreaFor(ItemAt(i))->InvalidateSizeConstraints();
	fRowColumnManager->UpdateConstraints();
}


void BALMLayout::_RemoveSelfFromTab(XTab* tab) { tab->LayoutLeaving(this); }
void BALMLayout::_RemoveSelfFromTab(YTab* tab) { tab->LayoutLeaving(this); }

bool BALMLayout::_HasTabInLayout(XTab* tab) { return tab->IsInLayout(this); }
bool BALMLayout::_HasTabInLayout(YTab* tab) { return tab->IsInLayout(this); }

bool BALMLayout::_AddedTab(XTab* tab) { return tab->AddedToLayout(this); }
bool BALMLayout::_AddedTab(YTab* tab) { return tab->AddedToLayout(this); }


BLayoutItem*
BALMLayout::_LayoutItemToAdd(BView* view)
{
	if (view->GetLayout())
		return view->GetLayout();
	return new(std::nothrow) BViewLayoutItem(view);
}


void
BALMLayout::_SetSolver(SharedSolver* solver)
{
	fSolver = solver;
	fSolver->AcquireReference();
	fSolver->RegisterLayout(this);
	fRowColumnManager = new RowColumnManager(Solver());
}


status_t
BALMLayout::Perform(perform_code d, void* arg)
{
	return BAbstractLayout::Perform(d, arg);
}


void BALMLayout::_ReservedALMLayout1() {}
void BALMLayout::_ReservedALMLayout2() {}
void BALMLayout::_ReservedALMLayout3() {}
void BALMLayout::_ReservedALMLayout4() {}
void BALMLayout::_ReservedALMLayout5() {}
void BALMLayout::_ReservedALMLayout6() {}
void BALMLayout::_ReservedALMLayout7() {}
void BALMLayout::_ReservedALMLayout8() {}
void BALMLayout::_ReservedALMLayout9() {}
void BALMLayout::_ReservedALMLayout10() {}


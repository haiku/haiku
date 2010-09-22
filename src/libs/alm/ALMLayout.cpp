/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#include "ALMLayout.h"

#include <math.h>		// for floor
#include <new>

#include "Area.h"
#include "Column.h"
#include "ResultType.h"
#include "Row.h"
#include "XTab.h"
#include "YTab.h"


const BSize kUnsetSize(B_SIZE_UNSET, B_SIZE_UNSET);
const BSize kMinSize(0, 0);
const BSize kMaxSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);


/**
 * Constructor.
 * Creates new layout engine.
 */
BALMLayout::BALMLayout()
	:
	BAbstractLayout()
{
	fLeft = new XTab(&fSolver);
	fRight = new XTab(&fSolver);
	fTop = new YTab(&fSolver);
	fBottom = new YTab(&fSolver);

	// the Left tab is always at x-position 0, and the Top tab is always at y-position 0
	fLeft->SetRange(0, 0);
	fTop->SetRange(0, 0);

	// cached layout values
	// need to be invalidated whenever the layout specification is changed
	fMinSize = kUnsetSize;
	fMaxSize = kUnsetSize;
	fPreferredSize = kUnsetSize;

	fPerformancePath = NULL;
}


BALMLayout::~BALMLayout()
{
	
}


/**
 * Adds a new x-tab to the specification.
 *
 * @return the new x-tab
 */
XTab*
BALMLayout::AddXTab()
{
	return new XTab(&fSolver);
}


/**
 * Adds a new y-tab to the specification.
 *
 * @return the new y-tab
 */
YTab*
BALMLayout::AddYTab()
{
	return new YTab(&fSolver);
}


/**
 * Adds a new row to the specification.
 *
 * @return the new row
 */
Row*
BALMLayout::AddRow()
{
	return new Row(&fSolver);
}


/**
 * Adds a new row to the specification that is glued to the given y-tabs.
 *
 * @param top
 * @param bottom
 * @return the new row
 */
Row*
BALMLayout::AddRow(YTab* top, YTab* bottom)
{
	Row* row = new Row(&fSolver);
	if (top != NULL)
		row->Constraints()->AddItem(row->Top()->IsEqual(top));
	if (bottom != NULL)
		row->Constraints()->AddItem(row->Bottom()->IsEqual(bottom));
	return row;
}


/**
 * Adds a new column to the specification.
 *
 * @return the new column
 */
Column*
BALMLayout::AddColumn()
{
	return new Column(&fSolver);
}


/**
 * Adds a new column to the specification that is glued to the given x-tabs.
 *
 * @param left
 * @param right
 * @return the new column
 */
Column*
BALMLayout::AddColumn(XTab* left, XTab* right)
{
	Column* column = new Column(&fSolver);
	if (left != NULL)
		column->Constraints()->AddItem(column->Left()->IsEqual(left));
	if (right != NULL)
		column->Constraints()->AddItem(column->Right()->IsEqual(right));
	return column;
}


/**
 * Adds a new area to the specification, setting only the necessary minimum size constraints.
 *
 * @param left				left border
 * @param top				top border
 * @param right			right border
 * @param bottom			bottom border
 * @param content			the control which is the area content
 * @param minContentSize	minimum content size
 * @return the new area
 */
Area*
BALMLayout::AddArea(XTab* left, YTab* top, XTab* right, YTab* bottom,
	BView* content, BSize minContentSize)
{
	BLayoutItem* item = AddView(content);
	Area* area = _AreaForItem(item);
	if (!area)
		return NULL;

	area->Init(&fSolver, left, top, right, bottom, content,
		minContentSize);
	return area;
}


/**
 * Adds a new area to the specification, setting only the necessary minimum size constraints.
 *
 * @param row				the row that defines the top and bottom border
 * @param column			the column that defines the left and right border
 * @param content			the control which is the area content
 * @param minContentSize	minimum content size
 * @return the new area
 */
Area*
BALMLayout::AddArea(Row* row, Column* column, BView* content,
	BSize minContentSize)
{
	BLayoutItem* item = AddView(content);
	Area* area = _AreaForItem(item);
	if (!area)
		return NULL;

	area->Init(&fSolver, row, column, content, minContentSize);
	return area;
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
BALMLayout::AddArea(XTab* left, YTab* top, XTab* right, YTab* bottom,
	BView* content)
{
	BLayoutItem* item = AddView(content);
	Area* area = _AreaForItem(item);
	if (!area)
		return NULL;

	area->Init(&fSolver, left, top, right, bottom, content,
		BSize(0, 0));
	area->SetDefaultBehavior();
	area->SetAutoPreferredContentSize(false);
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
BALMLayout::AddArea(Row* row, Column* column, BView* content)
{
	BLayoutItem* item = AddView(content);
	Area* area = _AreaForItem(item);
	if (!area)
		return NULL;

	area->Init(&fSolver, row, column, content, BSize(0, 0));
	area->SetDefaultBehavior();
	area->SetAutoPreferredContentSize(false);
	return area;
}


/**
 * Finds the area that contains the given control.
 *
 * @param control	the control to look for
 * @return the area that contains the control
 */
Area*
BALMLayout::AreaOf(BView* control)
{
	return _AreaForItem(ItemAt(IndexOfView(control)));
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
BALMLayout::InvalidateLayout(bool children)
{
	BLayout::InvalidateLayout(children);
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
	if (Area* area = _AreaForItem(item)) {
		item->SetLayoutData(NULL);
		delete area;
	}
}


/**
 * Calculate and set the layout.
 * If no layout specification is given, a specification is reverse engineered automatically.
 */
void
BALMLayout::DerivedLayoutItems()
{
	_UpdateAreaConstraints();

	// Enforced absolute positions of Right and Bottom
	BRect area(LayoutArea());
	Right()->SetRange(area.right, area.right);
	Bottom()->SetRange(area.bottom, area.bottom);

	_SolveLayout();

	// if new layout is infasible, use previous layout
	if (fSolver.Result() == INFEASIBLE)
		return;

	if (fSolver.Result() != OPTIMAL) {
		fSolver.Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). ",
			fSolver.Result());
		printf("Saved specification in file failed-layout.txt\n");
	}

	// set the calculated positions and sizes for every area
	for (int32 i = 0; i < CountItems(); i++)
		_AreaForItem(ItemAt(i))->DoLayout();
}


/**
 * Gets the path of the performance log file.
 *
 * @return the path of the performance log file
 */
char*
BALMLayout::PerformancePath() const
{
	return fPerformancePath;
}


/**
 * Sets the path of the performance log file.
 *
 * @param path	the path of the performance log file
 */
void
BALMLayout::SetPerformancePath(char* path)
{
	fPerformancePath = path;
}


LinearSpec*
BALMLayout::Solver()
{
	return &fSolver;
}


void
BALMLayout::_SolveLayout()
{
	// if autoPreferredContentSize is set on an area,
	// readjust its preferredContentSize and penalties settings
	for (int32 i = 0; i < CountItems(); i++) {
		Area* currentArea = _AreaForItem(ItemAt(i));
		if (currentArea && currentArea->AutoPreferredContentSize())
			currentArea->SetDefaultBehavior();
	}

	// Try to solve the layout until the result is OPTIMAL or INFEASIBLE,
	// maximally 15 tries sometimes the solving algorithm encounters numerical
	// problems (NUMFAILURE), and repeating the solving often helps to overcome
	// them.
	BFile* file = NULL;
	if (fPerformancePath != NULL) {
		file = new BFile(fPerformancePath,
			B_READ_WRITE | B_CREATE_FILE | B_OPEN_AT_END);
	}

	ResultType result;
	for (int32 tries = 0; tries < 15; tries++) {
		result = fSolver.Solve();
		if (fPerformancePath != NULL) {
			/*char buffer [100];
			file->Write(buffer, sprintf(buffer, "%d\t%fms\t#vars=%ld\t"
				"#constraints=%ld\n", result, fSolver.SolvingTime(),
				fSolver.Variables()->CountItems(),
				fSolver.Constraints()->CountItems()));*/
		}
		if (result == OPTIMAL || result == INFEASIBLE)
			break;
	}
	delete file;
}


/**
 * Caculates the miminum size.
 */
BSize
BALMLayout::_CalculateMinSize()
{
	_UpdateAreaConstraints();

	SummandList* oldObjFunction = fSolver.ObjFunction();
	SummandList* newObjFunction = new SummandList(2);
	newObjFunction->AddItem(new Summand(1.0, fRight));
	newObjFunction->AddItem(new Summand(1.0, fBottom));
	fSolver.SetObjFunction(newObjFunction);
	_SolveLayout();
	fSolver.SetObjFunction(oldObjFunction);
	fSolver.UpdateObjFunction();
	delete newObjFunction->ItemAt(0);
	delete newObjFunction->ItemAt(1);
	delete newObjFunction;

	if (fSolver.Result() == UNBOUNDED)
		return kMinSize;
	if (fSolver.Result() != OPTIMAL) {
		fSolver.Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). "
			"Saved specification in file failed-layout.txt", fSolver.Result());
	}

	return BSize(Right()->Value() - Left()->Value(),
		Bottom()->Value() - Top()->Value());
}


/**
 * Caculates the maximum size.
 */
BSize
BALMLayout::_CalculateMaxSize()
{
	_UpdateAreaConstraints();

	SummandList* oldObjFunction = fSolver.ObjFunction();
	SummandList* newObjFunction = new SummandList(2);
	newObjFunction->AddItem(new Summand(-1.0, fRight));
	newObjFunction->AddItem(new Summand(-1.0, fBottom));
	fSolver.SetObjFunction(newObjFunction);
	_SolveLayout();
	fSolver.SetObjFunction(oldObjFunction);
	fSolver.UpdateObjFunction();
	delete newObjFunction->ItemAt(0);
	delete newObjFunction->ItemAt(1);
	delete newObjFunction;

	if (fSolver.Result() == UNBOUNDED)
		return kMaxSize;
	if (fSolver.Result() != OPTIMAL) {
		fSolver.Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). "
			"Saved specification in file failed-layout.txt", fSolver.Result());
	}

	return BSize(Right()->Value() - Left()->Value(),
		Bottom()->Value() - Top()->Value());
}


/**
 * Caculates the preferred size.
 */
BSize
BALMLayout::_CalculatePreferredSize()
{
	_UpdateAreaConstraints();

	_SolveLayout();
	if (fSolver.Result() != OPTIMAL) {
		fSolver.Save("failed-layout.txt");
		printf("Could not solve the layout specification (%d). "
			"Saved specification in file failed-layout.txt", fSolver.Result());
	}

	return BSize(Right()->Value() - Left()->Value(),
		Bottom()->Value() - Top()->Value());
}


Area*
BALMLayout::_AreaForItem(BLayoutItem* item) const
{
	if (!item)
		return NULL;
	return static_cast<Area*>(item->LayoutData());
}


void
BALMLayout::_UpdateAreaConstraints()
{
	for (int i = 0; i < CountItems(); i++)
		_AreaForItem(ItemAt(i))->InvalidateSizeConstraints();
}

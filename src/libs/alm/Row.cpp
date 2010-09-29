/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */


#include "Row.h"

#include "ALMLayout.h"
#include "OperatorType.h"
#include "YTab.h"

#include <SupportDefs.h>

	
/**
 * The top boundary of the row.
 */
YTab*
Row::Top() const
{
	return fTop;
}


/**
 * The bottom boundary of the row.
 */
YTab*
Row::Bottom() const
{
	return fBottom;
}


/**
 * Gets the row directly above this row.
 */
Row*
Row::Previous() const
{
	return fPrevious;
}


/**
 * Sets the row directly above this row.
 * May be null.
 */
void
Row::SetPrevious(Row* value)
{
	// if there should be no row directly above this row, then we have to
	// separate any such row and can remove any constraint that was used 
	// to glue this row to it
	if (value == NULL) {
		if (fPrevious == NULL) 
			return;
		fPrevious->fNext = NULL;
		fPrevious->fNextGlue = NULL;
		fPrevious = NULL;
		delete fPreviousGlue;
		fPreviousGlue = NULL;
		return;
	}
	
	// otherwise we have to set up the pointers and the glue constraint accordingly
	if (value->fNext != NULL)
		value->SetNext(NULL);
	if (fPrevious != NULL)
		SetPrevious(NULL);
		
	fPrevious = value;
	fPrevious->fNext = this;
	value->fNextGlue = value->fBottom->IsEqual(fTop);
	fPreviousGlue = value->fNextGlue;
}


/**
 * Gets the row directly below this row.
 */
Row*
Row::Next() const
{
	return fNext;
}


/**
 * Sets the row directly below this row.
 * May be null.
 */
void
Row::SetNext(Row* value)
{
	// if there should be no row directly below this row, then we have to
	// separate any such row and can remove any constraint that was used 
	// to glue this row to it
	if (value == NULL) {
		if (fNext == NULL)
			return;
		fNext->fPrevious = NULL;
		fNext->fPreviousGlue = NULL;
		fNext = NULL;
		delete fNextGlue;
		fNextGlue = NULL;
		return;
	}
	
	// otherwise we have to set up the pointers and the glue constraint accordingly
	if (value->fPrevious != NULL)
		value->SetPrevious(NULL);
	if (fNext != NULL)
		SetNext(NULL);
		
	fNext = value;
	fNext->fPrevious = this;
	value->fPreviousGlue = fBottom->IsEqual(value->fTop);
	fNextGlue = value->fPreviousGlue;
}


/**
 * Inserts the given row directly above this row.
 * 
 * @param row	the row to insert
 */
void
Row::InsertBefore(Row* row)
{
	SetPrevious(row->fPrevious);
	SetNext(row);
}


/**
 * Inserts the given row directly below this row.
 * 
 * @param row	the row to insert
 */
void
Row::InsertAfter(Row* row)
{
	SetNext(row->fNext);
	SetPrevious(row);
}


/**
 * Constrains this row to have the same height as the given row.
 * 
 * @param row	the row that should have the same height
 * @return the resulting same-height constraint
 */
Constraint*
Row::HasSameHeightAs(Row* row)
{
	Constraint* constraint = fLS->AddConstraint(
		-1.0, fTop, 1.0, fBottom, 1.0, row->fTop, -1.0, row->fBottom,
		OperatorType(EQ), 0.0);
	fConstraints.AddItem(constraint);
	return constraint;
}


/**
 * Gets the constraints.
 */
ConstraintList*
Row::Constraints() const
{
	return const_cast<ConstraintList*>(&fConstraints);
}


/**
 * Destructor.
 * Removes the row from the specification.
 */
Row::~Row()
{
	if (fPrevious != NULL) 
		fPrevious->SetNext(fNext);
	for (int32 i = 0; i < fConstraints.CountItems(); i++)
		delete (Constraint*)fConstraints.ItemAt(i);
	delete fTop;
	delete fBottom;
}


/**
 * Constructor.
 */
Row::Row(LinearSpec* ls)
{
	fLS = ls;
	fTop = new YTab(ls);
	fBottom = new YTab(ls);
}


/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "Column.h"

#include "ALMLayout.h"
#include "Tab.h"


using namespace LinearProgramming;


/**
 * The left boundary of the column.
 */
XTab*
Column::Left() const
{
	return fLeft;
}


/**
 * The right boundary of the column.
 */
XTab*
Column::Right() const
{
	return fRight;
}


/**
 * Gets the column directly to the left of this column.
 */
Column*
Column::Previous() const
{
	return fPrevious;
}


/**
 * Sets the column directly to the left of this column.
 * May be null.
 */
void
Column::SetPrevious(Column* value)
{
	// if there should be no column directly left of this column, then we have to
	// separate any such column and can remove any constraint that was used 
	// to glue this column to it
	if (value == NULL) {
		if (fPrevious == NULL) return;
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
	value->fNextGlue = value->Right()->IsEqual(Left());
	fPreviousGlue = value->fNextGlue;
}


/**
 * Gets the column directly to the right of this column.
 */
Column*
Column::Next() const
{
	return fNext;
}


/**
 * Sets the column directly to the right of this column.
 * May be null.
 */
void
Column::SetNext(Column* value)
{
	// if there should be no column directly right of this column, then we have to
	// separate any such column and can remove any constraint that was used 
	// to glue this column to it
	if (value == NULL) {
		if (fNext == NULL) return;
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
	value->fPreviousGlue = Right()->IsEqual(value->Left());
	fNextGlue = value->fPreviousGlue;
}


/**
 * Inserts the given column directly to the left of this column.
 * 
 * @param column	the column to insert
 */
void
Column::InsertBefore(Column* column)
{
	SetPrevious(column->fPrevious);
	SetNext(column);
}


/**
 * Inserts the given column directly to the right of this column.
 * 
 * @param column	the column to insert
 */
void
Column::InsertAfter(Column* column)
{
	SetNext(column->fNext);
	SetPrevious(column);
}


/**
 * Constrains this column to have the same width as the given column.
 * 
 * @param column	the column that should have the same width
 * @return the resulting same-width constraint
 */
Constraint*
Column::HasSameWidthAs(Column* column)
{
	Constraint* constraint = fLS->AddConstraint(
		-1.0, Left(), 1.0, Right(), 1.0, column->Left(), -1.0, column->Right(),
		kEQ, 0.0);
	fConstraints.AddItem(constraint);
	return constraint;
}


ConstraintList*
Column::Constraints() const
{
	return const_cast<ConstraintList*>(&fConstraints);
}


/**
 * Destructor.
 * Removes the column from the specification.
 */
Column::~Column()
{
	if (fPrevious != NULL) 
		fPrevious->SetNext(fNext);
	for (int32 i = 0; i < fConstraints.CountItems(); i++)
		delete fConstraints.ItemAt(i);
	delete fLeft;
	delete fRight;
}


/**
 * Constructor.
 */
Column::Column(BALMLayout* layout)
{
	fLS = layout->Solver();
	fLeft = layout->AddXTab();
	fRight = layout->AddXTab();
}


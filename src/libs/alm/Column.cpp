/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#include "Column.h"

#include "ALMLayout.h"
#include "Area.h"
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
 * Destructor.
 * Removes the column from the specification.
 */
Column::~Column()
{
	delete fPrefSizeConstraint;
}


/**
 * Constructor.
 */
Column::Column(LinearSpec* ls, XTab* left, XTab* right)
	:
	fLeft(left),
	fRight(right),
	fLS(ls),
	fPrefSizeConstraint(NULL)
{

}

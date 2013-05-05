/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "Row.h"

#include "ALMLayout.h"
#include "Area.h"
#include "Tab.h"


using namespace LinearProgramming;


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
 * Destructor.
 * Removes the row from the specification.
 */
Row::~Row()
{
	delete fPrefSizeConstraint;
}


/**
 * Constructor.
 */
Row::Row(LinearSpec* ls, YTab* top, YTab* bottom)
	:
	fTop(top),
	fBottom(bottom),
	fLS(ls),
	fPrefSizeConstraint(NULL)
{

}


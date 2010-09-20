/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include "YTab.h"


/**
 * Constructor.
 */
YTab::YTab(LinearSpec* ls)
	: Variable(ls)
{
	fTopLink = false;
}


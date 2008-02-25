/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#include "XTab.h"
#include "BALMLayout.h"


/**
 * Constructor.
 */
XTab::XTab(BALMLayout* ls)
	: Variable(ls)
{
	fLeftLink = false;
}


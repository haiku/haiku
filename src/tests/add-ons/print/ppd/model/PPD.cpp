/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "PPD.h"

#include <stdio.h>

PPD::PPD()
	: StatementList(true)
	, fSymbols(false)
{
}

PPD::~PPD()
{
}

void PPD::Print()
{
	printf("<ppd>\n");
	StatementList::Print();
	printf("</ppd>\n");
}

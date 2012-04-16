/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ReversePathCommand.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-ReversePathCmd"


// constructor
ReversePathCommand::ReversePathCommand(VectorPath* path)
	: PathCommand(path)
{
}

// destructor
ReversePathCommand::~ReversePathCommand()
{
}

// Perform
status_t
ReversePathCommand::Perform()
{
	fPath->Reverse();

	return B_OK;
}

// Undo
status_t
ReversePathCommand::Undo()
{
	return Perform();
}

// GetName
void
ReversePathCommand::GetName(BString& name)
{
	name << B_TRANSLATE("Reverse Path");
}

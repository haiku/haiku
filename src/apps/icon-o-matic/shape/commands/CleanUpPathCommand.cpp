/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CleanUpPathCommand.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-CleanUpPathCmd"


// constructor
CleanUpPathCommand::CleanUpPathCommand(VectorPath* path)
	: PathCommand(path),
	  fOriginalPath()
{
	if (fPath)
		fOriginalPath = *fPath;
}

// destructor
CleanUpPathCommand::~CleanUpPathCommand()
{
}

// Perform
status_t
CleanUpPathCommand::Perform()
{
	fPath->CleanUp();

	return B_OK;
}

// Undo
status_t
CleanUpPathCommand::Undo()
{
	*fPath = fOriginalPath;

	return B_OK;
}

// GetName
void
CleanUpPathCommand::GetName(BString& name)
{
	name << B_TRANSLATE("Clean Up Path");
}

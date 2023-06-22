/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "MovePathsCommand.h"

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-MovePathsCmd"


MovePathsCommand::MovePathsCommand(Container<VectorPath>* container,
		VectorPath** paths, int32 count, int32 toIndex)
	: MoveCommand<VectorPath>(container, paths, count, toIndex)
{
}


MovePathsCommand::~MovePathsCommand()
{
}


void
MovePathsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Move Paths");
	else
		name << B_TRANSLATE("Move Path");
}

/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "AddPathsCommand.h"

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddPathsCmd"


AddPathsCommand::AddPathsCommand(Container<VectorPath>* container,
		const VectorPath* const* paths, int32 count, bool ownsPaths, int32 index)
	: AddCommand<VectorPath>(container, paths, count, ownsPaths, index)
{
}


AddPathsCommand::~AddPathsCommand()
{
}


void
AddPathsCommand::GetName(BString& name)
{
	if (fOwnsItems) {
		if (fCount > 1)
			name << B_TRANSLATE("Add Paths");
		else
			name << B_TRANSLATE("Add Path");
	} else {
		if (fCount > 1)
			name << B_TRANSLATE("Assign Paths");
		else
			name << B_TRANSLATE("Assign Path");
	}
}

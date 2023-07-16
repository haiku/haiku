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
#include <StringFormat.h>

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
	static BStringFormat addFormat(B_TRANSLATE("Add {0, plural, "
		"one{Path} other{Paths}}"));
	static BStringFormat assignFormat(B_TRANSLATE("Assign {0, plural, "
		"one{Path} other{Paths}}"));
	if (fOwnsItems)
		addFormat.Format(name, fCount);
	else
		assignFormat.Format(name, fCount);
}

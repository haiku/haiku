/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "AddShapesCommand.h"

#include <Catalog.h>
#include <Locale.h>

#include "Shape.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddShapesCmd"


AddShapesCommand::AddShapesCommand(Container<Shape>* container,
		const Shape* const* shapes, int32 count, int32 index)
	: AddCommand<Shape>(container, shapes, count, true, index)
{
}


AddShapesCommand::~AddShapesCommand()
{
}


void
AddShapesCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Add Shapes");
	else
		name << B_TRANSLATE("Add Shape");
}

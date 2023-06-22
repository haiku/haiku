/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "RemoveShapesCommand.h"

#include <Catalog.h>
#include <Locale.h>

#include "Shape.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemoveShapesCmd"


RemoveShapesCommand::RemoveShapesCommand(
		Container<Shape>* container, const int32* indices, int32 count)
	: RemoveCommand<Shape>(container, indices, count)
{
}


RemoveShapesCommand::~RemoveShapesCommand()
{
}


void
RemoveShapesCommand::GetName(BString& name)
{
//	if (fCount > 1)
//		name << _GetString(MOVE_MODIFIERS, "Move Shapes");
//	else
//		name << _GetString(MOVE_MODIFIER, "Move Shape");
	if (fCount > 1)
		name << B_TRANSLATE("Remove Shapes");
	else
		name << B_TRANSLATE("Remove Shape");
}

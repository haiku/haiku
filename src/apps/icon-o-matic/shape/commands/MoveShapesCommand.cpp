/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "MoveShapesCommand.h"

#include <Catalog.h>
#include <Locale.h>

#include "Shape.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-MoveShapesCommand"


MoveShapesCommand::MoveShapesCommand(Container<Shape>* container,
		Shape** shapes, int32 count, int32 toIndex)
	: MoveCommand<Shape>(container, shapes, count, toIndex)
{
}


MoveShapesCommand::~MoveShapesCommand()
{
}


void
MoveShapesCommand::GetName(BString& name)
{
//	if (fCount > 1)
//		name << _GetString(MOVE_MODIFIERS, "Move Shapes");
//	else
//		name << _GetString(MOVE_MODIFIER, "Move Shape");
	if (fCount > 1)
		name << B_TRANSLATE("Move Shapes");
	else
		name << B_TRANSLATE("Move Shape");
}

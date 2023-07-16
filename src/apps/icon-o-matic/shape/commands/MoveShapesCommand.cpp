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
#include <StringFormat.h>

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
	static BStringFormat format(B_TRANSLATE("Move {0, plural, "
		"one{Shape} other{Shapes}}"));
	format.Format(name, fCount);
}

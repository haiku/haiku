/*
 * Copyright 2006, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "MoveStylesCommand.h"

#include <Catalog.h>
#include <Locale.h>
#include <StringFormat.h>

#include "Style.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-MoveStylesCmd"


MoveStylesCommand::MoveStylesCommand(Container<Style>* container,
		Style** styles, int32 count, int32 toIndex)
	: MoveCommand<Style>(container, styles, count, toIndex)
{
}


MoveStylesCommand::~MoveStylesCommand()
{
}


void
MoveStylesCommand::GetName(BString& name)
{
	static BStringFormat format(B_TRANSLATE("Move {0, plural, "
		"one{style} other{styles}}"));
	format.Format(name, fCount);
}

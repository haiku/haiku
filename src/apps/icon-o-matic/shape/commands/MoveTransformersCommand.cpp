/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "MoveTransformersCommand.h"

#include <Catalog.h>
#include <Locale.h>
#include <StringFormat.h>

#include "Transformer.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-MoveTransformersCmd"


MoveTransformersCommand::MoveTransformersCommand(Container<Transformer>* container,
		Transformer** transformers, int32 count, int32 toIndex)
	: MoveCommand<Transformer>(container, transformers, count, toIndex)
{
}


MoveTransformersCommand::~MoveTransformersCommand()
{
}


void
MoveTransformersCommand::GetName(BString& name)
{
	static BStringFormat format(B_TRANSLATE("Move {0, plural, "
		"one{transformer} other{transformers}}"));
	format.Format(name, fCount);
}

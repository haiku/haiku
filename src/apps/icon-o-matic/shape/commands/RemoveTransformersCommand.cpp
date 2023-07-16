/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "RemoveTransformersCommand.h"

#include <Catalog.h>
#include <Locale.h>
#include <StringFormat.h>

#include "Transformer.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemoveTransformersCmd"


RemoveTransformersCommand::RemoveTransformersCommand(Container<Transformer>* container,
													 const int32* indices,
													 int32 count)
	: RemoveCommand<Transformer>(container, indices, count)
{
}


RemoveTransformersCommand::~RemoveTransformersCommand()
{
}


void
RemoveTransformersCommand::GetName(BString& name)
{
	static BStringFormat format(B_TRANSLATE("Remove {0, plural, "
		"one{Transformer} other{Transformers}}"));
	format.Format(name, fCount);
}

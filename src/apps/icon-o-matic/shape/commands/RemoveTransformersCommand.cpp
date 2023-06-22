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
//	if (fCount > 1)
//		name << _GetString(MOVE_TRANSFORMERS, "Move Transformers");
//	else
//		name << _GetString(MOVE_TRANSFORMER, "Move Transformer");
	if (fCount > 1)
		name << B_TRANSLATE("Remove Transformers");
	else
		name << B_TRANSLATE("Remove Transformer");
}

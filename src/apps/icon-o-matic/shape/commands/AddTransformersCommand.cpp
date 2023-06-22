/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "AddTransformersCommand.h"

#include <Catalog.h>
#include <Locale.h>

#include "Transformer.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddTransformersCmd"


AddTransformersCommand::AddTransformersCommand(Container<Transformer>* container,
		const Transformer* const* transformers, int32 count, int32 index)
	: AddCommand<Transformer>(container, transformers, count, true, index)
{
}


AddTransformersCommand::~AddTransformersCommand()
{
}


void
AddTransformersCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Add Transformers");
	else
		name << B_TRANSLATE("Add Transformer");
}

/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "AddStylesCommand.h"

#include <Catalog.h>
#include <Locale.h>
#include <StringFormat.h>

#include "Style.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddStylesCmd"


AddStylesCommand::AddStylesCommand(Container<Style>* container,
		const Style* const* styles, int32 count, int32 index)
	: AddCommand<Style>(container, styles, count, true, index)
{
}


AddStylesCommand::~AddStylesCommand()
{
}


void
AddStylesCommand::GetName(BString& name)
{
	static BStringFormat format(B_TRANSLATE("Add {0, plural, "
		"one{Style} other{Styles}}"));
	format.Format(name, fCount);
}

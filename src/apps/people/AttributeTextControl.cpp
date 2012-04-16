/*
 * Copyright 2005-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include "AttributeTextControl.h"

#include <string.h>
#include <malloc.h>

#include <Font.h>
#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "People"


AttributeTextControl::AttributeTextControl(const char* label,
		const char* attribute)
	:
	BTextControl(NULL, "", NULL),
	fAttribute(attribute),
	fOriginalValue()
{
	if (label != NULL && label[0] != 0)
		SetLabel(BString(B_TRANSLATE("%attribute_label:"))
			.ReplaceFirst("%attribute_label", label));
	SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
}


AttributeTextControl::~AttributeTextControl()
{
}


bool
AttributeTextControl::HasChanged()
{
	return fOriginalValue != Text();
}


void
AttributeTextControl::Revert()
{
	if (HasChanged())
		SetText(fOriginalValue);
}


void
AttributeTextControl::Update()
{
	fOriginalValue = Text();
}

/*
 * Copyright 2005-2010, Haiku, Inc. All rights reserved.
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


AttributeTextControl::AttributeTextControl(const char* label,
		const char* attribute)
	:
	BTextControl(NULL, "", NULL),
	fAttribute(attribute),
	fOriginalValue()
{
	if (label != NULL && label[0] != 0)
		SetLabel(BString(label).Append(":"));
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

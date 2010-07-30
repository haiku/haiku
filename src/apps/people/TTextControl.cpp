//--------------------------------------------------------------------
//	
//	TTextControl.cpp
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <string.h>
#include <malloc.h>
#include <Font.h>

#include "TTextControl.h"


TTextControl::TTextControl(const char *label, const char *text)
	:
	BTextControl(NULL, text, NULL)
{

	if (label && label[0])
		SetLabel(BString(label).Append(":"));
	SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fOriginal = text;
}


TTextControl::~TTextControl()
{
}


void
TTextControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();

	BTextView* text = (BTextView *)ChildAt(0);
	text->SetMaxBytes(255);
}


bool
TTextControl::Changed()
{
	return fOriginal != Text();
}


void
TTextControl::Revert(void)
{
	if (Changed())
		SetText(fOriginal);
}


void
TTextControl::Update(void)
{
	fOriginal = Text();
}

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


TTextControl::TTextControl(BRect r, const char *label, int32 offset,
	const char *text, int32 modificationMessage, int32 msg)
	: BTextControl(r, "", "", text, new BMessage(msg))
{
	SetModificationMessage(new BMessage(modificationMessage));

	if (label[0]) {
		char newLabel[B_FILE_NAME_LENGTH];
		int32 length = strlen(label);
		memcpy(newLabel, label, length);
		newLabel[length] = ':';
		newLabel[length + 1] = '\0';

		SetLabel(newLabel);
	}
	SetDivider(offset);
	SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	if (text == NULL)
		text = "";
	fOriginal = strdup(text);
}


TTextControl::~TTextControl(void)
{
	free(fOriginal);
}


void
TTextControl::AttachedToWindow(void)
{
	BTextControl::AttachedToWindow();

	BTextView* text = (BTextView *)ChildAt(0);
	text->SetMaxBytes(255);
}


bool
TTextControl::Changed(void)
{
	return strcmp(fOriginal, Text());
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
	fOriginal = (char *)realloc(fOriginal, strlen(Text()) + 1);
	strcpy(fOriginal, Text());
}

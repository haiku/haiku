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

//====================================================================

TTextControl::TTextControl(BRect r, char *label, int32 length,
						   char *text, int32 mod_msg, int32 msg)
			 :BTextControl(r, "", label, text, new BMessage(msg))
{
	SetModificationMessage(new BMessage(mod_msg));

	fLabel = (char *)malloc(strlen(label) + 1);
	strcpy(fLabel, label);
	fOriginal = (char *)malloc(strlen(text) + 1);
	strcpy(fOriginal, text);
	fLength = length;
}

//--------------------------------------------------------------------

TTextControl::~TTextControl(void)
{
	free(fLabel);
	free(fOriginal);
}

//--------------------------------------------------------------------

void TTextControl::AttachedToWindow(void)
{
	BTextView	*text;

	BTextControl::AttachedToWindow();

	SetDivider(StringWidth(fLabel) + 7);
	text = (BTextView *)ChildAt(0);
	text->SetMaxBytes(fLength - 1);
}

//--------------------------------------------------------------------

bool TTextControl::Changed(void)
{
	return strcmp(fOriginal, Text());
}

//--------------------------------------------------------------------

void TTextControl::Revert(void)
{
	if (Changed())
		SetText(fOriginal);
}

//--------------------------------------------------------------------

void TTextControl::Update(void)
{
	fOriginal = (char *)realloc(fOriginal, strlen(Text()) + 1);
	strcpy(fOriginal, Text());
}

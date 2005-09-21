/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Reworked from DarkWyrm version in CDPlayer
 */
#include <stdlib.h>
#include <string.h>
#include "DrawButton.h"

DrawButton::DrawButton(BRect frame, const char *name,  const char *labelOn, const char* labelOff,
		BMessage *msg, int32 resize, int32 flags)
	: PaneSwitch(frame, name, "", resize, flags)
{
	fLabelOn = strdup(labelOn);
	fLabelOff = strdup(labelOff);
	SetMessage(msg);
}


DrawButton::~DrawButton(void)
{
	free(fLabelOn);
	free(fLabelOff);
}


void
DrawButton::Draw(BRect update)
{
	BPoint point(18,9);
	if (Value()) {
		DrawString(fLabelOn, point);
	} else {
		DrawString(fLabelOff, point);
	}
	PaneSwitch::Draw(update);
}

/*
 * Copyright 2008 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modéen 			fredrik@modeen.se
 */

#include "PortItem.h"

PortItem::PortItem(const char* label)
:BStringItem(label)
{}


PortItem::~PortItem()
{}

void
PortItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	BStringItem::DrawItem(owner, frame, complete);
}

BString
PortItem::GetOldJoystickName()
{
	return fOldSelectedJoystick;
}


BString
PortItem::GetJoystickName()
{
	return fSelectedJoystick;
}


void
PortItem::SetJoystickName(BString str)
{
	fOldSelectedJoystick = fSelectedJoystick;
	fSelectedJoystick = str;
}

/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ActionMenuItem.h"

#include <new>

#include <Looper.h>


// #pragma mark - ActionMenuItem


ActionMenuItem::ActionMenuItem(const char* label, BMessage* message,
	char shortcut, uint32 modifiers)
	:
	BMenuItem(label, message, shortcut, modifiers)
{
}


ActionMenuItem::ActionMenuItem(BMenu* menu,	BMessage* message)
	:
	BMenuItem(menu, message)
{
}


ActionMenuItem::~ActionMenuItem()
{
}


void
ActionMenuItem::PrepareToShow(BLooper* parentLooper, BHandler* targetHandler)
{
}


bool
ActionMenuItem::Finish(BLooper* parentLooper, BHandler* targetHandler,
	bool force)
{
	return false;
}


void
ActionMenuItem::ItemSelected()
{
}

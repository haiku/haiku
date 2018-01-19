/*
 * Copyright 2018, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TypeHandlerMenuItem.h"

#include "TypeHandler.h"


TypeHandlerMenuItem::TypeHandlerMenuItem(const char* label,	BMessage* message,
	char shortcut, uint32 modifiers)
	:
	ActionMenuItem(label, message, shortcut, modifiers),
	fTypeHandler(NULL)
{
}


TypeHandlerMenuItem::~TypeHandlerMenuItem()
{
	if (fTypeHandler != NULL)
		fTypeHandler->ReleaseReference();
}


void
TypeHandlerMenuItem::ItemSelected()
{
	// if the item was selected, acquire a reference
	// on behalf of the message target, as ours will be released
	// when our menu item is freed.
	fTypeHandler->AcquireReference();
}


status_t
TypeHandlerMenuItem::SetTypeHandler(TypeHandler* handler)
{
	status_t error = Message()->AddPointer("handler", handler);
	if (error != B_OK)
		return error;

	fTypeHandler = handler;
	return B_OK;
}

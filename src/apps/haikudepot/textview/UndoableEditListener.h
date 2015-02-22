/*
 * Copyright 2015, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef UNDOABLE_EDIT_LISTENER_H
#define UNDOABLE_EDIT_LISTENER_H


#include <Referenceable.h>

class TextDocument;
class UndoableEditRef;


class UndoableEditListener : public BReferenceable {
public:
								UndoableEditListener();
	virtual						~UndoableEditListener();

	virtual	void				UndoableEditHappened(
									const TextDocument* document,
									const UndoableEditRef& edit);
};


typedef BReference<UndoableEditListener> UndoableEditListenerRef;


#endif // UNDOABLE_EDIT_LISTENER_H

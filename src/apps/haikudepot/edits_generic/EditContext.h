/*
 * Copyright 2013 Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef EDIT_CONTEXT_H
#define EDIT_CONTEXT_H

#include <Referenceable.h>

/**
 * EditContext is passed to UndoableEdits in Perform(), Undo(), and Redo().
 * It provides a context in which the user performs these operations
 * and can be used for example to control the selection or visible
 * portion of the document to focus the user's attention on the
 * elements affected by the UndoableEdit.
 */
class EditContext : public BReferenceable {
public:
								EditContext();
	virtual						~EditContext();
};

typedef BReference<EditContext> EditContextRef;

#endif // EDIT_CONTEXT_H

/*
 * Copyright 2012, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef EDIT_STACK_H
#define EDIT_STACK_H

#include "List.h"
#include "UndoableEdit.h"

class EditStack {
public:
								EditStack();
	virtual						~EditStack();

			bool				Push(const UndoableEditRef& edit);
			UndoableEditRef		Pop();

			const UndoableEditRef&	Top() const;

			bool				IsEmpty() const;

private:
			typedef List<UndoableEditRef, false>	EditList;

			EditList			fEdits;
};

#endif // EDIT_STACK_H

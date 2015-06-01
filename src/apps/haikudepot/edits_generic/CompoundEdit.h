/*
 * Copyright 2006-2112, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef COMPOUND_EDIT_H
#define COMPOUND_EDIT_H

#include <String.h>

#include "List.h"
#include "UndoableEdit.h"

class CompoundEdit : public UndoableEdit {
public:
								CompoundEdit(const char* name);
	virtual						~CompoundEdit();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform(EditContext& context);
	virtual status_t			Undo(EditContext& context);
	virtual status_t			Redo(EditContext& context);

	virtual void				GetName(BString& name);

			bool				AppendEdit(const UndoableEditRef& edit);

private:
			typedef List<UndoableEditRef, false, 2>	EditList;

			EditList			fEdits;
			BString				fName;
};

#endif // COMPOUND_EDIT_H

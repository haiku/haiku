/*
 * Copyright 2006-2012 Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef UNDOABLE_EDIT_H
#define UNDOABLE_EDIT_H

#include <Referenceable.h>

class BString;
class EditContext;

class UndoableEdit : public BReferenceable {
public:
								UndoableEdit();
	virtual						~UndoableEdit();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform(EditContext& context);
	virtual status_t			Undo(EditContext& context);
	virtual status_t			Redo(EditContext& context);

	virtual void				GetName(BString& name);

	virtual	bool				UndoesPrevious(const UndoableEdit* previous);
	virtual	bool				CombineWithNext(const UndoableEdit* next);
	virtual	bool				CombineWithPrevious(
									const UndoableEdit* previous);

	inline	bigtime_t			TimeStamp() const
									{ return fTimeStamp; }

protected:
			bigtime_t			fTimeStamp;
};

typedef BReference<UndoableEdit> UndoableEditRef;

#endif // UNDOABLE_EDIT_H

/*
 * Copyright 2006-2012, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef EDIT_MANAGER_H
#define EDIT_MANAGER_H

#include "EditStack.h"
#include "UndoableEdit.h"

class BString;
class EditContext;


class EditManager {
public:
	class Listener {
	public:
		virtual					~Listener();
		virtual	void			EditManagerChanged(
									const EditManager* manager) = 0;
	};

public:
								EditManager();
	virtual						~EditManager();

			status_t			Perform(UndoableEdit* edit,
									EditContext& context);
			status_t			Perform(const UndoableEditRef& edit,
									EditContext& context);

			status_t			Undo(EditContext& context);
			status_t			Redo(EditContext& context);

			bool				GetUndoName(BString& name);
			bool				GetRedoName(BString& name);

			void				Clear();
			void				Save();
			bool				IsSaved();

			bool				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			status_t			_AddEdit(const UndoableEditRef& edit);

			void				_NotifyListeners();

private:
			EditStack			fUndoHistory;
			EditStack			fRedoHistory;
			UndoableEditRef		fEditAtSave;

	typedef List<Listener*, true, 4> ListenerList;
			ListenerList		fListeners;
};

#endif // EDIT_MANAGER_H

/*
 * Copyright 2006-2012, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef EDIT_MANAGER_H
#define EDIT_MANAGER_H

#include <vector>
#include <stack>

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

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			status_t			_AddEdit(const UndoableEditRef& edit);

			void				_NotifyListeners();

private:
			std::stack<UndoableEditRef>
								fUndoHistory;
			std::stack<UndoableEditRef>
								fRedoHistory;
			UndoableEditRef		fEditAtSave;
			std::vector<Listener*>
								fListeners;
};

#endif // EDIT_MANAGER_H

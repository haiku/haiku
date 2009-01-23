/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef COMMAND_STACK_H
#define COMMAND_STACK_H

#include <stack>

#include "Notifier.h"

class BString;
class RWLocker;
class Command;

class CommandStack : public Notifier {
 public:
								CommandStack(RWLocker* locker);
	virtual						~CommandStack();

			status_t			Perform(Command* command);

			status_t			Undo();
			status_t			Redo();

			bool				GetUndoName(BString& name);
			bool				GetRedoName(BString& name);

			void				Clear();
			void				Save();
			bool				IsSaved();

 private:
			status_t			_AddCommand(Command* command);

			RWLocker*			fLocker;

	typedef std::stack<Command*> command_stack;

			command_stack		fUndoHistory;
			command_stack		fRedoHistory;
			Command*			fSavedCommand;
};

#endif // COMMAND_STACK_H

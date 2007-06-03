/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CommandStack.h"

#include <stdio.h>
#include <string.h>

#include <Locker.h>
#include <String.h>

#include "Command.h"
#include "RWLocker.h"

// constructor
CommandStack::CommandStack(RWLocker* locker)
	: Notifier()
	, fLocker(locker)
	, fSavedCommand(NULL)
{
}

// destructor
CommandStack::~CommandStack()
{
	Clear();
}

// Perform
status_t
CommandStack::Perform(Command* command)
{
	if (!fLocker->WriteLock())
		return B_ERROR;

	status_t ret = command ? B_OK : B_BAD_VALUE;
	if (ret == B_OK)
		ret = command->InitCheck();

	if (ret == B_OK)
		ret = command->Perform();

	if (ret == B_OK)
		ret = _AddCommand(command);

	if (ret != B_OK) {
		// no one else feels responsible...
		delete command;
	}
	fLocker->WriteUnlock();

	Notify();

	return ret;
}

// Undo
status_t
CommandStack::Undo()
{
	if (!fLocker->WriteLock())
		return B_ERROR;

	status_t status = B_ERROR;
	if (!fUndoHistory.empty()) {
		Command* command = fUndoHistory.top();
		fUndoHistory.pop();
		status = command->Undo();
		if (status == B_OK)
			fRedoHistory.push(command);
		else
			fUndoHistory.push(command);
	}
	fLocker->WriteUnlock();

	Notify();

	return status;
}

// Redo
status_t
CommandStack::Redo()
{
	if (!fLocker->WriteLock())
		return B_ERROR;

	status_t status = B_ERROR;
	if (!fRedoHistory.empty()) {
		Command* command = fRedoHistory.top();
		fRedoHistory.pop();
		status = command->Redo();
		if (status == B_OK)
			fUndoHistory.push(command);
		else
			fRedoHistory.push(command);
	}
	fLocker->WriteUnlock();

	Notify();

	return status;
}

// UndoName
bool
CommandStack::GetUndoName(BString& name)
{
	bool success = false;
	if (fLocker->ReadLock()) {
		if (!fUndoHistory.empty()) {
			name << " ";
			fUndoHistory.top()->GetName(name);
			success = true;
		}
		fLocker->ReadUnlock();
	}
	return success;
}

// RedoName
bool
CommandStack::GetRedoName(BString& name)
{
	bool success = false;
	if (fLocker->ReadLock()) {
		if (!fRedoHistory.empty()) {
			name << " ";
			fRedoHistory.top()->GetName(name);
			success = true;
		}
		fLocker->ReadUnlock();
	}
	return success;
}

// Clear
void
CommandStack::Clear()
{
	if (fLocker->WriteLock()) {
		while (!fUndoHistory.empty()) {
			delete fUndoHistory.top();
			fUndoHistory.pop();
		}
		while (!fRedoHistory.empty()) {
			delete fRedoHistory.top();
			fRedoHistory.pop();
		}
		fLocker->WriteUnlock();
	}

	Notify();
}

// Save
void
CommandStack::Save()
{
	if (fLocker->WriteLock()) {
		if (!fUndoHistory.empty())
			fSavedCommand = fUndoHistory.top();
		fLocker->WriteUnlock();
	}

	Notify();
}

// IsSaved
bool
CommandStack::IsSaved()
{
	bool saved = false;
	if (fLocker->ReadLock()) {
		saved = fUndoHistory.empty();
		if (fSavedCommand && !saved) {
			if (fSavedCommand == fUndoHistory.top())
				saved = true;
		}
		fLocker->ReadUnlock();
	}
	return saved;
}

// #pragma mark -

// _AddCommand
status_t
CommandStack::_AddCommand(Command* command)
{
	status_t status = B_OK;

	bool add = true;
	if (!fUndoHistory.empty()) {
		// try to collapse commands to a single command
		// or remove this and the previous command if
		// they reverse each other
		if (Command* top = fUndoHistory.top()) {
			if (command->UndoesPrevious(top)) {
				add = false;
				fUndoHistory.pop();
				delete top;
				delete command;
			} else if (top->CombineWithNext(command)) {
				add = false;
				delete command;
				// after collapsing, the command might
				// have changed it's mind about InitCheck()
				// (the commands reversed each other)
				if (top->InitCheck() < B_OK) {
					fUndoHistory.pop();
					delete top;
				}
			} else if (command->CombineWithPrevious(top)) {
				fUndoHistory.pop();
				delete top;
				// after collapsing, the command might
				// have changed it's mind about InitCheck()
				// (the commands reversed each other)
				if (command->InitCheck() < B_OK) {
					delete command;
					add = false;
				}
			}
		}
	}
	if (add) {
		try {
			fUndoHistory.push(command);
		} catch (...) {
			status = B_ERROR;
		}
	}

	if (status == B_OK) {
		// the redo stack needs to be empty
		// as soon as a command was added (also in case of collapsing)
		while (!fRedoHistory.empty()) {
			delete fRedoHistory.top();
			fRedoHistory.pop();
		}
	}

	return status;
}



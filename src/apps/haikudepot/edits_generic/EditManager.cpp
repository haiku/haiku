/*
 * Copyright 2006-2012, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */

#include "EditManager.h"

#include <algorithm>

#include <stdio.h>
#include <string.h>

#include <Locker.h>
#include <String.h>


EditManager::Listener::~Listener()
{
}


EditManager::EditManager()
{
}


EditManager::~EditManager()
{
	Clear();
}


status_t
EditManager::Perform(UndoableEdit* edit, EditContext& context)
{
	if (edit == NULL)
		return B_BAD_VALUE;

	return Perform(UndoableEditRef(edit, true), context);
}


status_t
EditManager::Perform(const UndoableEditRef& edit, EditContext& context)
{
	status_t ret = edit.IsSet() ? B_OK : B_BAD_VALUE;
	if (ret == B_OK)
		ret = edit->InitCheck();

	if (ret == B_OK)
		ret = edit->Perform(context);

	if (ret == B_OK) {
		ret = _AddEdit(edit);
		if (ret != B_OK)
			edit->Undo(context);
	}

	_NotifyListeners();

	return ret;
}


status_t
EditManager::Undo(EditContext& context)
{
	status_t status = B_ERROR;
	if (!fUndoHistory.empty()) {
		UndoableEditRef edit(fUndoHistory.top());
		fUndoHistory.pop();
		status = edit->Undo(context);
		if (status == B_OK)
			fRedoHistory.push(edit);
		else
			fUndoHistory.push(edit);
	}

	_NotifyListeners();

	return status;
}


status_t
EditManager::Redo(EditContext& context)
{
	status_t status = B_ERROR;
	if (!fRedoHistory.empty()) {
		UndoableEditRef edit(fRedoHistory.top());
		fRedoHistory.pop();
		status = edit->Redo(context);
		if (status == B_OK)
			fUndoHistory.push(edit);
		else
			fRedoHistory.push(edit);
	}

	_NotifyListeners();

	return status;
}


bool
EditManager::GetUndoName(BString& name)
{
	if (!fUndoHistory.empty()) {
		name << " ";
		fUndoHistory.top()->GetName(name);
		return true;
	}
	return false;
}


bool
EditManager::GetRedoName(BString& name)
{
	if (!fRedoHistory.empty()) {
		name << " ";
		fRedoHistory.top()->GetName(name);
		return true;
	}
	return false;
}


void
EditManager::Clear()
{
	while (!fUndoHistory.empty())
		fUndoHistory.pop();
	while (!fRedoHistory.empty())
		fRedoHistory.pop();

	_NotifyListeners();
}


void
EditManager::Save()
{
	if (!fUndoHistory.empty())
		fEditAtSave = fUndoHistory.top();

	_NotifyListeners();
}


bool
EditManager::IsSaved()
{
	bool saved = fUndoHistory.empty();
	if (fEditAtSave.IsSet() && !saved) {
		if (fEditAtSave == fUndoHistory.top())
			saved = true;
	}
	return saved;
}


// #pragma mark -


void
EditManager::AddListener(Listener* listener)
{
	return fListeners.push_back(listener);
}


void
EditManager::RemoveListener(Listener* listener)
{
	fListeners.erase(std::remove(
		fListeners.begin(),
		fListeners.end(),
		listener), fListeners.end());
}


// #pragma mark -


status_t
EditManager::_AddEdit(const UndoableEditRef& edit)
{
	status_t status = B_OK;

	bool add = true;
	if (!fUndoHistory.empty()) {
		// Try to collapse edits to a single edit
		// or remove this and the previous edit if
		// they reverse each other
		const UndoableEditRef& top = fUndoHistory.top();
		if (edit->UndoesPrevious(top.Get())) {
			add = false;
			fUndoHistory.pop();
		} else if (top->CombineWithNext(edit.Get())) {
			add = false;
			// After collapsing, the edit might
			// have changed it's mind about InitCheck()
			// (the commands reversed each other)
			if (top->InitCheck() != B_OK) {
				fUndoHistory.pop();
			}
		} else if (edit->CombineWithPrevious(top.Get())) {
			fUndoHistory.pop();
			// After collapsing, the edit might
			// have changed it's mind about InitCheck()
			// (the commands reversed each other)
			if (edit->InitCheck() != B_OK) {
				add = false;
			}
		}
	}
	if (add)
		fUndoHistory.push(edit);

	if (status == B_OK) {
		// The redo stack needs to be empty
		// as soon as an edit was added (also in case of collapsing)
		while (!fRedoHistory.empty()) {
			fRedoHistory.pop();
		}
	}

	return status;
}


void
EditManager::_NotifyListeners()
{
	// Iterate a copy of the list, so we don't crash if listeners
	// detach themselves while being notified.
	std::vector<Listener*> listeners(fListeners);

	std::vector<Listener*>::const_iterator it;
	for (it = listeners.begin(); it != listeners.end(); it++) {
		Listener* listener = *it;
		listener->EditManagerChanged(this);
	}
}


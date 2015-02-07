/*
 * Copyright 2006-2012, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */

#include "EditManager.h"

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
	status_t ret = edit.Get() != NULL ? B_OK : B_BAD_VALUE;
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
	if (!fUndoHistory.IsEmpty()) {
		UndoableEditRef edit(fUndoHistory.Top());
		fUndoHistory.Pop();
		status = edit->Undo(context);
		if (status == B_OK)
			fRedoHistory.Push(edit);
		else
			fUndoHistory.Push(edit);
	}

	_NotifyListeners();

	return status;
}


status_t
EditManager::Redo(EditContext& context)
{
	status_t status = B_ERROR;
	if (!fRedoHistory.IsEmpty()) {
		UndoableEditRef edit(fRedoHistory.Top());
		fRedoHistory.Pop();
		status = edit->Redo(context);
		if (status == B_OK)
			fUndoHistory.Push(edit);
		else
			fRedoHistory.Push(edit);
	}

	_NotifyListeners();

	return status;
}


bool
EditManager::GetUndoName(BString& name)
{
	if (!fUndoHistory.IsEmpty()) {
		name << " ";
		fUndoHistory.Top()->GetName(name);
		return true;
	}
	return false;
}


bool
EditManager::GetRedoName(BString& name)
{
	if (!fRedoHistory.IsEmpty()) {
		name << " ";
		fRedoHistory.Top()->GetName(name);
		return true;
	}
	return false;
}


void
EditManager::Clear()
{
	while (!fUndoHistory.IsEmpty())
		fUndoHistory.Pop();
	while (!fRedoHistory.IsEmpty())
		fRedoHistory.Pop();

	_NotifyListeners();
}


void
EditManager::Save()
{
	if (!fUndoHistory.IsEmpty())
		fEditAtSave = fUndoHistory.Top();

	_NotifyListeners();
}


bool
EditManager::IsSaved()
{
	bool saved = fUndoHistory.IsEmpty();
	if (fEditAtSave.Get() != NULL && !saved) {
		if (fEditAtSave == fUndoHistory.Top())
			saved = true;
	}
	return saved;
}


// #pragma mark -


bool
EditManager::AddListener(Listener* listener)
{
	return fListeners.Add(listener);
}


void
EditManager::RemoveListener(Listener* listener)
{
	fListeners.Remove(listener);
}


// #pragma mark -


status_t
EditManager::_AddEdit(const UndoableEditRef& edit)
{
	status_t status = B_OK;

	bool add = true;
	if (!fUndoHistory.IsEmpty()) {
		// Try to collapse edits to a single edit
		// or remove this and the previous edit if
		// they reverse each other
		const UndoableEditRef& top = fUndoHistory.Top();
		if (edit->UndoesPrevious(top.Get())) {
			add = false;
			fUndoHistory.Pop();
		} else if (top->CombineWithNext(edit.Get())) {
			add = false;
			// After collapsing, the edit might
			// have changed it's mind about InitCheck()
			// (the commands reversed each other)
			if (top->InitCheck() != B_OK) {
				fUndoHistory.Pop();
			}
		} else if (edit->CombineWithPrevious(top.Get())) {
			fUndoHistory.Pop();
			// After collapsing, the edit might
			// have changed it's mind about InitCheck()
			// (the commands reversed each other)
			if (edit->InitCheck() != B_OK) {
				add = false;
			}
		}
	}
	if (add) {
		if (!fUndoHistory.Push(edit))
			status = B_NO_MEMORY;
	}

	if (status == B_OK) {
		// The redo stack needs to be empty
		// as soon as an edit was added (also in case of collapsing)
		while (!fRedoHistory.IsEmpty()) {
			fRedoHistory.Pop();
		}
	}

	return status;
}


void
EditManager::_NotifyListeners()
{
	int32 count = fListeners.CountItems();
	if (count == 0)
		return;
	// Iterate a copy of the list, so we don't crash if listeners
	// detach themselves while being notified.
	ListenerList listenersCopy(fListeners);
	if (listenersCopy.CountItems() != count)
		return;
	for (int32 i = 0; i < count; i++)
		listenersCopy.ItemAtFast(i)->EditManagerChanged(this);
}


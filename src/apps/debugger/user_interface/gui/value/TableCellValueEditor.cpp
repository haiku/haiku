/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TableCellValueEditor.h"

#include "Value.h"


TableCellValueEditor::TableCellValueEditor()
	:
	BReferenceable(),
	fInitialValue(NULL)
{
}


TableCellValueEditor::~TableCellValueEditor()
{
	if (fInitialValue != NULL)
		fInitialValue->ReleaseReference();
}


void
TableCellValueEditor::AddListener(Listener* listener)
{
	fListeners.Add(listener);
}


void
TableCellValueEditor::RemoveListener(Listener* listener)
{
	fListeners.Remove(listener);
}


void
TableCellValueEditor::SetInitialValue(Value* value)
{
	if (fInitialValue != NULL)
		fInitialValue->ReleaseReference();

	fInitialValue = value;
	if (fInitialValue != NULL)
		fInitialValue->AcquireReference();
}


void
TableCellValueEditor::NotifyEditBeginning()
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TableCellEditBeginning();
	}
}


void
TableCellValueEditor::NotifyEditCancelled()
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TableCellEditCancelled();
	}
}


void
TableCellValueEditor::NotifyEditCompleted(Value* newValue)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TableCellEditEnded(newValue);
	}
}


// #pragma mark - TableCellValueEditor::Listener


TableCellValueEditor::Listener::~Listener()
{
}


void
TableCellValueEditor::Listener::TableCellEditBeginning()
{
}


void
TableCellValueEditor::Listener::TableCellEditCancelled()
{
}


void
TableCellValueEditor::Listener::TableCellEditEnded(Value* newValue)
{
}



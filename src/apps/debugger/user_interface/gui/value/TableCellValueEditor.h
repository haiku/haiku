/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_VALUE_EDITOR_H
#define TABLE_CELL_VALUE_EDITOR_H

#include <Referenceable.h>

#include <util/DoublyLinkedList.h>


class BView;
class Value;


class TableCellValueEditor : public BReferenceable {
public:
	class Listener;
								TableCellValueEditor();
	virtual						~TableCellValueEditor();

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

	inline	Value*				InitialValue() const { return fInitialValue; }
			void				SetInitialValue(Value* value);

	virtual	BView*				GetView() = 0;

protected:
			void				NotifyEditBeginning();
			void				NotifyEditCancelled();
			void				NotifyEditCompleted(Value* newValue);

private:
	typedef DoublyLinkedList<Listener> ListenerList;

private:
			ListenerList		fListeners;
			Value*				fInitialValue;
};


class TableCellValueEditor::Listener : public
	DoublyLinkedListLinkImpl<TableCellValueEditor::Listener> {
public:
	virtual						~Listener();

	virtual	void				TableCellEditBeginning();
	virtual	void				TableCellEditCancelled();
	virtual	void				TableCellEditEnded(Value* newValue);
};

#endif	// TABLE_CELL_VALUE_EDITOR_H

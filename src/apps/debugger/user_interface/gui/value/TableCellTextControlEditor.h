/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_TEXT_CONTROL_EDITOR_H
#define TABLE_CELL_TEXT_CONTROL_EDITOR_H

#include <TextControl.h>

#include "TableCellFormattedValueEditor.h"


// common base class for editors that input a value via a text field
class TableCellTextControlEditor : public TableCellFormattedValueEditor,
	public BTextControl {
public:
								TableCellTextControlEditor(
									::Value* initialValue,
									ValueFormatter* formatter);
	virtual						~TableCellTextControlEditor();

	status_t					Init();

	virtual	BView*				GetView();

	virtual	bool				ValidateInput() const = 0;

	virtual status_t			GetValueForInput(::Value*& _output) const = 0;
									// returns reference

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
};

#endif	// TABLE_CELL_TEXT_CONTROL_EDITOR_H

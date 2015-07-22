/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_OPTION_POPUP_EDITOR_H
#define TABLE_CELL_OPTION_POPUP_EDITOR_H

#include <OptionPopUp.h>

#include "TableCellFormattedValueEditor.h"


// common base class for editors that have a fixed set of chooseable
// values known up front
class TableCellOptionPopUpEditor : public TableCellFormattedValueEditor,
	protected BOptionPopUp {
public:
								TableCellOptionPopUpEditor(
									::Value* initialValue,
									ValueFormatter* formatter);
	virtual						~TableCellOptionPopUpEditor();

	status_t					Init();

	virtual	BView*				GetView();

	virtual	status_t			ConfigureOptions() = 0;

protected:
	virtual	status_t			GetSelectedValue(::Value*& _value) const = 0;

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
};

#endif	// TABLE_CELL_TEXT_CONTROL_EDITOR_H

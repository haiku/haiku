/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLE_EDIT_WINDOW_H
#define VARIABLE_EDIT_WINDOW_H


#include <Window.h>

#include "TableCellValueEditor.h"


class BButton;
class Value;
class ValueNode;


class VariableEditWindow : public BWindow,
	private TableCellValueEditor::Listener {
public:
								VariableEditWindow(Value* initialValue,
									ValueNode* node,
									TableCellValueEditor* editor,
									BHandler* target);

								~VariableEditWindow();

	static	VariableEditWindow* Create(Value* initialValue,
									ValueNode* node,
									TableCellValueEditor* editor,
									BHandler* closeTarget);
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();
	virtual	bool				QuitRequested();

	// TableCellValueEditor::Listener
	virtual	void				TableCellEditBeginning();
	virtual	void				TableCellEditCancelled();
	virtual	void				TableCellEditEnded(Value* newValue);

private:
			void	 			_Init();

private:
			BButton*			fCancelButton;
			BButton*			fSaveButton;
			BHandler*			fTarget;
			ValueNode*			fNode;
			Value*				fInitialValue;
			Value*				fNewValue;
			TableCellValueEditor* fEditor;
};

#endif // VARIABLE_EDIT_WINDOW_H

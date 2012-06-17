/*
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CREATION_PARAMETER_EDITOR
#define _CREATION_PARAMETER_EDITOR


#include <PartitionParameterEditor.h>

#include <CheckBox.h>
#include <String.h>
#include <View.h>


class PrimaryPartitionEditor : public BPartitionParameterEditor {
public:
								PrimaryPartitionEditor();
	virtual						~PrimaryPartitionEditor();

	virtual		bool			FinishedEditing();
	virtual		BView*			View();
	virtual		status_t		GetParameters(BString* parameters);

	virtual		status_t		PartitionTypeChanged(const char* type);

private:
				BView*			fView;
				BCheckBox*		fActiveCheckBox;
				BString			fParameters;
};


#endif //_CREATION_PARAMETER_EDITOR

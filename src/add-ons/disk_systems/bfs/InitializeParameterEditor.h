/*
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INITIALIZE_PARAMETER_EDITOR
#define _INITIALIZE_PARAMETER_EDITOR


#include <PartitionParameterEditor.h>

#include <MenuField.h>
#include <TextControl.h>
#include <String.h>
#include <View.h>


class InitializeBFSEditor : public BPartitionParameterEditor {
public:
								InitializeBFSEditor();
	virtual						~InitializeBFSEditor();

	virtual		bool			FinishedEditing();
	virtual		BView*			View();
	virtual		status_t		GetParameters(BString* parameters);

	virtual		status_t		PartitionNameChanged(const char* name);

private:
				void			_CreateViewControls();

				BView*			fView;
				BTextControl*	fNameTC;
				BMenuField*		fBlockSizeMF;
				BString			fParameters;
};


#endif //_INITIALIZE_PARAMETER_EDITOR

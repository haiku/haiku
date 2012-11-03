/*
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INITIALIZE_PARAMETER_EDITOR
#define _INITIALIZE_PARAMETER_EDITOR


#include <PartitionParameterEditor.h>
#include <String.h>

class BCheckBox;
class BMenuField;
class BTextControl;
class BView;


class InitializeNTFSEditor : public BPartitionParameterEditor {
public:
								InitializeNTFSEditor();
	virtual						~InitializeNTFSEditor();

	virtual		bool			FinishedEditing();
	virtual		BView*			View();
	virtual		status_t		GetParameters(BString* parameters);

	virtual		status_t		PartitionNameChanged(const char* name);

private:
				void			_CreateViewControls();

				BView*			fView;
				BTextControl*	fNameTC;

				BString			fParameters;
};


#endif //_INITIALIZE_PARAMETER_EDITOR

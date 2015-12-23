/*
 * Copyright 2015, François Revol <revol@free.fr>
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


class InitializeFATEditor : public BPartitionParameterEditor {
public:
								InitializeFATEditor();
	virtual						~InitializeFATEditor();

	virtual		void			SetTo(BPartition* partition);

	virtual		bool			ValidateParameters() const;
	virtual		status_t		ParameterChanged(const char* name,
									const BVariant& variant);

	virtual		BView*			View();

	virtual		status_t		GetParameters(BString& parameters);

//	virtual		status_t		PartitionNameChanged(const char* name);

private:
				void			_CreateViewControls();

				BView*			fView;
				BTextControl*	fNameControl;
				BMenuField*		fFatBitsMenuField;

				BString			fParameters;
};


#endif //_INITIALIZE_PARAMETER_EDITOR

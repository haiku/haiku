/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PRIMARY_PARAMETER_EDITOR
#define _PRIMARY_PARAMETER_EDITOR


#include <PartitionParameterEditor.h>

#include <CheckBox.h>
#include <String.h>
#include <View.h>


class PrimaryPartitionEditor : public BPartitionParameterEditor {
public:
								PrimaryPartitionEditor();
	virtual						~PrimaryPartitionEditor();

	virtual		BView*			View();

	virtual		status_t		ParameterChanged(const char* name,
									const BVariant& variant);

	virtual		status_t		GetParameters(BString& parameters);

private:
				BView*			fView;
				BCheckBox*		fActiveCheckBox;
};


#endif // _PRIMARY_PARAMETER_EDITOR

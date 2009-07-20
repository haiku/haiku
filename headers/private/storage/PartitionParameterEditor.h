/*
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PARTITION_PARAMETER_EDITOR_H
#define _PARTITION_PARAMETER_EDITOR_H


#include <View.h>


// BPartitionParameterEditor
class BPartitionParameterEditor {
public:
								BPartitionParameterEditor();
	virtual						~BPartitionParameterEditor();

	virtual		bool			FinishedEditing();

	virtual		BView*			View();

	virtual		status_t		GetParameters(BString* parameters);

	// TODO: Those are child creation specific and shouldn't be in a generic
	// interface. Something like a
	// GenericPartitionParameterChanged(partition_parameter_type,
	//		const BVariant&)
	// would be better.
	virtual		status_t		PartitionTypeChanged(const char* type);
	virtual		status_t		PartitionNameChanged(const char* name);
};


#endif //_PARTITION_PARAMETER_EDITOR_H

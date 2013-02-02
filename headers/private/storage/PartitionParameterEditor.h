/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PARTITION_PARAMETER_EDITOR_H
#define _PARTITION_PARAMETER_EDITOR_H


#include <View.h>


class BMessage;
class BPartition;
class BVariant;


class BPartitionParameterEditor {
public:
								BPartitionParameterEditor();
	virtual						~BPartitionParameterEditor();

	virtual		void			SetTo(BPartition* partition);

				void			SetModificationMessage(BMessage* message);
				BMessage*		ModificationMessage() const;

	virtual		BView*			View();

	virtual		bool			ValidateParameters() const;
	virtual		status_t		ParameterChanged(const char* name,
									const BVariant& variant);

	virtual		status_t		GetParameters(BString& parameters);

private:
				BMessage*		fModificationMessage;
};


#endif // _PARTITION_PARAMETER_EDITOR_H

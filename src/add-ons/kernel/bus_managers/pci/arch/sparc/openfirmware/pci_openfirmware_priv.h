/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef PCI_BUS_MANAGER_SPARC_OPEN_FIRMWARE_PRIV_H
#define PCI_BUS_MANAGER_SPARC_OPEN_FIRMWARE_PRIV_H

#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>


struct StringArrayPropertyValue;


// implementations

// TODO


// property support

struct PropertyValue {
	PropertyValue()
		: value(NULL)
	{
	}

	~PropertyValue()
	{
		free(value);
	}

	char	*value;
	int		length;
};

struct StringArrayPropertyValue : PropertyValue {

	char *NextElement(int &cookie) const;
	bool ContainsElement(const char *value) const;
};

status_t	openfirmware_get_property(int package, const char *propertyName,
				PropertyValue &value);

#endif	// PCI_BUS_MANAGER_SPARC_OPEN_FIRMWARE_PRIV_H

/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "pci_openfirmware.h"

#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>
#include <platform/openfirmware/pci.h>

#include "pci_openfirmware_priv.h"


typedef status_t (*probeFunction)(int, const StringArrayPropertyValue&);

static const probeFunction sProbeFunctions[] = {
	ppc_openfirmware_probe_uninorth,
	ppc_openfirmware_probe_grackle,
	NULL,
};


status_t
ppc_openfirmware_pci_controller_init(void)
{
	char path[256];
	int cookie = 0;
	while (of_get_next_device(&cookie, 0, "pci", path, sizeof(path))
			== B_OK) {
dprintf("ppc_openfirmware_pci_controller_init(): pci device node: %s\n", path);
		// get the device node and the "compatible" property
		int deviceNode = of_finddevice(path);
		StringArrayPropertyValue compatible;
		status_t error = openfirmware_get_property(deviceNode, "compatible",
			compatible);
		if (error != B_OK) {
			dprintf("ppc_openfirmware_pci_controller_init: Failed to get "
				"\"compatible\" property for pci device: %s\n", path);
			continue;
		}

		// probe
		for (int i = 0; sProbeFunctions[i]; i++) {
			error = sProbeFunctions[i](deviceNode, compatible);
			if (error == B_OK)
				break;
		}
	}

	return B_OK;
}


// #pragma mark - support functions


char *
StringArrayPropertyValue::NextElement(int &cookie) const
{
	if (cookie >= length)
		return NULL;

	char *result = value + cookie;
	cookie += strnlen(result, length - cookie) + 1;
	return result;
}


bool
StringArrayPropertyValue::ContainsElement(const char *value) const
{
	int cookie = 0;
	while (char *checkValue = NextElement(cookie)) {
		if (strcmp(checkValue, value) == 0)
			return true;
	}

	return false;
}


status_t
openfirmware_get_property(int package, const char *propertyName,
	PropertyValue &value)
{
	value.length = of_getproplen(package, propertyName);
	if (value.length < 0)
		return B_ENTRY_NOT_FOUND;

	value.value = (char*)malloc(value.length);
	if (!value.value)
		return B_NO_MEMORY;

	if (of_getprop(package, propertyName, value.value, value.length)
		== OF_FAILED) {
		return B_ERROR;
	}

	return B_OK;
}


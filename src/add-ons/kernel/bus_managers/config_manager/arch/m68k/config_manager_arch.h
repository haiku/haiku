/*
 * Copyright 2007 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * arch-specific config manager
 *
 * Authors (in chronological order):
 *              François Revol (revol@free.fr)
 */
#ifndef _CONFIG_MANAGER_ARCH_H
#define _CONFIG_MANAGER_ARCH_H

#include <ISA.h>
#include <PCI.h>
#include <isapnp.h>
#include <config_manager.h>

struct hardcoded_device {
	struct device_info info;
	union {
		struct isa_info isa;
		pci_info pci;
		//pcmcia...
	} bus;
	struct possible_device_configurations configs;
};

#endif /* _CONFIG_MANAGER_ARCH_H */


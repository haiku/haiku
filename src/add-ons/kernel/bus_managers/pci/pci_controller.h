/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __PCI_CONTROLLER_H
#define __PCI_CONTROLLER_H

#include <bus/PCI.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t	pci_controller_init(void);
status_t	pci_controller_finalize(void);
status_t	pci_controller_add(pci_controller_module_info *controller, void *cookie);

#ifdef __cplusplus
}
#endif

#endif

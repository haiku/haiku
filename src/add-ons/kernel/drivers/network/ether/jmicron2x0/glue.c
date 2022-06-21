/*
 * Copyright 2009, Axel Dörfler. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>

HAIKU_FBSD_DRIVER_GLUE(jmicron2x0, jme, pci);
HAIKU_DRIVER_REQUIREMENTS(FBSD_SWI_TASKQUEUE);

HAIKU_FBSD_MII_DRIVER(jmphy);
NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();

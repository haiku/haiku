/*
 * Copyright 2009, Axel Dörfler. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>

HAIKU_FBSD_DRIVER_GLUE(jmicron2x0, jme, pci);

HAIKU_FBSD_MII_DRIVER(jmphy);
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE | FBSD_FAST_TASKQUEUE);
NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();

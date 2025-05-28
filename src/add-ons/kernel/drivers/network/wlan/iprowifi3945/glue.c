/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <sys/haiku-module.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(iprowifi3945, wpi, pci)
NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();
HAIKU_DRIVER_REQUIREMENTS(OBSD_WLAN);
HAIKU_FIRMWARE_VERSION(1);
HAIKU_FIRMWARE_NAME_MAP({
	{"wpi-3945abg", "iwlwifi-3945-15.ucode"}
});


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	// We only support MSI, so we handle all interrupts.
	return 1;
}

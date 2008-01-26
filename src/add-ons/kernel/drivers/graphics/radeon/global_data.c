/*
	Copyright (c) 2002, Thomas Kurschel

	Part of Radeon kernel driver
	Global variables
*/

#include "radeon_driver.h"


int debug_level_flow = 4;
int debug_level_info = 4;
int debug_level_error = 4;

radeon_devices *devices;
pci_module_info *pci_bus;
agp_gart_module_info *sAGP;

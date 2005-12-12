/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "intel.h"
#include "amd.h"
#include "via.h"


module_info *gModules[] = {
	(module_info *)&gIntelModule,
	(module_info *)&gAMDModule,
	(module_info *)&gVIAModule,
	NULL
};

/*
 * Copyright 2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SMBIOS_MODULE_H_
#define _SMBIOS_MODULE_H_


#include <OS.h>
#include <module.h>


typedef struct smbios_module_info {
	module_info	info;

	bool	(*match_vendor_product)(const char* vendor, const char* product);
} smbios_module_info;


#define SMBIOS_MODULE_NAME "generic/smbios/driver_v1"


#endif	// _SMBIOS_MODULE_H_

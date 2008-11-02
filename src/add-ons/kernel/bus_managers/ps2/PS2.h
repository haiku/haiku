/*
 * Copyright 2005 Marcus Overhagen
 * Distributed under the terms of the MIT License.
 *
 * PS/2 bus manager
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */
#ifndef _PS2_H_
#define _PS2_H_

#include <bus_manager.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
	bus_manager_info	binfo;

	int32 (*function1)();
	int32 (*function2)();

} ps2_module_info;

#define	B_PS2_MODULE_NAME		"bus_managers/ps2/v1"


#ifdef __cplusplus
}
#endif

#endif


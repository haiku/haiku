/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@berlios.de
 */
#ifndef _YARROW_RNG_H
#define _YARROW_RNG_H


#include <OS.h>

#include "random.h"


#define YARROW_RNG_SIM_MODULE_NAME "bus_managers/random/yarrow_rng/device/v1"


#ifdef __cplusplus
extern "C" {
#endif


extern random_module_info gYarrowRandomModule;


#ifdef __cplusplus
}
#endif


#endif /* _YARROW_RNG_H */

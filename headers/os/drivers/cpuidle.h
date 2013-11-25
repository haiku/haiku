/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CPUIDLE_MODULE_H
#define _CPUIDLE_MODULE_H


#include <module.h>


#define CPUIDLE_MODULES_PREFIX	"power/cpuidle"


typedef struct cpuidle_module_info {
	module_info		info;

	float			rank;

	void			(*idle)(void);
	void			(*wait)(int32* variable, int32 test);
} cpuidle_module_info;


#endif		// _CPUIDLE_MODULE_H

/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CPUFREQ_H
#define _CPUFREQ_H


#include <module.h>


#define CPUFREQ_MODULES_PREFIX	"power/cpufreq"


const int kCPUPerformanceScaleMax = 1000;

typedef struct cpufreq_module_info {
	module_info		info;

	status_t		(*increase_performance)(int delta, bool allowBoost);
	status_t		(*decrease_performance)(int delta);
} cpufreq_module_info;


#endif	// _CPUFREQ_H


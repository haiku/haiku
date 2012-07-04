/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CPUIDLE_MODULE_H
#define _CPUIDLE_MODULE_H


#include <module.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CPUIDLE_CSTATE_MAX	8
#define CSTATE_NAME_LENGTH	32
#define B_CPUIDLE_MODULE_NAME "idle/generic/cpuidle/v1"


struct CpuidleStats {
	uint64		usageCount;
	bigtime_t	usageTime;
};


struct CpuidleInfo {
	int32			cstateSleep;
	CpuidleStats	stats[CPUIDLE_CSTATE_MAX];
};


struct CpuidleCstate {
	char	name[CSTATE_NAME_LENGTH];
	int32	latency;
	int32	(*EnterIdle)(int32 state, CpuidleCstate *cstate);
	void	*pData;
};


struct CpuidleModuleInfo {
	module_info		info;
	CpuidleCstate	cStates[CPUIDLE_CSTATE_MAX];
	int32			cStateCount;
};


#ifdef __cplusplus
}
#endif

#endif		// _CPUIDLE_MODULE_H

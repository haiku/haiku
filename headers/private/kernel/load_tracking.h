/*
 * Copyright 2013 Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_LOAD_TRACKING_H
#define _KERNEL_LOAD_TRACKING_H


#include <OS.h>


const int32 kMaxLoad = 1000;
const bigtime_t kLoadMeasureInterval = 1000;
const bigtime_t kIntervalInaccuracy = kLoadMeasureInterval / 4;


static inline int32
compute_load(bigtime_t& measureTime, bigtime_t& measureActiveTime, int32& load,
	bigtime_t now)
{
	if (measureTime == 0) {
		measureTime = now;
		return -1;
	}

	bigtime_t deltaTime = now - measureTime;

	if (deltaTime < kLoadMeasureInterval)
		return -1;

	int32 oldLoad = load;
	ASSERT(oldLoad >= 0 && oldLoad <= kMaxLoad);

	int32 newLoad = measureActiveTime * kMaxLoad;
	newLoad /= max_c(deltaTime, 1);
	newLoad = max_c(min_c(newLoad, kMaxLoad), 0);

	measureActiveTime = 0;
	measureTime = now;

	deltaTime += kIntervalInaccuracy;
	bigtime_t n = deltaTime / kLoadMeasureInterval;
	ASSERT(n > 0);

	if (n > 10)
		load = newLoad;
	else {
		newLoad *= (1 << n) - 1;
		load = (load + newLoad) / (1 << n);
		ASSERT(load >= 0 && load <= kMaxLoad);
	}

	return oldLoad;
}


#endif	// _KERNEL_LOAD_TRACKING_H

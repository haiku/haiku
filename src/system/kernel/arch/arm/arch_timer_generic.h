/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCH_ARM_TIMER_GENERIC_H
#define ARCH_ARM_TIMER_GENERIC_H

#include <new>
#include <SupportDefs.h>

#include "soc.h"

class ARMGenericTimer : public HardwareTimer {
public:
	void SetTimeout(bigtime_t timeout);
	void Clear();
	bigtime_t Time();

	static status_t Init() {
		ARMGenericTimer *timer = new(std::nothrow) ARMGenericTimer();
		return timer != NULL ? B_OK : B_NO_MEMORY;
	}

	static bool IsAvailable();
protected:
	ARMGenericTimer();

private:
	static int32 _InterruptWrapper(void *data);
	int32 HandleInterrupt();

	uint32_t fTimerFrequency;
};

#endif /* ARCH_ARM_TIMER_GENERIC_H */

#ifndef ARCH_ARM_SOC_OMAP3_H
#define ARCH_ARM_SOC_OMAP3_H

class OMAP3InterruptController;

#include "soc.h"

#include <new>


class OMAP3InterruptController : public InterruptController {
public:
	OMAP3InterruptController(uint32_t reg_base);
	void EnableInterrupt(int irq);
	void DisableInterrupt(int irq);
	void HandleInterrupt();

protected:
	void SoftReset();

	area_id fRegArea;
	uint32 *fRegBase;
	uint32 fNumPending;
};

class OMAP3Timer : public HardwareTimer {
public:
	void SetTimeout(bigtime_t timeout);
	bigtime_t Time();
	void Clear();

	static status_t Init(uint32_t reg_base, uint32_t interrupt) {
		if (sInstance == NULL) {
			OMAP3Timer *timer = new(std::nothrow) OMAP3Timer(reg_base, interrupt);
			// XXX implement InitCheck() functionality
			return timer != NULL ? B_OK : B_NO_MEMORY;
		} else {
			// XXX We have multiple timers; just create the first one
			// and ignore the rest
			return B_OK;
		}
	}

private:
	OMAP3Timer(uint32_t reg_base, uint32_t interrupt);

	static int32 _InterruptWrapper(void *data);
	int32 HandleInterrupt();

	bigtime_t fSystemTime;
	area_id fRegArea;
	uint32 *fRegBase;
	int fInterrupt;
};

#endif /* ARCH_ARM_SOC_OMAP3_H */

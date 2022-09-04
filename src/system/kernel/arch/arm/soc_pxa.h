#ifndef ARCH_ARM_SOC_PXA_H
#define ARCH_ARM_SOC_PXA_H

class PXAInterruptController;

#include "soc.h"

#include <new>


class PXAInterruptController : public InterruptController {
public:
	PXAInterruptController(uint32_t reg_base);
	void EnableInterrupt(int irq);
	void DisableInterrupt(int irq);
	void HandleInterrupt();

protected:
	area_id fRegArea;
	uint32 *fRegBase;
};


class PXATimer : public HardwareTimer {
public:
	void SetTimeout(bigtime_t timeout);
	void Clear();
	bigtime_t Time();

	static status_t Init(uint32_t reg_base) {
		PXATimer *timer = new(std::nothrow) PXATimer(reg_base);
		// XXX implement InitCheck() functionality
		return timer != NULL ? B_OK : B_NO_MEMORY;
	}

protected:
	PXATimer(uint32_t reg_base);

	area_id fRegArea;
	uint32 *fRegBase;
	bigtime_t fSystemTime;
private:
	static int32 _InterruptWrapper(void *data);
	int32 HandleInterrupt();
};


#endif // ARCH_ARM_SOC_PXA_H

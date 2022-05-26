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

#if 0
	static status_t Init(fdt_module_info *fdt, fdt_device_node node, void *cookie) {
		PXATimer *timer = new(std::nothrow) PXATimer(fdt, node);
		// XXX implement InitCheck() functionality
		return timer != NULL ? B_OK : B_NO_MEMORY;
	}
#endif

protected:
	//PXATimer(fdt_module_info *fdt, fdt_device_node node);

	area_id fRegArea;
	uint32 *fRegBase;
	bigtime_t fSystemTime;
private:
	static int32 _InterruptWrapper(void *data);
	int32 HandleInterrupt();
};


#endif // ARCH_ARM_SOC_PXA_H

#ifndef ARCH_ARM_SOC_PXA_H
#define ARCH_ARM_SOC_PXA_H

class PXAInterruptController;

#include "soc.h"

#include <new>


class PXAInterruptController : public InterruptController {
public:
	void EnableInterrupt(int irq);
	void DisableInterrupt(int irq);
	void HandleInterrupt();

	static status_t Init(fdt_module_info *fdt, fdt_device_node node, void *cookie) {
		InterruptController *ic = new(std::nothrow) PXAInterruptController(fdt, node);
		// XXX implement InitCheck() functionality
		return ic != NULL ? B_OK : B_NO_MEMORY;
	}

protected:
	PXAInterruptController(fdt_module_info *fdt, fdt_device_node node);

	area_id fRegArea;
	uint32 *fRegBase;
};


class PXATimer : public HardwareTimer {
public:
	void SetTimeout(bigtime_t timeout);
	void Clear();
	bigtime_t Time();

	static status_t Init(fdt_module_info *fdt, fdt_device_node node, void *cookie) {
		PXATimer *timer = new(std::nothrow) PXATimer(fdt, node);
		// XXX implement InitCheck() functionality
		return timer != NULL ? B_OK : B_NO_MEMORY;
	}

protected:
	PXATimer(fdt_module_info *fdt, fdt_device_node node);

	area_id fRegArea;
	uint32 *fRegBase;
	bigtime_t fSystemTime;
private:
	static int32 _InterruptWrapper(void *data);
	int32 HandleInterrupt();
};


#endif // ARCH_ARM_SOC_PXA_H

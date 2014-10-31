#ifndef ARCH_ARM_SOC_H
#define ARCH_ARM_SOC_H

class InterruptController;

#include <drivers/bus/FDT.h>
#include <private/kernel/int.h>
#include <private/kernel/timer.h>

// ------------------------------------------------------ InterruptController

class InterruptController {
public:
	virtual void EnableInterrupt(int irq) = 0;
	virtual void DisableInterrupt(int irq) = 0;

	virtual void HandleInterrupt() = 0;

	static InterruptController* Get() {
		return sInstance;
	}

protected:
	InterruptController(fdt_module_info *fdtModule, fdt_device_node node)
		: fFDT(fdtModule), fNode(node) {
		if (sInstance) {
			panic("Multiple InterruptController objects created; that is currently unsupported!");
		}
		sInstance = this;
	}

	// Keep our node around as we might want to grab attributes from it
	fdt_module_info *fFDT;
	fdt_device_node fNode;

	static InterruptController *sInstance;
};


// ------------------------------------------------------ HardwareTimer

class HardwareTimer {
public:
	virtual void SetTimeout(bigtime_t timeout) = 0;
	virtual bigtime_t Time() = 0;
	virtual void Clear() = 0;

	static HardwareTimer* Get() {
		return sInstance;
	}

protected:
	HardwareTimer(fdt_module_info *fdtModule, fdt_device_node node)
		: fFDT(fdtModule), fNode(node) {
		if (sInstance) {
			panic("Multiple HardwareTimer objects created; that is currently unsupported!");
		}
		sInstance = this;
	}

	// Keep our node around as we might want to grab attributes from it
	fdt_module_info *fFDT;
	fdt_device_node fNode;

	static HardwareTimer *sInstance;
};

#endif // ARCH_ARM_SOC_H

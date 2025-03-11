#ifndef ARCH_ARM_SOC_H
#define ARCH_ARM_SOC_H

class InterruptController;

#include <drivers/bus/FDT.h>
#include <private/kernel/interrupts.h>
#include <private/kernel/timer.h>

// ------------------------------------------------------ InterruptController

class InterruptController {
public:
	virtual void EnableInterrupt(int32 irq) = 0;
	virtual void DisableInterrupt(int32 irq) = 0;

	virtual void HandleInterrupt() = 0;

	static InterruptController* Get() {
		return sInstance;
	}

protected:
	InterruptController()
	{
		if (sInstance) {
			panic("Multiple InterruptController objects created; that is currently unsupported!");
		}
		sInstance = this;
	}

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
	HardwareTimer()
	{
		if (sInstance) {
			panic("Multiple HardwareTimer objects created; that is currently unsupported!");
		}
		sInstance = this;
	}

	static HardwareTimer *sInstance;
};

#endif // ARCH_ARM_SOC_H

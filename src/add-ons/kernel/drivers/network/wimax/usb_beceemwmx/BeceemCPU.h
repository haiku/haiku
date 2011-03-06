/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the MIT license.
 *
 *	Description: Wrangle Beceem CPU control calls
 */
#ifndef _USB_BECEEM_CPU_H_
#define _USB_BECEEM_CPU_H_


#include <ByteOrder.h>
#include "DeviceStruct.h"


#define CLOCK_RESET_CNTRL_REG_1 0x0F00000C


class BeceemCPU {
public:
								BeceemCPU();
			status_t			CPUInit(WIMAX_DEVICE* swmxdevice);
			status_t			CPURun();
			status_t			CPUReset();

// yuck.  These are in a parent class
	virtual	status_t			ReadRegister(unsigned int reg, size_t size,
									uint32_t* buffer) { return NULL; };
	virtual	status_t			WriteRegister(unsigned int reg, size_t size,
									uint32_t* buffer) { return NULL; };
	virtual	status_t			BizarroReadRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
										{ return NULL; };
	virtual status_t			BizarroWriteRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
										{ return NULL; };

private:
			WIMAX_DEVICE*		fWmxDevice;

};
#endif // _USB_BECEEM_CPU_H_


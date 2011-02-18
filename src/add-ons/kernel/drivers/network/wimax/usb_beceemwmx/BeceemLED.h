/*
 *	Beceem WiMax USB Driver
 *	Copyright 2010-2011 Haiku, Inc. All rights reserved.
 *	Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 */
#ifndef _USB_BECEEM_LED_H_
#define _USB_BECEEM_LED_H_


#include <ByteOrder.h>
#include "DeviceStruct.h"


class BeceemLED
{
public:
								BeceemLED();
			status_t			LEDInit(WIMAX_DEVICE* wmxdevice);
			status_t			LEDThreadTerminate();
			status_t			LEDOff(unsigned int index);
			status_t			LEDOn(unsigned int index);
			status_t			LightsOut();
	static	status_t			LEDThread(void *cookie);

			WIMAX_DEVICE*		pwmxdevice;

	// yuck.  These are in a parent class
	virtual status_t			ReadRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
									{ return NULL; };
	virtual status_t			WriteRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
									{ return NULL; };
	virtual status_t			BizarroReadRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
									{ return NULL; };
	virtual status_t			BizarroWriteRegister(unsigned int reg,
									size_t size, uint32_t* buffer)
									{ return NULL; };

private:
		status_t	GPIOReset();

};


typedef enum _LEDColors{
	RED_LED = 1,
	BLUE_LED = 2,
	YELLOW_LED = 3,
	GREEN_LED = 4
} LEDColors;


#endif	// _USB_BECEEM_LED_H


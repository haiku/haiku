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
			status_t			LEDOff(uint32 index);
			status_t			LEDOn(uint32 index);
			status_t			LightsOut();
	static	status_t			LEDThread(void *cookie);

			WIMAX_DEVICE*		pwmxdevice;

	// yuck.  These are in a parent class
	virtual status_t			ReadRegister(uint32 reg,
									size_t size, uint32* buffer)
									{ return NULL; };
	virtual status_t			WriteRegister(uint32 reg,
									size_t size, uint32* buffer)
									{ return NULL; };
	virtual status_t			BizarroReadRegister(uint32 reg,
									size_t size, uint32* buffer)
									{ return NULL; };
	virtual status_t			BizarroWriteRegister(uint32 reg,
									size_t size, uint32* buffer)
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


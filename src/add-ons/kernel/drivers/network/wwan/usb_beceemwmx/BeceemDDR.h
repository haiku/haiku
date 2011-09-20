/*
 *  Beceem WiMax USB Driver.
 *  Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *  Distributed under the terms of the MIT license.
 *
 *	Description: Wrangle Beceem volatile DDR memory.
 */
#ifndef _USB_BECEEM_DDR_H_
#define _USB_BECEEM_DDR_H_


#include <ByteOrder.h>
#include "DeviceStruct.h"


class BeceemDDR {
public:
								BeceemDDR();
			status_t			DDRInit(WIMAX_DEVICE* swmxdevice);

			WIMAX_DEVICE*		fWmxDevice;

	// yuck.  These are in a child class class
	virtual status_t			ReadRegister(uint32 reg, size_t size,
									uint32* buffer) { return NULL; };
	virtual status_t			WriteRegister(uint32 reg, size_t size,
									uint32* buffer) { return NULL; };
	virtual status_t			BizarroReadRegister(uint32 reg,
									size_t size, uint32* buffer)
										{ return NULL; };
	virtual status_t			BizarroWriteRegister(uint32 reg,
									size_t size, uint32* buffer)
										{ return NULL; };

};


#endif /* _USB_BECEEM_DDR_H_ */

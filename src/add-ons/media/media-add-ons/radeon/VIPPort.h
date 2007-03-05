/******************************************************************************
/
/	File:			VIP.h
/
/	Description:	ATI Radeon Video Input Port (VIP) interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __VIP_PORT_H__
#define __VIP_PORT_H__

#include "Radeon.h"

enum vip_port_device {
	C_VIP_PORT_DEVICE_0 		= 0,
	C_VIP_PORT_DEVICE_1 		= 1,
	C_VIP_PORT_DEVICE_2 		= 2,
	C_VIP_PORT_DEVICE_3 		= 3
};

enum vip_port_register {
	C_VIP_VENDOR_DEVICE_ID		= 0x0000,
		C_VIP_VENDOR_ID			= BITS(15:0),
		C_VIP_DEVICE_ID			= BITS(31:16),
	
	C_VIP_SUB_VENDOR_DEVICE_ID	= 0x0004,
		C_VIP_SUB_VENDOR_ID		= BITS(15:0),
		C_VIP_SUB_DEVICE_ID		= BITS(31:16),
		
	C_VIP_COMMAND_STATUS		= 0x0008,
		C_VIP_POWER_ST			= BITS(1:0),
		C_VIP_XHOST_ON			= BITS(2:2),
		C_VIP_P1_SUPPORT		= BITS(16:16),
		C_VIP_P2_SUPPORT		= BITS(17:17),
		C_VIP_HOST_CAP			= BITS(19:18),
	
	C_VIP_REVISION_ID			= 0x000c,
		C_VIP_REVISION_ID_MASK	= BITS(15:0)
};
	
class CVIPPort {
public:
	CVIPPort(CRadeon & radeon);

	~CVIPPort();
	
	status_t InitCheck() const;
	
	CRadeon & Radeon();
	
	int Register(int device, int address) {
		return fRadeon.VIPRegister( device, address );
	}
	
	void SetRegister(int device, int address, int value) {
		fRadeon.SetVIPRegister( device, address, value );
	}
	
	int ReadFifo(int device, uint32 address, uint32 count, uint8 *buffer) {
		return fRadeon.VIPReadFifo( device, address, count, buffer );
	}
	
	int WriteFifo(int device, uint32 address, uint32 count, uint8 *buffer) {
		return fRadeon.VIPWriteFifo( device, address, count, buffer );
	}
	
	int FindVIPDevice( uint32 device_id ) {
		return fRadeon.FindVIPDevice( device_id );
	}

private:
	CRadeon & fRadeon;
};

#endif

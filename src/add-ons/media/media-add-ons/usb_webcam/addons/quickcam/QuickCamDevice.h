/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _QUICK_CAM_DEVICE_H
#define _QUICK_CAM_DEVICE_H

#include "CamDevice.h"

#define STV_REG_COUNT	0x0c
// Control registers of the STV0600 ASIC
#define STV_I2C_WRITE   0x400
#define STV_I2C_WRITE1  0x400
#define STV_I2C_READ   0x1410
#define STV_ISO_ENABLE 0x1440
#define STV_SCAN_RATE  0x1443
#define STV_ISO_SIZE   0x15c1
#define STV_Y_CTRL     0x15c3
#define STV_X_CTRL     0x1680
#define STV_REG00      0x1500
#define STV_REG01      0x1501
#define STV_REG02      0x1502
#define STV_REG03      0x1503
#define STV_REG04      0x1504
#define STV_REG23      0x0423


// This class represents each webcam
class QuickCamDevice : public CamDevice {
	public:
						QuickCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device);
						~QuickCamDevice();
	virtual bool		SupportsBulk();
	virtual bool		SupportsIsochronous();
	virtual status_t	StartTransfer();
	virtual status_t	StopTransfer();

	// generic register-like access
	virtual ssize_t		WriteReg(uint16 address, uint8 *data, size_t count=1);
	virtual ssize_t		ReadReg(uint16 address, uint8 *data, size_t count=1, bool cached=false);

	// I2C-like access
	virtual status_t	GetStatusIIC();
	virtual status_t	WaitReadyIIC();
	virtual ssize_t		WriteIIC(uint8 address, uint8 *data, size_t count=1);
	virtual ssize_t		ReadIIC(uint8 address, uint8 *data);
	virtual ssize_t		ReadIIC8(uint8 address, uint8 *data);
	virtual ssize_t		ReadIIC16(uint8 address, uint16 *data);
	virtual status_t	SetIICBitsMode(size_t bits=8);

	private:
	virtual status_t	SendCommand(uint8 dir, uint8 request, uint16 value,
									uint16 index, uint16 length, void* data);
};

// the addon itself, that instanciate

class QuickCamDeviceAddon : public CamDeviceAddon {
	public:
						QuickCamDeviceAddon(WebCamMediaAddOn* webcam);
	virtual 			~QuickCamDeviceAddon();

	virtual const char	*BrandName();
	virtual QuickCamDevice	*Instantiate(CamRoster &roster, BUSBDevice *from);

};

#endif /* _QUICK_CAM_CAM_DEVICE_H */

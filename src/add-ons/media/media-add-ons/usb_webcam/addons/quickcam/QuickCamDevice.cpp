/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "QuickCamDevice.h"
#include "CamDebug.h"
#include "CamSensor.h"


const usb_webcam_support_descriptor kSupportedDevices[] = {
{{ 0, 0, 0, 0x046d, 0x0840 }, "Logitech", "QuickCam Express", NULL },
{{ 0, 0, 0, 0x046d, 0x0850 }, "Logitech", "QuickCam Express LEGO", NULL },
{{ 0, 0, 0, 0, 0}, NULL, NULL, NULL }
};



QuickCamDevice::QuickCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device)
          :CamDevice(_addon, _device)
{
	status_t err;

	// linux seems to infer this sets I2C controller to 8 or 16 bit mode...
	// sensors will set to the mode they want when probing
	SetIICBitsMode(8);
	err = ProbeSensor();
	if (err < B_OK) {
		// reset I2C mode to 8 bit as linux driver does
		SetIICBitsMode(8);
		// not much we can do anyway
	}

	fInitStatus = B_OK;
}


QuickCamDevice::~QuickCamDevice()
{
	
}


bool
QuickCamDevice::SupportsBulk()
{
	return true;
}


bool
QuickCamDevice::SupportsIsochronous()
{
	return true;
}


status_t
QuickCamDevice::StartTransfer()
{
	SetScale(1);
	if (Sensor())
		SetVideoFrame(BRect(0, 0, Sensor()->MaxWidth()-1, Sensor()->MaxHeight()-1));
	
	//SetVideoFrame(BRect(0, 0, 320-1, 240-1));

DumpRegs();
#if 0
	err = ReadReg(SN9C102_CHIP_CTRL, &r, 1, true);
	if (err < 0)
		return err;
	r |= 0x04;
	err = WriteReg8(SN9C102_CHIP_CTRL, r);
	if (err < 0)
		return err;
#endif
	return CamDevice::StartTransfer();
}


status_t
QuickCamDevice::StopTransfer()
{
	status_t err;
	
DumpRegs();
	err = CamDevice::StopTransfer();
#if 0
//	if (err < 0)
//		return err;
	err = ReadReg(SN9C102_CHIP_CTRL, &r, 1, true);
	if (err < 0)
		return err;
	r &= ~0x04;
	err = WriteReg8(SN9C102_CHIP_CTRL, r);
	if (err < 0)
		return err;
#endif
	return err;
}


ssize_t
QuickCamDevice::WriteReg(uint16 address, uint8 *data, size_t count)
{
	PRINT((CH "(%u, @%p, %" B_PRIuSIZE ")" CT, address, data, count));
	return SendCommand(USB_REQTYPE_DEVICE_OUT, 0x04, address, 0, count, data);
}


ssize_t
QuickCamDevice::ReadReg(uint16 address, uint8 *data, size_t count, bool cached)
{
	PRINT((CH "(%u, @%p, %" B_PRIuSIZE ", %d)" CT, address, data, count,
		cached));
	memset(data, 0xaa, count); // linux drivers do that without explaining why !?
	return SendCommand(USB_REQTYPE_DEVICE_IN, 0x04, address, 0, count, data);
}


status_t
QuickCamDevice::GetStatusIIC()
{
	status_t err = B_ERROR;
	uint8 status = 0;
#warning WRITEME
	//dprintf(ID "i2c_status: error 0x%08lx, status = %02x\n", err, status);
	if (err < 0)
		return err;
	return (status&0x08)?EIO:0;
}


status_t
QuickCamDevice::WaitReadyIIC()
{
#warning WRITEME
	return EBUSY;
}


ssize_t
QuickCamDevice::WriteIIC(uint8 address, uint8 *data, size_t count)
{
	int i;
	uint8 buffer[0x23];
	if (count > 16)
		return EINVAL;
	memset(buffer, 0, sizeof(buffer));
	buffer[0x20] = Sensor() ? Sensor()->IICWriteAddress() : 0;
	buffer[0x21] = count - 1;
	buffer[0x22] = 0x01;
	for (i = 0; i < count; i++) {
		buffer[i] = address + i;
		buffer[i+16] = data[i];
	}
	return SendCommand(USB_REQTYPE_DEVICE_OUT, 0x04, STV_I2C_WRITE, 0, 0x23, buffer);
}


ssize_t
QuickCamDevice::ReadIIC(uint8 address, uint8 *data)
{
	return ReadIIC(address, data);
}


ssize_t
QuickCamDevice::ReadIIC8(uint8 address, uint8 *data)
{
	status_t err;
	uint8 buffer[0x23];
	memset(buffer, 0, sizeof(buffer));
	buffer[0x20] = Sensor() ? Sensor()->IICReadAddress() : 0;
	buffer[0x21] = 1 - 1;
	buffer[0x22] = 0x03;
	buffer[0] = address;
	err = SendCommand(USB_REQTYPE_DEVICE_OUT, 0x04, STV_I2C_WRITE, 0, 0x23, buffer);
	PRINT((CH ": SendCommand: %s" CT, strerror(err)));
	if (err < B_OK)
		return err;

	buffer[0] = 0xaa;
	err = SendCommand(USB_REQTYPE_DEVICE_IN, 0x04, STV_I2C_READ, 0, 0x1, buffer);
	PRINT((CH ": SendCommand: %s" CT, strerror(err)));
	if (err < B_OK)
		return err;

	*data = buffer[0];
	PRINT((CH ": 0x%02x" CT, *data));
	return 1;
}


ssize_t
QuickCamDevice::ReadIIC16(uint8 address, uint16 *data)
{
	status_t err;
	uint8 buffer[0x23];
	memset(buffer, 0, sizeof(buffer));
	buffer[0x20] = Sensor() ? Sensor()->IICReadAddress() : 0;
	buffer[0x21] = 1 - 1;
	buffer[0x22] = 0x03;
	buffer[0] = address;
	err = SendCommand(USB_REQTYPE_DEVICE_OUT, 0x04, STV_I2C_WRITE, 0, 0x23, buffer);
	if (err < B_OK)
		return err;

	buffer[0] = 0xaa;
	buffer[1] = 0xaa;
	err = SendCommand(USB_REQTYPE_DEVICE_IN, 0x04, STV_I2C_READ, 0, 0x2, buffer);
	PRINT((CH ": SendCommand: %s" CT, strerror(err)));
	if (err < B_OK)
		return err;

	if (fChipIsBigEndian)
		*data = B_HOST_TO_BENDIAN_INT16(*(uint16 *)(&buffer[0]));
	else
		*data = B_HOST_TO_LENDIAN_INT16(*(uint16 *)(&buffer[0]));
	PRINT((CH ": 0x%04x" CT, *data));
	return 2;
}


status_t
QuickCamDevice::SetIICBitsMode(size_t bits)
{
	switch (bits) {
		case 8:
			WriteReg8(STV_REG23, 0);
			break;
		case 16:
			WriteReg8(STV_REG23, 1);
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}


status_t
QuickCamDevice::SendCommand(uint8 dir, uint8 request, uint16 value,
							uint16 index, uint16 length, void* data)
{
	size_t ret;
	if (!GetDevice())
		return ENODEV;
	if (length > GetDevice()->MaxEndpoint0PacketSize())
		return EINVAL;
	ret = GetDevice()->ControlTransfer(
				USB_REQTYPE_VENDOR | dir, 
				request, value, index, length, data);
	return ret;
}


QuickCamDeviceAddon::QuickCamDeviceAddon(WebCamMediaAddOn* webcam)
	: CamDeviceAddon(webcam)
{
	SetSupportedDevices(kSupportedDevices);
}


QuickCamDeviceAddon::~QuickCamDeviceAddon()
{
}


const char *
QuickCamDeviceAddon::BrandName()
{
	return "QuickCam";
}


QuickCamDevice *
QuickCamDeviceAddon::Instantiate(CamRoster &roster, BUSBDevice *from)
{
	return new QuickCamDevice(*this, from);
}


extern "C" status_t
B_WEBCAM_MKINTFUNC(quickcam)
(WebCamMediaAddOn* webcam, CamDeviceAddon **addon)
{
	*addon = new QuickCamDeviceAddon(webcam);
	return B_OK;
}

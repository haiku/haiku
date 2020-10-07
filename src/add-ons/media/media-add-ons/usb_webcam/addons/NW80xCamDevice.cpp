/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "NW80xCamDevice.h"
#include "CamDebug.h"
#include "CamSensor.h"

// reference drivers:
// http://nw802.cvs.sourceforge.net
// http://nw802.cvs.sourceforge.net/nw802/nw802-2.4/
// http://www.medias.ne.jp/~takam/bsd/NetBSD.html#nw802
// https://dev.openwrt.org/attachment/ticket/2319/nw802-patch.txt
// win binary driver template, readme has interesting info:
// http://www.bulgar-bg.com/Downloads/drivers/PCAMDriver/
// http://www.bulgar-bg.com/Downloads/drivers/PCAMDriver/Readme.txt

const usb_webcam_support_descriptor kSupportedDevices[] = {
{{ 0, 0, 0, 0x046d, 0xd001 }, "Logitech", "QuickCam Pro", "??" }, // Alan's
// other IDs according to nw802 linux driver:
{{ 0, 0, 0, 0x052b, 0xd001 }, "Ezonics", "EZCam Pro", "??" },
{{ 0, 0, 0, 0x055f, 0xd001 }, "Mustek"/*"PCLine"*/, "WCam 300"/*"PCL-W300"*/, "??" },
{{ 0, 0, 0, 0x06a5, 0xd001 }, "Divio", "NW802", "??" },
{{ 0, 0, 0, 0x06a5, 0x0000 }, "Divio", "NW800", "??" },
{{ 0, 0, 0, 0, 0}, NULL, NULL, NULL }
};


#warning TODO!

// datasheets: (scarce)
// http://www.digchip.com/datasheets/parts/datasheet/132/NW800.php
// http://www.digchip.com/datasheets/parts/datasheet/132/NW802.php
// http://web.archive.org/web/*/divio.com/*
// http://web.archive.org/web/20020217173519/divio.com/NW802.html
//
// supported sensors:
// Sensor        Model # Data Width Voltage Timing
// Conexant     CN0352     10 bits   3.3 V  Master
// Elecvision   EVS110K     8 bits   3.3 V  Slave
// HP (Agilent) HDC1000    10 bits   3.3 V  Master
// Hyundai      HB7121B     8 bits   3.3 V  Master
// Pixart       PAS006AC    9 bits   3.3 V  Master
// TASC         TAS5110A    9 bits   3.8 V  Slave
//
// http://www.wifi.com.ar/english/doc/webcam/ov511cameras.html says:
// 06a5 (Divio)  	d800   Etoms ET31X110 (A.K.A Divio NW800)

NW80xCamDevice::NW80xCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device)
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


NW80xCamDevice::~NW80xCamDevice()
{
	
}


bool
NW80xCamDevice::SupportsBulk()
{
	return true;
}


bool
NW80xCamDevice::SupportsIsochronous()
{
	return true;
}


status_t
NW80xCamDevice::StartTransfer()
{
	status_t err;
	uint8 r;
	
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
NW80xCamDevice::StopTransfer()
{
	status_t err;
	uint8 r;
	
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
NW80xCamDevice::WriteReg(uint16 address, uint8 *data, size_t count)
{
	PRINT((CH "(%u, @%p, %" B_PRIuSIZE ")" CT, address, data, count));
	return SendCommand(USB_REQTYPE_DEVICE_OUT, 0x04, address, 0, count, data);
}


ssize_t
NW80xCamDevice::ReadReg(uint16 address, uint8 *data, size_t count, bool cached)
{
	PRINT((CH "(%u, @%p, %" B_PRIuSIZE ", %d)" CT, address, data, count,
		cached));
	memset(data, 0xaa, count); // linux drivers do that without explaining why !?
	return SendCommand(USB_REQTYPE_DEVICE_IN, 0x04, address, 0, count, data);
}


status_t
NW80xCamDevice::GetStatusIIC()
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
NW80xCamDevice::WaitReadyIIC()
{
	status_t err;
#warning WRITEME
	return EBUSY;
}


ssize_t
NW80xCamDevice::WriteIIC(uint8 address, uint8 *data, size_t count)
{
	status_t err;
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
NW80xCamDevice::ReadIIC(uint8 address, uint8 *data)
{
	return ReadIIC(address, data);
}


ssize_t
NW80xCamDevice::ReadIIC8(uint8 address, uint8 *data)
{
	status_t err;
	int i;
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
NW80xCamDevice::ReadIIC16(uint8 address, uint16 *data)
{
	status_t err;
	int i;
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
NW80xCamDevice::SetIICBitsMode(size_t bits)
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
NW80xCamDevice::SendCommand(uint8 dir, uint8 request, uint16 value,
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


NW80xCamDeviceAddon::NW80xCamDeviceAddon(WebCamMediaAddOn* webcam)
	: CamDeviceAddon(webcam)
{
	SetSupportedDevices(kSupportedDevices);
}


NW80xCamDeviceAddon::~NW80xCamDeviceAddon()
{
}


const char *
NW80xCamDeviceAddon::BrandName()
{
	return "NW80x-based";
}


NW80xCamDevice *
NW80xCamDeviceAddon::Instantiate(CamRoster &roster, BUSBDevice *from)
{
	return new NW80xCamDevice(*this, from);
}


extern "C" status_t
B_WEBCAM_MKINTFUNC(nw80xcam)
(WebCamMediaAddOn* webcam, CamDeviceAddon **addon)
{
	*addon = new NW80xCamDeviceAddon(webcam);
	return B_OK;
}

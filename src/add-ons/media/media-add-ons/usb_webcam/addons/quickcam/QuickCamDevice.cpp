#include "QuickCamDevice.h"
#include "CamDebug.h"
#include "CamSensor.h"

// see http://www.lrr.in.tum.de/~acher/quickcam/quickcam.html


const usb_named_support_descriptor kSupportedDevices[] = {
{{ 0, 0, 0, 0x046d, 0x0840 }, "Logitech", "QuickCam Express"},
{{ 0, 0, 0, 0x046d, 0x0850 }, "Logitech", "QuickCam Express LEGO"},
{{ 0, 0, 0, 0x046d, 0xd001 }, "Logitech", "QuickCam Express"}, // Alan's
{{ 0, 0, 0, 0, 0}, NULL, NULL }
};


// -----------------------------------------------------------------------------
QuickCamDevice::QuickCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device)
          :CamDevice(_addon, _device)
{
	fInitStatus = B_OK;
}

// -----------------------------------------------------------------------------
QuickCamDevice::~QuickCamDevice()
{
	
}

// -----------------------------------------------------------------------------
bool
QuickCamDevice::SupportsBulk()
{
	return true;
}

// -----------------------------------------------------------------------------
bool
QuickCamDevice::SupportsIsochronous()
{
	return true;
}

// -----------------------------------------------------------------------------
status_t
QuickCamDevice::StartTransfer()
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

// -----------------------------------------------------------------------------
status_t
QuickCamDevice::StopTransfer()
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

// -----------------------------------------------------------------------------
ssize_t
QuickCamDevice::WriteReg(uint16 address, uint8 *data, size_t count)
{
	PRINT((CH "(%u, @%p, %u)" CT, address, data, count));
	return SendCommand(USB_REQTYPE_DEVICE_OUT, 0x04, address, 0, count, data);
}

// -----------------------------------------------------------------------------
ssize_t
QuickCamDevice::ReadReg(uint16 address, uint8 *data, size_t count, bool cached)
{
	PRINT((CH "(%u, @%p, %u, %d)" CT, address, data, count, cached));
	return SendCommand(USB_REQTYPE_DEVICE_IN, 0x04, address, 0, count, data);
}

// -----------------------------------------------------------------------------
status_t
QuickCamDevice::GetStatusIIC()
{
	status_t err;
	uint8 status = 0;
#warning WRITEME
	//dprintf(ID "i2c_status: error 0x%08lx, status = %02x\n", err, status);
	if (err < 0)
		return err;
	return (status&0x08)?EIO:0;
}

// -----------------------------------------------------------------------------
status_t
QuickCamDevice::WaitReadyIIC()
{
	status_t err;
#warning WRITEME
	return EBUSY;
}

// -----------------------------------------------------------------------------
ssize_t
QuickCamDevice::WriteIIC(uint8 address, uint8 *data, size_t count)
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
	return SendCommand(USB_REQTYPE_DEVICE_OUT, 0x04, STV_I2C_WRITE, 0, 0x23, data);
}

// -----------------------------------------------------------------------------
ssize_t
QuickCamDevice::ReadIIC(uint8 address, uint8 *data)
{
#warning WRITEME
	return 1;
}


// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
QuickCamDeviceAddon::QuickCamDeviceAddon(WebCamMediaAddOn* webcam)
	: CamDeviceAddon(webcam)
{
	SetSupportedDevices(kSupportedDevices);
}

// -----------------------------------------------------------------------------
QuickCamDeviceAddon::~QuickCamDeviceAddon()
{
}

// -----------------------------------------------------------------------------
const char *
QuickCamDeviceAddon::BrandName()
{
	return "QuickCam";
}

// -----------------------------------------------------------------------------
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

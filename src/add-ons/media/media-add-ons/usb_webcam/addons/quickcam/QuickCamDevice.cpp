#include "QuickCamDevice.h"

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

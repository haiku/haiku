#include "CamDevice.h"
#include "CamSensor.h"
#include "CamDeframer.h"
#include "CamDebug.h"
#include "AddOn.h"

#include <OS.h>
#include <Autolock.h>

//#define DEBUG_WRITE_DUMP
//#define DEBUG_DISCARD_DATA
//#define DEBUG_READ_DUMP
//#define DEBUG_DISCARD_INPUT

#undef B_WEBCAM_DECLARE_SENSOR
#define B_WEBCAM_DECLARE_SENSOR(sensorclass,sensorname) \
extern "C" CamSensor *Instantiate##sensorclass(CamDevice *cam);
#include "CamInternalSensors.h"
#undef B_WEBCAM_DECLARE_SENSOR
typedef CamSensor *(*SensorInstFunc)(CamDevice *cam);
struct { const char *name; SensorInstFunc instfunc; } kSensorTable[] = {
#define B_WEBCAM_DECLARE_SENSOR(sensorclass,sensorname) \
{ #sensorname, &Instantiate##sensorclass },
#include "CamInternalSensors.h"
{ NULL, NULL },
};
#undef B_WEBCAM_DECLARE_SENSOR

// -----------------------------------------------------------------------------
CamDevice::CamDevice(CamDeviceAddon &_addon, BUSBDevice* _device)
	: fInitStatus(B_NO_INIT),
	  fCamDeviceAddon(_addon),
	  fDevice(_device),
	  fSupportedDeviceIndex(-1),
	  fTransferEnabled(false),
	  fLocker("WebcamDeviceLock"),
	  fSensor(NULL)
{
	// fill in the generic flavor
	memset(&fFlavorInfo, 0, sizeof(fFlavorInfo));
	_addon.WebCamAddOn()->FillDefaultFlavorInfo(&fFlavorInfo);
	// if we use id matching, cache the index to the list
	if (fCamDeviceAddon.SupportedDevices())
	{
		fSupportedDeviceIndex = fCamDeviceAddon.Sniff(_device);
		fFlavorInfoNameStr = "";
		fFlavorInfoNameStr << fCamDeviceAddon.SupportedDevices()[fSupportedDeviceIndex].vendor << " USB Webcam";
		fFlavorInfoInfoStr = "";
		fFlavorInfoInfoStr << fCamDeviceAddon.SupportedDevices()[fSupportedDeviceIndex].vendor;
		fFlavorInfoInfoStr << " (" << fCamDeviceAddon.SupportedDevices()[fSupportedDeviceIndex].product << ") USB Webcam";
		fFlavorInfo.name = (char *)fFlavorInfoNameStr.String();
		fFlavorInfo.info = (char *)fFlavorInfoInfoStr.String();
	}
#ifdef DEBUG_WRITE_DUMP
	fDumpFD = open("/boot/home/webcam.out", O_CREAT|O_RDWR, 0644);
#endif
#ifdef DEBUG_READ_DUMP
	fDumpFD = open("/boot/home/webcam.out", O_RDONLY, 0644);
#endif
	fBufferLen = 1*B_PAGE_SIZE;
	fBuffer = (uint8 *)malloc(fBufferLen);
}

// -----------------------------------------------------------------------------
CamDevice::~CamDevice()
{
	close(fDumpFD);
	free(fBuffer);
	if (fDeframer)
		delete fDeframer;
}
					
// -----------------------------------------------------------------------------
status_t
CamDevice::InitCheck()
{
	return fInitStatus;
}
					
// -----------------------------------------------------------------------------
bool 
CamDevice::Matches(BUSBDevice* _device)
{
	return (_device) == (fDevice);
}

// -----------------------------------------------------------------------------
BUSBDevice*
CamDevice::GetDevice()
{
	return fDevice;
}

// -----------------------------------------------------------------------------
void
CamDevice::Unplugged()
{
	fDevice = NULL;
	fBulkIn = NULL;
}

// -----------------------------------------------------------------------------
bool
CamDevice::IsPlugged()
{
	return (fDevice != NULL);
}

// -----------------------------------------------------------------------------
const char *
CamDevice::BrandName()
{
	if (fCamDeviceAddon.SupportedDevices() && (fSupportedDeviceIndex > -1))
		return fCamDeviceAddon.SupportedDevices()[fSupportedDeviceIndex].vendor;
	return "<unknown>";
}

// -----------------------------------------------------------------------------
const char *
CamDevice::ModelName()
{
	if (fCamDeviceAddon.SupportedDevices() && (fSupportedDeviceIndex > -1))
		return fCamDeviceAddon.SupportedDevices()[fSupportedDeviceIndex].product;
	return "<unknown>";
}

// -----------------------------------------------------------------------------
bool
CamDevice::SupportsBulk()
{
	return false;
}

// -----------------------------------------------------------------------------
bool
CamDevice::SupportsIsochronous()
{
	return false;
}

// -----------------------------------------------------------------------------
status_t
CamDevice::StartTransfer()
{
	status_t err = B_OK;
	PRINT((CH "()" CT));
	if (fTransferEnabled)
		return EALREADY;
	fPumpThread = spawn_thread(_DataPumpThread, "USB Webcam Data Pump", 50, this);
	if (fPumpThread < B_OK)
		return fPumpThread;
	if (fSensor)
		err = fSensor->StartTransfer();
	if (err < B_OK)
		return err;
	fTransferEnabled = true;
	resume_thread(fPumpThread);
	PRINT((CH ": transfer enabled" CT));
	return B_OK;
}

// -----------------------------------------------------------------------------
status_t
CamDevice::StopTransfer()
{
	status_t err = B_OK;
	PRINT((CH "()" CT));
	if (!fTransferEnabled)
		return EALREADY;
	if (fSensor)
		err = fSensor->StopTransfer();
	if (err < B_OK)
		return err;
	fTransferEnabled = false;
	wait_for_thread(fPumpThread, &err);
	return B_OK;
}

// -----------------------------------------------------------------------------
status_t
CamDevice::SetVideoFrame(BRect frame)
{
	fVideoFrame = frame;
	return B_OK;
}

// -----------------------------------------------------------------------------
status_t
CamDevice::SetScale(float scale)
{
	return B_OK;
}

// -----------------------------------------------------------------------------
status_t
CamDevice::SetVideoParams(float brightness, float contrast, float hue, float red, float green, float blue)
{
	return B_OK;
}

// -----------------------------------------------------------------------------
size_t
CamDevice::MinRawFrameSize()
{
	return 0;
}

// -----------------------------------------------------------------------------
size_t
CamDevice::MaxRawFrameSize()
{
	return 0;
}

// -----------------------------------------------------------------------------
bool
CamDevice::ValidateStartOfFrameTag(const uint8 *tag, size_t taglen)
{
	return true;
}

// -----------------------------------------------------------------------------
bool
CamDevice::ValidateEndOfFrameTag(const uint8 *tag, size_t taglen, size_t datalen)
{
	return true;
}

// -----------------------------------------------------------------------------
status_t
CamDevice::GetFrameBitmap(BBitmap **bm)
{
	return EINVAL;
}

// -----------------------------------------------------------------------------
status_t
CamDevice::FillFrameBuffer(BBuffer *buffer)
{
	return EINVAL;
}

// -----------------------------------------------------------------------------
bool
CamDevice::Lock()
{
	return fLocker.Lock();
}

// -----------------------------------------------------------------------------
void
CamDevice::Unlock()
{
	fLocker.Unlock();
}

// -----------------------------------------------------------------------------
ssize_t
CamDevice::WriteReg(uint16 address, uint8 *data, size_t count)
{
	return ENOSYS;
}

// -----------------------------------------------------------------------------
ssize_t
CamDevice::WriteReg8(uint16 address, uint8 data)
{
	return WriteReg(address, &data, sizeof(uint8));
}

// -----------------------------------------------------------------------------
ssize_t
CamDevice::WriteReg16(uint16 address, uint16 data)
{
	// XXX: ENDIAN???
	return WriteReg(address, (uint8 *)&data, sizeof(uint16));
}

// -----------------------------------------------------------------------------
ssize_t
CamDevice::ReadReg(uint16 address, uint8 *data, size_t count, bool cached)
{
	return ENOSYS;
}

// -----------------------------------------------------------------------------
/*
status_t
CamDevice::GetStatusIIC()
{
	return ENOSYS;
}
*/
// -----------------------------------------------------------------------------
/*status_t
CamDevice::WaitReadyIIC()
{
	return ENOSYS;
}
*/
// -----------------------------------------------------------------------------
ssize_t
CamDevice::WriteIIC(uint8 address, uint8 *data, size_t count)
{
	return ENOSYS;
}

// -----------------------------------------------------------------------------
ssize_t
CamDevice::WriteIIC8(uint8 address, uint8 data)
{
	return WriteIIC(address, &data, 1);
}

// -----------------------------------------------------------------------------
ssize_t
CamDevice::ReadIIC(uint8 address, uint8 *data)
{
	return ENOSYS;
}

// -----------------------------------------------------------------------------
CamSensor *
CamDevice::CreateSensor(const char *name)
{
	int i;
	for (i = 0; kSensorTable[i].name; i++) {
		if (!strcmp(kSensorTable[i].name, name))
			return kSensorTable[i].instfunc(this);
	}
	return NULL;
}

// -----------------------------------------------------------------------------
void
CamDevice::SetDataInput(BDataIO *input)
{
	fDataInput = input;
}

// -----------------------------------------------------------------------------
status_t
CamDevice::DataPumpThread()
{
	if (SupportsBulk()) {
		PRINT((CH ": using Bulk" CT));
		while (fTransferEnabled) {
			ssize_t len = -1;
			BAutolock lock(fLocker);
			if (!lock.IsLocked())
				break;
			if (!fBulkIn)
				break;
#ifndef DEBUG_DISCARD_INPUT
			len = fBulkIn->BulkTransfer(fBuffer, fBufferLen);
#endif
			
			//PRINT((CH ": got %d bytes" CT, len));
#ifdef DEBUG_WRITE_DUMP
			write(fDumpFD, fBuffer, len);
#endif
#ifdef DEBUG_READ_DUMP
			if ((len = read(fDumpFD, fBuffer, fBufferLen)) < fBufferLen)
				lseek(fDumpFD, 0LL, SEEK_SET);
#endif

			if (len <= 0) {
				PRINT((CH ": BulkIn: %s" CT, strerror(len)));
				break;
			}

#ifndef DEBUG_DISCARD_DATA
			if (fDataInput) {
				fDataInput->Write(fBuffer, len);
				// else drop
			}
#endif
			//snooze(2000);
		}
	}
	if (SupportsIsochronous()) {
		;//XXX: TODO
	}
	return B_OK;
}

// -----------------------------------------------------------------------------
int32
CamDevice::_DataPumpThread(void *_this)
{
	CamDevice *dev = (CamDevice *)_this;
	return dev->DataPumpThread();
}

// -----------------------------------------------------------------------------
void
CamDevice::DumpRegs()
{
}

// -----------------------------------------------------------------------------
CamDeviceAddon::CamDeviceAddon(WebCamMediaAddOn* webcam)
	: fWebCamAddOn(webcam),
	  fSupportedDevices(NULL)
{
}

// -----------------------------------------------------------------------------
CamDeviceAddon::~CamDeviceAddon()
{
}

// -----------------------------------------------------------------------------
const char *
CamDeviceAddon::BrandName()
{
	return "<unknown>";
}

// -----------------------------------------------------------------------------
status_t
CamDeviceAddon::Sniff(BUSBDevice *device)
{
	PRINT((CH ": Sniffing for %s" CT, BrandName()));
	if (!fSupportedDevices)
		return ENODEV;
	if (!device)
		return EINVAL;
	for (uint32 i = 0; fSupportedDevices[i].vendor; i++)
	{
/*		PRINT((CH "{%u,%u,%u,0x%x,0x%x} <> {%u,%u,%u,0x%x,0x%x}" CT,
			device.Class(), device.Subclass(), device.Protocol(), device.VendorID(), device.ProductID(),
			fSupportedDevices[i].desc.dev_class, fSupportedDevices[i].desc.dev_subclass, fSupportedDevices[i].desc.dev_protocol, fSupportedDevices[i].desc.vendor, fSupportedDevices[i].desc.product));*/
/*		if (device.Class() != fSupportedDevices[i].desc.dev_class)
			continue;
		if (device.Subclass() != fSupportedDevices[i].desc.dev_subclass)
			continue;
		if (device.Protocol() != fSupportedDevices[i].desc.dev_protocol)
			continue;*/
		if (device->VendorID() != fSupportedDevices[i].desc.vendor)
			continue;
		if (device->ProductID() != fSupportedDevices[i].desc.product)
			continue;
		return i;
	}
	return ENODEV;
}

// -----------------------------------------------------------------------------
CamDevice *
CamDeviceAddon::Instantiate(CamRoster &roster, BUSBDevice *from)
{
	return NULL;
}

// -----------------------------------------------------------------------------
void
CamDeviceAddon::SetSupportedDevices(const usb_named_support_descriptor *devs)
{
	fSupportedDevices = devs;
}


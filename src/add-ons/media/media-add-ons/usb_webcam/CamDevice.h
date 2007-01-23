#ifndef _CAM_DEVICE_H
#define _CAM_DEVICE_H

#include <OS.h>
#include <image.h>
#include <usb/USBKit.h>
#include <Locker.h>
#include <media/MediaAddOn.h>
#include <support/String.h>


typedef struct {
	usb_support_descriptor desc;
	const char *vendor;
	const char *product;
} usb_named_support_descriptor;

class CamRoster;
class CamDeviceAddon;
class CamSensor;
class CamDeframer;
class WebCamMediaAddOn;
class BBitmap;
class BBuffer;

// This class represents each webcam
class CamDevice
{
	public: 
						CamDevice(CamDeviceAddon &_addon, BUSBDevice* _device);
	virtual				~CamDevice();

	virtual status_t	InitCheck();
		bool			Matches(BUSBDevice* _device);
		BUSBDevice*		GetDevice();
	virtual void		Unplugged(); // called before the BUSBDevice deletion
	virtual bool		IsPlugged(); // asserts on-line hardware
	
	virtual const char*	BrandName();
	virtual const char*	ModelName();
	const flavor_info*	FlavorInfo() const { return &fFlavorInfo; };
	virtual bool		SupportsBulk();
	virtual bool		SupportsIsochronous();
	virtual status_t	StartTransfer();
	virtual status_t	StopTransfer();
	virtual bool		TransferEnabled() const { return fTransferEnabled; };

	virtual status_t	SetVideoFrame(BRect rect);
	virtual BRect		VideoFrame() const { return fVideoFrame; };
	virtual status_t	SetScale(float scale);
	virtual status_t	SetVideoParams(float brightness, float contrast, float hue, float red, float green, float blue);

	// for use by deframer
	virtual size_t		MinRawFrameSize();
	virtual size_t		MaxRawFrameSize();
	virtual bool		ValidateStartOfFrameTag(const uint8 *tag, size_t taglen);
	virtual bool		ValidateEndOfFrameTag(const uint8 *tag, size_t taglen, size_t datalen);

	// several ways to get raw frames
	virtual status_t	GetFrameBitmap(BBitmap **bm);
	virtual status_t	FillFrameBuffer(BBuffer *buffer);

	// locking
	bool				Lock();
	void				Unlock();
	BLocker*			Locker() { return &fLocker; };

	// sensor chip handling
	CamSensor*			Sensor() const { return fSensor; };

	// generic register-like access
	virtual ssize_t		WriteReg(uint16 address, uint8 *data, size_t count=1);
	virtual ssize_t		WriteReg8(uint16 address, uint8 data);
	virtual ssize_t		WriteReg16(uint16 address, uint16 data);
	virtual ssize_t		ReadReg(uint16 address, uint8 *data, size_t count=1, bool cached=false);
	
	// I2C-like access
	//virtual status_t	GetStatusIIC();
	//virtual status_t	WaitReadyIIC();
	virtual ssize_t		WriteIIC(uint8 address, uint8 *data, size_t count);
	virtual ssize_t		WriteIIC8(uint8 address, uint8 data);
	virtual ssize_t		ReadIIC(uint8 address, uint8 *data);
	
	
	void				SetDataInput(BDataIO *input);
	virtual status_t	DataPumpThread();
	static int32		_DataPumpThread(void *_this);
	
	virtual void		DumpRegs();
	
	protected:
	CamSensor			*CreateSensor(const char *name);
		status_t		fInitStatus;
		flavor_info		fFlavorInfo;
		media_format	fMediaFormat;
		BString			fFlavorInfoNameStr;
		BString			fFlavorInfoInfoStr;
		CamSensor*		fSensor;
		CamDeframer*	fDeframer;
		BDataIO*		fDataInput; // where data from usb goes, likely fDeframer
		const BUSBEndpoint*	fBulkIn;

	private:
friend class CamDeviceAddon;
		CamDeviceAddon&	fCamDeviceAddon;
		BUSBDevice*		fDevice;
		int				fSupportedDeviceIndex;
		bool			fTransferEnabled;
		thread_id		fPumpThread;
		BLocker			fLocker;
		uint8			*fBuffer;
		size_t			fBufferLen;
		BRect			fVideoFrame;
		int fDumpFD;
};

// the addon itself, that instanciate

class CamDeviceAddon
{
	public:
						CamDeviceAddon(WebCamMediaAddOn* webcam);
	virtual 			~CamDeviceAddon();
						
	virtual const char*	BrandName();
	virtual status_t	Sniff(BUSBDevice *device);
	virtual CamDevice*	Instantiate(CamRoster &roster, BUSBDevice *from);

	void				SetSupportedDevices(const usb_named_support_descriptor *devs);
	const usb_named_support_descriptor*	SupportedDevices() const { return fSupportedDevices; };
	WebCamMediaAddOn*	WebCamAddOn() const { return fWebCamAddOn; };

	private:
	WebCamMediaAddOn*	fWebCamAddOn;
	const usb_named_support_descriptor*	fSupportedDevices; // last is {{0,0,0,0,0}, NULL, NULL}
};

// internal modules
#define B_WEBCAM_MKINTFUNC(modname) \
get_webcam_addon_##modname

// external addons -- UNIMPLEMENTED
extern "C" status_t get_webcam_addon(WebCamMediaAddOn* webcam, CamDeviceAddon **addon);
#define B_WEBCAM_ADDON_INSTANTIATION_FUNC_NAME "get_webcam_addon"


#endif _CAM_DEVICE_H

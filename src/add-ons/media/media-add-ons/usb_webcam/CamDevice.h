/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CAM_DEVICE_H
#define _CAM_DEVICE_H


#include <OS.h>
#include <image.h>
#ifdef __HAIKU__
//#if 1
#	include <USB3.h>
#	include <USBKit.h>
#	define SUPPORT_ISO
#else
#	include <USB.h>
#	include <usb/USBKit.h>
#endif
#include <Locker.h>
#include <MediaAddOn.h>
#include <String.h>
#include <Rect.h>

class BBitmap;
class BBuffer;
class BDataIO;
class BParameterGroup;
class CamRoster;
class CamDeviceAddon;
class CamSensor;
class CamDeframer;
class WebCamMediaAddOn;


typedef struct {
	usb_support_descriptor desc;
	const char *vendor;
	const char *product;
	const char *sensors; // possible sensors this cam uses (comma separated)
} usb_webcam_support_descriptor;

// This class represents each webcam
class CamDevice {
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

	virtual status_t	SuggestVideoFrame(uint32 &width, uint32 &height);
	virtual status_t	AcceptVideoFrame(uint32 &width, uint32 &height);
	virtual status_t	SetVideoFrame(BRect rect);
	virtual BRect		VideoFrame() const { return fVideoFrame; };
	virtual status_t	SetScale(float scale);
	virtual status_t	SetVideoParams(float brightness, float contrast, float hue, float red, float green, float blue);

	virtual void		AddParameters(BParameterGroup *group, int32 &index);
	virtual status_t	GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size);
	virtual status_t	SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size);


	// for use by deframer
	virtual size_t		MinRawFrameSize();
	virtual size_t		MaxRawFrameSize();
	virtual bool		ValidateStartOfFrameTag(const uint8 *tag, size_t taglen);
	virtual bool		ValidateEndOfFrameTag(const uint8 *tag, size_t taglen, size_t datalen);

	// several ways to get raw frames
	virtual status_t	WaitFrame(bigtime_t timeout);
	virtual status_t	GetFrameBitmap(BBitmap **bm, bigtime_t *stamp=NULL);
	virtual status_t	FillFrameBuffer(BBuffer *buffer, bigtime_t *stamp=NULL);

	// locking
	bool				Lock();
	void				Unlock();
	BLocker*			Locker() { return &fLocker; };

	// sensor chip handling
	CamSensor*			Sensor() const { return fSensor; };

	virtual status_t	PowerOnSensor(bool on);

	// generic register-like access
	virtual ssize_t		WriteReg(uint16 address, uint8 *data, size_t count=1);
	virtual ssize_t		WriteReg8(uint16 address, uint8 data);
	virtual ssize_t		WriteReg16(uint16 address, uint16 data);
	virtual ssize_t		ReadReg(uint16 address, uint8 *data, size_t count=1, bool cached=false);

	ssize_t				OrReg8(uint16 address, uint8 data, uint8 mask=0xff);
	ssize_t				AndReg8(uint16 address, uint8 data);
	
	// I2C-like access
	//virtual status_t	GetStatusIIC();
	//virtual status_t	WaitReadyIIC();
	virtual ssize_t		WriteIIC(uint8 address, uint8 *data, size_t count);
	virtual ssize_t		WriteIIC8(uint8 address, uint8 data);
	virtual ssize_t		WriteIIC16(uint8 address, uint16 data);
	virtual ssize_t		ReadIIC(uint8 address, uint8 *data);
	virtual ssize_t		ReadIIC8(uint8 address, uint8 *data);
	virtual ssize_t		ReadIIC16(uint8 address, uint16 *data);
	virtual status_t	SetIICBitsMode(size_t bits=8);
	
	
	void				SetDataInput(BDataIO *input);
	virtual status_t	DataPumpThread();
	static int32		_DataPumpThread(void *_this);
	
	virtual void		DumpRegs();
	
	protected:
	virtual status_t	SendCommand(uint8 dir, uint8 request, uint16 value,
									uint16 index, uint16 length, void* data);
	virtual status_t	ProbeSensor();
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
		const BUSBEndpoint*	fIsoIn;
		int32			fFirstParameterID;
		bigtime_t		fLastParameterChanges;

	protected:
		friend class CamDeviceAddon;
		CamDeviceAddon&	fCamDeviceAddon;
		BUSBDevice*		fDevice;
		int				fSupportedDeviceIndex;
		bool			fChipIsBigEndian;
		bool			fTransferEnabled;
		thread_id		fPumpThread;
		BLocker			fLocker;
		uint8			*fBuffer;
		size_t			fBufferLen;
		BRect			fVideoFrame;
		int fDumpFD;
};

// the addon itself, that instanciate

class CamDeviceAddon {
	public:
						CamDeviceAddon(WebCamMediaAddOn* webcam);
	virtual 			~CamDeviceAddon();
						
	virtual const char*	BrandName();
	virtual status_t	Sniff(BUSBDevice *device);
	virtual CamDevice*	Instantiate(CamRoster &roster, BUSBDevice *from);

	void				SetSupportedDevices(const usb_webcam_support_descriptor *devs);
	const usb_webcam_support_descriptor*	SupportedDevices() const
		{ return fSupportedDevices; };
	WebCamMediaAddOn*	WebCamAddOn() const { return fWebCamAddOn; };

	private:
	WebCamMediaAddOn*	fWebCamAddOn;
	const usb_webcam_support_descriptor*	fSupportedDevices; // last is {{0,0,0,0,0}, NULL, NULL, NULL }
};

// internal modules
#define B_WEBCAM_MKINTFUNC(modname) \
get_webcam_addon_##modname

// external addons -- UNIMPLEMENTED
extern "C" status_t get_webcam_addon(WebCamMediaAddOn* webcam,
	CamDeviceAddon **addon);
#define B_WEBCAM_ADDON_INSTANTIATION_FUNC_NAME "get_webcam_addon"

#endif // _CAM_DEVICE_H

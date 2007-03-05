/******************************************************************************
/
/	File:			Theater.h
/
/	Description:	ATI Rage Theater Video Decoder interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/
#ifndef __THEATER_H__
#define __THEATER_H__

#include "Radeon.h"
#include "VIPPort.h"

enum theater_identifier {
//	C_THEATER_VIP_VENDOR_ID			= 0x1002,
	C_THEATER100_VIP_DEVICE_ID			= 0x4D541002,
	C_THEATER200_VIP_DEVICE_ID			= 0x4d4a1002
};


enum theater_standard {
	// TK: rearranged to match spec order
	C_THEATER_NTSC			= 0,
	C_THEATER_NTSC_JAPAN	= 1,
	C_THEATER_NTSC_443		= 2,
	C_THEATER_PAL_M			= 3,
	C_THEATER_PAL_N			= 4,
	C_THEATER_PAL_NC		= 5,
	C_THEATER_PAL_BDGHI		= 6,
	C_THEATER_PAL_60		= 7,
	C_THEATER_SECAM			= 8
};

enum theater_source {
	C_THEATER_TUNER			= 0,
	C_THEATER_COMPOSITE		= 1,
	C_THEATER_SVIDEO		= 2
};

class CTheater {
public:
	CTheater(CRadeon & radeon, int device);
	
	virtual ~CTheater();
	
	virtual status_t InitCheck() const = 0;
	
	virtual void Reset() = 0;
	
	virtual void SetEnable(bool enable, bool vbi) = 0;
	
	virtual void SetStandard(theater_standard standard, theater_source source) = 0;
	
	virtual void SetSize(int hactive, int vactive) = 0;

	virtual void SetDeinterlace(bool deinterlace) = 0;
	
	virtual void SetSharpness(int sharpness) = 0;
	
	virtual void SetBrightness(int brightness) = 0;

	virtual void SetContrast(int contrast) = 0;
	
	virtual void SetSaturation(int saturation) = 0;

	virtual void SetHue(int hue) = 0;

	virtual int CurrentLine() = 0;
	
	virtual void getActiveRange( theater_standard standard, CRadeonRect &rect ) = 0;
	
	virtual void getVBIRange( theater_standard standard, CRadeonRect &rect ) = 0;
		
	virtual void PrintToStream() = 0;
	
	uint32 Capabilities() const;

public:
	int Register(int index);

	int Register(int index, int mask);
	
	void SetRegister(int index, int value);
	
	void SetRegister(int index, int mask, int value);
	
protected:
	CVIPPort fPort;
	int fDevice;
	radeon_video_clock fClock;
	int fTunerPort;
	int fCompositePort;
	int fSVideoPort;
	theater_standard fStandard;
	theater_source fSource;
	int fBrightness;
	int fContrast;
	int fSaturation;
	int fHue;
	int fHActive;
	int fVActive;
	bool fDeinterlace;
};

#endif

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
	C_THEATER_VIP_VENDOR_ID			= 0x1002,
	C_THEATER_VIP_DEVICE_ID			= 0x4d54
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
	CTheater(CRadeon & radeon);
	
	~CTheater();
	
	status_t InitCheck() const;
	
	void Reset();
	
	void SetEnable(bool enable, bool vbi);
	
	void SetStandard(theater_standard standard, theater_source source);
	
	void SetSize(int hactive, int vactive);

	void SetDeinterlace(bool deinterlace);
	
	void SetSharpness(int sharpness);
	
	void SetBrightness(int brightness);

	void SetContrast(int contrast);
	
	void SetSaturation(int saturation);

	void SetHue(int hue);

	int CurrentLine();
	
	void getActiveRange( theater_standard standard, CRadeonRect &rect );
	
	void getVBIRange( theater_standard standard, CRadeonRect &rect );
		
	void PrintToStream();
	
private:
	void SetClock(theater_standard standard, radeon_video_clock clock);

	void SetADC(theater_standard standard, theater_source source);

	void SetHSYNC(theater_standard standard);

	void WaitHSYNC();
	
	void SetVSYNC(theater_standard standard);

	void WaitVSYNC();
	
	void SetSyncGenerator(theater_standard standard);

	void SetCombFilter(theater_standard standard, theater_source source);

	void SetLuminanceProcessor(theater_standard standard);
	
	void SetLuminanceLevels(theater_standard standard, int brightness, int contrast);

	void SetChromaProcessor(theater_standard standard);
		
	void SetChromaLevels(theater_standard standard, int saturation, int hue);

	void SetClipWindow(theater_standard standard, bool vbi);

	void SetScaler(theater_standard standard, int hactive, int vactive, bool deinterlace);

public:
	int Register(int index);

	int Register(int index, int mask);
	
	void SetRegister(int index, int value);
	
	void SetRegister(int index, int mask, int value);
	
private:
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

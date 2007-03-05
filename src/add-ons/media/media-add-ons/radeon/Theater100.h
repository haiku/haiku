/******************************************************************************
/
/	File:			Theater.h
/
/	Description:	ATI Rage Theater Video Decoder interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __THEATER100_H__
#define __THEATER100_H__

#include "Theater.h"
#include "Radeon.h"
#include "VIPPort.h"

class CTheater100 : public CTheater
{
public:
	CTheater100(CRadeon & radeon, int device);
	
	~CTheater100();
	
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
	
};

#endif

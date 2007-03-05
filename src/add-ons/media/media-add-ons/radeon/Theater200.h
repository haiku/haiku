/******************************************************************************
/
/	File:			Theater200.h
/
/	Description:	ATI Rage Theater Video Decoder interface.
/
/	Based on code from X.org
/
*******************************************************************************/

#ifndef __THEATER200_H__
#define __THEATER200_H__

#include "Theater.h"
#include "Radeon.h"
#include "VIPPort.h"

enum theater200_state
{
	MODE_UNINITIALIZED,
	MODE_INITIALIZATION_IN_PROGRESS,
	MODE_INITIALIZED_FOR_TV_IN
};

class CTheater200 : public CTheater {
public:
	CTheater200(CRadeon & radeon, int device);
	
	~CTheater200();
	
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
	
	// help no idea
	void getActiveRange( theater_standard standard, CRadeonRect &rect );
	
	// help no idea.
	void getVBIRange( theater_standard standard, CRadeonRect &rect );
		
	void PrintToStream();
	
private:

	status_t DspInit();
	
	status_t DspLoad( struct rt200_microc_data* microc_datap );
	
	status_t DspGetMicrocode( char* micro_path, 
								char* micro_type, 
									struct rt200_microc_data* microc_datap );
	
	status_t DSPLoadMicrocode( char* micro_path,
								char* micro_type, 
									struct rt200_microc_data* microc_datap );
	
	void DSPCleanMicrocode(struct rt200_microc_data* microc_datap);
	
	status_t DspSendCommand( uint32 fb_scratch1, uint32 fb_scratch0 );
	
	void InitTheatre();
	
	int DSPDownloadMicrocode();
	
	void ShutdownTheatre();
	
	// in accelerant?
	void ResetTheatreRegsForNoTVout();
	void ResetTheatreRegsForTVout();
	
	int32 DspSetVideostreamformat(int32 format);
	int32 DspGetSignalLockStatus();
	int32 DspAudioMute(int8 left, int8 right);
	int32 DspSetAudioVolume(int8 left, int8 right, int8 auto_mute);
	int32 DspConfigureI2SPort(int8 tx_mode, int8 rx_mode, int8 clk_mode);
	int32 DspConfigureSpdifPort(int8 state);
	
	// does nothing now
	void SetClock(theater_standard standard, radeon_video_clock clock){;};

	// source not correct values for PAL NTSC etc...
	void SetADC(theater_standard standard, theater_source source);
	
	// does nothing now
	void SetHSYNC(theater_standard standard){;};

	void WaitHSYNC();
	
	// does nothing now
	void SetVSYNC(theater_standard standard){;};

	void WaitVSYNC();
	
	// does nothing now
	void SetSyncGenerator(theater_standard standard){;};

	// does nothing now
	void SetCombFilter(theater_standard standard, theater_source source){;};

	// does nothing now
	void SetLuminanceProcessor(theater_standard standard){;};
	
	void SetLuminanceLevels(theater_standard standard, int brightness, int contrast);
	
	// does nothing now
	void SetChromaProcessor(theater_standard standard);
		
	void SetChromaLevels(theater_standard standard, int saturation, int hue);

	// does nothing now
	void SetClipWindow(theater_standard standard, bool vbi);

	// um, help
	void SetScaler(theater_standard standard, int hactive, int vactive, bool deinterlace);

public:
	int ReadFifo(uint32 address, uint8 *buffer);
	
	int WriteFifo(uint32 address, uint32 count, uint8 *buffer);
	
private:
	theater200_state fMode;
	char* microcode_path;
	char* microcode_type;
};

#endif

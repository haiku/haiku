/******************************************************************************
/
/	File:			VideoIn.h
/
/	Description:	High-Level ATI Radeon Video Capture Interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __VIDEO_IN_H__
#define __VIDEO_IN_H__

#include "Radeon.h"
#include "Capture.h"
//#include "Overlay.h"
#include "I2CPort.h"
#include "VIPPort.h"
#include "Tuner.h"
#include "MSP3430.h"
#include "Theater.h"
#include "Theater100.h"
#include "Theater200.h"

enum video_in_source {
	C_VIDEO_IN_TUNER,
	C_VIDEO_IN_COMPOSITE,
	C_VIDEO_IN_SVIDEO,
};

enum { C_VIDEO_IN_SOURCE_MAX = 2 };

enum video_in_standard {
	C_VIDEO_IN_NTSC,
	C_VIDEO_IN_NTSC_JAPAN,
	C_VIDEO_IN_NTSC_443,
	C_VIDEO_IN_PAL_M,
	C_VIDEO_IN_PAL_BDGHI,
	C_VIDEO_IN_PAL_N,
	C_VIDEO_IN_PAL_60,
	C_VIDEO_IN_PAL_NC,
	C_VIDEO_IN_SECAM,
	C_VIDEO_IN_NTSC_RAW
};	

enum { C_VIDEO_IN_STANDARD_MAX = 8 };

enum video_in_capture_mode {
	C_VIDEO_IN_FIELD,
	C_VIDEO_IN_BOB,
	C_VIDEO_IN_WEAVE
};

enum { C_VIDEO_IN_CAPTURE_MODE_MAX = 2 };

enum video_in_resolution {
	C_VIDEO_IN_NTSC_SQ_WIDTH	= 640,
	C_VIDEO_IN_NTSC_SQ_HEIGHT	= 480,
	
	C_VIDEO_IN_NTSC_CCIR_WIDTH	= 720,
	C_VIDEO_IN_NTSC_CCIR_HEIGHT	= 480,
	
	C_VIDEO_IN_PAL_SQ_WIDTH		= 768,
	C_VIDEO_IN_PAL_SQ_HEIGHT	= 576,
	
	C_VIDEO_IN_PAL_CCIR_WIDTH	= 720,
	C_VIDEO_IN_PAL_CCIR_HEIGHT	= 576
};

/*enum video_in_frame_rate {
	C_VIDEO_IN_NTSC_FRAME_RATE	= 29976,
	C_VIDEO_IN_PAL_FRAME_RATE	= 25000
};*/

enum video_in_capabilities {
	C_VIDEO_IN_HAS_SOUND		= 1 << 0,
	C_VIDEO_IN_HAS_TUNER		= 1 << 1,
	C_VIDEO_IN_HAS_COMPOSITE	= 1 << 2,
	C_VIDEO_IN_HAS_SVIDEO		= 1 << 3
};

class CVideoIn {
public:
	CVideoIn( const char *dev_name );
	
	~CVideoIn();
	
	status_t InitCheck();

	int Capabilities() const;
	
public:
	void Start(video_in_source source, video_in_standard standard,
			   video_in_capture_mode mode, int width, int height);
	
	void Stop();

	int Capture(color_space colorSpace, void * bits, int bitsLength,
				int bytesPerRow, int * sequence, short * number, bigtime_t * when);
	
public:
	void SetBrightness(int brightness);
	
	void SetContrast(int contrast);
	
	void SetSaturation(int saturation);
	
	void SetHue(int hue);
	
	void SetSharpness(int sharpness);

	void SetFrequency(float frequency, float picture);

	float FrequencyForChannel(int channel, video_in_standard standard);
	
	bool SetChannel(int channel, video_in_standard standard);
	
	int32 getFrameRate( video_in_standard standard );
	
	void getActiveRange( video_in_standard standard, CRadeonRect &rect );
	
private:
	void FreeBuffers();
	
	void Trace(const char * message) const;
		
private:
	CRadeon fRadeon;
	CCapture fCapture;
	CI2CPort fI2CPort;
	CTheater* fTheater;
	CTuner fTuner;
	CMSP3430 fSound;
	int32 fBuffer0;
	int32 fBuffer1;
	int32 fBuffer0Handle;
	int32 fBuffer1Handle;
	void *convert_buffer;
	int fBufferLength;
	int fBufferBytesPerRow;
	int fBufferSequence;
	bigtime_t fBufferPeriod;
	bool started;
};

#endif

/******************************************************************************
/
/	File:			VideoIn.cpp
/
/	Description:	High-Level ATI Radeon Video Capture Interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <Debug.h>
#include "VideoIn.h"
#include <memory.h>

static const theater_standard kStandard[] = {
	C_THEATER_NTSC,
	C_THEATER_NTSC_JAPAN,
	C_THEATER_NTSC_443,
	C_THEATER_PAL_M,
	C_THEATER_PAL_BDGHI,
	C_THEATER_PAL_N,
	C_THEATER_PAL_60,
	C_THEATER_PAL_NC,
	C_THEATER_SECAM,
//	C_THEATER_NTSC_RAW
};

static const theater_source kSource[] = {
	C_THEATER_TUNER,
	C_THEATER_COMPOSITE,
	C_THEATER_SVIDEO
};

static const capture_buffer_mode kMode[] = {
	C_RADEON_CAPTURE_FIELD_DOUBLE,
	C_RADEON_CAPTURE_BOB_DOUBLE,
	C_RADEON_CAPTURE_WEAVE_DOUBLE
};
	
static const struct {
	struct {
		int width, height;
	} total;
	struct {
		int left, top;
		int width, height;
	} active;
	struct {
		int left, top;
		int width, height;
	} vbi;
} kTiming[] = {
	{ {  910, 525 }, { 112, 37, 755, 480 }, {  73, 13, 798, 20 } },	// NTSC-M
	{ {  910, 525 }, { 112, 37, 755, 480 }, {  73, 13, 798, 24 } },	// NTSC-Japan
	{ { 1042, 525 }, { 126, 37, 940, 480 }, {  73, 13, 798, 24 } }, // NTSC-443
	{ {  910, 525 }, { 103, 37, 764, 480 }, {  73, 13, 798, 24 } }, // PAL-M
	
	{ { 1135, 625 }, { 154, 35, 928, 576 }, { 132, 11, 924, 24 } },	// PAL-BDGHI
	{ { 1135, 625 }, { 154, 35, 928, 576 }, { 132, 11, 924, 24 } }, // PAL-N
	{ { 1125, 625 }, { 132, 37, 736, 480 }, { 100, 13, 770, 26 } }, // PAL-60
	{ {  910, 625 }, { 125, 35, 957, 576 }, { 132, 11, 924, 24 } }, // PAL-NC
	{ { 1135, 625 }, { 149, 35, 933, 576 }, { 132, 11, 924, 24 } },	// SECAM

	{ {  910, 525 }, { 112, 37, 755, 480 }, {  73, 13, 798, 22 } }	// NTSC-Raw
};

class CTheater100;
class CTheater200;

CVideoIn::CVideoIn( const char *dev_name )
	:	fRadeon( dev_name ),
		fCapture(fRadeon),
		fI2CPort(fRadeon),
		fTheater(NULL),
		fTuner(fI2CPort),
		fSound(fI2CPort),
		fBuffer0(0),
		fBuffer1(0),
		fBuffer0Handle(0),
		fBuffer1Handle(0),
		convert_buffer( NULL ),
		fBufferLength(0),
		fBufferBytesPerRow(0),
		fBufferSequence(0),
		fBufferPeriod(0),
		started( false )
{

	Trace("CVideoIn::CVideoIn()");

}

CVideoIn::~CVideoIn()
{
	Trace("CVideoIn::~CVideoIn()");

	FreeBuffers();	
}

status_t CVideoIn::InitCheck()
{
	status_t res;
	int device;
	
	Trace("CVideoIn::InitCheck()");
	if( (res = fRadeon.InitCheck()) != B_OK )
		return res;
		
	if( (res = fCapture.InitCheck()) != B_OK )
		return res;
		
	if( (res = fI2CPort.InitCheck()) != B_OK )
		return res;
		
	//debugger("init");
	// detect type of theatre and initialise specific theater class
	if ((device = fRadeon.FindVIPDevice( C_THEATER100_VIP_DEVICE_ID )) != -1)
	{
		Trace("CVideoIn::Found Rage Theater 100");
		fTheater = new CTheater100(fRadeon, device);
	}
	else if ((device = fRadeon.FindVIPDevice( C_THEATER200_VIP_DEVICE_ID )) != -1)
	{
		Trace("CVideoIn::Found Rage Theater 200");
		fTheater = new CTheater200(fRadeon, device);
	}
	
	if (fTheater)
	{
		res = fTheater->InitCheck();
	}
	else
	{
		res = B_ERROR;
	}
		
	return res;
}

int CVideoIn::Capabilities() const
{
	return  fTheater->Capabilities() +
			(fTuner.InitCheck() == B_OK ? C_VIDEO_IN_HAS_TUNER : 0) +
		   (fSound.InitCheck() == B_OK ? C_VIDEO_IN_HAS_SOUND : 0);
}

void CVideoIn::Start(video_in_source source, video_in_standard standard,
					 video_in_capture_mode mode, int width, int height)
{
	char buffer[256];
	sprintf(buffer, "CVideoIn::Start(%s, %d, %d)",
		mode == C_VIDEO_IN_FIELD ? "C_VIDEO_IN_FIELD" :
		mode ==	C_VIDEO_IN_BOB ? "C_VIDEO_IN_BOB" : "C_VIDEO_IN_WEAVE", width, height);
	Trace(buffer);
	
	if( started )
		Stop();
		
	switch (mode) {
	case C_VIDEO_IN_FIELD:
	case C_VIDEO_IN_BOB:
		width = Clamp(width, 0, kTiming[standard].active.width);
		height = Clamp(height, 0, kTiming[standard].active.height / 2);
		break;
	case C_VIDEO_IN_WEAVE:
		width = Clamp(width, 0, kTiming[standard].active.width);
		height = Clamp(height, 0, kTiming[standard].active.height);
		break;
	}		

	fBufferBytesPerRow = (2 * width + 15) & ~15;
	fBufferLength = (fBufferBytesPerRow * height + 15) & ~15;
	
	FreeBuffers();

	// TBD:
	// no error handling !!!!
	fRadeon.AllocateGraphicsMemory( 
		mt_local, 
		mode == C_VIDEO_IN_BOB ? 4 * fBufferLength : 2 * fBufferLength,
		&fBuffer0, &fBuffer0Handle );
		
	fRadeon.AllocateGraphicsMemory( 
		mt_local, 
		mode == C_VIDEO_IN_BOB ? 2 * fBufferLength : 1 * fBufferLength,
		&fBuffer1, &fBuffer1Handle );	
		
	convert_buffer = malloc( mode == C_VIDEO_IN_BOB ? 4 * fBufferLength : 2 * fBufferLength );
	
	fBufferPeriod = 1000000000LL / getFrameRate( standard );
	
	if( mode == C_VIDEO_IN_BOB )
		fBufferPeriod >>= 1;

	fTheater->SetStandard(kStandard[standard], kSource[source]);
	fTheater->SetSize(width, (mode != C_VIDEO_IN_WEAVE ? 2 * height : height));

	fCapture.SetBuffer(C_RADEON_CAPTURE_CCIR656, kMode[mode], fBuffer0, fBuffer1, fBufferLength, fBufferBytesPerRow >> 1);
	fCapture.SetClip(0, kTiming[standard].vbi.height, width - 1, kTiming[standard].vbi.height + (mode != C_VIDEO_IN_WEAVE ? height : height >> 1) - 1);

	fTheater->SetEnable(true, false);
	if( fSound.InitCheck() == B_OK )
		fSound.SetEnable(true);
	fCapture.SetInterrupts(true);
	fCapture.Start();
}

void CVideoIn::Stop()
{
	Trace("CVideoIn::Stop()");
	
	if( !started )
		return;

	fCapture.Stop();
	fCapture.SetInterrupts(false);
	if( fSound.InitCheck() == B_OK )
		fSound.SetEnable(false);	
	fTheater->SetEnable(false, false);
	
	FreeBuffers();
}

void CVideoIn::FreeBuffers()
{
	if( fBuffer0Handle > 0 ) {
		fRadeon.FreeGraphicsMemory( mt_local, fBuffer0Handle );
		fBuffer0Handle = 0;
	}
		
	if( fBuffer1Handle > 0 ) {
		fRadeon.FreeGraphicsMemory( mt_local, fBuffer1Handle );
		fBuffer1Handle = 0;
	}
	
	if( convert_buffer != NULL ) {
		free( convert_buffer );
		convert_buffer = NULL;
	}
}

void CVideoIn::SetBrightness(int brightness)
{
	Trace("CVideoIn::SetBrightness()");
	
	fTheater->SetBrightness(brightness);
}

void CVideoIn::SetContrast(int contrast)
{
	Trace("CVideoIn::SetContrast()");
	
	fTheater->SetContrast(contrast);
}

void CVideoIn::SetSaturation(int saturation)
{
	Trace("CVideoIn::SetSaturation()");
	
	fTheater->SetSaturation(saturation);
}

void CVideoIn::SetHue(int hue)
{
	Trace("CVideoIn::SetHue()");
	
	fTheater->SetHue(hue);
}

void CVideoIn::SetSharpness(int sharpness)
{
	Trace("CVideoIn::SetSharpness()");

	fTheater->SetSharpness(sharpness);
}

void CVideoIn::SetFrequency(float frequency, float picture)
{
	Trace("CVideoIn::SetFrequency()");
	
	if (fTuner.Type() != C_TUNER_NONE)
		fTuner.SweepFrequency(frequency, picture);
}

float CVideoIn::FrequencyForChannel(int channel, video_in_standard standard)
{
	float frequency = 0;

	Trace("CVideoIn::FrequencyForChannel()");

	if (fTuner.Type() != C_TUNER_NONE) {
		switch (standard) {
		case C_VIDEO_IN_NTSC:
		case C_VIDEO_IN_NTSC_RAW:
			// NTSC Cable
			if (channel >= 2 && channel <= 6) {
				frequency = 55.25 + 6.00 * (channel - 2);
			}
			else if (channel >= 7 && channel <= 13) {
				frequency = 175.25 + 6.00 * (channel - 7);
			}
			else if (channel >= 14 && channel <= 22) {
				frequency = 121.25 + 6.00 * (channel - 14);
			}
			else if (channel >= 23 && channel <= 36) {
				frequency = 217.25 + 6.00 * (channel - 23);
			}
			else if (channel >= 37 && channel <= 62) {
				frequency = 301.25 + 6.00 * (channel - 37);
			}
			else if (channel >= 63 && channel <= 94) {
				frequency = 457.25 + 6.00 * (channel - 63);
			}
			else if (channel >= 95 && channel <= 99) {
				frequency = 91.25 + 6.00 * (channel - 95);
			}
			else if (channel >= 100 && channel <= 125) {
				frequency = 649.25 + 6.00 * (channel - 100);
			}
			else {
				frequency = 0;
			}
			break;
		case C_VIDEO_IN_NTSC_JAPAN:
		case C_VIDEO_IN_NTSC_443:
		case C_VIDEO_IN_PAL_M:
		case C_VIDEO_IN_PAL_BDGHI:
		case C_VIDEO_IN_PAL_N:
		case C_VIDEO_IN_PAL_60:
		case C_VIDEO_IN_PAL_NC:
		case C_VIDEO_IN_SECAM:
			break;
		}
	}
	return frequency;
}

bool CVideoIn::SetChannel(int channel, video_in_standard standard)
{
	Trace("CVideoIn::SetChannel()");
	
	if (fTuner.Type() == C_TUNER_NONE)
		return true;

	const float frequency = FrequencyForChannel(channel, standard);
		
	switch (standard) {
	case C_VIDEO_IN_NTSC:
	case C_VIDEO_IN_NTSC_RAW:
		return fTuner.SweepFrequency(frequency, C_TUNER_NTSC_PICTURE_CARRIER / 100.0f);
		break;
	case C_VIDEO_IN_NTSC_JAPAN:
	case C_VIDEO_IN_NTSC_443:
	case C_VIDEO_IN_PAL_M:
	case C_VIDEO_IN_PAL_BDGHI:
	case C_VIDEO_IN_PAL_N:
	case C_VIDEO_IN_PAL_60:
	case C_VIDEO_IN_PAL_NC:
	case C_VIDEO_IN_SECAM:
		return fTuner.SweepFrequency(frequency, C_TUNER_PAL_PICTURE_CARRIER / 100.0f);
	}
	return false;
}

int CVideoIn::Capture(color_space colorSpace, void * bits, int bitsLength,
					  int bytesPerRow, int * sequence, short * number, bigtime_t * when)
{
//	Trace("CVideoIn::Capture()");

	int mask, counter;

	if ((mask = fCapture.WaitInterrupts(sequence, when, fBufferPeriod)) == 0)
		return 0;

	*number = ((mask & (C_RADEON_CAPTURE_BUF0_INT | C_RADEON_CAPTURE_BUF1_INT)) != 0 ? 0 : 1);
	
	int32 captured_buffer = 
		((mask & C_RADEON_CAPTURE_BUF0_INT) != 0 ? fBuffer0 :
		 (mask & C_RADEON_CAPTURE_BUF1_INT) != 0 ? fBuffer1 :
		 (mask & C_RADEON_CAPTURE_BUF0_EVEN_INT) != 0 ? fBuffer0 + fBufferLength :
		 (mask & C_RADEON_CAPTURE_BUF1_EVEN_INT) != 0 ? fBuffer1 + fBufferLength : 0);
		 
	/*PRINT(("colorSpace:%x, bitsLength: %d, fBufferLength: %d, bytesPerRow: %d, fBufferBytesPerRow: %d\n",
		colorSpace, bitsLength, fBufferLength, bytesPerRow, fBufferBytesPerRow ));*/
	
	// always copy into main memory first, even if it must be converted by CPU - 
	// reading from graphics mem is incredibly slow
	if (colorSpace == B_YCbCr422 && bitsLength <= fBufferLength && bytesPerRow == fBufferBytesPerRow) {
		fRadeon.DMACopy( captured_buffer, bits, bitsLength, true, false );
	}
	
	else if (colorSpace == B_RGB32 && bitsLength <= 2 * fBufferLength && bytesPerRow == 2 * fBufferBytesPerRow) {	
		fRadeon.DMACopy( captured_buffer, convert_buffer, fBufferLength, true, false );

#define RGB32
#include "yuv_converter.h"
#undef RGB32

	}
	
	else if (colorSpace == B_RGB16 && bitsLength <= fBufferLength && bytesPerRow == fBufferBytesPerRow) {
		fRadeon.DMACopy( captured_buffer, convert_buffer, fBufferLength, true, false );

#define RGB16
#include "yuv_converter.h"
#undef RGB16

	}
	
	else if (colorSpace == B_RGB15 && bitsLength <= fBufferLength && bytesPerRow == fBufferBytesPerRow) {
		fRadeon.DMACopy( captured_buffer, convert_buffer, fBufferLength, true, false );

#define RGB15
#include "yuv_converter.h"
#undef RGB15

	}
	else if (colorSpace == B_GRAY8 && 2 * bitsLength <= fBufferLength && 2 * bytesPerRow == fBufferBytesPerRow) {
		fRadeon.DMACopy( captured_buffer, convert_buffer, fBufferLength, true, false );
		
		static const unsigned short mask[] = {
			0x00ff, 0x00ff, 0x00ff, 0x00ff };

		asm volatile(
		"1:\n"
			"movq		0x00(%0),%%mm0\n"	// mm0 = Cr2' Y3 Cb2' Y2 Cr0' Y1 Cb0' Y0
			"movq		0x08(%0),%%mm1\n"	// mm1 = Cr6' Y7 Cb6' Y6 Cr4' Y5 Cb4' Y4	
			"pand		%3,%%mm0\n"			// mm0 =  00  Y3  00  Y2  00  Y1  00  Y0
			"pand		%3,%%mm1\n"			// mm1 =  00  Y7  00  Y6  00  Y5  00  Y4
			"packuswb	%%mm1,%%mm0\n"		// mm0 =  Y7  Y6  Y5  Y4  Y3  Y2  Y1  Y0
			"movq		%%mm0,(%1)\n"		// destination[0] = mm0
			"addl		$0x10,%0\n"			// source += 16
			"addl		$0x08,%1\n"			// destination += 8
			"subl		$0x08,%2\n"
			"jg			1b\n"
			"emms\n"
			:
			: "r" (convert_buffer), "r" (bits), "r" (bitsLength), "m" (mask));
	}
	else if( colorSpace == B_NO_COLOR_SPACE ) {
		// special case: only wait for image but don't copy it
		;
	} 
	else {
		PRINT(("CVideoIn::Capture() - Bad buffer format\n"));
	}
	
	counter = *sequence - fBufferSequence;
	fBufferSequence = *sequence;
	return counter;
}

void CVideoIn::Trace(const char * message) const
{
	PRINT(("\x1b[0;30;34m%s\x1b[0;30;47m\n", message));
}

int32 CVideoIn::getFrameRate( video_in_standard standard )
{
	// TODO: I'm not really sure about these values
	static const int32 frame_rate[] = {
		29976, 29976, 29976, 25000, 25000, 25000, 29976, 25000, 25000, 29976
	};
	
	return frame_rate[standard];
}

void CVideoIn::getActiveRange( video_in_standard standard, CRadeonRect &rect )
{
	// in theory, we would ask fTheatre;
	// in practice, values retrieved from there don't work; 
	// e.g. for PAL, according to Theatre, VBI height is 38, but you must set up the
	// clipping unit to height 24! I have no clue what goes wrong there
	rect.SetTo( 
		kTiming[standard].active.left,
		kTiming[standard].active.top,
		kTiming[standard].active.left + kTiming[standard].active.width - 1,
		kTiming[standard].active.top + kTiming[standard].active.height - 1 );
}

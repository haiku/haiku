/******************************************************************************
/
/	File:			Capture.cpp
/
/	Description:	ATI Radeon Capture Unit interface.
/
/	Copyright 2001, Carlos Hasan
/
/	TK:	something about synchronization: I removed all the FIFO wait 
/		functions as they aren't thread-safe and AFAIK not needed as
/		only 2D/3D register accesses are buffered
/
*******************************************************************************/

#include <Debug.h>
#include "Capture.h"

CCapture::CCapture(CRadeon & radeon)
	:	fRadeon(radeon),
		fMode(C_RADEON_CAPTURE_FIELD_SINGLE),
		fFormat(C_RADEON_CAPTURE_CCIR656),
		fOffset0(0),
		fOffset1(0),
		fVBIOffset0(0),
		fVBIOffset1(0),
		fSize(0),
		fVBISize(0),
		fPitch(0),
		fClip(),
		fVBIClip()
{
	PRINT(("CCapture::CCapture()\n"));
}

CCapture::~CCapture()
{
	PRINT(("CCapture::~CCapture()\n"));
}

status_t CCapture::InitCheck() const
{
	return fRadeon.InitCheck();
}

void CCapture::SetBuffer(capture_stream_format format, capture_buffer_mode mode,
						 int offset0, int offset1, int size, int pitch)
{
	PRINT(("CCapture::SetBuffer(%s, %s, 0x%08x, 0x%08x, 0x%08x, %d)\n",
		"BROOKTREE\0CCIR656\0\0\0ZVIDEO\0\0\0\0VIP"+10*format,
		"FIELD-SINGLE\0FIELD-DOUBLE\0BOB-SINGLE\0\0\0"
		"BOB-DOUBLE\0\0\0WEAVE-SINGLE\0WEAVE-DOUBLE"+13*mode,
		offset0, offset1, size, pitch));
	
	fMode = mode;
	fFormat = format;
	fOffset0 = offset0 + fRadeon.VirtualMemoryBase();
	fOffset1 = offset1 + fRadeon.VirtualMemoryBase();
	fSize = size;
	fPitch = pitch;
}

void CCapture::SetClip(int left, int top, int right, int bottom)
{
	PRINT(("CCapture::SetClip(%d, %d, %d, %d)\n",
		left, top, right, bottom));
	
	fClip.SetTo(left, top, right, bottom);
}

void CCapture::SetVBIBuffer(int offset0, int offset1, int size)
{
	PRINT(("CCapture::SetVBIBuffer(0x%08x, 0x%08x, %d)\n", offset0, offset1, size));

	fVBIOffset0 = offset0 + fRadeon.VirtualMemoryBase();
	fVBIOffset1 = offset1 + fRadeon.VirtualMemoryBase();
	fVBISize = size;
}
	
void CCapture::SetVBIClip(int left, int top, int right, int bottom)
{
	PRINT(("CCapture::SetVBIClip(%d, %d, %d, %d)\n",
		left, top, right, bottom));
	
	fVBIClip.SetTo(left, top, right, bottom);
}

void CCapture::Start(bool vbi)
{
	PRINT(("CCapture::Start(%d)\n", vbi));
	
	// initially the capture unit is disabled
	//fRadeon.WaitForFifo(2);
	SetRegister(C_RADEON_CAP0_TRIG_CNTL, C_RADEON_CAP0_TRIGGER_W_NO_ACTION);
	
	// select buffer offset and pitch
	//fRadeon.WaitForFifo(5);
	
	switch (fMode) {
	case C_RADEON_CAPTURE_FIELD_SINGLE:
		/* capture single field, single buffer */
		SetRegister(C_RADEON_CAP0_BUF0_OFFSET, fOffset0);
		SetRegister(C_RADEON_CAP0_BUF_PITCH, fPitch << 1);
		break;

	case C_RADEON_CAPTURE_FIELD_DOUBLE:
		/* capture single field, double buffer */
		SetRegister(C_RADEON_CAP0_BUF0_OFFSET, fOffset0);
		SetRegister(C_RADEON_CAP0_BUF1_OFFSET, fOffset1);
		SetRegister(C_RADEON_CAP0_BUF_PITCH, fPitch << 1);
		break;

	case C_RADEON_CAPTURE_BOB_SINGLE:
		/* capture interlaced frame, single buffer */
		SetRegister(C_RADEON_CAP0_BUF0_OFFSET, fOffset0);
		SetRegister(C_RADEON_CAP0_BUF0_EVEN_OFFSET, fOffset0 + fSize);
		SetRegister(C_RADEON_CAP0_BUF_PITCH, fPitch << 1);
		break;

	case C_RADEON_CAPTURE_BOB_DOUBLE:
		/* capture interlaced frame, double buffer */
		SetRegister(C_RADEON_CAP0_BUF0_OFFSET, fOffset0);
		SetRegister(C_RADEON_CAP0_BUF0_EVEN_OFFSET, fOffset0 + fSize);
		SetRegister(C_RADEON_CAP0_BUF1_OFFSET, fOffset1);
		SetRegister(C_RADEON_CAP0_BUF1_EVEN_OFFSET, fOffset1 + fSize);
		SetRegister(C_RADEON_CAP0_BUF_PITCH, fPitch << 1);
		break;

	case C_RADEON_CAPTURE_WEAVE_SINGLE:
		/* capture deinterlaced frame, single buffer */
		SetRegister(C_RADEON_CAP0_BUF0_OFFSET, fOffset0);
		SetRegister(C_RADEON_CAP0_BUF0_EVEN_OFFSET, fOffset0 + (fPitch << 1));
		SetRegister(C_RADEON_CAP0_BUF_PITCH, fPitch << 2);
		break;

	case C_RADEON_CAPTURE_WEAVE_DOUBLE:
		/* capture deinterlaced frame, double buffer */
		SetRegister(C_RADEON_CAP0_BUF0_OFFSET, fOffset0);
		SetRegister(C_RADEON_CAP0_BUF0_EVEN_OFFSET, fOffset0 + (fPitch << 1));
		SetRegister(C_RADEON_CAP0_BUF1_OFFSET, fOffset1);
		SetRegister(C_RADEON_CAP0_BUF1_EVEN_OFFSET, fOffset1 + (fPitch << 1));
		SetRegister(C_RADEON_CAP0_BUF_PITCH, fPitch << 2);
		break;
	}

	// select VBI buffer offset
	//fRadeon.WaitForFifo(4);
	
	// FIXME: change according to the buffering mode?
	SetRegister(C_RADEON_CAP0_VBI0_OFFSET, fVBIOffset0);
	SetRegister(C_RADEON_CAP0_VBI1_OFFSET, fVBIOffset0 + fVBISize);

	SetRegister(C_RADEON_CAP0_VBI2_OFFSET, fVBIOffset1);
	SetRegister(C_RADEON_CAP0_VBI3_OFFSET, fVBIOffset1 + fVBISize);
		
	// select capture clipping window
	//fRadeon.WaitForFifo(2);

	SetRegister(C_RADEON_CAP0_H_WINDOW,
		((fClip.Left()  <<  1) & C_RADEON_CAP0_H_START) |
		((fClip.Width() << 17) & C_RADEON_CAP0_H_WIDTH));

	SetRegister(C_RADEON_CAP0_V_WINDOW,
		((fClip.Top()    <<  0) & C_RADEON_CAP0_V_START) |
		((fClip.Bottom() << 16) & C_RADEON_CAP0_V_END));

	// select VBI clipping window
	//fRadeon.WaitForFifo(2);
	
	SetRegister(C_RADEON_CAP0_VBI_H_WINDOW,
		((fVBIClip.Left()  <<  0) & C_RADEON_CAP0_VBI_H_START) |
		((fVBIClip.Width() << 16) & C_RADEON_CAP0_VBI_H_WIDTH));

	SetRegister(C_RADEON_CAP0_VBI_V_WINDOW,
		((fVBIClip.Top()    <<  0) & C_RADEON_CAP0_VBI_V_START) |
		((fVBIClip.Bottom() << 16) & C_RADEON_CAP0_VBI_V_END));
		
	// select buffer type, input mode, video format and buffering mode
	//fRadeon.WaitForFifo(10);

	switch (fMode) {
	case C_RADEON_CAPTURE_FIELD_SINGLE:
	case C_RADEON_CAPTURE_FIELD_DOUBLE:
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_BUF_TYPE, C_RADEON_CAP0_BUF_TYPE_FIELD);
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_ONESHOT_MODE, C_RADEON_CAP0_ONESHOT_MODE_FIELD);
		break;
		
	case C_RADEON_CAPTURE_BOB_SINGLE:
	case C_RADEON_CAPTURE_BOB_DOUBLE:
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_BUF_TYPE, C_RADEON_CAP0_BUF_TYPE_ALTERNATING);
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_ONESHOT_MODE, C_RADEON_CAP0_ONESHOT_MODE_FIELD);
		break;
		
	case C_RADEON_CAPTURE_WEAVE_SINGLE:
	case C_RADEON_CAPTURE_WEAVE_DOUBLE:
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_BUF_TYPE, C_RADEON_CAP0_BUF_TYPE_FRAME);
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_ONESHOT_MODE, C_RADEON_CAP0_ONESHOT_MODE_FRAME);
		break;
	}
	
	switch (fMode) {
	case C_RADEON_CAPTURE_FIELD_SINGLE:
	case C_RADEON_CAPTURE_BOB_SINGLE:
	case C_RADEON_CAPTURE_WEAVE_SINGLE:
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_BUF_MODE, C_RADEON_CAP0_BUF_MODE_SINGLE);
		break;
		
	case C_RADEON_CAPTURE_FIELD_DOUBLE:
	case C_RADEON_CAPTURE_BOB_DOUBLE:
	case C_RADEON_CAPTURE_WEAVE_DOUBLE:
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_BUF_MODE, C_RADEON_CAP0_BUF_MODE_DOUBLE);
		break;
	}

	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_INPUT_MODE, C_RADEON_CAP0_INPUT_MODE_CONTINUOUS);
	SetRegister(C_RADEON_CAP0_CONFIG,
		C_RADEON_CAP0_VIDEO_IN_FORMAT | C_RADEON_CAP0_VIDEO_SIGNED_UV, C_RADEON_CAP0_VIDEO_IN_VYUY422);
	
	
	// select stream format and port mode
	//fRadeon.WaitForFifo(4);
	
	switch (fFormat) {
	case C_RADEON_CAPTURE_BROOKTREE:
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_STREAM_FORMAT, C_RADEON_CAP0_STREAM_BROOKTREE);
		SetRegister(C_RADEON_CAP0_PORT_MODE_CNTL, C_RADEON_CAP0_PORT_WIDTH_8_BITS | C_RADEON_CAP0_PORT_LOWER_BYTE_USED);
		break;
	case C_RADEON_CAPTURE_CCIR656:
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_STREAM_FORMAT, C_RADEON_CAP0_STREAM_CCIR656);
		SetRegister(C_RADEON_CAP0_PORT_MODE_CNTL, C_RADEON_CAP0_PORT_WIDTH_8_BITS | C_RADEON_CAP0_PORT_LOWER_BYTE_USED);
		break;
	case C_RADEON_CAPTURE_ZOOMVIDEO:
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_STREAM_FORMAT, C_RADEON_CAP0_STREAM_ZV);
		SetRegister(C_RADEON_CAP0_PORT_MODE_CNTL, C_RADEON_CAP0_PORT_WIDTH_16_BITS | C_RADEON_CAP0_PORT_LOWER_BYTE_USED);
		break;
	case C_RADEON_CAPTURE_VIP:
		SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_STREAM_FORMAT, C_RADEON_CAP0_STREAM_VIP);
		SetRegister(C_RADEON_CAP0_PORT_MODE_CNTL, C_RADEON_CAP0_PORT_WIDTH_16_BITS | C_RADEON_CAP0_PORT_LOWER_BYTE_USED);
		break;
	}

	// set capture mirror mode, field sense, downscaler/decimator, enable 3:4 pull down
	//fRadeon.WaitForFifo(16);
	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_MIRROR_EN, 0);
	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_ONESHOT_MIRROR_EN, 0);
	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_START_FIELD, C_RADEON_CAP0_START_ODD_FIELD);
	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_HORZ_DOWN, C_RADEON_CAP0_HORZ_DOWN_1X);
	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_VERT_DOWN, C_RADEON_CAP0_VERT_DOWN_1X);
	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_VBI_HORZ_DOWN, C_RADEON_CAP0_VBI_HORZ_DOWN_1X);
	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_HDWNS_DEC, C_RADEON_CAP0_DECIMATOR);
	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_SOFT_PULL_DOWN_EN, C_RADEON_CAP0_SOFT_PULL_DOWN_EN);

	// prepare to enable capture
	//fRadeon.WaitForFifo(14);
	
	// disable test and debug modes	
	SetRegister(C_RADEON_TEST_DEBUG_CNTL, 0);
	SetRegister(C_RADEON_CAP0_VIDEO_SYNC_TEST, 0);
	SetRegister(C_RADEON_CAP0_DEBUG, C_RADEON_CAP0_V_SYNC);
	
	// connect capture engine to AMC connector
	SetRegister(C_RADEON_VIDEOMUX_CNTL, 1, 1);

	// select capture engine clock source to PCLK
	SetRegister(C_RADEON_FCP_CNTL, C_RADEON_FCP0_SRC_PCLK);
	
	// enable capture unit
	SetRegister(C_RADEON_CAP0_TRIG_CNTL,
		C_RADEON_CAP0_TRIGGER_W_CAPTURE | C_RADEON_CAP0_EN);
	
	// enable VBI capture
	SetRegister(C_RADEON_CAP0_CONFIG, C_RADEON_CAP0_VBI_EN,
		(vbi ? C_RADEON_CAP0_VBI_EN : 0));
}

void CCapture::Stop()
{
	PRINT(("CCapture::Stop()\n"));

	// disable capture unit
	//fRadeon.WaitForFifo(4);

	// disable capture unit	
	SetRegister(C_RADEON_CAP0_TRIG_CNTL, C_RADEON_CAP0_TRIGGER_W_NO_ACTION);
	
	// disable the capture engine clock (set to ground)
	fRadeon.SetRegister(C_RADEON_FCP_CNTL, C_RADEON_FCP0_SRC_GND);
}

void CCapture::SetInterrupts(bool enable)
{
	PRINT(("CCapture::SetInterrupts(%d)\n", enable));

	fRadeon.SetRegister(C_RADEON_CAP_INT_CNTL,
		C_RADEON_CAP0_BUF0_INT_EN |	C_RADEON_CAP0_BUF0_EVEN_INT_EN |
		C_RADEON_CAP0_BUF1_INT_EN |	C_RADEON_CAP0_BUF1_EVEN_INT_EN |
		C_RADEON_CAP0_VBI0_INT_EN |	C_RADEON_CAP0_VBI1_INT_EN,
		(enable ?	C_RADEON_CAP0_BUF0_INT_EN | C_RADEON_CAP0_BUF0_EVEN_INT_EN |
					C_RADEON_CAP0_BUF1_INT_EN |	C_RADEON_CAP0_BUF1_EVEN_INT_EN |
					C_RADEON_CAP0_VBI0_INT_EN |	C_RADEON_CAP0_VBI1_INT_EN : 0));

	// clear any stick interrupt
	fRadeon.SetRegister(C_RADEON_CAP_INT_STATUS, 
		C_RADEON_CAP0_BUF0_INT_AK |	C_RADEON_CAP0_BUF0_EVEN_INT_AK |
		C_RADEON_CAP0_BUF1_INT_AK |	C_RADEON_CAP0_BUF1_EVEN_INT_AK |
		C_RADEON_CAP0_VBI0_INT_AK |	C_RADEON_CAP0_VBI1_INT_AK);
}

int CCapture::WaitInterrupts(int * sequence, bigtime_t * when, bigtime_t timeout)
{
	int mask;

	if (fRadeon.WaitInterrupt(&mask, sequence, when, timeout) == B_OK) {
		/*
		int mask = fRadeon.Register(C_RADEON_CAP_INT_STATUS);
		
		fRadeon.SetRegister(C_RADEON_CAP_INT_STATUS, 
			C_RADEON_CAP0_BUF0_INT_AK |	C_RADEON_CAP0_BUF0_EVEN_INT_AK |
			C_RADEON_CAP0_BUF1_INT_AK |	C_RADEON_CAP0_BUF1_EVEN_INT_AK |
			C_RADEON_CAP0_VBI0_INT_AK |	C_RADEON_CAP0_VBI1_INT_AK);
		*/
		
		return 
			((mask & C_RADEON_CAP0_BUF0_INT) != 0 ? C_RADEON_CAPTURE_BUF0_INT : 0) |
			((mask & C_RADEON_CAP0_BUF1_INT) != 0 ? C_RADEON_CAPTURE_BUF1_INT : 0) |
			((mask & C_RADEON_CAP0_BUF0_EVEN_INT) != 0 ? C_RADEON_CAPTURE_BUF0_EVEN_INT : 0) |
			((mask & C_RADEON_CAP0_BUF1_EVEN_INT) != 0 ? C_RADEON_CAPTURE_BUF1_EVEN_INT : 0) |
			((mask & C_RADEON_CAP0_VBI0_INT) != 0 ? C_RADEON_CAPTURE_VBI0_INT : 0) |
			((mask & C_RADEON_CAP0_VBI1_INT) != 0 ? C_RADEON_CAPTURE_VBI1_INT : 0);
	}
	return 0;
}

int CCapture::Register(radeon_register index, int mask)
{
	return fRadeon.Register(index, mask);
}

void CCapture::SetRegister(radeon_register index, int value)
{
	fRadeon.SetRegister(index, value);
}

void CCapture::SetRegister(radeon_register index, int mask, int value)
{
	fRadeon.SetRegister(index, mask, value);
}

/******************************************************************************
/
/	File:			Capture.h
/
/	Description:	ATI Radeon Capture Unit interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include "Radeon.h"

enum capture_buffer_mode {
	C_RADEON_CAPTURE_FIELD_SINGLE	= 0,
	C_RADEON_CAPTURE_FIELD_DOUBLE	= 1,
	C_RADEON_CAPTURE_BOB_SINGLE		= 2,
	C_RADEON_CAPTURE_BOB_DOUBLE		= 3,
	C_RADEON_CAPTURE_WEAVE_SINGLE	= 4,
	C_RADEON_CAPTURE_WEAVE_DOUBLE	= 5
};
	
enum capture_stream_format {
	C_RADEON_CAPTURE_BROOKTREE		= 0,
	C_RADEON_CAPTURE_CCIR656		= 1,
	C_RADEON_CAPTURE_ZOOMVIDEO		= 2,
	C_RADEON_CAPTURE_VIP			= 3
};

enum capture_interrupt_mask {
	C_RADEON_CAPTURE_BUF0_INT		= 1 << 0,
	C_RADEON_CAPTURE_BUF0_EVEN_INT	= 1 << 1,
	C_RADEON_CAPTURE_BUF1_INT		= 1 << 2,
	C_RADEON_CAPTURE_BUF1_EVEN_INT	= 1 << 3,
	C_RADEON_CAPTURE_VBI0_INT		= 1 << 4,
	C_RADEON_CAPTURE_VBI1_INT		= 1 << 5
};
	
class CCapture {
public:
	CCapture(CRadeon & radeon);
	
	~CCapture();

	status_t InitCheck() const;
	
	void SetBuffer(capture_stream_format format, capture_buffer_mode mode,
				   int offset0, int offset1, int size, int pitch);

	void SetClip(int left, int top, int right, int bottom);

	void SetVBIBuffer(int offset0, int offset1, int size);
		
	void SetVBIClip(int left, int top, int right, int bottom);
	
	void Start(bool vbi = false);

	void Stop();

public:
	void SetInterrupts(bool enable);
	
	int WaitInterrupts(int * sequence, bigtime_t * when, bigtime_t timeout);
	
private:
	int Register(radeon_register index, int mask);

	void SetRegister(radeon_register index, int value);
	
	void SetRegister(radeon_register index, int mask, int value);
		
private:
	CRadeon & fRadeon;
	capture_buffer_mode fMode;
	capture_stream_format fFormat;
	int fOffset0;
	int fOffset1;
	int fVBIOffset0;
	int fVBIOffset1;
	int fSize;
	int fVBISize;
	int fPitch;
	CRadeonRect fClip;
	CRadeonRect fVBIClip;
};

#endif

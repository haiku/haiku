/******************************************************************************
/
/	File:			CC.h
/
/	Description:	Closed caption module.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __CC_H__
#define __CC_H__

#include "Capture.h"

enum cc_mode {
	C_RADEON_CC1	= 0,
	C_RADEON_CC2	= 1,
	C_RADEON_CC3	= 2,
	C_RADEON_CC4	= 3
};

enum cc_window {
	C_RADEON_CC_VBI_LINE_WIDTH	= 2048,
	C_RADEON_CC_BLANK_START		= 4*112,
	C_RADEON_CC_VBI_LINE_NUMBER	= 19,
	C_RADEON_CC_BIT_DURATION 	= 56,
	C_RADEON_CC_BITS_PER_FIELD	= 16,
	C_RADEON_CC_LEVEL_THRESHOLD = 32,

	C_RADEON_CC_LINE			= 21,
	
	C_RADEON_CC_CHANNELS		= 4,
	C_RADEON_CC_ROWS			= 15,
	C_RADEON_CC_COLUMNS			= 32
};
	
class CCaption {
public:
	CCaption(CCapture & capture);
	
	~CCaption();
	
	status_t InitCheck() const;

	void SetMode(cc_mode mode);

	bool Decode(const unsigned char * buffer0,
				const unsigned char * buffer1);

private:
	bool DecodeBits(const unsigned char * buffer, int & data);
	
	void DisplayCaptions() const;
	
private:
	CCapture & fCapture;
	cc_mode fChannel;
	int fRow;
	int fColumn;
	int fColor;
	int fLastControlCode;
	short fText[C_RADEON_CC_ROWS][C_RADEON_CC_COLUMNS];
	short fDisplayText[C_RADEON_CC_ROWS][C_RADEON_CC_COLUMNS];
};

#endif

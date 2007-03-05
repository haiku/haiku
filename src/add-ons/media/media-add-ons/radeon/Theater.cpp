/******************************************************************************
/
/	File:			Theater.cpp
/
/	Description:	ATI Rage Theater Video Decoder interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <Debug.h>
#include "Theater.h"
#include "VideoIn.h"
#include "TheatreReg.h"
#include "lendian_bitfield.h"

CTheater::CTheater(CRadeon & radeon, int device)
	:	fPort(radeon),
		fDevice(device),
		fClock(C_RADEON_NO_VIDEO_CLOCK),
		fTunerPort(0),
		fCompositePort(0),
		fSVideoPort(0),
		fStandard(C_THEATER_NTSC),
		fSource(C_THEATER_TUNER),
		fBrightness(0),
		fContrast(0),
		fSaturation(0),
		fHue(0),
		fDeinterlace(true)
{
}

CTheater::~CTheater(){};

uint32 CTheater::Capabilities() const
{
	uint32 caps = 0;

	if (fCompositePort)
		caps |= C_VIDEO_IN_HAS_COMPOSITE;
	if (fSVideoPort)
		caps |= C_VIDEO_IN_HAS_SVIDEO;

	return caps;
}

int CTheater::Register(int index)
{
	return fPort.Register(fDevice, index);
}

int CTheater::Register(int index, int mask)
{
	return fPort.Register(fDevice, index) & mask;
}

void CTheater::SetRegister(int index, int value)
{
	fPort.SetRegister(fDevice, index, value);
}

void CTheater::SetRegister(int index, int mask, int value)
{
	if ((value & ~mask) != 0)
		PRINT(("WARNING: CTheater::SetRegister(0x%04x, 0x%08x, 0x%08x)\n", index, mask, value));

	fPort.SetRegister(fDevice, index,
		(fPort.Register(fDevice, index) & ~mask) | (value & mask));
}

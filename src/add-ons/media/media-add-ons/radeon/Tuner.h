/******************************************************************************
/
/	File:			Tuner.h
/
/	Description:	Philips Desktop TV Tuners interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __TUNER_H__
#define __TUNER_H__

#include "I2CPort.h"

enum tuner_type {
    C_TUNER_NONE              		= 0x1400,   /* Unknown */
    C_TUNER_FI1236            		= 0x1401,   /* NTSC M/N */
    C_TUNER_FI1236J           		= 0x1402,   /* NTSC Japan */
    C_TUNER_FI1236MK2         		= 0x1403,   /* NTSC M/N */
    C_TUNER_FI1216            		= 0x1404,   /* PAL B/G */
    C_TUNER_FI1216MK2         		= 0x1405,   /* PAL B/G */
    C_TUNER_FI1216MF          		= 0x1406,   /* PAL B/G, SECAM L/L' */
    C_TUNER_FI1246            		= 0x1407,   /* PAL I */
    C_TUNER_FI1256            		= 0x1408,   /* SECAM D/K */
    C_TUNER_TEMIC_FN5AL_PAL	  		= 0x1409,	/* PAL I/B/G/DK */
    C_TUNER_TEMIC_FN5AL_SECAM 		= 0x140a	/* SECAM DK */
};

enum tuner_picture_carrier {
	C_TUNER_NTSC_PICTURE_CARRIER	= 4575,
	C_TUNER_PAL_PICTURE_CARRIER		= 3890,
	C_TUNER_SECAM_PICTURE_CARRIER	= 3890
};

class CTuner {
public:
	CTuner(CI2CPort & port);
	
	~CTuner();
	
	status_t InitCheck() const;

	const char * Name() const;
	
	tuner_type Type() const;
	
	bool SetFrequency(float frequency, float picture);

	bool SweepFrequency(float frequency, float picture);

	bool HasSignal(void);

	int Status();
	
	bool IsLocked();

	int ADC();
	
private:
	void SetParameters(int divider, int control, int band);
	
private:
	CI2CPort & fPort;
	tuner_type fType;
	int fAddress;
	int fDivider;
};

#endif

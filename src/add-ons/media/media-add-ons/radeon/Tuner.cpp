/******************************************************************************
/
/	File:			Tuner.cpp
/
/	Description:	Philips Desktop TV Tuners interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <Debug.h>
#include "Tuner.h"

enum tuner_control_bits {
    CHANGE_PUMP             = 0x40,     /* change pump settings */
    CHANGE_PUMP_FAST        = 0x00,     /*  for fast tuning */
    CHANGE_PUMP_SLOW        = 0x40,     /*  for moderate speed tuning */

    TEST_MODE               = 0x38,     /* test mode settings */
    TEST_MODE_NORMAL        = 0x08,     /*  for normal operation */

    RATIO_SELECT            = 0x06,     /* ratio select */
    RATIO_SELECT_FAST       = 0x00,     /*  fast picture search */
    RATIO_SELECT_SLOW       = 0x02,     /*  slow picture search */
    RATIO_SELECT_NORMAL     = 0x06,     /*  normal picture search */

    OS_MODE                 = 0x01,     /* PLL disabling */
    OS_MODE_NORMAL          = 0x00,     /*  for normal operation */
    OS_MODE_SWITCH          = 0x01      /*  change pump to high impedance */
};

enum tuner_status_bits {
    STB_POR                 = 0x80,     /* power on reset */
    STB_FL                  = 0x40,     /* in-lock flag */
    STB_INF                 = 0x38,     /* digital information */
    STB_ADC                 = 0x07      /* A/D converter */
};

/*****************************************************************************
/
/   Tuner Parameters and Frequencies for TV Antenna and Cable Channels
/
******************************************************************************/

static const int kBandsPerTuner   = 5;

static const struct {
    tuner_type brand;
    const char* name;
    struct {
        int pll;
        float minFrequency, maxFrequency;
    } bands[kBandsPerTuner];
} kTunerTable[] = {
    /* Philips FI1236 NTSC M/N */
    { C_TUNER_FI1236, "FI1236 NTSC",
      { { 0xa2,  54.00, 157.25 },
        { 0x94, 162.00, 451.25 },
        { 0x34, 451.25, 463.25 },
        { 0x31, 463.25, 801.25 },
        { 0x00,   0.00,   0.00 } } },

    /* Philips FI1236J NTSC Japan */
    { C_TUNER_FI1236J, "FI1236J NTSC Japan",
      { { 0xa2,  54.00, 157.25 },
        { 0x94, 162.00, 451.25 },
        { 0x34, 451.25, 463.25 },
        { 0x31, 463.25, 801.25 },
        { 0x00,   0.00,   0.00 } } },

    /* Philips FI1236MK2 NTSC M/N */
    { C_TUNER_FI1236MK2, "FI1236MK2 NTSC",
      { { 0xa0,  55.25, 160.00 },
        { 0x90, 160.00, 454.00 },
        { 0x30, 454.00, 855.25 },
        { 0x00,   0.00,   0.00 },
        { 0x00,   0.00,   0.00 } } },

    /* Philips FI1216 PAL B/G */
    { C_TUNER_FI1216, "FI1216 PAL",
      { { 0xa2,  45.00, 140.25 },
        { 0xa4, 140.25, 168.25 },
        { 0x94, 168.25, 447.25 },
        { 0x34, 447.25, 463.25 },
        { 0x31, 463.25, 855.25 } } },

    /* Philips FI1216MK2 PAL B/G */
    { C_TUNER_FI1216MK2, "FI1216MK2 PAL",
      { { 0xa0,  48.25, 170.00 },
        { 0x90, 170.00, 450.00 },
        { 0x30, 450.00, 855.25 },
        { 0x00,   0.00,   0.00 },
        { 0x00,   0.00,   0.00 } } },

    /* Philips FI1216MF PAL B/G, SECAM L/L' */
    { C_TUNER_FI1216MF, "FI1216MF PAL/SECAM",
      { { 0xa1,  45.00, 168.25 },
        { 0x91, 168.25, 447.25 },
        { 0x31, 447.25, 855.25 },
        { 0x00,   0.00,   0.00 },
        { 0x00,   0.00,   0.00 } } },

    /* Philips FI1246 PAL I */
    { C_TUNER_FI1246, "FI1246 PAL I",
      { { 0xa2,  45.00, 140.25 },
        { 0xa4, 140.25, 168.25 },
        { 0x94, 168.25, 447.25 },
        { 0x34, 447.25, 463.25 },
        { 0x31, 463.25, 855.25 } } },

    /* Philips FI1256 SECAM D/K */
    { C_TUNER_FI1256, "FI1256 SECAM",
      { { 0xa0,  48.25, 168.25 },
        { 0x90, 175.25, 455.25 },
        { 0x30, 463.25, 863.25 },
        { 0x00,   0.00,   0.00 },
        { 0x00,   0.00,   0.00 } } },

    /* Temic FN5AL PAL I/B/G/DK, SECAM DK */
    { C_TUNER_TEMIC_FN5AL_PAL, "Temic FN5AL PAL",
      { { 0xa2,  45.00, 140.25 },
        { 0xa4, 140.25, 168.25 },
        { 0x94, 168.25, 447.25 },
        { 0x31, 447.25, 855.25 },
        { 0x00,   0.00,   0.00 } } },

    /* Temic FN5AL PAL I/B/G/DK, SECAM DK */
    { C_TUNER_TEMIC_FN5AL_SECAM, "Temic FN5AL SECAM",
      { { 0xa0,  48.25, 168.25 },
        { 0x90, 175.25, 455.25 },
        { 0x30, 463.25, 863.25 },
        { 0x00,   0.00,   0.00 },
        { 0x00,   0.00,   0.00 } } } };

static const int kNumTuners = sizeof(kTunerTable) / sizeof(kTunerTable[0]);


CTuner::CTuner(CI2CPort & port)
	:	fPort(port),
		fType(C_TUNER_NONE),
		fAddress(0xC6),
		fDivider(0)
{
	PRINT(("CTuner::CTuner()\n"));
	
	if( fPort.InitCheck() == B_OK ) {
		radeon_video_tuner tuner;
		radeon_video_decoder video;
		radeon_video_clock clock;
		int dummy;
		
		fPort.Radeon().GetMMParameters(tuner, video, clock, dummy, dummy, dummy);
		switch (tuner) {
		case C_RADEON_NO_TUNER:
			fType = C_TUNER_NONE;
			break;
		case C_RADEON_FI1236_MK1_NTSC:
			fType = C_TUNER_FI1236;
			break;
		case C_RADEON_FI1236_MK2_NTSC_JAPAN:
			fType = C_TUNER_FI1236J;
			break;
		case C_RADEON_FI1216_MK2_PAL_BG:
			fType = C_TUNER_FI1216;
			break;
		case C_RADEON_FI1246_MK2_PAL_I:
			fType = C_TUNER_FI1246;
			break;
		case C_RADEON_FI1216_MF_MK2_PAL_BG_SECAM_L:
			fType = C_TUNER_FI1216MF;
			break;
		case C_RADEON_FI1236_MK2_NTSC:
			fType = C_TUNER_FI1236MK2;
			break;
		case C_RADEON_FI1256_MK2_SECAM_DK:
			fType = C_TUNER_FI1256;
			break;
		case C_RADEON_FI1216_MK2_PAL_BG_SECAM_L:
			fType = C_TUNER_FI1216MK2;
			break;
		case C_RADEON_TEMIC_FN5AL_PAL_IBGDK_SECAM_DK:
			fType = C_TUNER_TEMIC_FN5AL_PAL;
			break;
		default:
			fType = C_TUNER_FI1236;
			break;
		}
	
	    for (fAddress = 0xc0; fAddress <= 0xc6; fAddress += 0x02) {
	        if (fPort.Probe(fAddress)) {
	        	PRINT(("CTuner::CTuner() - TV Tuner found at I2C port 0x%02x\n", fAddress));
				break;
			}
	    }
	}
	if( InitCheck() != B_OK )
		PRINT(("CTuner::CTuner() - TV Tuner not found!\n"));
}

CTuner::~CTuner()
{
	PRINT(("CTuner::~CTuner()\n"));
}

status_t CTuner::InitCheck() const
{
    return (fType != C_TUNER_NONE && fAddress >= 0xc0 && fAddress <= 0xc6) ? 
    	B_OK : B_ERROR;
}

bool CTuner::SetFrequency(float frequency, float picture)
{
	//PRINT(("CTuner::SetFrequency(%g, %g)\n", frequency, picture));

    for (int index = 0; index < kNumTuners; index++) {
        if (kTunerTable[index].brand == fType) {
            for (int band = 0; band < kBandsPerTuner; band++) {
                if (frequency >= kTunerTable[index].bands[band].minFrequency &&
                    frequency <= kTunerTable[index].bands[band].maxFrequency) {
                    SetParameters(
                        (int) (16.0 * (frequency + picture)),
                        CHANGE_PUMP_FAST | RATIO_SELECT_NORMAL |
                        TEST_MODE_NORMAL | OS_MODE_NORMAL,
                        kTunerTable[index].bands[band].pll |
                        ((Status() & STB_INF) >> 3));
                    return true;
                }
            }
        }
    }
    return false;
}

bool CTuner::SweepFrequency(float frequency, float picture)
{
	PRINT(("CTuner::SweepFrequency(%g, %g)\n", frequency, picture));

    float centerFrequency = frequency;

    /* sweeping down */
    do {
        SetFrequency(frequency, picture);
        for (int timeout = 0; timeout < 10; timeout++) {
            snooze(20000);
            if (IsLocked())
                break;
	    }
    } while (ADC() == 0  &&
             (frequency -= 0.5000) > centerFrequency - 3.000);

    /* sweeping up */
    do {
        SetFrequency(frequency, picture);
        for (int timeout = 0; timeout < 10; timeout++) {
            snooze(20000);
            if (IsLocked())
                break;
        }
    } while (ADC() != 0 &&
             (frequency += 0.0625) < centerFrequency + 0.500);

    return (ADC() == 0);
}

const char * CTuner::Name() const
{
    for (int index = 0; index < kNumTuners; index++) {
        if (kTunerTable[index].brand == fType)
            return kTunerTable[index].name;
    }
    return 0;
}

tuner_type CTuner::Type() const
{
	return fType;
}

bool CTuner::HasSignal(void)
{
    return (IsLocked() && ADC() <= 2);
}

int CTuner::Status(void)
{
    char buffer[1];
    fPort.Read(fAddress, buffer, sizeof(buffer));
    return buffer[0] & 0xff;
}

int CTuner::ADC(void)
{
    return Status() & STB_ADC;
}

bool CTuner::IsLocked(void)
{
    return (Status() & STB_FL) != 0;
}

void CTuner::SetParameters(int divider, int control, int band)
{
    char buffer[4];

    if (divider > fDivider) {
        buffer[0] = (divider >> 8) & 0x7f;
        buffer[1] = (divider >> 0) & 0xff;

        buffer[2] = (control | 0x80);
        buffer[3] = (band & 0xff);
    }
    else {
        buffer[0] = (control | 0x80);
        buffer[1] = (band & 0xff);

        buffer[2] = (divider >> 8) & 0x7f;
        buffer[3] = (divider >> 0) & 0xff;
    }

    fDivider = divider;

    fPort.Write(fAddress, buffer, sizeof(buffer));
}

/******************************************************************************
/
/	File:			MSP3430.cpp
/
/	Description:	Micronas Multistandard Sound Processor (MSP) interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <Debug.h>
#include "MSP3430.h"

enum MSP3430_register {
    MSP3430_CONTROL					= 0x00,				// R/W control register
    MSP3430_WR_DEM					= 0x10,				// Write demodulator register
    MSP3430_RD_DEM					= 0x11,				// Read demodulator register
    MSP3430_WR_DSP					= 0x12,				// Read DSP register
    MSP3430_RD_DSP					= 0x13				// Write DSP register
};

enum MSP3430_control_register {
	MSP3430_CONTROL_NORMAL			= 0x0000,			// normal operation mode
	MSP3430_CONTROL_RESET			= 0x8000			// reset mode
};

enum MSP3430_wr_dem_register {
	MSP3430_DEM_STANDARD_SEL		= 0x0020,			// standard selection
	MSP3430_DEM_MODUS				= 0x0030,			// demodulator options
	MSP3430_DEM_I2S_CONFIG			= 0x0040			// I2S configuration
};

enum MSP3430_rd_dem_register {
	MSP3430_DEM_STANDARD_RES		= 0x007e,			// standard detection result
	MSP3430_DEM_STATUS				= 0x0200			// status register
};

enum MSP3430_wr_dsp_register {
	// preprocessing
	MSP3430_DSP_FM_PRESCALE			= 0x000e,			// FM/AM analog signal prescale
		MSP3430_DSP_PRE_FM			= BITS(15:8),
		MSP3430_DSP_FM_MATRIX		= BITS(7:0),
	MSP3430_DSP_NICAM_PRESCALE		= 0x0010,			// NICAM digital signal prescale
		MSP34300_DSP_PRE_NICAM		= BITS(15:8),
	MSP3430_DSP_PRE_I2S2			= 0x0012,			// I2S digital signal prescale
	MSP3430_DSP_PRE_I2S1			= 0x0016,
	MSP3430_DSP_PRE_SCART			= 0x000d,			// SCART prescale

	// source select and output channel matrix
	MSP3430_DSP_SRC_MAT_MAIN		= 0x0008,			// loudspeaker source and matrix
		MSP3430_DSP_SRC_MAIN		= BITS(15:8),
		MSP3430_DSP_MAT_MAIN		= BITS(7:0),
	MSP3430_DSP_SRC_MAT_AUX			= 0x0009,			// headphone source and matrix
		MSP3430_DSP_SRC_AUX			= BITS(15:8),
		MSP3430_DSP_MAT_AUX			= BITS(7:0),
	MSP3430_DSP_SRC_MAT_SCART1		= 0x000a,			// SCART1 source and matrix
		MSP3430_DSP_SRC_SCART1		= BITS(15:8),
		MSP3430_DSP_MAT_SCART1		= BITS(7:0),
	MSP3430_DSP_SRC_MAT_SCART2		= 0x0041,			// SCART2 source and matrix
		MSP3430_DSP_SRC_SCART2		= BITS(15:8),
		MSP3430_DSP_MAT_SCART2		= BITS(7:0),
	MSP3430_DSP_SRC_MAT_I2S			= 0x000b,			// I2S source and matrix
		MSP3430_DSP_SRC_I2S			= BITS(15:8),
		MSP3430_DSP_MAT_I2S			= BITS(7:0),
	MSP3430_DSP_SRC_MAT_QPEAK		= 0x000c,			// QuasiPeak detector source and matrix
		MSP3430_DSP_SRC_QPEAK		= BITS(15:8),
		MSP3430_DSP_MAT_QPEAK		= BITS(7:0),
	
	// loudspeaker and headphone processing
	MSP3430_DSP_VOL_MAIN			= 0x0000,			// loudspeaker volume
	MSP3430_DSP_VOL_AUX				= 0x0006,			// headphone volume
	MSP3430_DSP_AVC					= 0x0029,			// automatic volume correction
	MSP3430_DSP_BAL_MAIN			= 0x0001,			// loudspeaker balance
	MSP3430_DSP_BAL_AUX				= 0x0030,			// headphone balance
	MSP3430_DSP_TONE_MODE			= 0x0020,			// select bass/treble or equalizer
	MSP3430_DSP_BASS_MAIN			= 0x0002,			// loudspeaker bass
	MSP3430_DSP_BASS_AUX			= 0x0031,			// headphone bass
	MSP3430_DSP_TREB_MAIN			= 0x0003,			// loudspeaker treble
	MSP3430_DSP_TREB_AUX			= 0x0032,			// headphone treble
	MSP3430_DSP_EQUAL_BAND1			= 0x0021,			// equalizer coefficients
	MSP3430_DSP_EQUAL_BAND2			= 0x0022,
	MSP3430_DSP_EQUAL_BAND3			= 0x0023,
	MSP3430_DSP_EQUAL_BAND4			= 0x0024,
	MSP3430_DSP_EQUAL_BAND5			= 0x0025,
	MSP3430_DSP_LOUD_MAIN			= 0x0004,			// loudspeaker loudness
	MSP3430_DSP_LOUD_AUX			= 0x0033,			// headphone loudness
	MSP3430_DSP_SPAT_MAIN			= 0x0005,			// spatial effects
	
	// subwoofer output channel
	MSP3430_DSP_SUBW_LEVEL			= 0x002c,			// subwoofer level
	MSP3430_DSP_SUBW_FREQ			= 0x002d,			// subwoofer frequency

	// micronas dynamic bass
	MSP3430_DSP_MDB_STR				= 0x0068,			// MDB strength
	MSP3430_DSP_MDB_LIM				= 0x0069,			// MDB limit
	MSP3430_DSP_MDB_HMC				= 0x006a,			// MDB harmonic content
	MSP3430_DSP_MDB_LP				= 0x006b,			// MDB low frequency
	MSP3430_DSP_MDB_HP				= 0x006c,			// MDB high frequency
	
	// SCART output channel
	MSP3430_DSP_VOL_SCART1			= 0x0007,			// SCART1 volume
	MSP3430_DSP_VOL_SCART2			= 0x0040,			// SCART2 volume
	
	// SCART switches and digital I/O pins
	MSP3430_DSP_ACB_REG				= 0x0013,			// SCART switches
	MSP3430_DSP_BEEPER				= 0x0014			// Beeper volume and frequency
};

enum MSP3430_rd_dsp_register {
	// quasi-peak detector readout
	MSP3430_DSP_QPEAK_L				= 0x0019,			// Quasipeak detector left and right
	MSP3430_DSP_QPEAK_R				= 0x001a,
	
	// MSP 34x0G version readout
	MSP3430_DSP_MSP_HARD_REVISION	= 0x001e,			// MSP hardware and revision
		MSP3430_DSP_MSP_HARD		= BITS(15:8),
		MSP3430_DSP_MSP_REVISION	= BITS(7:0),
	MSP3430_DSP_MSP_PRODUCT_ROM		= 0x001f,			// MSP product and ROM version
		MSP3430_DSP_MSP_PRODUCT		= BITS(15:8),
		MSP3430_DSP_MSP_ROM			= BITS(7:0)
};

enum MSP3430_sound_standard {
	C_MSP3430_AUTOMATIC			= 0x0001,	// Automatic Detection (*)
	C_MSP3430_M_FM_STEREO		= 0x0002,	// NTSC M/N (*)
	C_MSP3430_BG_FM_STEREO		= 0x0003,	// PAL B/G (*)
	C_MSP3430_DK1_FM_STEREO		= 0x0004,	// (*)
	C_MSP3430_DK2_FM_STEREO		= 0x0005,	// 
	C_MSP3430_DK_FM_MONO		= 0x0006,	// 
	C_MSP3430_DK3_FM_STEREO		= 0x0007,	// 
	C_MSP3430_BG_NICAM_FM		= 0x0008,	// PAL B/G (*)
	C_MSP3430_L_NICAM_AM		= 0x0009,	// (*)
	C_MSP3430_I_NICAM_FM		= 0x000A,	// PAL I (*)
	C_MSP3430_DK_NICAM_FM		= 0x000B,	// (*)
	C_MSP3430_DK_NICAM_FM_HDEV2	= 0x000C,
	C_MSP3430_DK_NICAM_FM_HDEV3	= 0x000D,
	C_MSP3430_BTSC_STEREO		= 0x0020,	// BTSC Stereo (*)
	C_MSP3430_BTSC_MONO_SAP		= 0x0021,
	C_MSP3430_M_JAPAN_STEREO	= 0x0030,	// NTSC Japan (*)
	C_MSP3430_FM_RADIO			= 0x0040,	// FM Radio (*)
	C_MSP3430_SAT_MONO			= 0x0050,	// Satellite Mono
	C_MSP3430_SAT_STEREO		= 0x0051,	// Satellite Stereo
	C_MSP3430_SAT_ASTRA_RADIO	= 0x0060	// Astra Digital Radio
};

enum MSP3430_channel_source {
	C_MSP3430_SOURCE_FM			= 0x00,		// FM/AM Mono
	C_MSP3430_SOURCE_STEREO		= 0x01,		// Stereo or A/B (NICAM)
	C_MSP3430_SOURCE_SCART		= 0x02,		// SCART Input
	C_MSP3430_SOURCE_LANG_A		= 0x03,		// Stereo/Language A
	C_MSP3430_SOURCE_LANG_B		= 0x04,		// Stereo/Language B
	C_MSP3430_SOURCE_I2S1		= 0x05,		// I2S1 Input
	C_MSP3430_SOURCE_I2S2		= 0x06		// I2S2 Input
};

enum MSP3430_channel_matrix {
	C_MSP3430_MATRIX_SOUND_A	= 0x00,		// Sound A Mono
	C_MSP3430_MATRIX_SOUND_B	= 0x10,		// Sound B Mono
	C_MSP3430_MATRIX_STEREO		= 0x20,		// Stereo
	C_MSP3430_MATRIX_MONO		= 0x30		// Mono
};



/*

-------------------------------------------------------------------
System			Sound			Sound				Color
				Carrier (MHz)	Modulation			System
-------------------------------------------------------------------
Satellite		6.5/5.85		FM-Mono/NICAM		PAL
				6.5				FM-Mono
				7.02/7.2		FM-Stereo			PAL
				7.38/7.56		ASTRA Digital Radio
				etc.
				4.5/4.724212	FM-Stereo (A2)		NTSC
-------------------------------------------------------------------
NTSC M/N		4.5				FM-FM (EIA-J)		NTSC
-------------------------------------------------------------------
				4.5				BTSC Stereo + SAP	NTSC, PAL
FM-Radio		10.7			FM-Stereo Radio		.
-------------------------------------------------------------------





*/

CMSP3430::CMSP3430(CI2CPort & port)
	:	fPort(port),
		fAddress(0)
{
	PRINT(("CMSP3430::CMSP3430()\n"));
	
	if( fPort.InitCheck() == B_OK ) {
		for (fAddress = 0x80; fAddress <= 0x80; fAddress += 0x02) {
	        if (fPort.Probe(fAddress)) {
	        	PRINT(("CMSP3430::CMSP3430() - Sound Processor found at I2C port 0x%02x\n", fAddress));
				break;
			}
		}
	}
	if( InitCheck() != B_OK )
		PRINT(("CMSP3430::CMSP3430() - Sound processor not found!\n"));
}

CMSP3430::~CMSP3430()
{
	PRINT(("CMSP3430::~CMSP3430()\n"));
}

status_t CMSP3430::InitCheck() const
{
	status_t res;
	
	res = fPort.InitCheck();
	if( res != B_OK )
		return res;
		
	return (fAddress >= 0x80 && fAddress <= 0x80) ? B_OK : B_ERROR;
}

void CMSP3430::SetEnable(bool enable)
{
	PRINT(("CMSP3430::SetEnable(%d)\n", enable));
	
	SetControlRegister(MSP3430_CONTROL_RESET);
	SetControlRegister(MSP3430_CONTROL_NORMAL);

	if (enable) {
		SetRegister(MSP3430_WR_DEM, MSP3430_DEM_MODUS, 0x2003);
		SetRegister(MSP3430_WR_DEM, MSP3430_DEM_STANDARD_SEL, C_MSP3430_BTSC_STEREO); // 0x20
		SetRegister(MSP3430_WR_DSP, MSP3430_DSP_FM_PRESCALE, 0x2403);
		SetRegister(MSP3430_WR_DSP, MSP3430_DSP_SRC_MAT_MAIN, 0x0320);
		SetRegister(MSP3430_WR_DSP, MSP3430_DSP_VOL_MAIN, 0x7300);
		SetRegister(MSP3430_WR_DSP, MSP3430_DSP_SRC_MAT_MAIN, 0x0320);
	}
	else {
		SetRegister(MSP3430_WR_DSP, MSP3430_DSP_VOL_MAIN, 0x0000);
	}
}

#if 0
#pragma mark -

void CMSP3430::SetI2S(bool fast)
{
	// select 2 x 16 bit (1024 MHz) or 2 x 32 bit (2048) frequency
	SetRegister(MSP3430_WR_DEM, I2S_CONFIG, fast ? 0x0001 : 0x0000);
}

bool CMSP3430::IsSAP()
{
	// bilingual sound mode or SAP present?
	if ((Register(MSP3430_RD_DEM, STATUS) & 0x0100) != 0x0000)
		return true;
	return false;
}

bool CMSP3430::IsMonoNICAM()
{
	// independent mono sound (only NICAM)?
	if ((Register(MSP3430_RD_DEM, STATUS) & 0x0080) != 0x0000)
		return true;
	return false;
}

bool CMSP3430::IsStereo()
{
	// mono/stereo indication
	if ((Register(MSP3430_RD_DEM, STATUS) & 0x0040) != 0x0000)
		return true;
	return false;
}

bool CMSP3430::IsFM()
{
	// is analog sound standard (FM or AM) active?
	if ((Register(MSP3430_RD_DEM, STATUS) & 0x0120) == 0x0000)
		return true;
	return false;
}

bool CMSP3430::IsNICAM()
{
	// digital sound (NICAM) available?
	if ((Register(MSP3430_RD_DEM, STATUS) & 0x0100) == 0x0100)
		return true;
	return false;
}

bool CMSP3430::HasSecondaryCarrier()
{
	// detected secondary carrier (2nd A2 or SAP sub-carrier)
	if ((Register(MSP3430_RD_DEM, STATUS) & 0x0004) != 0x0000)
		return true;
	return false;
}	

bool CMSP3430::HasPrimaryCarrier()
{
	// detected primary carrier (Mono or MPX carrier)
	if ((Register(MSP3430_RD_DEM, STATUS) & 0x0002) != 0x0000)
		return true;
	return false;
}

void CMSP3430::SetStandard(MSP3430_standard standard)
{
	fStandard = standard;
	
	// set sound mode, FM/AM matrix, source channels
	switch (standard) {
	C_MSP3430_STANDARD_BG_FM:
	C_MSP3430_STANDARD_DK_FM:
		// Sound Mode	FM Matrix		FM/AM Source	Stereo A/B Source
		// ---------------------------------------------------------------
		// Mono 		Sound A Mono	Mono			Mono
	C_MSP3430_STANDARD_M_KOREA:
	C_MSP3430_STANDARD_M_JAPAN:
		// Stereo 		Korean Stereo	Stereo			Stereo
		// Stereo 		German Stereo	Stereo			Stereo
		// Lang A & B	No Matrix		Left=A, Right=B	Left=A, Right=B
		
	C_MSP3430_STANDARD_BG_NICAM:
	
	
	}
	
	SetDemodulator();
}

void CMSP3430::SetFMPrescale(int gain, MSP3430_fm_matrix matrix, bool mute)
{
	// set FM/AM prescale and FM matrix mode
	fFMPrescale = (mute ? 0x00 : Clamp(gain, 0x00, 0x7f)) << 8;
	/// see table 6--17, Appendix B.
	switch (matrix) {
	case C_MSP3430_MATRIX_NONE:
		fFMPrescale |= 0x0000;
		break;
	case C_MSP3430_MATRIX_GERMAN_STEREO_BG:
		fFMPrescale |= 0x0001;
		break;
	case C_MSP3430_MATRIX_KOREAN_STEREO:
		fFMPrescale |= 0x0002;
		break;
	case C_MSP3430_MATRIX_SOUND_A_MONO:
		fFMPrescale |= 0x0003;
		break;
	case C_MSP3430_MATROX_SOUND_B_MONO:
		fFMPrescale |= 0x0004;
		break;
	}
	
	SetDemodulator();
}

void CMSP3430::SetNICAMPrescale(int gain, bool mute)
{
	// set NICAM prescale to 0..12 dB
	fNICAMPrescale = (mute ? 0x00 : Clamp(0x20 + (0x5f * gain) / 12, 0x20, 0x7f)) << 8;

	SetDemodulator();
}

void CMSP3430::SetI2SPrescale(int gain, bool mute)
{
	// set digital I2S input prescale 0..18 dB
	fI2SPrescale = (mute ? 0x00 : Clamp(0x10 + (0x7f * gain) / 18, 0x10, 0x7f) << 8;
	SetSCARTInput();
}

void CMSP3430::SetSCARTPrescale(int gain, bool mute)
{
	// set SCART input signal prescale 0..14 dB
	fSCARTPrescale = (mute ? 0x00 : Clamp(0x19 + (0x66 * gain) / 14, 0x19, 0x7f) << 8;
	SetSCARTInput();
}

void CMSP3430::SetSource(MSP3430_source_channel speaker,
						 MSP3430_source_channel headphone,
						 MSP3430_source_channel scart1,
						 MSP3430_source_channel scart2,
						 MSP3430_source_channel i2s,
						 MSP3430_source_channel qpeak)
{
	// set source and matrix..
	// select FM/AM, Stereo A/B, Stereo A, Stereo B, SCART, I2S1 or I2S2 source channels
	// and select Sound A Mono, Sound B Mono, Stereo or Mono mode
	fSrcMain = loudspeaker;
	fSrcAux = headphone;
	fSrcSCART1 = scart1;
	fSrcSCART2 = scart2;
	fSrcI2S = i2s;
	fSrcQPeak = qpeak;
	SetOutputChannels();
}


void CMSP3430::SetVolume(int level, bool mute, MSP3430_clipping_mode mode,
				MSP3430_automatic_volume automatic)
{
	// set volume between -114 dB and 12 dB with 1 dB step size
	fVolume = (mute ? 0x00 : Clamp(level + 0x73, 0x01, 0x7f)) << 8;
	switch (mode) {
	case C_MSP3430_CLIP_REDUCE_VOLUME:
		fVolume |= 0x00;
		break;
	case C_MSP3430_CLIP_REDUCE_TONE:
		fVolume |= 0x01;
		break;
	case C_MSP3430_CLIP_COMPROMISE:
		fVolume |= 0x02;
		break;
	case C_MSP3430_CLIP_DYNAMIC:
		fVolume |= 0x03;
		break;
	}
	
	// enable/disable automatic volume correction
	switch (automatic) {
	case C_MSP3430_AUTOMATIC_VOLUME_OFF:
		fAVC = 0x0000;
		break;
	case C_MSP3430_AUTOMATIC_VOLUME_20_MS:
		fAVC = 0x8100;
		break;
	case C_MSP3430_AUTOMATIC_VOLUME_2_SEC:
		fAVC = 0x8200;
		break;
	case C_MSP3430_AUTOMATIC_VOLUME_4_SEC:
		fAVC = 0x8400;
		break;
	case C_MSP3430_AUTOMATIC_VOLUME_8_SEC:
		fAVC = 0x8800;
		break;
	}
	SetOutputChannels();
}

void CMSP3430::SetBalance(int balance, MSP3430_balance_mode mode)
{
	switch (mode) {
	case C_MSP3430_BALANCE_LINEAR:
		// set balance between -100% (right) and 100% (left)
		fBalance = (((0x7f * Clamp(balance, -100, 100)) / 100) << 8) + 0x0000;
		break;
	
	case C_MSP3430_BALANCE_LOGARITHMIC:
		// set balance between -128 dB (right) and 127 dB (left)
		fBalance = (((0x7f * Clamp(balance, -128, 127)) / 127) << 8) + 0x0001;
		break;
	}
	SetOutputChannels();
}


void CMSP3430::SetEqualizer(int bass, int treble)
{
	// set bass to -12..12 dB or 14..20 dB
	if (bass <= 12)
		fBass = Clamp(0x00 + 8 * bass, -0x60, 0x60) << 8;
	else
		fBass = Clamp(0x68 + 4 * (bass - 14), 0x68, 0x7f) << 8;
	
	// set treble to -12..15 dB
	fTreble = Clamp(0x00 + 8 * treble, -0x60, 0x78) << 8;

	// enable bass/treble and disable equalizer
	fToneControl = 0x00;
	fEqualizer[0] = fEqualizer[1] = fEqualizer[2] =
	fEqualizer[3] = fEqualizer[4] = fEqualizer[5] = 0x00;
	
	SetOutputChannels();
}

void CMSP3430::SetEqualizer(int gain[5])
{
	// set five band equalizer (120 Hz, 500 Hz, 1500 Hz, 5000 Hz, 10000 Hz)
	for (int index = 0; index < 5; index++)
		fEqualizer[index] = Clamp(0x00 + 8 * gain[index], -0x60, 0x60) << 8;
	
	// disable bass/treble and enable equalizer
	fToneControl = 0xff;
	fBass = fTreble = 0x00;

	SetOutputChannels();
}

void CMSP3430::SetLoudness(int loudness, MSP3430_loudness_mode mode)
{
	// set loudness gain between 0 dB and 17 dB
	fLoudness = Clamp(0x00 + 4 * loudness, 0x00, 0x44) << 8;
	switch (mode) {
	case C_MSP3430_LOUDNESS_NORMAL:
		fLoudness |= 0x0000;
		break;
	case C_MSP3430_LOUDNESS_SUPER_BASS:
		fLoudness |= 0x0004;
		break;
	}

	SetOutputChannels();
}

void CMSP3430::SetSpatial(int strength, MSP3430_spatial_mode mode, MSP3430_spatial_highpass pass)
{
	// enlarge or reduce spatial effects -100% to +100%
	fSpatial = Clamp(0x00 + (0x7f * strength) / 100, -0x80, 0x7f) << 8;
	switch (mode) {
	case C_MSP3430_SPATIAL_MODE_A:
		fSpatial |= 0x0000;
		break;
	case C_MSP3430_SPATIAL_MODE_B:
		fSpatial |= 0x0020;
		break;
	}
	switch (pass) {
	case C_MSP3430_SPATIAL_MAX_HIGH_PASS:
		fSpatial |= 0x0000;
		break;	
	case C_MSP3430_SPATIAL_2_3_HIGH_PASS:
		fSpatial |= 0x0002;
		break;	
	case C_MSP3430_SPATIAL_1_3_HIGH_PASS:
		fSpatial |= 0x0004;
		break;	
	case C_MSP3430_SPATIAL_MIN_HIGH_PASS:
		fSpatial |= 0x0006;
		break;	
	case C_MSP3430_SPATIAL_AUTO_HIGH_PASS:
		fSpatial |= 0x0008;
		break;	
	}
	SetOutputChannels();
}

void CMSP3430::SetSubwoofer(int level, int frequency, bool mute, MSP3430_subwoofer_mode mode)
{
	// set subwoofer level between -128 dB and 12 dB
	fSubwooferLevel = (mute ? 0x80 : Clamp(0x00 + level, -128, 12)) << 8;
	
	// set subwoofer corner frequency between 50..400 Hz
	fSubwooferFrequency = Clamp(frequency / 10, 5, 40) << 8;
	switch (mode) {
	case C_MSP3430_SUBWOOFER_UNFILTERED:
		fSubwooferFrequency |= 0x0000;
		break;
	case C_MSP3430_SUBWOOFER_HIGH_PASS:
		fSubwooferFrequency |= 0x0001;
		break;
	case C_MSP3430_SUBWOOFER_MDB_MAIN:
		fSubwooferFrequency |= 0x0002;
		break;
	}
	
	// disable MDB?
	fMDBStrength = 0x0000;
	fMDBLimit = 0x0000;
	fMDBHarmonic = 0x0000;
	fMDBHighPass = 0x0a00;
	fMDBLowPass = 0x0a00;
	
	SetSubwooferAndMDBOutputChannels();
}

void CMSP3430::SetMDB(int strength, int limit, int harmonic,
					  int minfrequency, int maxfrequency, bool mute)
{
	// set MDB effect strength 0..127
	fMDBStrength = (mute ? 0x00 : Clamp(strength, 0x00, 0x7f)) << 7;
	
	// set MDB amplitude limit between -32..0 dBFS
	fMDBLimit = Clamp(limit, -32, 0) << 8;
	
	// set MDB harmonic content 0..100%
	fMDBHarmonic = Clamp((0x7f * harmonic) / 100, 0x00, 0x7f) << 8;
	
	// set MDB low pass corner frequency 50..300 Hz
	fMDBLowPass = Clamp(minFrequency / 10, 5, 30) << 8;
	
	// set MDB high pass corner frequency 20..300 Hz
	fMDBHighPass = Clamp(maxFrequency / 10, 2, 30) << 8;
	
	// disable subwoofer level adjustments
	fSubwooferLevel = 0x0000;
	fSubwooferFrequency = 0x0002;
	
	// for MDB, use dynamic clipping mode
	fVolume = (fVolume & ~0x000f) | 0x0003;

	SetSubwooferAndMDBOutputChannels();
}

void CMSP3430::SetSCART(int level1, int level2, bool mute,
						MSP3430_scart_input input,
						MSP3430_scart_output output1,
						MSP3430_scart_output output2)
{
	// set volume SCART1/2 output channel -114..12 dB
	fSCART1Volume = (mute ? 0x00 : Clamp(0x73 + (0x7f * level1) / 12, 0x00, 0x7f)) << 8;
	fSCART2Volume = (mute ? 0x00 : Clamp(0x73 + (0x7f * level2) / 12, 0x00, 0x7f)) << 8;

	fSCART1Volume |= 0x0001;	
	fSCART2Volume |= 0x0001;
	
	// SCART DSP input select
	fACB = 0x0000;
	switch (input) {
	case C_MSP3430_SCART_INPUT_SCART1:
		fACB = 0x0000;
		break;
	case C_MSP3430_SCART_INPUT_MONO:	
		fACB = 0x0100;
		break;
	case C_MSP3430_SCART_INPUT_SCART2:
		fACB = 0x0200;
		break;
	case C_MSP3430_SCART_INPUT_SCART3:	
		fACB = 0x0300;
		break;
	case C_MSP3430_SCART_INPUT_SCART4:
		fACB = 0x0020;
		break;
	case C_MSP3430_SCART_INPUT_MUTE:
		fACB = 0x0320;
		break;
	}
	
	// SCART1 output select
	switch (output1) {
	case C_MSP3430_SCART_OUTPUT_SCART3:
		fACB |= 0x0000;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART2:
		fACB |= 0x0400;
		break;
	case C_MSP3430_SCART_OUTPUT_MONO:
		fACB |= 0x0800;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART1_DA:
		fACB |= 0x0c00;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART2_DA:
		fACB |= 0x0080;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART1_INPUT:
		fACB |= 0x0480;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART4_INPUT:
		fACB |= 0x0880;
		break;
	case C_MSP3430_SCART_OUTPUT_MUTE:
		fACB |= 0x0c80;
		break;
	}

	// SCART2 output select
	switch (output2) {
	case C_MSP3430_SCART_OUTPUT_SCART3:
		fACB |= 0x0000;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART2:
		fACB |= 0x1000;
		break;
	case C_MSP3430_SCART_OUTPUT_MONO:
		fACB |= 0x2000;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART1_DA:
		fACB |= 0x3000;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART2_DA:
		fACB |= 0x0200;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART1_INPUT:
		fACB |= 0x1200;
		break;
	case C_MSP3430_SCART_OUTPUT_SCART4_INPUT:
		fACB |= 0x2200;
		break;
	case C_MSP3430_SCART_OUTPUT_MUTE:
		fACB |= 0x3200;
		break;
	}
	
	SetSCARTSignalPath();
	SetOutputChannels();
}

void CMSP3430::SetBeeper(int volume, MSP3430_beeper_frequency frequency, bool mute)
{
	// set beeper volume 0..127
	int beeper = (mute ? 0x00 : Clamp(volume, 0x00, 0x7f)) << 8;
	
	// set beeper frequency 16, 1000 or 4000 Hz
	switch (frequency) {
	case C_MSP3430_BEEPER_16_HZ:
		beeper |= 0x0001;
		break;
	case C_MSP3430_BEEPER_1_KHZ:
		beeper |= 0x0040;
		break;
	case C_MSP3430_BEEPER_4_KHZ:
		beeper |= 0x00ff;
		break;
	}
	SetRegister(MSP3430_WR_DSP, BEEPER, beeper);
}

void CMSP3430::SetQuasiPeak(int left, int right)
{
	// set quasipeak detector readout -32768..32767
	fQPeakLeft = left;
	fQPeakRight = right;

	SetOutputChannels();
}

void CMSP3430::GetVersion(char * version)
{
	int revision = Register(MSP3430_RD_DSP, MSP_HARD_REVISION);
	int product = Register(MSP3430_RD_DSP, MSP_PRODUCT_ROM);
	
	sprintf(version, "MSP34%02d%c-%c%d",
		((product  & MSP_PRODUCT)  >> 8),
		((revision & MSP_REVISION) >> 0) + 0x40,
		((revision & MSP_HARD)     >> 8) + 0x40,
		((product  & MSP_ROM)      >> 0));
}

#pragma mark -

void CMSP3430::Reset()
{
	// software reset
	SetControlRegister(MSP3430_CONTROL_RESET);
	SetControlRegister(MSP3430_CONTROL_NORMAL);
}

void CMSP3430::SetSCARTSignalPath()
{
	// select SCART DSP input and output
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_ACB_REG,fACB);
}

void CMSP3430::SetDemodulator()
{
	// set preferred mode and sound IF input
	// (automatic, enable STATUS change, detect 4.5 MHz carrier as BTSC)
	SetRegister(MSP3430_WR_DEM, MODUS, 0x2003);
	
	// set preferred prescale (FM and NICAM)
	SetRegister(MSP3430_WR_DSP, PRE_FM, fFMPrescale);
	SetRegister(MSP3430_WR_DSP, PRE_NICAM, fNICAMPrescale);
	
	// automatic sound standard selection
	SetRegister(MSP3430_WR_DEM, MSP3430_DEM_STANDARD_SEL, 0x0001);
	
	// readback the detected TV sound or FM-radio standard
	while ((fStandard = Register(MSP3430_RD_DEM, STANDARD_RES)) >= 0x0800)
		snooze(100000);	
}

void CMSP3430::SetSCARTInput()
{
	// select preferred prescale for SCART and I2C
	SetRegister(MSP3430_WR_DSP, PRE_SCART, fSCARTPrescale);
	SetRegister(MSP3430_WR_DSP, PRE_I2S1, fI2SPrescale);
	SetRegister(MSP3430_WR_DSP, PRE_I2S2, fI2SPrescale);
}

void CMSP3430::SetOutputChannels()
{
	// select source channel and matrix for each output
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_SRC_MAT_MAIN, fMainMatrix);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_SRC_MAT_AUX, fAuxMatrix);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_SRC_MAT_SCART1, fSCART1Matrix);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_SRC_MAT_SCART2, fSCART2Matrix);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_SRC_MAT_I2S, fI2SMatrix);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_SRC_MAT_QPEAK, fQPeakMatrix);
	
	// set audio baseband processing
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_TONE_MODE, fToneControl);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_BASS_MAIN, fBass);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_BASS_AUX, fBass);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_TREB_MAIN, fTreble);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_TREB_AUX, fTreble);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_EQUAL_BAND1, fEqualizer[0]);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_EQUAL_BAND2, fEqualizer[1]);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_EQUAL_BAND3, fEqualizer[2]);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_EQUAL_BAND4, fEqualizer[3]);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_EQUAL_BAND5, fEqualizer[4]);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_LOUD_MAIN, fLoudness);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_LOUD_AUX, fLoudness);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_SPAT_MAIN, fSpatial);
	
	// select volume for each output channel
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_VOL_MAIN, fVolume);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_VOL_AUX, fVolume);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_AVC, fAVC);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_BAL_MAIN, fBalance);
	SetRegister(MSP3430_WR_DSP, MSP3430_DSP_BAL_AUX, fBalance);
	
	SetRegister(MSP3430_WR_DSP, VOL_SCART1, fSCART1Volume);
	SetRegister(MSP3430_WR_DSP, VOL_SCART2, fSCART2Volume);
	
	// set quasipeak detector readout
	SetRegister(MSP3430_WR_DSP, QPEAK_L, fQPeakLeft);
	SetRegister(MSP3430_WR_DSP, QPEAK_R, fQPeakRight);
}

void CMSP3430::SetSubwooferAndMDBOutputChannels()
{
	// set subwoofer output channel
	SetRegister(MSP3430_WR_DSP, SUBW_LEVEL, fSubwooferLevel);
	SetRegister(MSP3430_WR_DSP, SUBW_FREQ, fSubwooferFrequency);

	// set MDB control
	SetRegister(MSP3430_WR_DSP, MDB_STR, fMDBStrength);
	SetRegister(MSP3430_WR_DSP, MDB_LIM, fMDBLimit);
	SetRegister(MSP3430_WR_DSP, MDB_HMC, fMDBHarmonic);
	SetRegister(MSP3430_WR_DSP, MDB_LP, fMDBLowPass);
	SetRegister(MSP3430_WR_DSP, MDB_HP, fMDBHighPass);
}

#pragma mark -
#endif

int CMSP3430::ControlRegister()
{
	char message[1], result[2];

	message[0] = MSP3430_CONTROL;
	if (fPort.Write(fAddress, message, sizeof(message), result, sizeof(result)))
		return ((result[0] << 8) & 0xff00) + ((result[1] << 0) & 0x00ff);
	return 0;
}

void CMSP3430::SetControlRegister(int value)
{
	char message[3];
	
	message[0] = MSP3430_CONTROL;
	message[1] = value >> 8;
	message[2] = value >> 0;
	
	if (!fPort.Write(fAddress, message, sizeof(message)))
		PRINT(("CMSP3430::SetControl() - error\n"));
}

int CMSP3430::Register(int address, int subaddress)
{
	char message[3], response[2];
	
	message[0] = address;
	message[1] = subaddress >> 8;
	message[2] = subaddress >> 0;
	
	if (fPort.Write(fAddress, message, sizeof(message), response, sizeof(response)))
		return ((response[0] << 8) & 0xff00) + ((response[1] << 0) & 0x00ff);
	return 0;
}

void CMSP3430::SetRegister(int address, int subaddress, int value)
{
	char message[5];
	
	message[0] = address;
	message[1] = subaddress >> 8;
	message[2] = subaddress >> 0;
	message[3] = value >> 8;
	message[4] = value >> 0;
	
	if (!fPort.Write(fAddress, message, sizeof(message)))
		PRINT(("CMSP3430::SetRegister() - error\n"));
}

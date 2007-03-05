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
#include "Theater100.h"
#include "Theater.h"
#include "TheatreReg.h"
#include "lendian_bitfield.h"

CTheater100::CTheater100(CRadeon & radeon, int device)
:CTheater(radeon, device)
{
	PRINT(("CTheater100::CTheater100()\n"));
	
	if( fPort.InitCheck() == B_OK ) {
		radeon_video_tuner tuner;
		radeon_video_decoder video;
		
		radeon.GetMMParameters(tuner, video, fClock,
			fTunerPort, fCompositePort, fSVideoPort);
		
		if (fClock != C_RADEON_VIDEO_CLOCK_29_49892_MHZ &&
			fClock != C_RADEON_VIDEO_CLOCK_27_00000_MHZ)
			PRINT(("CTheater100::CTheater100() - Unsupported crystal clock!\n"));

		//fDevice = fPort.FindVIPDevice( C_THEATER100_VIP_DEVICE_ID );
	
	}
	
	if( InitCheck() != B_OK )
		PRINT(("CTheater100::CTheater100() - Rage Theater not found!\n"));
}

CTheater100::~CTheater100()
{
	PRINT(("CTheater100::~CTheater100()\n"));
	
	if( InitCheck() == B_OK )
		SetEnable(false, false);
}

status_t CTheater100::InitCheck() const
{
	status_t res;
	
	res = fPort.InitCheck();
	if( res != B_OK )
		return res;
		
	return (fDevice >= C_VIP_PORT_DEVICE_0 && fDevice <= C_VIP_PORT_DEVICE_3) ? B_OK : B_ERROR;
}

void CTheater100::Reset()
{
	PRINT(("CTheater100::Reset()\n"));
	
	SetHue(0);
	SetBrightness(0);
	SetSaturation(0);
	SetContrast(0);
	SetSharpness(false);
}

// disable/enable capturing
void CTheater100::SetEnable(bool enable, bool vbi)
{
	PRINT(("CTheater100::SetEnable(%d, %d)\n", enable, vbi));

#if 0
	//@ reset ADC?
	SetRegister(VIP_ADC_CNTL, ADC_CPRESET, ADC_CPRESET);
	snooze(1000);
	SetRegister(VIP_ADC_CNTL, ADC_CPRESET, 0);
	snooze(1000);
	SetRegister(VIP_ADC_CNTL, ADC_PDWN, ADC_PDWN_DOWN);
#endif

	
	WaitVSYNC();

	/* Disable the Video In, Scaler and DVS port */
	SetRegister(VIP_MASTER_CNTL, VIN_ASYNC_RST, VIN_ASYNC_RST);
	SetRegister(VIP_MASTER_CNTL, DVS_ASYNC_RST, DVS_ASYNC_RST);

	/* select the reference clock for the Video In */
	SetRegister(VIP_CLOCK_SEL_CNTL, VIN_CLK_SEL, VIN_CLK_SEL_REF_CLK);

	/* reset the VIN/L54 PLL clocks */
	SetRegister(VIP_PLL_CNTL1, VINRST, VINRST);
	SetRegister(VIP_PLL_CNTL1, L54RST, L54RST);

	/* power down the ADC block */
	SetRegister(VIP_ADC_CNTL, ADC_PDWN, ADC_PDWN);

	/* set DVS port to input mode */
	SetRegister(VIP_DVS_PORT_CTRL, DVS_DIRECTION, DVS_DIRECTION_INPUT);

	/* select DVS clock to 8xFsc and disable continuous mode */
	SetRegister(VIP_DVS_PORT_CTRL, DVS_CLK_SELECT, DVS_CLK_SELECT_8X);
	SetRegister(VIP_DVS_PORT_CTRL, CONTINUOUS_STREAM, 0);

	if (enable) {
		WaitVSYNC();

		SetClock(fStandard, fClock);
		SetADC(fStandard, fSource);
		SetLuminanceProcessor(fStandard);
		SetChromaProcessor(fStandard);
		SetVSYNC(fStandard);
		SetClipWindow(fStandard, vbi);
		SetCombFilter(fStandard, fSource);
		SetHSYNC(fStandard);
		SetSyncGenerator(fStandard);
		SetScaler(fStandard, fHActive, fVActive, fDeinterlace);
		
		/* Enable ADC block */
		SetRegister(VIP_ADC_CNTL, ADC_PDWN, ADC_PDWN_UP);

		WaitVSYNC();

		/* Enable the Video In, Scaler and DVS port */
		SetRegister(VIP_MASTER_CNTL, VIN_ASYNC_RST, 0);
		SetRegister(VIP_MASTER_CNTL, DVS_ASYNC_RST, 0);

		/* set DVS port to output mode */
		SetRegister(VIP_DVS_PORT_CTRL, DVS_DIRECTION, DVS_DIRECTION_OUTPUT);

		//WaitHSYNC();

		/* restore luminance and chroma settings */
		SetLuminanceLevels(fStandard, fBrightness, fContrast);
		SetChromaLevels(fStandard, fSaturation, fHue);
	}
}

void CTheater100::SetStandard(theater_standard standard, theater_source source)
{
	PRINT(("CTheater100::SetStandard(%s, %s)\n",
		"NTSC\0\0\0\0\0\0NTSC-J\0\0\0\0NTSC-443\0\0PAL-M\0\0\0\0\0"
		"PAL-N\0\0\0\0\0PAL-NC\0\0\0\0PAL-BDGHI\0PAL-60\0\0\0\0"
		"SECAM\0\0\0\0\0"+10*standard,
		"TUNER\0COMP\0\0SVIDEO"+6*source));
		
	fStandard = standard;
	fSource = source;
}

void CTheater100::SetSize(int hactive, int vactive)
{
	PRINT(("CTheater100::SetSize(%d, %d)\n", hactive, vactive));
	
	fHActive = hactive;
	fVActive = vactive;
}

void CTheater100::SetDeinterlace(bool deinterlace)
{
	PRINT(("CTheater100::SetDeinterlace(%d)\n", deinterlace));
	
	fDeinterlace = deinterlace;
}

void CTheater100::SetSharpness(int sharpness)
{
	PRINT(("CTheater100::SetSharpness(%d)\n", sharpness));
	
	SetRegister(VIP_H_SCALER_CONTROL, H_SHARPNESS, sharpness << 25);
}

void CTheater100::SetBrightness(int brightness)
{
	PRINT(("CTheater100::SetBrightness(%d)\n", brightness));
	
	fBrightness = brightness;
	SetLuminanceLevels(fStandard, fBrightness, fContrast);
}

void CTheater100::SetContrast(int contrast)
{
	PRINT(("CTheater100::SetContrast(%d)\n", contrast));

	fContrast = contrast;
	SetLuminanceLevels(fStandard, fBrightness, fContrast);
}

void CTheater100::SetSaturation(int saturation)
{
	PRINT(("CTheater100::SetSaturation(%d)\n", saturation));

	fSaturation = saturation;
	SetChromaLevels(fStandard, fSaturation, fHue);
}

void CTheater100::SetHue(int hue)
{
	PRINT(("CTheater100::SetHue(%d)\n", hue));

	fHue = hue;
	SetChromaLevels(fStandard, fSaturation, fHue);
}


// set pixel clock
void CTheater100::SetClock(theater_standard standard, radeon_video_clock clock)
{
	// set VIN PLL clock dividers
	int referenceDivider, feedbackDivider, postDivider;
	
	switch (standard) {
	case C_THEATER_NTSC:
	case C_THEATER_NTSC_JAPAN:
		if (clock == C_RADEON_VIDEO_CLOCK_29_49892_MHZ) {
			referenceDivider = 0x39;
			feedbackDivider = 0x14c;
			postDivider = 0x6;
		}
		else {
			referenceDivider = 0x0b;
			feedbackDivider = 0x46;
			postDivider = 0x6;
		}
		break;
	case C_THEATER_NTSC_443:
		if (clock == C_RADEON_VIDEO_CLOCK_29_49892_MHZ) {
			referenceDivider = 0x23;
			feedbackDivider = 0x88;
			postDivider = 0x7;
		}
		else {
			referenceDivider = 0x2c;
			feedbackDivider = 0x121;
			postDivider = 0x5;
		}
		break;
	case C_THEATER_PAL_M:
		if (clock == C_RADEON_VIDEO_CLOCK_29_49892_MHZ) {
			referenceDivider = 0x2c;
			feedbackDivider = 0x12b;
			postDivider = 0x7;
		}
		else {
			referenceDivider = 0x0b;
			feedbackDivider = 0x46;
			postDivider = 0x6;
		}
		break;
	case C_THEATER_PAL_BDGHI:
	case C_THEATER_PAL_N:
	case C_THEATER_PAL_60:
	case C_THEATER_SECAM:
		if (clock == C_RADEON_VIDEO_CLOCK_29_49892_MHZ) {
			referenceDivider = 0x0e;
			feedbackDivider = 0x65;
			postDivider = 0x6;
		}
		else {
			referenceDivider = 0x2c;
			feedbackDivider = 0x121;
			postDivider = 0x5;
		}
		break;
	case C_THEATER_PAL_NC:
		if (clock == C_RADEON_VIDEO_CLOCK_29_49892_MHZ) {
			referenceDivider = 0x23;
			feedbackDivider = 0x88;
			postDivider = 0x7;
		}
		else {
			referenceDivider = 0x37;
			feedbackDivider = 0x1d3;
			postDivider = 0x8;
		}
		break;
	default:
		PRINT(("CTheater100::SetClock() - Bad standard\n"));
		return;
	}

	// reset VIN PLL and select the reference clock
	SetRegister(VIP_CLOCK_SEL_CNTL, VIN_CLK_SEL, VIN_CLK_SEL_REF_CLK);
	SetRegister(VIP_PLL_CNTL1, VINRST, VINRST);
	SetRegister(VIP_PLL_CNTL1, L54RST, L54RST);
	
	// set up the VIN PLL clock control
	SetRegister(VIP_VIN_PLL_CNTL, VIN_M0, referenceDivider << 0);
	SetRegister(VIP_VIN_PLL_CNTL, VIN_N0, feedbackDivider << 11);
	SetRegister(VIP_VIN_PLL_CNTL, VIN_P, postDivider << 24);

	// active the VIN/L54 PLL and attach the VIN PLL to the VIN clock
	SetRegister(VIP_PLL_CNTL1, VINRST, 0);
	SetRegister(VIP_PLL_CNTL1, L54RST, 0);
	SetRegister(VIP_CLOCK_SEL_CNTL, VIN_CLK_SEL, VIN_CLK_SEL_VIPLL_CLK);

	PRINT(("CTheater100::SetClock(Fsamp=%g, Fref=%g)\n",
		((fClock == C_RADEON_VIDEO_CLOCK_29_49892_MHZ ? 29.49892 : 27.0) * feedbackDivider) / (referenceDivider * postDivider),
		(fClock == C_RADEON_VIDEO_CLOCK_29_49892_MHZ ? 29.49892 : 27.0)));
}


// setup analog-digital converter
void CTheater100::SetADC(theater_standard standard, theater_source source)
{
	PRINT(("CTheater100::SetADC(%c, %c)\n", "NJ4MNCB6S"[standard], "TCS"[source]));
	
	// set HW_DEBUG before setting the standard
	SetRegister(VIP_HW_DEBUG, 0x0000f000);
	
	// select the video standard
	switch (standard) {
	case C_THEATER_NTSC:
	case C_THEATER_NTSC_JAPAN:
	case C_THEATER_NTSC_443:
	case C_THEATER_PAL_M:
		SetRegister(VIP_STANDARD_SELECT, STANDARD_SEL, STANDARD_NTSC);
		break;
	case C_THEATER_PAL_BDGHI:
	case C_THEATER_PAL_N:
	case C_THEATER_PAL_60:
	case C_THEATER_PAL_NC:
		SetRegister(VIP_STANDARD_SELECT, STANDARD_SEL, STANDARD_PAL);
		break;
	case C_THEATER_SECAM:
		SetRegister(VIP_STANDARD_SELECT, STANDARD_SEL, STANDARD_SECAM);
		break;
	default:
		PRINT(("CTheater100::SetADC() - Bad standard\n"));
		return;
	}

	// select input connector and Y/C mode
	switch (source) {
	case C_THEATER_TUNER:
		SetRegister(VIP_ADC_CNTL, INPUT_SELECT, fTunerPort);
		SetRegister(VIP_STANDARD_SELECT, YC_MODE, YC_MODE_COMPOSITE);
		break;
	case C_THEATER_COMPOSITE:
		SetRegister(VIP_ADC_CNTL, INPUT_SELECT, fCompositePort);
		SetRegister(VIP_STANDARD_SELECT, YC_MODE, YC_MODE_COMPOSITE);
		break;
	case C_THEATER_SVIDEO:
		SetRegister(VIP_ADC_CNTL, INPUT_SELECT, fSVideoPort);
		SetRegister(VIP_STANDARD_SELECT, YC_MODE, YC_MODE_SVIDEO);
		break;
	default:
		PRINT(("CTheater100::SetADC() - Bad source\n"));
		return;
	}
	
	SetRegister(VIP_ADC_CNTL, I_CLAMP_SEL, I_CLAMP_SEL_22);
	SetRegister(VIP_ADC_CNTL, I_AGC_SEL, I_AGC_SEL_7);

	SetRegister(VIP_ADC_CNTL, EXT_CLAMP_CAP, EXT_CLAMP_CAP_EXTERNAL);
	SetRegister(VIP_ADC_CNTL, EXT_AGC_CAP, EXT_AGC_CAP_EXTERNAL);
	SetRegister(VIP_ADC_CNTL, ADC_DECI_BYPASS, ADC_DECI_WITH_FILTER);
	SetRegister(VIP_ADC_CNTL, VBI_DECI_BYPASS, VBI_DECI_WITH_FILTER);
	SetRegister(VIP_ADC_CNTL, DECI_DITHER_EN, 0 << 12);
	SetRegister(VIP_ADC_CNTL, ADC_CLK_SEL, ADC_CLK_SEL_8X);
	SetRegister(VIP_ADC_CNTL, ADC_BYPASS, ADC_BYPASS_INTERNAL);
	switch (standard) {
	case C_THEATER_NTSC:
	case C_THEATER_NTSC_JAPAN:
	case C_THEATER_NTSC_443:
	case C_THEATER_PAL_M:
		SetRegister(VIP_ADC_CNTL, ADC_CH_GAIN_SEL, ADC_CH_GAIN_SEL_NTSC);
		break;
	case C_THEATER_PAL_BDGHI:
	case C_THEATER_PAL_N:
	case C_THEATER_PAL_60:
	case C_THEATER_PAL_NC:
	case C_THEATER_SECAM:
		SetRegister(VIP_ADC_CNTL, ADC_CH_GAIN_SEL, ADC_CH_GAIN_SEL_PAL);
		break;
	}
	SetRegister(VIP_ADC_CNTL, ADC_PAICM, 1 << 18);

	SetRegister(VIP_ADC_CNTL, ADC_PDCBIAS, 2 << 20);
	SetRegister(VIP_ADC_CNTL, ADC_PREFHI, ADC_PREFHI_2_7);
	SetRegister(VIP_ADC_CNTL, ADC_PREFLO, ADC_PREFLO_1_5);

	SetRegister(VIP_ADC_CNTL, ADC_IMUXOFF, 0 << 26);
	SetRegister(VIP_ADC_CNTL, ADC_CPRESET, 0 << 27);
}


// setup horizontal sync PLL
void CTheater100::SetHSYNC(theater_standard standard)
{
	static const uint16 hs_line_total[] = {
		0x38E,	0x38E,	0x46F,	0x38D,	0x46F,	0x395,	0x46F,	0x467,	0x46F };
		
	static const uint32 hs_dto_inc[] = {
		0x40000,	0x40000,	0x40000,	0x40000,	0x40000,	0x40000,	0x40000,	0x40000,	0x3E7A2 };

	// TK: completely different in gatos
	static const uint8 hs_pll_sgain[] = {
		2,		2,		2,		2,		2,		2,		2,		2,		2 };
	static const uint8 hs_pll_fgain[] = {
		8,		8,		8,		8,		8,		8,		8,		8,		8 };

	static const uint8 gen_lock_delay[] = {
		0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10,	0x10 };
		
	static const uint8 min_pulse_width[] = {
		0x21,	0x21,	0x29,	0x21,	0x29,	0x21,	0x29,	0x29,	0x29 };
	static const uint8 max_pulse_width[] = {
		0x64,	0x64,	0x7D,	0x64,	0x7D,	0x65,	0x7D,	0x7D,	0x7D };
		
	static const uint16 win_close_limit[] = {
		0x0A0,	0x0A0,	0x0C7,	0x0A0,	0x0C7,	0x0A0,	0x0C7,	0x0C7,	0x0C7 };
	static const uint16 win_open_limit[] = {
		0x1B7,	0x1B7,	0x228,	0x1B7,	0x228,	0x1BB,	0x228,	0x224,	0x228 };


	// set number of samples per line
	SetRegister(VIP_HS_PLINE, HS_LINE_TOTAL, hs_line_total[standard]);
	
	SetRegister(VIP_HS_DTOINC, HS_DTO_INC, hs_dto_inc[standard]);
	
	SetRegister(VIP_HS_PLLGAIN, HS_PLL_SGAIN, hs_pll_sgain[standard] << 0);
	SetRegister(VIP_HS_PLLGAIN, HS_PLL_FGAIN, (uint32)hs_pll_fgain[standard] << 4);
	
	SetRegister(VIP_HS_GENLOCKDELAY, GEN_LOCK_DELAY, gen_lock_delay[standard]);

	// set min/max pulse width in samples	
	SetRegister(VIP_HS_MINMAXWIDTH, MIN_PULSE_WIDTH, min_pulse_width[standard] << 0);
	SetRegister(VIP_HS_MINMAXWIDTH, MAX_PULSE_WIDTH, (uint32)max_pulse_width[standard] << 8);

	SetRegister(VIP_HS_WINDOW_LIMIT, WIN_CLOSE_LIMIT, win_close_limit[standard] << 0);
	SetRegister(VIP_HS_WINDOW_LIMIT, WIN_OPEN_LIMIT, (uint32)win_open_limit[standard] << 16);
	

	PRINT(("CTheater100::SetHSYNC(total=%d, pulse=%d/%d, window=%d/%d)\n",
		Register(VIP_HS_PLINE, HS_LINE_TOTAL),
		Register(VIP_HS_MINMAXWIDTH, MIN_PULSE_WIDTH) >> 0,
		Register(VIP_HS_MINMAXWIDTH, MAX_PULSE_WIDTH) >> 8,
		Register(VIP_HS_WINDOW_LIMIT, WIN_CLOSE_LIMIT) >> 0,
		Register(VIP_HS_WINDOW_LIMIT, WIN_OPEN_LIMIT) >> 16));
}


// wait until horizontal scaler is locked
void CTheater100::WaitHSYNC()
{
	for (int timeout = 0; timeout < 1000; timeout++) {
		if (Register(VIP_HS_PULSE_WIDTH, HS_GENLOCKED) != 0)
			return;
		snooze(20);
	}
	PRINT(("CTheater100::WaitHSYNC() - wait for HSync locking time out!\n"));
}


// setup vertical sync and field detector
void CTheater100::SetVSYNC(theater_standard standard)
{
	static const uint16 vsync_int_trigger[] = {
		0x2AA,	0x2AA,	0x353,	0x2AA,	0x353,	0x2B0,	0x353,	0x34D,	0x353 };
	static const uint16 vsync_int_hold[] = {
		0x017,	0x017,	0x01C,	0x017,	0x01C,	0x017,	0x01C,	0x01C,	0x01C };
	// PAL value changed from 26b to 26d - else, odd/even field detection fails sometimes;
	// did the same for PAL N, PAL NC and SECAM
	static const uint16 vs_field_blank_start[] = {
		0x206,	0x206,	0x206,	0x206,	0x26d,	0x26d,	0x26d,	0x206,	0x26d };
	static const uint8 vs_field_blank_end[] = {
		0x00a,	0x00a,	0x00a,	0x00a,	0x02a,	0x02a,	0x02a,	0x00a,	0x02a };
	// NTSC value changed from 1 to 105 - else, odd/even fields were always swapped;
	// did the same for NTSC Japan, NTSC 443, PAL M and PAL 60
	static const uint16 vs_field_id_location[] = {
		0x105,	0x105,	0x105,	0x105,	0x1,	0x1,	0x1,	0x105,	0x1 };
	static const uint16 vs_frame_total[] = {
		0x217,	0x217,	0x217,	0x217,	0x27B,	0x27B,	0x27B,	0x217,	0x27B };

	SetRegister(VIP_VS_DETECTOR_CNTL, VSYNC_INT_TRIGGER, vsync_int_trigger[standard] << 0);
	SetRegister(VIP_VS_DETECTOR_CNTL, VSYNC_INT_HOLD, (uint32)vsync_int_hold[standard] << 16);

	SetRegister(VIP_VS_BLANKING_CNTL, VS_FIELD_BLANK_START, vs_field_blank_start[standard] << 0);
	SetRegister(VIP_VS_BLANKING_CNTL, VS_FIELD_BLANK_END, (uint32)vs_field_blank_end[standard] << 16);
	SetRegister(VIP_VS_FRAME_TOTAL, VS_FRAME_TOTAL, vs_frame_total[standard]);

	SetRegister(VIP_VS_FIELD_ID_CNTL, VS_FIELD_ID_LOCATION, vs_field_id_location[standard] << 0);

	// auto-detect fields
	SetRegister(VIP_VS_COUNTER_CNTL, FIELD_DETECT_MODE, FIELD_DETECT_DETECTED);
	
	// don't flip fields
	SetRegister(VIP_VS_COUNTER_CNTL, FIELD_FLIP_EN, 0 );

	PRINT(("CTheater100::SetVSYNC(total=%d)\n",
		Register(VIP_VS_FRAME_TOTAL, VS_FRAME_TOTAL)));
}

// wait until a visible line is viewed
void CTheater100::WaitVSYNC()
{
	for (int timeout = 0; timeout < 1000; timeout++) {
		int lineCount = Register(VIP_VS_LINE_COUNT, VS_LINE_COUNT);
		if (lineCount > 1 && lineCount < 20)
			return;
		snooze(20);
	}
	PRINT(("CTheater100::WaitVSYNC() - wait for VBI timed out!\n"));
}


// setup timing generator
void CTheater100::SetSyncGenerator(theater_standard standard)
{
	static const uint16 blank_int_start[] = {
		0x031,	0x031,	0x046,	0x031,	0x046,	0x046,	0x046,	0x031,	0x046 };
	static const uint8 blank_int_length[] = {
		0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F };
		
	static const uint16 sync_tip_start[] = {
		0x0372,	0x0372,	0x0453,	0x0371,	0x0453,	0x0379,	0x0453,	0x044B,	0x0453 };
	static const uint8 sync_tip_length[] = {
		0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F };

	static const uint8 uv_int_start[] = {
		0x03B,	0x03B,	0x052,	0x03B,	0x052,	0x03B,	0x052,	0x03C,	0x068 };
	static const uint8 u_int_length[] = {
		0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F };
	static const uint8 v_int_length[] = {
		0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F,	0x0F };

	// set blank interrupt position
	SetRegister(VIP_SG_BLACK_GATE, BLANK_INT_START, blank_int_start[standard] );
	SetRegister(VIP_SG_BLACK_GATE, BLANK_INT_LENGTH, (uint32)blank_int_length[standard] << 8);

	SetRegister(VIP_SG_SYNCTIP_GATE, SYNC_TIP_START, sync_tip_start[standard]);
	SetRegister(VIP_SG_SYNCTIP_GATE, SYNC_TIP_LENGTH, (uint32)sync_tip_length[standard] << 12);

	SetRegister(VIP_SG_UVGATE_GATE, UV_INT_START, uv_int_start[standard] << 0);
	
	SetRegister(VIP_SG_UVGATE_GATE, U_INT_LENGTH, (uint32)u_int_length[standard] << 8);
	SetRegister(VIP_SG_UVGATE_GATE, V_INT_LENGTH, (uint32)v_int_length[standard] << 12);

	PRINT(("CTheater100::SetSyncGenerator(black=%d/%d, synctip=%d/%d, uvgate=%d/%d-%d)\n",
		Register(VIP_SG_BLACK_GATE, BLANK_INT_START) >> 0,
		Register(VIP_SG_BLACK_GATE, BLANK_INT_LENGTH) >> 8,
		Register(VIP_SG_SYNCTIP_GATE, SYNC_TIP_START),
		Register(VIP_SG_SYNCTIP_GATE, SYNC_TIP_LENGTH) >> 12,
		Register(VIP_SG_UVGATE_GATE, UV_INT_START),
		Register(VIP_SG_UVGATE_GATE, U_INT_LENGTH) >> 8,
		Register(VIP_SG_UVGATE_GATE, V_INT_LENGTH) >> 12));
}


// setup input comb filter.
// this is really ugly but I cannot find a scheme
void CTheater100::SetCombFilter(theater_standard standard, theater_source source)
{
	enum {
		_3Tap_2D_adaptive_Comb = 1,	// composite
		_2Tap_C_combed_Y_Sub = 2,
		_2Tap_C_combed_Y_combed = 3,
		_3Tap_C_combed_Y_Sub = 4,
		_3Tap_C_combed_Y_combed = 5,
		YC_mode_Comb_filter_off = 6,	// S-Video
		YC_mode_2Tap_YV_filter = 7,
		YC_mode_3Tap_YV_filter = 8
	};

	// make sure to keep bitfield in sync with register definition!
	// we could define each component as an uint8, but this would waste space
	// and would require an extra register-composition 
	typedef struct {
		LBITFIELD32_12 (
			comb_hck			: 8, 
			comb_vck			: 8, 
			comb_filter_en		: 1, 
			comb_adaptiv_en		: 1,
			comb_bpfmuxsel		: 3,
			comb_coutsel		: 2,
			comb_sumdiff0sel	: 1, 
			comb_sumdiff1sel	: 2,
			comb_yvlpfsel		: 1, 
			comb_dlylinesel		: 2, 
			comb_ydlyinsel		: 2, 
			comb_ysubbw			: 1
		);
	} comb_cntl0;
	
	typedef struct {
		LBITFIELD32_7 (
			comb_ydlyoutsel		: 2,
			comb_coresize		: 2, 
			comb_ysuben			: 1, 
			comb_youtsel		: 1,
			comb_syncpfsel		: 2, 
			comb_synclpfrst		: 1, 
			comb_debug			: 1
		);
	} comb_cntl1;

	typedef struct {
		LBITFIELD32_4 (
			comb_hyk0			: 8, 
			comb_vyk0			: 8, 
			comb_hyk1			: 8, 
			comb_vyk1			: 8
		);
	} comb_cntl2;

	typedef struct {
		LBITFIELD32_2 (
			comb_tap0length		: 16,
			comb_tap1length		: 12
		);
	} comb_line_length;

	typedef struct {
		const uint8				*types;
		const comb_cntl0		*cntl0;
		const comb_cntl1		*cntl1;
		const comb_cntl2		*cntl2;
		const comb_line_length	*line_length;
	} comb_settings;
	
	static const uint8 comb_types_ntsc_m[] = {
		_3Tap_2D_adaptive_Comb,
		_2Tap_C_combed_Y_Sub,
		_2Tap_C_combed_Y_combed,	
		_3Tap_C_combed_Y_Sub,
		_3Tap_C_combed_Y_combed,	
		YC_mode_Comb_filter_off,
		YC_mode_2Tap_YV_filter,	
		YC_mode_3Tap_YV_filter,
		0
	};

	static const comb_cntl0 comb_cntl0_ntsc_m[] = {
		{	0x90,	0x80,	1,	1,	0,	2,	0,	1,	0,	1,	0,	0 },
		{	0,	0,	1,	0,	3,	2,	0,	0,	0,	1,	0,	0 },
		{	0,	0,	1,	0,	3,	2,	0,	0,	0,	1,	1,	0 },
		{	0,	0,	1,	0,	1,	2,	0,	1,	0,	1,	0,	0 },
		{	0,	0,	1,	0,	1,	2,	0,	1,	1,	1,	0,	0 },
		{	0,	0,	0,	0,	5,	2,	0,	0,	0,	1,	2,	0 },
		{	0,	0,	0,	0,	5,	2,	0,	0,	0,	1,	1,	0 },
		{	0,	0,	0,	0,	5,	2,	0,	0,	1,	1,	0,	0 }
	};
	
	static const comb_cntl1 comb_cntl1_ntsc_m[] = {
		{	0,	0,	1,	0,	0,	0,	0 }, 
		{	2,	0,	1,	0,	0,	0,	0 }, 
		{	3,	0,	0,	0,	0,	0,	0 }, 
		{	0,	0,	1,	0,	1,	0,	0 }, 
		{	3,	0,	0,	0,	1,	0,	0 }, 
		{	1,	0,	0,	0,	2,	0,	0 }, 
		{	3,	0,	0,	0,	0,	0,	0 }, 
		{	3,	0,	0,	0,	1,	0,	0 }
	};
		
	static const comb_cntl2 comb_cntl2_ntsc_m[] = {
		{	0x10,	0x10,	0x16,	0x16 },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF }
	};
		
	static const comb_line_length comb_line_length_ntsc_m[] = {
		{	0x38A,	0x718 }, 
		{	0x38A,	0x718 }, 
		{	0x38A,	0x718 }, 
		{	0x38A,	0x718 }, 
		{	0x38A,	0x718 }, 
		{	0,		0 }, 
		{	0x38A,	0 }, 
		{	0x38A,	0x718 }
	};


	static const uint8 comb_types_ntsc_433[] = {
		_2Tap_C_combed_Y_Sub,
		_2Tap_C_combed_Y_combed,	
		_3Tap_C_combed_Y_Sub,
		_3Tap_C_combed_Y_combed,	
		YC_mode_Comb_filter_off,
		YC_mode_2Tap_YV_filter,	
		YC_mode_3Tap_YV_filter,
		0
	};
	
	static const comb_cntl0 comb_cntl0_ntsc_433[] = {
		{	0,	0,	1,	0,	3,	2,	0,	0,	0,	1,	0,	0 },
		{	0,	0,	1,	0,	3,	2,	0,	0,	0,	1,	1,	0 },
		{	0,	0,	1,	0,	1,	2,	0,	1,	0,	1,	0,	0 },
		{	0,	0,	1,	0,	1,	2,	0,	1,	1,	1,	0,	0 },
		{	0,	0,	0,	0,	5,	2,	0,	0,	0,	1,	2,	0 },
		{	0,	0,	0,	0,	5,	2,	0,	0,	0,	1,	1,	0 },
		{	0,	0,	0,	0,	5,	2,	0,	0,	1,	1,	0,	0 }
	};
	
	static const comb_cntl1 comb_cntl1_ntsc_433[] = {
		{	2,	0,	1,	0,	0,	0,	0 },
		{	3,	0,	0,	0,	0,	0,	0 },
		{	0,	0,	1,	0,	1,	0,	0 },
		{	3,	0,	0,	0,	1,	0,	0 },
		{	1,	0,	0,	0,	2,	0,	0 },
		{	3,	0,	0,	0,	0,	0,	0 },
		{	3,	0,	0,	0,	1,	0,	0 }
	};
	
	static const comb_cntl2 comb_cntl2_ntsc_433[] = {
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF }
	};
	
	static const comb_line_length comb_line_length_ntsc_433[] = {
		{	0x462,	0x8C9 },
		{	0x462,	0x8C9 },
		{	0x462,	0x8C9 },
		{	0x462,	0x8C9 },
		{	0,		0 },
		{	0x462,	0x8C9 },
		{	0x462,	0x8C9 }
	};
	

	static const uint8 comb_types_pal_m[] = {
		_2Tap_C_combed_Y_Sub,
		YC_mode_2Tap_YV_filter,	
		0
	};
	
	static const comb_cntl0 comb_cntl0_pal_m[] = {
		{	0,	0,	1,	0,	4,	0,	1,	2,	0,	0,	2,	0 },
		{	0,	0,	1,	0,	5,	0,	1,	2,	0,	0,	2,	0 }
	};

	static const comb_cntl1 comb_cntl1_pal_m[] = {
		{	1,	0,	1,	1,	2,	0,	0 },
		{	1,	0,	0,	1,	2,	0,	0 }
	};
	
	static const comb_cntl2 comb_cntl2_pal_m[] = {
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF }
	};

	static const comb_line_length comb_line_length_pal_m[] = {
		{	0x389,	0 },
		{	0x389,	0 }
	};


	static const uint8 comb_types_pal_n[] = {
		_3Tap_2D_adaptive_Comb,
		_2Tap_C_combed_Y_Sub,
		YC_mode_2Tap_YV_filter,
		0
	};

	static const comb_cntl0 comb_cntl0_pal_n[] = {
		{	0x90,	0x80,	1,	1,	0,	2,	0,	1,	0,	1,	0,	0 },
		{	0,	0,	1,	0,	4,	0,	1,	2,	0,	0,	2,	0 },
		{	0,	0,	1,	0,	5,	0,	1,	2,	0,	0,	2,	0 }
	};

	static const comb_cntl1 comb_cntl1_pal_n[] = {
		{	0,	0,	1,	0,	0,	0,	0 },
		{	1,	0,	1,	1,	2,	0,	0 },
		{	1,	0,	0,	1,	2,	0,	0 }
	};
	
	static const comb_cntl2 comb_cntl2_pal_n[] = {
		{	0x10,	0x10,	0x16,	0x16 },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF }
	};

	static const comb_line_length comb_line_length_pal_n[] = {
		{	0x46B,	0x8DA },
		{	0x46C,	0 },
		{	0x46C,	0 }
	};


	static const uint8 comb_types_pal_nc[] = {
		_3Tap_2D_adaptive_Comb,
		_2Tap_C_combed_Y_Sub,
		YC_mode_2Tap_YV_filter,
		0
	};

	// used to represent an N/A for easier copy'n'paste	
#define X 0

	static const comb_cntl0 comb_cntl0_pal_nc[] = {
		{	0x90,	0x80,	1,	1,	0,	2,	0,	1,	0,	1,	0,	0 },
		{	X,	X,	1,	0,	4,	0,	1,	2,	0,	0,	2,	0 },
		{	X,	X,	1,	0,	5,	0,	1,	2,	X,	0,	2,	0 }
	};
	
	static const comb_cntl1 comb_cntl1_pal_nc[] = {
		{	0,	0,	1,	0,	0,	0,	0 },
		{	1,	0,	1,	1,	2,	0,	0 },
		{	1,	0,	0,	1,	2,	0,	0 }
	};

	static const comb_cntl2 comb_cntl2_pal_nc[] = {
		{	0x10,	0x10,	0x16,	0x16 },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF }
	};

	static const comb_line_length comb_line_length_pal_nc[] = {
		{	0x391,	0x726 },
		{	0x394,	X },
		{	0x394,	X }
	};
	
	
	static const uint8 comb_types_pal[] = {
		_3Tap_2D_adaptive_Comb,
		_2Tap_C_combed_Y_Sub,
		YC_mode_2Tap_YV_filter,
		0
	};

	static const comb_cntl0 comb_cntl0_pal[] = {
		{	0x90,	0x80,	1,	1,	0,	2,	0,	1,	0,	1,	0,	0 },
		{	0,	0,	1,	0,	4,	0,	1,	2,	0,	0,	2,	0 },
		{	0,	0,	1,	0,	5,	0,	1,	2,	X,	0,	2,	0 }
	};
	
	static const comb_cntl1 comb_cntl1_pal[] = {
		{	0,	0,	1,	0,	0,	0,	0 },
		{	1,	0,	1,	1,	2,	0,	0 },
		{	1,	0,	0,	1,	2,	0,	0 }
	};

	static const comb_cntl2 comb_cntl2_pal[] = {	
		{	2,	1,	8,	6 },
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF }
	};

	static const comb_line_length comb_line_length_pal[] = {
		{	0x46B,	0x8DA },
		{	0x46C,	X },
		{	0x46C,	X }
	};


	static const uint8 comb_types_pal_60[] = {
		_2Tap_C_combed_Y_Sub,
		YC_mode_2Tap_YV_filter,
		0
	};

	static const comb_cntl0 comb_cntl0_pal_60[] = {
		{	0,	0,	1,	0,	4,	0,	1,	2,	0,	0,	2,	0 },
		{	0,	0,	1,	0,	5,	0,	1,	2,	0,	0,	2,	0 }
	};
	
	static const comb_cntl1 comb_cntl1_pal_60[] = {
		{	1,	0,	1,	1,	2,	0,	0 },
		{	1,	0,	0,	1,	2,	0,	0 }
	};

	static const comb_cntl2 comb_cntl2_pal_60[] = {	
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF }
	};

	static const comb_line_length comb_line_length_pal_60[] = {
		{	0x463,	0 },
		{	0x463,	0 }
	};

		
	static const uint8 comb_types_secam[] = {
		_2Tap_C_combed_Y_Sub,		// could be another type, spec is unclear here
		YC_mode_2Tap_YV_filter,
		0,
	};

	static const comb_cntl0 comb_cntl0_secam[] = {
		{	X,	X,	0,	0,	4,	X,	X,	X,	X,	2,	2,	1 },
		{	X,	X,	0,	0,	5,	X,	X,	X,	X,	2,	2,	X }
	};
	
	static const comb_cntl1 comb_cntl1_secam[] = {
		{	1,	0,	1,	0,	2,	0,	0 },
		{	1,	X,	0,	0,	2,	0,	0 }
	};
	
	static const comb_cntl2 comb_cntl2_secam[] = {	
		{	0xFF,	0xFF,	0xFF,	0xFF },
		{	0xFF,	0xFF,	0xFF,	0xFF }
	};
	
	static const comb_line_length comb_line_length_secam[] = {
		{	0x46A,	0 },
		{	0x46A,	0 }
	};

#undef X		
		
	static const comb_settings comb_settings_list[] = {
		{ comb_types_ntsc_m,	comb_cntl0_ntsc_m,		comb_cntl1_ntsc_m,		comb_cntl2_ntsc_m,		comb_line_length_ntsc_m },
		{ comb_types_ntsc_m,	comb_cntl0_ntsc_m,		comb_cntl1_ntsc_m,		comb_cntl2_ntsc_m,		comb_line_length_ntsc_m },
		{ comb_types_ntsc_433,	comb_cntl0_ntsc_433,	comb_cntl1_ntsc_433,	comb_cntl2_ntsc_433,	comb_line_length_ntsc_433 },
		{ comb_types_pal_m,		comb_cntl0_pal_m,		comb_cntl1_pal_m,		comb_cntl2_pal_m,		comb_line_length_pal_m },
		{ comb_types_pal_n,		comb_cntl0_pal_n,		comb_cntl1_pal_n,		comb_cntl2_pal_n,		comb_line_length_pal_n },
		{ comb_types_pal_nc,	comb_cntl0_pal_nc,		comb_cntl1_pal_nc,		comb_cntl2_pal_nc,		comb_line_length_pal_nc },
		{ comb_types_pal,		comb_cntl0_pal,			comb_cntl1_pal,			comb_cntl2_pal,			comb_line_length_pal },
		{ comb_types_pal_60,	comb_cntl0_pal_60,		comb_cntl1_pal_60,		comb_cntl2_pal_60,		comb_line_length_pal_60 },
		{ comb_types_secam,		comb_cntl0_secam,		comb_cntl1_secam,		comb_cntl2_secam,		comb_line_length_secam }
	};

	int min_type, max_type, type;
	const comb_settings *settings;
	int i = 0;

	PRINT(("CTheater100::SetCombFilter(%c, %c)\n", "NJ4MNCB6S"[standard], "TCS"[source]));

	// I don't really understand what the different types mean;
	// what is particularly strange is that many types are defined for few standards only
	if( source == C_THEATER_TUNER || source == C_THEATER_COMPOSITE ) {
		min_type = _3Tap_2D_adaptive_Comb;
		max_type = _3Tap_C_combed_Y_combed;
	} else {
		min_type = YC_mode_Comb_filter_off;
		max_type = YC_mode_3Tap_YV_filter;
	}
	
	settings = &comb_settings_list[standard];
	
	for( type = min_type; type <= max_type; ++type ) {
		for( i = 0; settings->types[i]; ++i ) {
			if( settings->types[i] == type )
				break;
		}
		
		if( settings->types[i] != 0 )
			break;
	}
	
	if( type > max_type ) {
		PRINT(("CTheater100::SetCombFilter() - No settings for this standard and input type combination!!!\n"));
		return;
	}

	SetRegister(VIP_COMB_CNTL0, *(const int32 *)(settings->cntl0 + i));
	SetRegister(VIP_COMB_CNTL1, *(const int32 *)(settings->cntl1 + i));
	SetRegister(VIP_COMB_CNTL2, *(const int32 *)(settings->cntl2 + i));
	SetRegister(VIP_COMB_LINE_LENGTH, *(const int32 *)(settings->line_length + i));
	

	// reset the comb filter
	SetRegister(VIP_COMB_CNTL1, Register(VIP_COMB_CNTL1) ^ COMB_SYNCLPFRST);
	SetRegister(VIP_COMB_CNTL1, Register(VIP_COMB_CNTL1) ^ COMB_SYNCLPFRST);
}


// setup luma processor
void CTheater100::SetLuminanceProcessor(theater_standard standard)
{	
	static const uint16 synctip_ref0[] = {
		0x037,	0x037,	0x037,	0x037,	0x037,	0x037,	0x037,	0x037,	0x037 };
	static const uint16 synctip_ref1[] = {
		0x029,	0x029,	0x029,	0x029,	0x029,	0x026,	0x026,	0x026,	0x026 };
	static const uint16 clamp_ref[] = {
		0x03B,	0x03B,	0x03B,	0x03B,	0x03B,	0x03B,	0x03B,	0x03B,	0x03B };
	static const uint16 agc_peakwhite[] = {
		0x0FF,	0x0FF,	0x0FF,	0x0FF,	0x0FF,	0x0FF,	0x0FF,	0x0FF,	0x0FF };
	static const uint16 vbi_peakwhite[] = {
		0x0D2,	0x0D2,	0xD2,	0x0D2,	0x0D2,	0x0C6,	0x0C6,	0x0C6,	0x0C6 };
		
	static const uint16 wpa_threshold[] = {
		0x406,	0x406,	0x4FC,	0x406,	0x59C,	0x488,	0x59C,	0x59C,	0x57A };
	static const uint16 wpa_trigger_lo[] = {
		0x0B3,	0x0B3,	0x0B3,	0x0B3,	0x096,	0x096,	0x096,	0x0B3,	0x096 };
	static const uint16 wpa_trigger_hi[] = {
		0x21B,	0x21B,	0x21B,	0x21B,	0x1C2,	0x1C2,	0x1C2,	0x21B,	0x1C2 };
	static const uint16 lp_lockout_start[] = {
		0x206,	0x206,	0x206,	0x206,	0x263,	0x263,	0x263,	0x206,	0x263 };
	// PAL: changed 0x2c to 0x0c; NTSC: changed 0x21 to 0x0b
	static const uint16 lp_lockout_end[] = {
		0x00B,	0x00B,	0x00B,	0x00B,	0x00C,	0x00C,	0x00C,	0x00B,	0x00C };

	PRINT(("CTheater100::SetLuminanceProcessor(%c)\n", "NJ4MNCB6S"[standard]));

	SetRegister(VIP_LP_AGC_CLAMP_CNTL0, SYNCTIP_REF0, synctip_ref0[standard] << 0);
	SetRegister(VIP_LP_AGC_CLAMP_CNTL0, SYNCTIP_REF1, (uint32)synctip_ref1[standard] << 8);
	SetRegister(VIP_LP_AGC_CLAMP_CNTL0, CLAMP_REF, (uint32)clamp_ref[standard] << 16);
	SetRegister(VIP_LP_AGC_CLAMP_CNTL0, AGC_PEAKWHITE, (uint32)agc_peakwhite[standard] << 24);
	SetRegister(VIP_LP_AGC_CLAMP_CNTL1, VBI_PEAKWHITE, (uint32)vbi_peakwhite[standard] << 0);
	
	SetRegister(VIP_LP_WPA_CNTL0, WPA_THRESHOLD, wpa_threshold[standard] << 0);
	SetRegister(VIP_LP_WPA_CNTL1, WPA_TRIGGER_LO, wpa_trigger_lo[standard] << 0);
	SetRegister(VIP_LP_WPA_CNTL1, WPA_TRIGGER_HI, (uint32)wpa_trigger_hi[standard] << 16);
	SetRegister(VIP_LP_VERT_LOCKOUT, LP_LOCKOUT_START, lp_lockout_start[standard] << 0);
	SetRegister(VIP_LP_VERT_LOCKOUT, LP_LOCKOUT_END, (uint32)lp_lockout_end[standard] << 16);
}


// setup brightness and contrast
void CTheater100::SetLuminanceLevels(theater_standard standard, int brightness, int contrast)
{
	double ref0, setup, gain;

	ref0 = Register(VIP_LP_AGC_CLAMP_CNTL0, SYNCTIP_REF0);

	switch (standard) {
	case C_THEATER_NTSC:
	case C_THEATER_PAL_M:
	case C_THEATER_NTSC_443:
		setup = 7.5 * ref0 / 40.0;
		gain = 219.0 / (92.5 * ref0 / 40.0);
		break;

	case C_THEATER_NTSC_JAPAN:
		setup = 0.0;
		gain = 219.0 / (100.0 * ref0 / 40.0);
		break;

	case C_THEATER_PAL_BDGHI:
	case C_THEATER_PAL_N:
	case C_THEATER_SECAM:
	case C_THEATER_PAL_60:
	case C_THEATER_PAL_NC:
		setup = 0.0;
		gain = 219.0 / (100.0 * ref0 / 43.0);
		break;
		
	default:
		setup = 0.0;
		gain = 0.0;
		break;
	}

	if (contrast <= -100)
		contrast = -99;

	/* set luminance processor constrast (7:0) */
	SetRegister(VIP_LP_CONTRAST, CONTRAST,
		int(64.0 * ((contrast + 100) / 100.0) * gain) << 0);

	/* set luminance processor brightness (13:0) */
	SetRegister(VIP_LP_BRIGHTNESS, BRIGHTNESS,
		int(16.0 * ((brightness - setup) + 16.0 / ((contrast + 100) * gain / 100.0))) & BRIGHTNESS);
}


// setup chroma demodulator
void CTheater100::SetChromaProcessor(theater_standard standard)
{
	PRINT(("CTheater100::SetChromaProcessor(%c)\n", "NJ4MNCB6S"[standard]));
	
	static const uint32 ch_dto_inc[] = {
		0x400000,	0x400000,	0x400000,	0x400000,	0x400000,	0x400000,	0x400000,	0x400000,	0x3E7A28 };
	static const uint8 ch_pll_sgain[] = {
		1,		1,		1,		1,		1,		1,		1,		1,		5 };
	static const uint8 ch_pll_fgain[] = {
		2,		2,		2,		2,		2,		2,		2,		2,		6 };
		
	static const uint8 ch_height[] = {
		0xCD,	0xCD,	0xCD,	0x91,	0x91,	0x9C,	0x9C,	0x9C,	0x66 };
	static const uint8 ch_kill_level[] = {
		0x0C0,	0xC0,	0xC0,	0x8C,	0x8C,	0x90,	0x90,	0x90,	0x60 };
	static const uint8 ch_agc_error_lim[] = {
		2,		2,		2,		2,		2,		2,		2,		2,		3 };
	static const uint8 ch_agc_filter_en[] = {
		0,		0,		0,		0,		0,		0,		1,		0,		0 };
	static const uint8 ch_agc_loop_speed[] = {
		0,		0,		0,		0,		0,		0,		0,		0,		0 };
		
	static const uint16 cr_burst_gain[] = {
		0x7A,	0x71,	0x7A,	0x7A,	0x7A,	0x7A,	0x7A,	0x7A,	0x1FF };
	static const uint16 cb_burst_gain[] = {
		0xAC,	0x9F,	0xAC,	0xAC,	0xAC,	0xAB,	0xAB,	0xAB,	0x1FF };
	static const uint16 crdr_active_gain[] = {
		0x7A,	0x71,	0x7A,	0x7A,	0x7A,	0x7A,	0x7A,	0x7A,	0x11C };
	static const uint16 cbdb_active_gain[] = {
		0xAC,	0x9F,	0xAC,	0xAC,	0xAC,	0xAB,	0xAB,	0xAB,	0x15A };
	static const uint16 cp_vert_lockout_start[] = {
		0x207,	0x207,	0x207,	0x207,	0x269,	0x269,	0x269,	0x207,	0x269 };
	static const uint8 cp_vert_lockout_end[] = {
		0x00E,	0x00E,	0x00E,	0x00E,	0x00E,	0x012,	0x012,	0x00E,	0x012 };

	SetRegister(VIP_CP_PLL_CNTL0, CH_DTO_INC, ch_dto_inc[standard] << 0);
	SetRegister(VIP_CP_PLL_CNTL0, CH_PLL_SGAIN, (uint32)ch_pll_sgain[standard] << 24);
	SetRegister(VIP_CP_PLL_CNTL0, CH_PLL_FGAIN, (uint32)ch_pll_fgain[standard] << 28);

	SetRegister(VIP_CP_AGC_CNTL, CH_HEIGHT, ch_height[standard] << 0);	
	SetRegister(VIP_CP_AGC_CNTL, CH_KILL_LEVEL, (uint32)ch_kill_level[standard] << 8);
	SetRegister(VIP_CP_AGC_CNTL, CH_AGC_ERROR_LIM, (uint32)ch_agc_error_lim[standard] << 16);
	SetRegister(VIP_CP_AGC_CNTL, CH_AGC_FILTER_EN, (uint32)ch_agc_filter_en[standard] << 18);
	SetRegister(VIP_CP_AGC_CNTL, CH_AGC_LOOP_SPEED, (uint32)ch_agc_loop_speed[standard] << 19);

	SetRegister(VIP_CP_BURST_GAIN, CR_BURST_GAIN, cr_burst_gain[standard] << 0);
	SetRegister(VIP_CP_BURST_GAIN, CB_BURST_GAIN, (uint32)cb_burst_gain[standard] << 16);
	
	SetRegister(VIP_CP_ACTIVE_GAIN, CRDR_ACTIVE_GAIN, crdr_active_gain[standard] << 0);
	SetRegister(VIP_CP_ACTIVE_GAIN, CBDB_ACTIVE_GAIN, (uint32)cbdb_active_gain[standard] << 16);

	SetRegister(VIP_CP_VERT_LOCKOUT, CP_LOCKOUT_START, cp_vert_lockout_start[standard] << 0);
	SetRegister(VIP_CP_VERT_LOCKOUT, CP_LOCKOUT_END, (uint32)cp_vert_lockout_end[standard] << 16);
}


// set colour saturation and hue.
// hue makes sense for NTSC only and seems to act as saturation for PAL
void CTheater100::SetChromaLevels(theater_standard standard, int saturation, int hue)
{
	int ref0;
	double gain, CRgain, CBgain;

	/* compute Cr/Cb gains */
	ref0 = Register(VIP_LP_AGC_CLAMP_CNTL0, SYNCTIP_REF0);

	switch (standard) {
	case C_THEATER_NTSC:
	case C_THEATER_NTSC_443:
	case C_THEATER_PAL_M:
		CRgain = (40.0 / ref0) * (100.0 / 92.5) * (1.0 / 0.877) * (112.0 / 70.1) / 1.5;
		CBgain = (40.0 / ref0) * (100.0 / 92.5) * (1.0 / 0.492) * (112.0 / 88.6) / 1.5;
		break;

	case C_THEATER_NTSC_JAPAN:
		CRgain = (40.0 / ref0) * (100.0 / 100.0) * (1.0 / 0.877) * (112.0 / 70.1) / 1.5;
		CBgain = (40.0 / ref0) * (100.0 / 100.0) * (1.0 / 0.492) * (112.0 / 88.6) / 1.5;
		break;

	case C_THEATER_PAL_BDGHI:
	case C_THEATER_PAL_60:
	case C_THEATER_PAL_NC:
	case C_THEATER_PAL_N:
		CRgain = (43.0 / ref0) * (100.0 / 92.5) * (1.0 / 0.877) * (112.0 / 70.1) / 1.5;
		CBgain = (43.0 / ref0) * (100.0 / 92.5) * (1.0 / 0.492) * (112.0 / 88.6) / 1.5;
		break;

	case C_THEATER_SECAM:
		CRgain = 32.0 * 32768.0 / 280000.0 / (33554432.0 / 35.46985) * (1.597 / 1.902) / 1.5;
		CBgain = 32.0 * 32768.0 / 230000.0 / (33554432.0 / 35.46985) * (1.267 / 1.505) / 1.5;
		break;
	
	default:
		PRINT(("CTheater100::SetChromaLevels() - Bad standard\n"));
		CRgain = 0.0;
		CBgain = 0.0;
		break;
	}
	
	if (saturation >= 0)
		gain = 1.0 + 4.9 * saturation / 100.0;
	else
		gain = 1.0 + saturation / 100.0;
		
	SetRegister(VIP_CP_ACTIVE_GAIN, CRDR_ACTIVE_GAIN, int(128 * CRgain * gain) << 0);
	SetRegister(VIP_CP_ACTIVE_GAIN, CBDB_ACTIVE_GAIN, int(128 * CBgain * gain) << 16);

	if (hue >= 0)
		hue = (256 * hue) / 360;
	else
		hue = (256 * (hue + 360)) / 360;

	SetRegister(VIP_CP_HUE_CNTL, HUE_ADJ, hue << 0);
}


// these values are used by scaler as well
static const uint16 h_active_start[] = {
	0x06b,	0x06B,	0x07E,	0x067,	0x09A,	0x07D,	0x09A,	0x084,	0x095 };
static const uint16 h_active_end[] = {
	0x363,	0x363,	0x42A,	0x363,	0x439,	0x439,	0x439,	0x363,	0x439 };
static const uint16 v_active_start[] = {
	0x025,	0x025,	0x025,	0x025,	0x02E,	0x02E,	0x02E,	0x025,	0x02E };
// PAL height is too small (572 instead of 576 lines), but changing 0x269 to 0x26d
// leads to trouble, and the last 2 lines seem to be used for VBI data 
// (read: garbage) anyway
static const uint16 v_active_end[] = {
	0x204,	0x204,	0x204,	0x204,	0x269,	0x269,	0x269,	0x204,	0x269 };
static const uint16 h_vbi_wind_start[] = {
	0x064,	0x064,	0x064,	0x064,	0x084,	0x084,	0x084,	0x064,	0x084 };
static const uint16 h_vbi_wind_end[] = {
	0x366,	0x366,	0x366,	0x366,	0x41F,	0x41F,	0x41F,	0x366,	0x41F };
static const uint16 v_vbi_wind_start[] = {
	0x00b,	0x00b,	0x00b,	0x00b,	0x008,	0x008,	0x008,	0x00b,	0x008 };
static const uint16 v_vbi_wind_end[] = {
	0x024,	0x024,	0x024,	0x024,	0x02d,	0x02d,	0x02d,	0x024,	0x02d };
	
void CTheater100::getActiveRange( theater_standard standard, CRadeonRect &rect )
{
	rect.SetTo( 
		h_active_start[standard], v_active_start[standard],
		h_active_end[standard], v_active_end[standard] );
}

void CTheater100::getVBIRange( theater_standard standard, CRadeonRect &rect )
{
	rect.SetTo( 
		h_vbi_wind_start[standard], v_vbi_wind_start[standard],
		h_vbi_wind_end[standard], v_vbi_wind_end[standard] );
}

// program clipping engine
void CTheater100::SetClipWindow(theater_standard standard, bool vbi)
{		
	// set horizontal active window
	SetRegister(VIP_H_ACTIVE_WINDOW, H_ACTIVE_START, h_active_start[standard] << 0);
	SetRegister(VIP_H_ACTIVE_WINDOW, H_ACTIVE_END, (uint32)h_active_end[standard] << 16);
	
	// set vertical active window
	SetRegister(VIP_V_ACTIVE_WINDOW, V_ACTIVE_START, v_active_start[standard] << 0);
	SetRegister(VIP_V_ACTIVE_WINDOW, V_ACTIVE_END, (uint32)v_active_end[standard] << 16);
	
	// set horizontal VBI window
	SetRegister(VIP_H_VBI_WINDOW, H_VBI_WIND_START, h_vbi_wind_start[standard] << 0);
	SetRegister(VIP_H_VBI_WINDOW, H_VBI_WIND_END, (uint32)h_vbi_wind_end[standard] << 16);
	
	// set vertical VBI window
	SetRegister(VIP_V_VBI_WINDOW, V_VBI_WIND_START, v_vbi_wind_start[standard] << 0);
	SetRegister(VIP_V_VBI_WINDOW, V_VBI_WIND_END, (uint32)v_vbi_wind_end[standard]  << 16);

	// set VBI scaler control
	SetRegister(VIP_VBI_SCALER_CONTROL, (1 << 16) & VBI_SCALING_RATIO);
	
	// enable/disable VBI capture
	SetRegister(VIP_VBI_CONTROL, VBI_CAPTURE_ENABLE,
		vbi ? VBI_CAPTURE_EN : VBI_CAPTURE_DIS);
	
	PRINT(("CTheater100::SetClipWindow(active=%d/%d/%d/%d, vbi=%d/%d/%d/%d)\n",
		Register(VIP_H_ACTIVE_WINDOW, H_ACTIVE_START) >> 0,
		Register(VIP_H_ACTIVE_WINDOW, H_ACTIVE_END) >> 16,
		Register(VIP_V_ACTIVE_WINDOW, V_ACTIVE_START) >> 0,
		Register(VIP_V_ACTIVE_WINDOW, V_ACTIVE_END) >> 16,
		Register(VIP_H_VBI_WINDOW, H_VBI_WIND_START) >> 0,
		Register(VIP_H_VBI_WINDOW, H_VBI_WIND_END) >> 16,
		Register(VIP_V_VBI_WINDOW, V_VBI_WIND_START) >> 0,
		Register(VIP_V_VBI_WINDOW, V_VBI_WIND_END) >> 16));
		
}


// setup capture scaler.
void CTheater100::SetScaler(theater_standard standard, int hactive, int vactive, bool deinterlace)
{
	int oddOffset, evenOffset;
	uint16 h_active_width, v_active_height;
	
//	ASSERT(vactive <= 511);

	// TK: Gatos uses different values here
	h_active_width = h_active_end[standard] - h_active_start[standard] + 1;
	v_active_height = v_active_end[standard] - v_active_start[standard] + 1;
	
	// for PAL, we have 572 lines only, but need 576 lines;
	// my attempts to find those missing lines all failed, so if the application requests
	// 576 lines, we had to upscale the video which is not supported by hardware;
	// solution: restrict to 572 lines - the scaler will fill out the missing lines with black
	if( vactive > v_active_height )
		vactive = v_active_height;
	
	if (deinterlace) {
		// progressive scan
		evenOffset = oddOffset = 512 - (int) ((512 * vactive) / v_active_height);
	}
	else {
		// interlaced
		evenOffset = (int) ((512 * vactive) / v_active_height);
		oddOffset = 2048 - evenOffset;
	}

	// set scale input window
	SetRegister(VIP_SCALER_IN_WINDOW, H_IN_WIND_START, h_active_start[standard] << 0);
	SetRegister(VIP_SCALER_IN_WINDOW, V_IN_WIND_START, (uint32)v_active_start[standard] << 16);

	SetRegister(VIP_SCALER_OUT_WINDOW, H_OUT_WIND_WIDTH, hactive << 0);
	SetRegister(VIP_SCALER_OUT_WINDOW, V_OUT_WIND_HEIGHT, (vactive / 2) << 16);

	SetRegister(VIP_H_SCALER_CONTROL, H_SCALE_RATIO, (((uint32)h_active_width << 16) / hactive) << 0);
	SetRegister(VIP_V_SCALER_CONTROL, V_SCALE_RATIO, ((vactive << 11) / v_active_height) << 0);

	// enable horizontal and vertical scaler
	SetRegister(VIP_H_SCALER_CONTROL, H_BYPASS, 
		h_active_width == hactive ? H_BYPASS : 0);
	SetRegister(VIP_V_SCALER_CONTROL, V_BYPASS, 
		v_active_height == vactive ? V_BYPASS : 0);

	// set deinterlace control
	SetRegister(VIP_V_SCALER_CONTROL, V_DEINTERLACE_ON, deinterlace ? V_DEINTERLACE_ON : 0);
	SetRegister(VIP_V_DEINTERLACE_CONTROL, EVENF_OFFSET, evenOffset << 0);
	SetRegister(VIP_V_DEINTERLACE_CONTROL, ODDF_OFFSET, oddOffset << 11);

	SetRegister(VIP_V_SCALER_CONTROL, V_DEINTERLACE_ON, deinterlace ? V_DEINTERLACE_ON : 0);
	
	PRINT(("CTheater100::SetScaler(active=%d/%d/%d/%d, scale=%d/%d)\n",
		Register(VIP_SCALER_IN_WINDOW, H_IN_WIND_START) >> 0,
		Register(VIP_SCALER_IN_WINDOW, V_IN_WIND_START) >> 16,
		hactive, vactive,
		Register(VIP_H_SCALER_CONTROL, H_SCALE_RATIO),
		Register(VIP_V_SCALER_CONTROL, V_SCALE_RATIO)));
}

int CTheater100::CurrentLine()
{
	return Register(VIP_VS_LINE_COUNT) & VS_LINE_COUNT;
}

void CTheater100::PrintToStream()
{
	PRINT(("<<< Rage Theater Registers >>>\n"));
	for (int index = 0x0400; index <= 0x06ff; index += 4) {
		int value = Register(index);
		value = value; // unused var if debug is off
		PRINT(("REG_0x%04x = 0x%08x\n", index, value));
	}	
}

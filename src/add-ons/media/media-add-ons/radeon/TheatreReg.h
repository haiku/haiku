/******************************************************************************
/
/	File:			Theater.cpp
/
/	Description:	ATI Rage Theater Video Decoder interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef _THEATRE_REG_H
#define _THEATRE_REG_H

enum theater_register {
	// TVOut Front End Datapath
	VIP_RGB_CNTL					= 0x0048,
		RGB_IS_888_PACK				= BITS(0:0),	// Select 24/16 bit (888/565) RGB mode
		UV_DITHER_EN				= BITS(3:2),	// Select dithering mode for U/V data
		SWITCH_TO_BLUE				= BITS(4:4),	// Replace input video with blue screen

	VIP_TVO_SYNC_PAT_ACCUM			= 0x0108,
		H_PATTERN_ACCUM				= BITS(10:0),
		SCAN_REF_OFFSET				= BITS(15:11),
		BYTE0						= BITS(23:16),
		BYTE1						= BITS(31:24),

	VIP_TV0_SYNC_THRESHOLD			= 0x010c,
		MAX_H_PATTERNS				= BITS(10:0),
		MIN_H_PATTERNS				= BITS(27:16),

	VIP_TVO_SYNC_PAT_EXPECT			= 0x0110,
		SYNC_PAT_EXPECT_W0			= BITS(15:0),
		SYNC_PAT_EXPECT_W1			= BITS(23:16),

	VIP_DELAY_ONE_MAP_A				= 0x0114,
		DELAY_ONE_MAP_W0			= BITS(15:0),
		DELAY_ONE_MAP_W1			= BITS(31:16),

	VIP_DELAY_ONE_MAP_B				= 0x0118,
		DELAY_ONE_MAP_W2			= BITS(15:0),

	VIP_DELAY_ZERO_MAP_A			= 0x011c,
		DELAY_ZERO_MAP_W0			= BITS(15:0),
		DELAY_ZERO_MAP_W1			= BITS(31:16),

	VIP_DELAY_ZERO_MAP_B			= 0x0120,
		DELAY_ZERO_MAP_W2			= BITS(15:0),

	VIP_TVO_DATA_DELAY_A			= 0x0140,
		TVO_DATA0_DELAY				= BITS(5:0),
		TVO_DATA1_DELAY				= BITS(13:8),
		TVO_DATA2_DELAY				= BITS(21:16),
		TVO_DATA3_DELAY				= BITS(29:24),

	VIP_TVO_DATA_DELAY_B			= 0x0144,
		TVO_DATA4_DELAY				= BITS(5:0),
		TVO_DATA5_DELAY				= BITS(13:8),
		TVO_DATA6_DELAY				= BITS(21:16),
		TVO_DATA7_DELAY				= BITS(29:24),

	VIP_VSCALER_CNTL1				= 0x01c0,
		UV_INC						= BITS(15:0),	// Vertical scaling of CRTC UV data
		UV_THINNER					= BITS(22:16),	// Number of lines in the UV data that
													// are not blended to create a line on TV
		Y_W_EN						= BITS(24:24),
		Y_DEL_W_SIG					= BITS(27:26),

	VIP_VSCALER_CNTL2				= 0x01c8,
		DITHER_MODE					= BITS(0:0),	// Select dithering mode
		Y_OUTPUT_DITHER_EN			= BITS(1:1),
		UV_OUTPUT_DITHER_EN			= BITS(2:2),
		UV_TO_BUF_DITHER_EN			= BITS(3:3),
		UV_ACCUM_INIT				= BITS(31:24),

	VIP_Y_FALL_CNTL					= 0x01cc,
		Y_FALL_ACCUM_INIT			= BITS(15:0),
		Y_FALL_PING_PONG			= BITS(16:16),
		Y_COEF_EN					= BITS(17:17),
		Y_COEF_VALUE				= BITS(31:24),

	VIP_Y_RISE_CNTL					= 0x01d0,
		Y_RISE_ACCUM_INIT			= BITS(15:0),
		Y_RISE_PING_PONG			= BITS(16:16),

	VIP_Y_SAW_TOOTH_CNTL			= 0x01d4,
		Y_SAW_TOOTH_MAP				= BITS(15:0),
		Y_SAW_TOOTH_SLOPE			= BITS(31:16),

	// TVOut Shadow CRTC
	VIP_HTOTAL						= 0x0080,
		D_HTOTAL					= BITS(10:0),	// Number of clocks per line (-1)

	VIP_HDISP						= 0x0084,
		D_HDISP						= BITS(9:0),	// Number of active pixels per line (-1)

	VIP_HSIZE						= 0x0088,
		D_HSIZE						= BITS(9:0),	// Unused in Rage Theater A21 and later

	VIP_HSTART						= 0x008c,
		D_HSTART					= BITS(10:0),

	VIP_HCOUNT						= 0x0090,		// Current horizontal CRT pixel within
		D_HCOUNT					= BITS(10:0),	// a line being processed

	VIP_VTOTAL						= 0x0094,		// Number of lines per frame (-1)
		D_VTOTAL					= BITS(9:0),

	VIP_VDISP						= 0x0098,		// Number of active lines (-1)
		D_VDISP						= BITS(9:0),

	VIP_VCOUNT						= 0x009c,		// Current vertical CRT line being processed
		D_VCOUNT					= BITS(9:0),

	VIP_VFTOTAL						= 0x00a0,		// Number of CRT frames that occur during
		D_FTOTAL					= BITS(3:0),	// one complete cycle of the TV timing (-1)

	VIP_DFCOUNT						= 0x00a4,		// Current CRT frame count, equivalent to the
		D_FCOUNT					= BITS(3:0),	// field number in a complete TV cycle

	VIP_DFRESTART					= 0x00a8,		// The frame/field during which a restart
		D_FRESTART					= BITS(3:0),	// will be generated when TV_MASTER is 0

	VIP_DHRESTART					= 0x00ac,		// Horizontal pixel during which a restart
		D_HRESTART					= BITS(10:0),	// will be generated when TV_MASTER is 0

	VIP_DVRESTART					= 0x00b0,		// Vertical line during which a restart
		D_VRESTART					= BITS(9:0),	// will be generated when TV_MASTER is 0

	VIP_SYNC_SIZE					= 0x00b4,		// Number of pixels expected in the
		D_SYNC_SIZE					= BITS(9:0),	// synchronization line

	VIP_FRAME_LOCK_CNTL				= 0x0100,
		MAX_LOCK_STR				= BITS(3:0),
		FIELD_SYNC_EN				= BITS(4:4),
		FIELD_SYNC_TRIGGER			= BITS(5:5),
		ACTUAL_LOCK_STR				= BITS(11:8),
		AVG_MISSED_SYNC				= BITS(15:12),

	VIP_SYNC_LOCK_CNTL				= 0x0104,
		PRI_TVO_DATA_LINE_SEL		= BITS(2:0),
		SEC_SYNC_CHECK_DEL			= BITS(13:8),
		MPP_ACTIVE_AS_MASK			= BITS(14:14),
		MPP_ACTIVE_DS_MASK			= BITS(15:15),
		SYNC_COMMAND				= BITS(18:16),
		SYNC_LOCK_TRIGGER			= BITS(21:21),
		DETECT_EN					= BITS(24:24),
		SELF_LOCK_EN				= BITS(25:25),
		MAN_LOCK_EN					= BITS(26:26),
		MAX_REF_OFFSET				= BITS(31:27),

	// TVOut Up Sampling Filter
	VIP_UPSAMP_COEFF0_0				= 0x0340,
		COEFF0_0					= BITS(5:0),
		COEFF0_1					= BITS(14:8),
		COEFF0_2					= BITS(22:16),
		COEFF0_3					= BITS(29:24),

	VIP_UPSAMP_COEFF0_1				= 0x0344,
		COEFF0_4					= BITS(7:0),
		COEFF0_5					= BITS(15:8),
		COEFF0_6					= BITS(21:16),
		COEFF0_7					= BITS(30:24),

	VIP_UPSAMP_COEFF0_2				= 0x0348,
		COEFF0_8					= BITS(6:0),
		COEFF0_9					= BITS(13:8),

	VIP_UPSAMP_COEFF1_0				= 0x034c,
		COEFF1_0					= BITS(5:0),
		COEFF1_1					= BITS(14:8),
		COEFF1_2					= BITS(22:16),
		COEFF1_3					= BITS(29:24),

	VIP_UPSAMP_COEFF1_1				= 0x0350,
		COEFF1_4					= BITS(7:0),
		COEFF1_5					= BITS(15:8),
		COEFF1_6					= BITS(21:16),
		COEFF1_7					= BITS(30:24),

	VIP_UPSAMP_COEFF1_2				= 0x0354,
		COEFF1_8					= BITS(6:0),
		COEFF1_9					= BITS(13:8),

	VIP_UPSAMP_COEFF2_0				= 0x0358,
		COEFF2_0					= BITS(5:0),
		COEFF2_1					= BITS(14:8),
		COEFF2_2					= BITS(22:16),
		COEFF2_3					= BITS(29:24),

	VIP_UPSAMP_COEFF2_1				= 0x035c,
		COEFF2_4					= BITS(7:0),
		COEFF2_5					= BITS(15:8),
		COEFF2_6					= BITS(21:16),
		COEFF2_7					= BITS(30:24),

	VIP_UPSAMP_COEFF2_2				= 0x0360,
		COEFF2_8					= BITS(6:0),
		COEFF2_9					= BITS(13:8),

	VIP_UPSAMP_COEFF3_0				= 0x0364,
		COEFF3_0					= BITS(5:0),
		COEFF3_1					= BITS(14:8),
		COEFF3_2					= BITS(22:16),
		COEFF3_3					= BITS(29:24),

	VIP_UPSAMP_COEFF3_1				= 0x0368,
		COEFF3_4					= BITS(7:0),
		COEFF3_5					= BITS(15:8),
		COEFF3_6					= BITS(21:16),
		COEFF3_7					= BITS(30:24),

	VIP_UPSAMP_COEFF3_2				= 0x036c,
		COEFF3_8					= BITS(6:0),
		COEFF3_9					= BITS(13:8),

	VIP_UPSAMP_COEFF4_0				= 0x0370,
		COEFF4_0					= BITS(5:0),
		COEFF4_1					= BITS(14:8),
		COEFF4_2					= BITS(22:16),
		COEFF4_3					= BITS(29:24),

	VIP_UPSAMP_COEFF4_1				= 0x0374,
		COEFF4_4					= BITS(7:0),
		COEFF4_5					= BITS(15:8),
		COEFF4_6					= BITS(21:16),
		COEFF4_7					= BITS(30:24),

	VIP_UPSAMP_COEFF4_2				= 0x0378,
		COEFF4_8					= BITS(6:0),
		COEFF4_9					= BITS(13:8),

	// TVOut Encoder
	VIP_SYNC_CNTL					= 0x0050,
		SYNC_OE						= BITS(0:0),	// Sync output enable
		SYNC_OUT					= BITS(1:1),	// Sync output data
		SYNC_IN						= BITS(2:2),	// Sync input data
		SYNC_PUB					= BITS(3:3),	// Sync pull-up enable
		SYNC_PD						= BITS(4:4),	// Sync pull-down enable
		SYNC_DRV					= BITS(5:5),	// Sync drive select
		SYNC_MX						= BITS(11:8),	// Sync mux

	VIP_HOST_READ_DATA				= 0x0180,
		HOST_RD_DATA_W0				= BITS(15:0),
		HOST_RD_DATA_W1				= BITS(27:16),

	VIP_HOST_WRITE_DATA				= 0x0184,
		HOST_WR_DATA_W0				= BITS(15:0),
		HOST_WR_DATA_W1				= BITS(27:16),

	VIP_HOST_RD_WT_CNTL				= 0x0188,
		HOST_ADR					= BITS(8:0),
		HOST_FIFO_RD				= BITS(12:12),
		HOST_FIFO_RD_ACK			= BITS(13:13),
		HOST_FIFO_WT				= BITS(14:14),
		HOST_FIFO_WT_ACK			= BITS(15:15),

	VIP_TIMING_CNTL					= 0x01c4,
		H_INC						= BITS(11:0),	// Horizontal scaling of the TV image
		REQ_DELAY					= BITS(18:16),
		REQ_Y_FIRST					= BITS(19:19),
		FORCE_BURST_ALWAYS			= BITS(21:21),
		UV_POST_SCALE_BYPASS		= BITS(23:23),
		UV_OUTPUT_POST_SCALE		= BITS(31:24),

	VIP_UPSAMP_AND_GAIN_CNTL		= 0x01e0,
		YUPSAMP_EN					= BITS(0:0),	// Enable Y upsampling filter
		YUPSAMP_FLAT				= BITS(1:1),	// Force Y upsampling to use centre tap
		UVUPSAMP_EN					= BITS(2:2),	// Enable U/V upsampling filters
		UVUPSAMP_FLAT				= BITS(3:3),	// Force U/V upsampling to use centre tap
		Y_BREAK_EN					= BITS(8:8),	// Enable Y break point
		UV_BREAK_EN					= BITS(10:10),	// Enable U/V break point

	VIP_GAIN_LIMIT_SETTINGS			= 0x01e4,
		Y_GAIN_LIMIT				= BITS(10:0),	// Gain limit for the luminance (Y)
		UV_GAIN_LIMIT				= BITS(24:16),	// Gain limit for the chrominance (U/V)

	VIP_LINEAR_GAIN_SETTINGS		= 0x01e8,
		Y_GAIN						= BITS(8:0),	// Gain for the luminance (1.8 fixed point)
		UV_GAIN						= BITS(24:16),	// Gain for the chrominance (1.8 fixed point)

	VIP_MODULATOR_CNTL1				= 0x0200,
		YFLT_EN						= BITS(2:2),	// Enable Composite/SVideo Y filter
		UVFLT_EN					= BITS(3:3),	// Enable U/V filters
		ALT_PHASE_EN				= BITS(6:6),	// Phase alternating line (0=NTSC, 1=PAL)
		SYNC_TIP_LEVEL				= BITS(7:7),	// Composite Y sync tip level
		SET_UP_LEVEL				= BITS(14:8),	// Video setup level
		BLANK_LEVEL					= BITS(22:16),	// Video blank level
		SLEW_RATE_LIMIT				= BITS(23:23),
		FORCE_BLACK_WHITE			= BITS(24:24),	// Force B&W video
		Y_FILT_BLEND				= BITS(31:28),	// Sharpness of Y filters

	VIP_MODULATOR_CNTL2				= 0x0204,
		U_BURST_LEVEL				= BITS(8:0),
		V_BUST_LEVEL				= BITS(24:16),

	VIP_PRE_DAC_MUX_CNTL			= 0x0240,
		Y_RED_EN					= BITS(0:0),
		C_GRN_EN					= BITS(1:1),
		CMP_BLU_EN					= BITS(2:2),
		DAC_DITHER_EN				= BITS(3:3),
		RED_MX						= BITS(7:4),
		GRN_MX						= BITS(11:8),
		BLU_MX						= BITS(15:12),
		FORCE_DAC_DATA				= BITS(25:16),
		YUPFILT_DISABLE				= BITS(26:26),
		CVUPFILT_DISABLE			= BITS(27:27),
		UUPFILT_DISABLE				= BITS(28:28),

	VIP_TV_DAC_CNTL					= 0x0280,
		NBLANK						= BITS(0:0),
		NHOLD						= BITS(1:1),
		PEDESTAL					= BITS(2:2),
		DASLEEP						= BITS(3:3),
		DETECT						= BITS(4:4),
		CMPOUT						= BITS(5:5),
		BGSLEEP						= BITS(6:6),
		STD							= BITS(9:8),
		MON							= BITS(15:12),

	VIP_CRC_CNTL					= 0x02c0,
		V_COMP_DATA_EN				= BITS(1:0),
		V_COMP_GATE					= BITS(2:2),
		V_COMP_EN					= BITS(3:3),
		RST_SUBC_ONRSTRT			= BITS(4:4),
		CRC_TV_RSTRT_SEL			= BITS(5:5),

	VIP_VIDEO_PORT_SIG				= 0x02c4,
		CRC_SIG						= BITS(29:0),

	VIP_UV_ADR						= 0x0300,
		MAX_UV_ADDR					= BITS(7:0),
		TABLE1_BOT_ADR				= BITS(15:8),
		TABLE3_TOP_ADR				= BITS(23:16),
		HCODE_TABLE_SEL				= BITS(26:25),
		VCODE_TABLE_SEL				= BITS(28:27),
		SWITCH_TABLE_REQ			= BITS(31:31),

	 // TVOut VBI Control
	VIP_VBI_CC_CNTL					= 0x02c8,
		VBI_CC_DATA					= BITS(15:0),	// VBI data for CC
		VBI_CC_WT					= BITS(24:24),	// Initiates a write cycle using VBI_CC_DATA
		VBI_CC_WT_ACK				= BITS(25:25),
		VBI_CC_HOLD					= BITS(26:26),
		VBI_DECODE_EN				= BITS(31:31),

	VIP_VBI_EDS_CNTL				= 0x02cc,
		VBI_EDS_DATA				= BITS(15:0),
		VBI_EDS_WT					= BITS(24:24),
		VBI_EDS_WT_ACK				= BITS(25:25),
		VBI_EDS_HOLD				= BITS(26:26),

	VIP_VBI_20BIT_CNTL				= 0x02d0,
		VBI_20BIT_DATA0				= BITS(15:0),
		VBI_20BIT_DATA1				= BITS(19:16),
		VBI_20BIT_WT				= BITS(24:24),
		VBI_20BIT_WT_ACK			= BITS(25:25),
		VBI_20BIT_HOLD				= BITS(26:26),

	VIP_VBI_DTO_CNTL				= 0x02d4,
		VBI_CC_DTO_P				= BITS(15:0),
		VBI_20BIT_DTO_P				= BITS(31:16),

	VIP_VBI_LEVEL_CNTL				= 0x02d8,
		VBI_CC_LEVEL				= BITS(6:0),
		VBI_20BIT_LEVEL				= BITS(14:8),
		VBI_CLK_RUNIN_GAIN			= BITS(24:16),

	// Video Decoder Horizontal Sync PLL Control
	VIP_HS_PLINE					= 0x0480,		// Pixels per line (910)
		HS_LINE_TOTAL				= BITS(10:0),

	VIP_HS_DTOINC					= 0x0484,		// ???
		HS_DTO_INC					= BITS(19:0),

	VIP_HS_PLLGAIN					= 0x0488,
		HS_PLL_SGAIN				= BITS(3:0),
		HS_PLL_FGAIN				= BITS(7:4),

	VIP_HS_MINMAXWIDTH				= 0x048c,
		MIN_PULSE_WIDTH				= BITS(7:0),
		MAX_PULSE_WIDTH				= BITS(15:8),

	VIP_HS_GENLOCKDELAY				= 0x0490,
		GEN_LOCK_DELAY				= BITS(7:0),

	VIP_HS_WINDOW_LIMIT				= 0x0494,
		WIN_CLOSE_LIMIT				= BITS(10:0),
		WIN_OPEN_LIMIT				= BITS(26:16),

	VIP_HS_WINDOW_OC_SPEED			= 0x0498,
		WIN_CLOSE_SPEED				= BITS(3:0),
		WIN_OPEN_SPEED				= BITS(7:4),

	VIP_HS_PULSE_WIDTH				= 0x049c,
		H_SYNC_PULSE_WIDTH			= BITS(7:0),
		HS_GENLOCKED				= BITS(8:8),	//   HPLL is locked?
		HS_SYNC_IN_WIN				= BITS(9:9),	//   Sync in Hwindow?

	VIP_HS_PLL_ERROR				= 0x04a0,
		HS_PLL_ERROR				= BITS(14:0),

	VIP_HS_PLL_FS_PATH				= 0x04a4,
		HS_PLL_FAST_PATH			= BITS(14:0),
		HS_PLL_SLOW_PATH			= BITS(30:16),

	// Video Decoder Comb Filter
	VIP_COMB_CNTL0					= 0x0440,
		COMB_HCK					= BITS(7:0),
		COMB_VCK					= BITS(15:8),
		COMB_FILTER_EN				= BITS(16:16),	// 0=fast AGC, CLAMP, and Chroma AGC loops (A41 ASIC only)
		COMB_ADAPTIVE_EN			= BITS(17:17),
		COMB_BPFMUXSEL				= BITS(20:18),
		COMB_COUTSEL				= BITS(22:21),
		COMB_SUMDIFF0SEL			= BITS(23:23),
		COMB_SUMDIFF1SEL			= BITS(25:24),
		COMB_YVLPFSEL				= BITS(26:26),
		COMB_DLYLINESEL				= BITS(28:27),
		COMB_YDLYINSEL				= BITS(30:29),
		COMB_YSUBBW					= BITS(31:31),

	VIP_COMB_CNTL1					= 0x0444,
		COMB_YDLYOUTSEL				= BITS(1:0),
		COMB_CORESIZE				= BITS(3:2),
		COMB_YSUBEN					= BITS(4:4),
		COMB_YOUTSEL				= BITS(5:5),
		COMB_SYNCLPFSEL				= BITS(7:6),
		COMB_SYNCLPFRST				= BITS(8:8),
		COMB_DEBUG					= BITS(9:9),

	VIP_COMB_CNTL2					= 0x0448,
		COMB_HYK0					= BITS(7:0),
		COMB_VYK0					= BITS(15:8),
		COMB_HYK1					= BITS(23:16),
		COMB_VYK1					= BITS(31:24),

	VIP_COMB_LINE_LENGTH			= 0x044c,
		COMB_TAP0LENGTH				= BITS(10:0),
		COMB_TAP1LENGTH				= BITS(27:16),

	VIP_NOISE_CNTL0					= 0x0450,
		NR_EN						= BITS(0:0),
		NR_GAIN_CNTL				= BITS(3:1),
		NR_BW_TRESH					= BITS(9:4),
		NR_GC_TRESH					= BITS(14:10),
		NR_COEF_DESPEC_IMODE		= BITS(15:15),

	// Video Decoder ADC Control
	VIP_ADC_CNTL					= 0x0400,
		INPUT_SELECT				= BITS(2:0),	// Video input mux select
			INPUT_SELECT_COMP0		= 0 << 0,		//   Tuner
			INPUT_SELECT_COMP1		= 1 << 0,		//   Front Comp1
			INPUT_SELECT_COMP2		= 2 << 0,		//   Rear Comp1
			INPUT_SELECT_YF_COMP3	= 3 << 0,		//   Front Comp2
			INPUT_SELECT_YR_COMP4	= 4 << 0,		//   Rear Comp2
			INPUT_SELECT_YCF_COMP3	= 5 << 0,		//   Front YC
			INPUT_SELECT_YCR_COMP4	= 6 << 0,		//   Rear YC
		I_CLAMP_SEL					= BITS(4:3),	// Clamp charge-pump current select
			I_CLAMP_SEL_0_3			= 0 << 3,		//   0.3 uA
			I_CLAMP_SEL_7			= 1 << 3,		//   7.0 uA
			I_CLAMP_SEL_15			= 2 << 3,		//   15.0 uA
			I_CLAMP_SEL_22			= 3 << 3,		//   22.0 uA
		I_AGC_SEL					= BITS(6:5),	// AGC charge-pump current select
			I_AGC_SEL_0_3			= 0 << 5,		//   0.3 uA
			I_AGC_SEL_7				= 1 << 5,		//   7.0 uA
			I_AGC_SEL_15			= 2 << 5,		//   15.0 uA
			I_AGC_SEL_22			= 3 << 5,		//   22.0 uA
		ADC_PDWN					= BITS(7:7),	// AGC power-down select
			ADC_PDWN_UP				= 0 << 7,		//   Power up (for capture mode)
			ADC_PDWN_DOWN			= 1 << 7,		//   Power down
		EXT_CLAMP_CAP				= BITS(8:8),	// Clamp charge cap select
			EXT_CLAMP_CAP_INTERNAL	= 0 << 8,		//   Use internal Clamp Cap.
			EXT_CLAMP_CAP_EXTERNAL	= 1 << 8,		//   Use external Clamp Cap.
		EXT_AGC_CAP					= BITS(9:9),	// AGC charge Cap. select
			EXT_AGC_CAP_INTERNAL	= 0 << 9,		//   Use internal AGC Cap.
			EXT_AGC_CAP_EXTERNAL	= 1 << 9,		//   Use external AGC Cap.
		ADC_DECI_BYPASS				= BITS(10:10),	// ADC video data decimation filter select
			ADC_DECI_WITH_FILTER	= 0 << 10,		//   Decimate ADC data with filtering
			ADC_DECI_WITHOUT_FILTER	= 1 << 10,		//   Decimate ADC data with no filtering
		VBI_DECI_BYPASS				= BITS(11:11),	// ADC VBI data decimation filter select
			VBI_DECI_WITH_FILTER	= 0 << 11,		//   Decimate VBI data from ADC with filtering
			VBI_DECI_WITHOUT_FILTER	= 1 << 11,		//   Decimate VBI data from ADC with no filtering
		DECI_DITHER_EN				= BITS(12:12),	// Decimation filter output dither enable
		ADC_CLK_SEL					= BITS(13:13),	// ADC clock select
			ADC_CLK_SEL_4X			= 0 << 13,		//   Run ADC at 4x Fsc
			ADC_CLK_SEL_8X			= 1 << 13,		//   Run ADC at 8x Fsc
		ADC_BYPASS					= BITS(15:14),	// ADC data path select
			ADC_BYPASS_INTERNAL		= 0 << 14,		//   Use internal ADC
			ADC_BYPASS_EXTERNAL		= 1 << 14,		//   Use external ADC
			ADC_BYPASS_SINGLE		= 2 << 14,		//   Use single step data
		ADC_CH_GAIN_SEL				= BITS(17:16),	// Analog Chroma gain select
			ADC_CH_GAIN_SEL_NTSC	= 0 << 16,		//   Set chroma gain for NTSC
			ADC_CH_GAIN_SEL_PAL		= 1 << 16,		//   Set chroma gain for PAL
		ADC_PAICM					= BITS(19:18),	// AMP common mode voltage select
		ADC_PDCBIAS					= BITS(21:20),	// DC 1.5V bias programmable select
		ADC_PREFHI					= BITS(23:22),	// ADC voltage reference high
			ADC_PREFHI_2_7			= 0 << 22,		//   2.7V (recommended)
			ADC_PREFHI_2_6			= 1 << 22,		//   2.6V
			ADC_PREFHI_2_5			= 2 << 22,		//   2.5V
			ADC_PREFHI_2_4			= 3 << 22,		//   2.4V
		ADC_PREFLO					= BITS(25:24),	// ADC voltage reference low
			ADC_PREFLO_1_8			= 0 << 24,		//   1.8V
			ADC_PREFLO_1_7			= 1 << 24,		//   1.7V
			ADC_PREFLO_1_6			= 2 << 24,		//   1.6V
			ADC_PREFLO_1_5			= 3 << 24,		//   1.5V (recommended)
		ADC_IMUXOFF					= BITS(26:26),
		ADC_CPRESET					= BITS(27:27),	// AGC charge pump reset

	VIP_ADC_DEBUG					= 0x0404,
		ADC_PTST					= BITS(0:0),	// AGC test mode enable
		ADC_PYPDN					= BITS(1:1),
		ADC_PCPDN					= BITS(2:2),	// Chroma AGC path power down mode
		ADC_PTSTA0					= BITS(3:3),	// AGC test mux A select bit 0
		ADC_PTSTA1					= BITS(4:4),	// AGC test mux A select bit 1
		ADC_PTSTB0					= BITS(5:5),	// AGC test mux B select bit 0
		ADC_PTSTB1					= BITS(6:6),	// AGC test mux B select bit 1
		ADC_TSTADC					= BITS(7:7),	// Luma & Chroma ADC test mode
		ADC_TSTPROBEY				= BITS(8:8),	// Luma AGC/ADC test mode
		ADC_TSTPROBEC				= BITS(9:9),	// Chroma AGC/ADC test mode
		ADC_TSTPROBEADC				= BITS(10:10),	// Chroma ADC test structure probe mode
		ADC_TSTADCBIAS				= BITS(11:11),	// Chroma ADC bias node probe mode
		ADC_TSTADCREFM				= BITS(12:12),	// Middle reference point for Luma & Chroma ADC probe
		ADC_TSTADCFBP				= BITS(13:13),	// Chroma ADC folding block positive output probe mode
		ADC_TSTADCFBN				= BITS(14:14),	// Chroma ADC folding block negative output probe mode
		ADC_TSTADCCMP1				= BITS(15:15),	// Chroma ADC comparator #1 output probe mode
		ADC_TSTADCCMP9				= BITS(16:16),	// Chroma ADC comparator #9 output probe mode
		ADC_TSTADCCMP17				= BITS(17:17),	// Chroma ADC comparator #19 output probe mode
		ADC_TSTADCLATCH				= BITS(18:18),	// Dummy latch test mode
		ADC_TSTADCCOMP				= BITS(19:19),	// Dummy comparator test mode

	VIP_THERMO2BIN_STATUS			= 0x040c,
		YOVERFLOW					= BITS(0:0),
		YUNDERFLOW					= BITS(1:1),
		YMSB_LOW_BY_ONE				= BITS(2:2),
		YMSB_HI_BY_ONE				= BITS(3:3),
		COVERFLOW					= BITS(4:4),
		CUNDERFLOW					= BITS(5:5),
		CMSB_LOW_BY_ONE				= BITS(6:6),
		CMSB_HI_BY_ONE				= BITS(7:7),

	// Video Decoder Sync Generator
	VIP_SG_BLACK_GATE				= 0x04c0,		// horizontal blank
		BLANK_INT_START				= BITS(7:0),	//   start of horizontal blank (49)
		BLANK_INT_LENGTH			= BITS(11:8),

	VIP_SG_SYNCTIP_GATE				= 0x04c4,		// synctip pulse
		SYNC_TIP_START				= BITS(10:0),	//   start of sync pulse (882)
		SYNC_TIP_LENGTH				= BITS(15:12),

	VIP_SG_UVGATE_GATE				= 0x04c8,		// chroma burst
		UV_INT_START				= BITS(7:0),	//   start of chroma burst (59)
		U_INT_LENGTH				= BITS(11:8),
		V_INT_LENGTH				= BITS(15:12),

	// Video Decoder Luminance Processor
	VIP_LP_AGC_CLAMP_CNTL0			= 0x0500,		// Luma AGC Clamp control
		SYNCTIP_REF0				= BITS(7:0),	//   40 IRE reference
		SYNCTIP_REF1				= BITS(15:8),
		CLAMP_REF					= BITS(23:16),
		AGC_PEAKWHITE				= BITS(31:24),

	VIP_LP_AGC_CLAMP_CNTL1			= 0x0504,		// Luma AGC Clamp control
		VBI_PEAKWHITE				= BITS(7:0),	//
		CLAMPLOOP_EN				= BITS(24:24),	//   Run Clamp loop
		CLAMPLOOP_INV				= BITS(25:25),	//   Negative Clamp Loop
		AGCLOOP_EN					= BITS(26:26),	//   Run AGC loop
		AGCLOOP_INV					= BITS(27:27),	//   Negative AGC loop

	VIP_LP_BRIGHTNESS				= 0x0508,		// Luma Brightness control
		BRIGHTNESS					= BITS(13:0),	//   Brightness level
		LUMAFLT_SEL					= BITS(15:15),	//   Select flat filter

	VIP_LP_CONTRAST					= 0x050c,		// Luma Contrast level
		CONTRAST					= BITS(7:0),	//   Contrast level
		DITHER_SEL					= BITS(9:8),	//   Dither selection
			DITHER_SEL_TRUNC		= 0 << 8,		//     Truncation
			DITHER_SEL_ROUND		= 1 << 8,		//     Round
			DITHER_SEL_4BIT			= 2 << 8,		//     4 bit error
			DITHER_SEL_9BIT			= 3 << 9,		//     9 bit error

	VIP_LP_SLICE_LIMIT				= 0x0510,
		SLICE_LIMIT_HI				= BITS(7:0),
		SLICE_LIMIT_LO				= BITS(15:8),
		SLICE_LIMIT					= BITS(23:16),

	VIP_LP_WPA_CNTL0				= 0x0514,
		WPA_THRESHOLD				= BITS(10:0),

	VIP_LP_WPA_CNTL1				= 0x0518,
		WPA_TRIGGER_LO				= BITS(9:0),
		WPA_TRIGGER_HI				= BITS(25:16),

	VIP_LP_BLACK_LEVEL				= 0x051c,
		BLACK_LEVEL					= BITS(12:0),

	VIP_LP_SLICE_LEVEL				= 0x0520,
		SLICE_LEVEL					= BITS(7:0),

	VIP_LP_SYNCTIP_LEVEL			= 0x0524,
		SYNCTIP_LEVEL				= BITS(12:0),

	VIP_LP_VERT_LOCKOUT				= 0x0528,
		LP_LOCKOUT_START			= BITS(9:0),
		LP_LOCKOUT_END				= BITS(25:16),

	// Video Decoder Vertical Sync Detector/Counter
	VIP_VS_DETECTOR_CNTL			= 0x0540,
		VSYNC_INT_TRIGGER			= BITS(10:0),
		VSYNC_INT_HOLD				= BITS(26:16),

	VIP_VS_BLANKING_CNTL			= 0x0544,
		VS_FIELD_BLANK_START		= BITS(9:0),
		VS_FIELD_BLANK_END			= BITS(25:16),

	VIP_VS_FIELD_ID_CNTL			= 0x0548,
		VS_FIELD_ID_LOCATION		= BITS(8:0),

	VIP_VS_COUNTER_CNTL				= 0x054c,		// Vertical Sync Counter control
		FIELD_DETECT_MODE			= BITS(1:0),	//   Field detection mode
			FIELD_DETECT_ARTIFICIAL	= 0 << 0,		//     Use artificial field
			FIELD_DETECT_DETECTED	= 1 << 0,		//     Use detected field
			FIELD_DETECT_AUTO		= 2 << 0,		//     Auto switch to Artificial if interlace is lost
			FIELD_DETECT_FORCE		= 3 << 0,		//     Use field force bit
		FIELD_FLIP_EN				= BITS(2:2),	// 	 Flip the fields
		FIELD_FORCE_EN				= BITS(3:3),	//   Force field number
		VSYNC_WINDOW_EN				= BITS(4:4),	//   Enable VSYNC window

	VIP_VS_FRAME_TOTAL				= 0x0550,
		VS_FRAME_TOTAL				= BITS(9:0),	// number of lines per frame

	VIP_VS_LINE_COUNT				= 0x0554,
		VS_LINE_COUNT				= BITS(9:0),	// current line counter
		VS_ITU656_VB				= BITS(13:13),
		VS_ITU656_FID				= BITS(14:14),
		VS_INTERLACE_DETECTED		= BITS(15:15),
		VS_DETECTED_LINES			= BITS(25:16),	// detected number of lines per frame
		CURRENT_FIELD				= BITS(27:27),	// current field number (odd or even)
		PREVIOUS_FIELD				= BITS(28:28),	// previous field number (odd or even)
		ARTIFICIAL_FIELD			= BITS(29:29),
		VS_WINDOW_COUNT				= BITS(31:30),

	// Video Decoder Chroma Processor
	VIP_CP_PLL_CNTL0				= 0x0580,
		CH_DTO_INC					= BITS(23:0),
		CH_PLL_SGAIN				= BITS(27:24),
		CH_PLL_FGAIN				= BITS(31:28),

	VIP_CP_PLL_CNTL1				= 0x0584,
		VFIR						= BITS(0:0),	// 0=disable, 1=enable phase filter FIR
		PFLIP						= BITS(1:1),	// 0=Use PAL/SECAM Vswitch as detected
													// 1=Flip detected PAL/SECAM Vswitch
		PFILT						= BITS(2:2),	// 0=Use sign bit of phase error for PAL Vswitch
													// 1=Use PAL Vswitch filter

	VIP_CP_HUE_CNTL					= 0x0588,
		HUE_ADJ						= BITS(7:0),	// Hue adjustment

	VIP_CP_BURST_GAIN				= 0x058c,
		CR_BURST_GAIN				= BITS(8:0),
		CB_BURST_GAIN				= BITS(24:16),

	VIP_CP_AGC_CNTL					= 0x0590,
		CH_HEIGHT					= BITS(7:0),
		CH_KILL_LEVEL				= BITS(15:8),
		CH_AGC_ERROR_LIM			= BITS(17:16),	// Force error to 0, 1, 2 or 3
		CH_AGC_FILTER_EN			= BITS(18:18),	// 0=disable, 1=enable filter
		CH_AGC_LOOP_SPEED			= BITS(19:19),	// 0=slow, 1=fast

	VIP_CP_ACTIVE_GAIN				= 0x0594,
		CRDR_ACTIVE_GAIN			= BITS(9:0),	// Saturation adjustment
		CBDB_ACTIVE_GAIN			= BITS(25:16),

	VIP_CP_PLL_STATUS0				= 0x0598,
		CH_GAIN_ACC0				= BITS(13:0),
		CH_GAIN_ACC1				= BITS(29:16),

	VIP_CP_PLL_STATUS1				= 0x059c,
		CH_VINT_OUT					= BITS(18:0),

	VIP_CP_PLL_STATUS2				= 0x05a0,
		CH_UINT_OUT					= BITS(12:0),
		CH_VSWITCH					= BITS(16:16),
		CH_SECAM_SWITCH				= BITS(17:17),
		CH_PAL_SWITCH				= BITS(18:18),
		CH_PAL_FLT_STAT				= BITS(21:19),
		CH_COLOR_KILL				= BITS(22:22),

	VIP_CP_PLL_STATUS3				= 0x05a4,
		CH_ERROR_INT0				= BITS(20:0),

	VIP_CP_PLL_STATUS4				= 0x05a8,
		CH_ERROR_INT1				= BITS(20:0),

	VIP_CP_PLL_STATUS5				= 0x05ac,
		CH_FAST_PATH				= BITS(24:0),

	VIP_CP_PLL_STATUS6				= 0x05b0,
		CH_SLOW_PATH				= BITS(24:0),

	VIP_CP_PLL_STATUS7				= 0x05b4,
		FIELD_BPHASE_COUNT			= BITS(5:0),
		BPHASE_BURST_COUNT			= BITS(13:8),

	VIP_CP_DEBUG_FORCE				= 0x05b8,
		GAIN_FORCE_DATA				= BITS(11:0),
		GAIN_FORCE_EN				= BITS(12:12),	// 0=disable, 1==enable force chroma gain

	VIP_CP_VERT_LOCKOUT				= 0x05bc,
		CP_LOCKOUT_START			= BITS(9:0),
		CP_LOCKOUT_END				= BITS(25:16),

	// Video Decoder Clip Engine and VBI Control
	VIP_H_ACTIVE_WINDOW				= 0x05c0,
		H_ACTIVE_START				= BITS(10:0),	// Horizotal active window
		H_ACTIVE_END				= BITS(26:16),

	VIP_V_ACTIVE_WINDOW				= 0x05c4,
		V_ACTIVE_START				= BITS(9:0),	// Vertical active window
		V_ACTIVE_END				= BITS(25:16),

	VIP_H_VBI_WINDOW				= 0x05c8,
		H_VBI_WIND_START			= BITS(10:0),	// Horizontal VBI window
		H_VBI_WIND_END				= BITS(26:16),

	VIP_V_VBI_WINDOW				= 0x05cc,
		V_VBI_WIND_START			= BITS(9:0),	// Vertical VBI window
		V_VBI_WIND_END				= BITS(25:16),

	VIP_VBI_CONTROL					= 0x05d0,
		VBI_CAPTURE_ENABLE			= BITS(1:0),	// Select VBI capture
			VBI_CAPTURE_DIS			= 0 << 0,		//   Disable VBI capture
			VBI_CAPTURE_EN			= 1 << 0,		//   Enable VBI capture
			VBI_CAPTURE_RAW			= 2 << 0,		//   Enable Raw Video capture


	// Video Decoder Standard
	VIP_STANDARD_SELECT				= 0x0408,
		STANDARD_SEL				= BITS(1:0),	// Select video standard
			STANDARD_NTSC			= 0 << 0,		//   NTSC
			STANDARD_PAL			= 1 << 0,		//   PAL
			STANDARD_SECAM			= 2 << 0,		//   SECAM
		YC_MODE						= BITS(2:2),	// Select YC video mode
			YC_MODE_COMPOSITE		= 0 << 2,		//   Composite
			YC_MODE_SVIDEO			= 1 << 2,		//   SVideo

	// Video In Scaler and DVS Port
	VIP_SCALER_IN_WINDOW			= 0x0618,		// Scaler In Window
		H_IN_WIND_START				= BITS(10:0),	//   Horizontal start
		V_IN_WIND_START				= BITS(25:16),	//   Vertical start

	VIP_SCALER_OUT_WINDOW			= 0x061c,		// Scaler Out Window
		H_OUT_WIND_WIDTH			= BITS(9:0),	//   Horizontal output window width
		V_OUT_WIND_HEIGHT			= BITS(24:16),	//   Vertical output window height

	VIP_H_SCALER_CONTROL			= 0x0600,		// Horizontal Scaler control
		H_SCALE_RATIO				= BITS(20:0),	//   Horizontal scale ratio (5.16 fixed point)
		H_SHARPNESS					= BITS(28:25),	//   Sharpness control (15=6dB high frequency boost)
		H_BYPASS					= BITS(30:30),	//   Horizontal bypass enable

	VIP_V_SCALER_CONTROL			= 0x0604,		// Vertical Scaler control
		V_SCALE_RATIO				= BITS(11:0),	//   Vertical scaling ratio (1.11 fixed point)
		V_DEINTERLACE_ON			= BITS(12:12),	//   Enable deinterlacing
		V_FIELD_FLIP				= BITS(13:13),	//   Invert field flag
		V_BYPASS					= BITS(14:14),	//   Enable vertical bypass
		V_DITHER_ON					= BITS(15:15),	//   Vertical path dither enable

	VIP_V_DEINTERLACE_CONTROL		= 0x0608,		// Deinterlace control
		EVENF_OFFSET				= BITS(10:0),	//   Even Field offset
		ODDF_OFFSET					= BITS(21:11),	//   Odd Field offset

	VIP_VBI_SCALER_CONTROL			= 0x060c,		// VBI Scaler control
		VBI_SCALING_RATIO			= BITS(16:0),	//   Scaling ratio for VBI data (1.16 fixed point)
		VBI_ALIGNER_ENABLE			= BITS(17:17),	//   VBI/Raw data aligner enable

	VIP_DVS_PORT_CTRL				= 0x0610,		// DVS Port control
		DVS_DIRECTION				= BITS(0:0),	//   DVS direction
			DVS_DIRECTION_INPUT		= 0 << 0,		//     Input mode
			DVS_DIRECTION_OUTPUT	= 1 << 0,		//     Output mode
		DVS_VBI_BYTE_SWAP			= BITS(1:1),	//   Output video stream type
			DVS_VBI_BYTE_SEQUENTIAL	= 0 << 1,		//     Sequential
			DVS_VBI_BYTE_SWAPPED	= 1 << 1,		//     Byte swapped
		DVS_CLK_SELECT				= BITS(2:2),	//   DVS output clock select
			DVS_CLK_SELECT_8X		= 0 << 2,		//     8x Fsc
			DVS_CLK_SELECT_27MHz	= 1 << 2,		//     27 MHz
		CONTINUOUS_STREAM			= BITS(3:3),	//   Enable continuous stream mode
		DVSOUT_CLK_DRV				= BITS(4:4),	// 0=high, 1=low DVS port output clock buffer drive strength
		DVSOUT_DATA_DRV				= BITS(5:5),	// 0=high, 1=low DVS port output data buffers driver strength

	VIP_DVS_PORT_READBACK			= 0x0614,		// DVS Port readback
		DVS_OUTPUT_READBACK			= BITS(7:0),	//   Data from DVS port fifo

	// Clock and Reset Control
	VIP_CLKOUT_GPIO_CNTL			= 0x0038,
		CLKOUT0_SEL					= BITS(2:0),	// Select output to CLKOUT0_GPIO0 pin
			CLKOUT0_SEL_REF_CLK		= 0 << 0,		//   Reference Clock
			CLKOUT0_SEL_L54_CLK		= 1 << 0,		//   Lockable 54 MHz Clock
			CLKOUT0_SEL_AUD_CLK		= 2 << 0,		//   Audio Source Clock
			CLKOUT0_SEL_DIV_AUD_CLK	= 3 << 0,		//   Divided Audio Source Clock
			CLKOUT0_SEL_BYTE_CLK	= 4 << 0,		//   Byte Clock
			CLKOUT0_SEL_PIXEL_CLK	= 5 << 0,		//   Pixel Clock
			CLKOUT0_SEL_TEST_MUX	= 6 << 0,		//   Clock Test Mux Output
			CLKOUT0_SEL_GPIO0_OUT	= 7 << 0,		//   GPIO0_OUT
		CLKOUT0_DRV					= BITS(3:3),	// Set drive strength for CLKOUT0_GPIO0 pin
			CLKOUT0_DRV_8mA			= 0 << 3,		//   8 mA
			CLKOUT0_DRV_4mA			= 1 << 3,		//   4 mA
		CLKOUT1_SEL					= BITS(6:4),	// Select output to CLKOUT1_GPIO1 pin
			CLKOUT1_SEL_REF_CLK		= 0 << 4,		//   Reference Clock
			CLKOUT1_SEL_L54_CLK		= 1 << 4,		//   Lockable 54 MHz Clock
			CLKOUT1_SEL_AUD_CLK		= 2 << 4,		//   Audio Source Clock
			CLKOUT1_SEL_DIV_AUD_CLK	= 3 << 4,		//   Divided Audio Source Clock
			CLKOUT1_SEL_PIXEL_CLK	= 4 << 4,		//   Pixel Clock
			CLKOUT1_SEL_SPDIF_CLK	= 5 << 4,		//   SPDIF Clock
			CLKOUT1_SEL_REG_CLK		= 6 << 4,		//   Register Clock
			CLKOUT1_SEL_GPIO1_OUT	= 7 << 4,		//   GPIO1_OUT
		CLKOUT1_DRV					= BITS(7:7),	// Set drive strength for CLKOUT1_GPIO1 pin
			CLKOUT1_DRV_8mA			= 0 << 7,		//   8 mA
			CLKOUT1_DRV_4mA			= 1 << 7,		//   4 mA
		CLKOUT2_SEL					= BITS(10:8),	// Select output to CLKOUT2_GPIO2 pin
			CLKOUT2_SEL_REF_CLK		= 0 << 0,		//   Reference Clock
			CLKOUT2_SEL_L54_CLK		= 1 << 0,		//   Lockable 54 MHz Clock
			CLKOUT2_SEL_AUD_CLK		= 2 << 0,		//   Audio Source Clock
			CLKOUT2_SEL_DIV_AUD_CLK	= 3 << 0,		//   Divided Audio Source Clock
			CLKOUT2_SEL_VIN_CLK		= 4 << 0,		//   Video In Clock
			CLKOUT2_SEL_VIN_SC_CLK	= 5 << 0,		//   Video In Scaler Clock
			CLKOUT2_SEL_TV_CLK		= 6 << 0,		//   TV Clock
			CLKOUT2_SEL_GPIO2_OUT	= 7 << 0,		//   GPIO2_OUT
		CLKOUT2_DRV					= BITS(11:11),	// Set drive strength for CLKOUT2_GPIO2 pin
			CLKOUT2_DRV_8mA			= 0 << 11,		//   8 mA
			CLKOUT2_DRV_4mA			= 1 << 11,		//   4 mA
		CLKOUT1_DIV					= BITS(23:16),	// Postdivider for CLKOUT1
		CLKOUT2_DIV					= BITS(31:24),	// Postdivider for CLKOUT2

	VIP_MASTER_CNTL					= 0x0040,		// Master control
		TV_ASYNC_RST				= BITS(0:0),	//   Reset several blocks that use the TV Clock
		CRT_ASYNC_RST				= BITS(1:1),	//   Reset several blocks that use the CRT Clock
		RESTART_PHASE_FIX			= BITS(3:3),	//
		TV_FIFO_ASYNC_RST			= BITS(4:4),	//
		VIN_ASYNC_RST				= BITS(5:5),	//   Reset several blocks that use the VIN, ADC or SIN Clock
		AUD_ASYNC_RST				= BITS(6:6),	//   Reset several blocks that use the SPDIF or I2S Clock
		DVS_ASYNC_RST				= BITS(7:7),	//   Reset several blocks that use the DVSOUT Clock
		CLKOUT_CLK_SEL				= BITS(8:8),	//   External BYTE clock
		CRT_FIFO_CE_EN				= BITS(9:9),	//
		TV_FIFO_CE_EN				= BITS(10:10),

	VIP_CLKOUT_CNTL					= 0x004c,
		CLKOUT_OE					= BITS(0:0),
		CLKOUT_PUB					= BITS(3:3),
		CLKOUT_PD					= BITS(4:4),
		CLKOUT_DRV					= BITS(5:5),

	VIP_TV_PLL_CNTL					= 0x00c0,
		TV_M0_LO					= BITS(7:0),
		TV_N0_LO					= BITS(16:8),
		TV_M0_HI					= BITS(20:18),
		TV_N0_HI					= BITS(22:21),
		TV_SLIP_EN					= BITS(23:23),
		TV_P						= BITS(27:24),
		TV_DTO_EN					= BITS(28:28),
		TV_DTO_TYPE					= BITS(29:29),
		TV_REF_CLK_SEL				= BITS(31:30),

	VIP_CRT_PLL_CNTL				= 0x00c4,
		CRT_M0_LO					= BITS(7:0),
		CRT_N0_LO					= BITS(16:8),
		CRT_M0_HI					= BITS(20:18),
		CRT_N0_HI					= BITS(22:21),
		CRTCLK_USE_CLKBY2			= BITS(25:25),
		CRT_MNFLIP_EN				= BITS(26:26),
		CRT_SLIP_EN					= BITS(27:27),
		CRT_DTO_EN					= BITS(28:28),
		CRT_DTO_TYPE				= BITS(29:29),
		CRT_REF_CLK_SEL				= BITS(31:30),

	VIP_PLL_CNTL0					= 0x00c8,
		TVRST						= BITS(1:1),
		CRTRST						= BITS(2:2),
		TVSLEEPB					= BITS(3:3),
		CRTSLEEPB					= BITS(4:4),
		TVPCP						= BITS(10:8),
		TVPVG						= BITS(12:11),
		TVPDC						= BITS(15:13),
		CRTPCP						= BITS(18:16),
		CRTPVG						= BITS(20:19),
		CRTPDC						= BITS(23:21),
		CKMONEN						= BITS(24:24),

	VIP_PLL_TEST_CNTL				= 0x00cc,
		PLL_TEST					= BITS(0:0),
		PLL_TST_RST					= BITS(1:1),
		PLL_TST_DIV					= BITS(2:2),
		PLL_TST_CNT_RST				= BITS(3:3),
		STOP_REF_CLK				= BITS(7:7),
		PLL_TST_SEL					= BITS(13:8),
		PLL_TEST_COUNT				= BITS(31:16),

	VIP_CLOCK_SEL_CNTL				= 0x00d0,
		TV_CLK_SEL					= BITS(0:0),
		CRT_CLK_SEL					= BITS(1:1),
		BYT_CLK_SEL					= BITS(3:2),
		PIX_CLK_SEL					= BITS(4:4),
		REG_CLK_SEL					= BITS(5:5),
		TST_CLK_SEL					= BITS(6:6),
		VIN_CLK_SEL					= BITS(7:7),	// Select VIN clock
			VIN_CLK_SEL_REF_CLK		= 0 << 7,		//   Select reference clock
			VIN_CLK_SEL_VIPLL_CLK	= 1 << 7,		//   Select VIN PLL clock
		BYT_CLK_DEL					= BITS(10:8),
		AUD_CLK_SEL					= BITS(11:11),
		L54_CLK_SEL					= BITS(12:12),
		MV_ZONE_1_PHASE				= BITS(13:13),
		MV_ZONE_2_PHASE				= BITS(14:14),
		MV_ZONE_3_PHASE				= BITS(15:15),

	VIP_VIN_PLL_CNTL				= 0x00d4,		// VIN PLL control
		VIN_M0						= BITS(10:0),	//   Reference divider
		VIN_N0						= BITS(21:11),	//   Feedback divider
		VIN_MNFLIP_EN				= BITS(22:22),	//   M/N flip enable
		VIN_P						= BITS(27:24),	//   Post divider
		VIN_REF_CLK_SEL				= BITS(31:30),	//   VIN reference source select
			VIN_REF_CLK				= 0 << 30,		//     Reference clock
			VIN_SEC_REF_CLK			= 1 << 30,		//     Secondary Reference Clock
			VIN_L54_CLK				= 2 << 30,		//     L54 PLL Clock
			VIN_SLIP_L54_CLK		= 3 << 30,		//     Slippable L54 PLL Clock

	VIP_VIN_PLL_FINE_CNTL			= 0x00d8,
		VIN_M1						= BITS(10:0),
		VIN_N1						= BITS(21:11),
		VIN_DIVIDER_SEL				= BITS(22:22),
		VIN_MNFLIP_REQ				= BITS(23:23),
		VIN_MNFLIP_DONE				= BITS(24:24),
		TV_LOCK_TO_VIN				= BITS(27:27),
		TV_P_FOR_VINCLK				= BITS(31:28),

	VIP_AUD_PLL_CNTL				= 0x00e0,
		AUD_M0						= BITS(10:0),
		AUD_N0						= BITS(21:11),
		AUD_MNFLIP_EN				= BITS(22:22),
		AUD_SLIP_EN					= BITS(23:23),
		AUD_P						= BITS(27:24),
		AUD_DTO_EN					= BITS(28:28),
		AUD_DTO_TYPE				= BITS(29:29),
		AUD_REF_CLK_SEL				= BITS(31:30),

	VIP_AUD_PLL_FINE_CNTL			= 0x00e4,
		AUD_M1						= BITS(10:0),
		AUD_N1						= BITS(21:11),
		AUD_DIVIDER_SEL				= BITS(22:22),
		AUD_MNFLIP_REQ				= BITS(23:23),
		AUD_MNFLIP_DONE				= BITS(24:24),
		AUD_SLIP_REQ				= BITS(25:25),
		AID_SLIP_DONE				= BITS(26:26),
		AUD_SLIP_COUNT				= BITS(31:28),

	VIP_AUD_CLK_DIVIDERS			= 0x00e8,
		SPDIF_P						= BITS(3:0),
		I2S_P						= BITS(7:4),
		DIV_AUD_P					= BITS(11:8),

	VIP_AUD_DTO_INCREMENTS			= 0x00ec,
		AUD_DTO_INC0				= BITS(15:0),
		AUD_DTO_INC1				= BITS(31:16),

	VIP_L54_PLL_CNTL				= 0x00f0,
		L54_M0						= BITS(7:0),
		L54_N0						= BITS(21:11),
		L54_MNFLIP_EN				= BITS(22:22),
		L54_SLIP_EN					= BITS(23:23),
		L54_P						= BITS(27:24),
		L54_DTO_EN					= BITS(28:28),
		L54_DTO_TYPE				= BITS(29:29),
		L54_REF_CLK_SEL				= BITS(30:30),

	VIP_L54_PLL_FINE_CNTL			= 0x00f4,
		L54_M1						= BITS(7:0),
		L54_N1						= BITS(21:11),
		L54_DIVIDER_SEL				= BITS(22:22),
		L54_MNFLIP_REQ				= BITS(23:23),
		L54_MNFLIP_DONE				= BITS(24:24),
		L54_SLIP_REQ				= BITS(25:25),
		L54_SLIP_DONE				= BITS(26:26),
		L54_SLIP_COUNT				= BITS(31:28),

	VIP_L54_DTO_INCREMENTS			= 0x00f8,
		L54_DTO_INC0				= BITS(15:0),
		L54_DTO_INC1				= BITS(31:16),

	VIP_PLL_CNTL1					= 0x00fc,
		VINRST						= BITS(1:1),	// 0=active, 1=reset
		AUDRST						= BITS(2:2),
		L54RST						= BITS(3:3),
		VINSLEEPB					= BITS(4:4),
		AUDSLEEPB					= BITS(5:5),
		L54SLEEPB					= BITS(6:6),
		VINPCP						= BITS(10:8),
		VINPVG						= BITS(12:11),
		VINPDC						= BITS(15:13),
		AUDPCP						= BITS(18:16),
		AUDPVG						= BITS(20:19),
		L54PCP						= BITS(26:24),
		L54PVG						= BITS(28:27),
		L54PDC						= BITS(31:29),

	// Audio Interfaces
	VIP_FIFOA_CONFIG				= 0x0800,
		ENT_FIFOA					= BITS(8:0),
		START_FIFOA					= BITS(17:16),
			START_FIFOA_ADDR_0		= 0 << 16,
			START_FIFOA_ADDR_64		= 1 << 16,
			START_FIFOA_ADDR_128	= 2 << 16,
			START_FIFOA_ADDR_192	= 3 << 16,
		END_FIFOA					= BITS(19:18),
			END_FIFOA_ADDR_63		= 0 << 18,
			END_FIFOA_ADDR_127		= 1 << 18,
			END_FIFOA_ADDR_191		= 2 << 18,
			END_FIFOA_ADDR_255		= 3 << 18,
		TEST_EN_FIFOA				= BITS(20:20),
		RST_FIFOA					= BITS(21:21),
		WT_FIFOA_FULL				= BITS(22:22),
		EMPTY_FIFOA					= BITS(23:23),

	VIP_FIFOB_CONFIG				= 0x0804,
		ENT_FIFOB					= BITS(8:0),
		START_FIFOB					= BITS(17:16),
		END_FIFOB					= BITS(19:18),
		TEST_EN_FIFOB				= BITS(20:20),
		RST_FIFOB					= BITS(21:21),
		WT_FIFOB_FULL				= BITS(22:22),
		EMPTY_FIFOB					= BITS(23:23),

	VIP_FIFOC_CONFIG				= 0x0808,
		ENT_FIFOC					= BITS(8:0),
		START_FIFOC					= BITS(17:16),
		END_FIFOC					= BITS(19:18),
		TEST_EN_FIFOC				= BITS(20:20),
		RST_FIFOC					= BITS(21:21),
		WT_FIFOC_FULL				= BITS(22:22),
		EMPTY_FIFOC					= BITS(23:23),

	VIP_SPDIF_PORT_CNTL				= 0x080c,
		SPDIF_PORT_EN				= BITS(0:0),
		AC3_BURST_TRIGGER			= BITS(1:1),
		AC3_BURST_ACTIVE			= BITS(2:2),
		AC3_STREAM_MODE				= BITS(3:3),
		SWAP_AC3_ORDER				= BITS(4:4),
		TX_ON_NOT_EMPTY				= BITS(5:5),
		SPDIF_UNDERFLOW_CLEAR		= BITS(6:6),
		SPDIF_UNDERFLOW				= BITS(7:7),
		SPDIF_UNDERFLOW_CNT			= BITS(15:8),
		SPDIF_OE					= BITS(16:16),
		SPDIF_DRV					= BITS(19:19),
		PREAMBLE_AND_IDLE_SW		= BITS(24:24),

	VIP_SPDIF_CHANNEL_STAT			= 0x0810,
		SPDIF_STATUS_BLOCK			= BITS(0:0),
		SPDIF_DATA_TYPE				= BITS(1:1),
		SPDIF_DIGITAL_COPY			= BITS(2:2),
		SPDIF_PREEMPHASIS			= BITS(5:3),
		SPDIF_MODE					= BITS(7:6),
		SPDIF_CATEGORY				= BITS(15:8),
		SPDIF_SRC_NUM				= BITS(19:16),
		SPDIF_NUM_CHANNELS			= BITS(23:20),
		SPDIF_SAMP_FREQ				= BITS(27:24),
		SPDIF_CLOCK_ACC				= BITS(29:28),

	VIP_SPDIF_AC3_PREAMBLE			= 0x0814,
		AC3_DATA_TYPE				= BITS(4:0),
		AC3_ERR_FLAG				= BITS(7:7),
		AC3_DATA_DEPEN				= BITS(12:8),
		AC3_STREAM_NUM				= BITS(15:13),
		AC3_LENGTH_CODE				= BITS(31:16),

	VIP_I2S_TRANSMIT_CNTL			= 0x0818,
		IISTX_PORT_EN				= BITS(0:0),
		IISTX_UNDERFLOW_CLEAR		= BITS(6:6),
		IISTX_UNDERFLOW				= BITS(7:7),
		IISTX_UNDERFLOW_FRAMES		= BITS(15:8),
		IIS_BITS_PER_CHAN			= BITS(21:16),
		IIS_SLAVE_EN				= BITS(24:24),
		IIS_LOOPBACK_EN				= BITS(25:25),
		ADO_OE						= BITS(26:26),
		ADIO_OE						= BITS(29:29),
		WS_OE						= BITS(30:30),
		BITCLK_OE					= BITS(31:31),

	VIP_I2S_RECEIVE_CNTL			= 0x081c,
		IISRX_PORT_EN				= BITS(0:0),
		LOOPBACK_NO_UNDERFLOW		= BITS(5:5),
		IISRX_OVERFLOW_CLEAR		= BITS(6:6),
		IISRX_OVERFLOW				= BITS(7:7),
		IISRX_OVERFLOW_FRAMES		= BITS(15:8),

	VIP_SPDIF_TX_CNT_REG			= 0x0820,
		SPDIF_TX_CNT				= BITS(23:0),
		SPDIF_TX_CNT_CLR			= BITS(24:24),

	VIP_IIS_TX_CNT_REG				= 0x0824,
		IIS_TX_CNT					= BITS(23:0),
		IIS_TX_CNT_CLR				= BITS(24:24),

	// Miscellaneous Registers
	VIP_HW_DEBUG					= 0x0010,
		HW_DEBUG_TBD				= BITS(15:0),

	VIP_SW_SCRATCH					= 0x0014,
		SW_SCRATCH_TBD				= BITS(15:0),

	VIP_I2C_CNTL_0					= 0x0020,
		I2C_DONE					= BITS(0:0),
		I2C_NACK					= BITS(1:1),
		I2C_HALT					= BITS(2:2),
		I2C_SOFT_RST				= BITS(5:5),
		SDA_DRIVE_EN				= BITS(6:6),
		I2C_DRIVE_SEL				= BITS(7:7),
		I2C_START					= BITS(8:8),
		I2C_STOP					= BITS(9:9),
		I2C_RECEIVE					= BITS(10:10),
		I2C_ABORT					= BITS(11:11),
		I2C_GO						= BITS(12:12),
		I2C_PRESCALE				= BITS(31:16),

	VIP_I2C_CNTL_1					= 0x0024,
		I2C_DATA_COUNT				= BITS(3:0),
		I2C_ADDR_COUNT				= BITS(10:8),
		SCL_DRIVE_EN				= BITS(16:16),
		I2C_SEL						= BITS(17:17),
		I2C_TIME_LIMIT				= BITS(31:24),

	VIP_I2C_DATA					= 0x0028,
		I2C_DATA					= BITS(7:0),

	VIP_INT_CNTL					= 0x002c,
		I2C_INT_EN					= BITS(0:0),
		SPDIF_UF_INT_EN				= BITS(1:1),
		IISTX_UF_INT_EN				= BITS(2:2),
		IISTX_OF_INT_EN				= BITS(3:3),
		VIN_VSYNC_INT_EN			= BITS(4:4),
		VIN_VACTIVE_END_INT_EN		= BITS(5:5),
		VSYNC_DIFF_OVER_LIMIT_INT_EN= BITS(6:6),
		I2C_INT_AK					= BITS(16:16),
		I2C_INT						= BITS(16:16),
		SPDIF_UF_INT_AK				= BITS(17:17),
		SPDIF_UF_INT				= BITS(17:17),
		IISTX_UF_INT_AK				= BITS(18:18),
		IISTX_UF_INT				= BITS(18:18),
		IISRX_OF_INT_AK				= BITS(19:19),
		IISRX_OF_INT				= BITS(19:19),
		VIN_VSYNC_INT_AK			= BITS(20:20),
		VIN_VSYNC_INT				= BITS(20:20),
		VIN_VACTIVE_END_INT_AK		= BITS(21:21),
		VIN_VACTIVE_END_INT			= BITS(21:21),
		VSYNC_DIFF_OVER_LIMIT_INT_AK= BITS(22:22),
		VSYNC_DIFF_OVER_LIMIT_INT	= BITS(22:22),

	VIP_GPIO_INOUT					= 0x0030,
		CLKOUT0_GPIO0_OUT			= BITS(0:0),
		CLKOUT0_GPIO1_OUT			= BITS(1:1),
		CLKOUT0_GPIO2_OUT			= BITS(2:2),
		GPIO_6TO3_OUT				= BITS(6:3),
		SPDIF_GPIO_OUT				= BITS(7:7),
		ADO_GPIO_OUT				= BITS(8:8),
		ADIO_GPIO_OUT				= BITS(9:9),
		WS_GPIO_OUT					= BITS(10:10),
		BITCLK_GPIO_OUT				= BITS(11:11),
		HAD_GPIO_OUT				= BITS(13:12),
		CLKOUT0_GPIO0_IN			= BITS(16:16),
		CLKOUT0_GPIO1_IN			= BITS(17:17),
		CLKOUT0_GPIO2_IN			= BITS(18:18),
		GPIO_6TO3_IN				= BITS(22:19),
		SPDIF_GPIO_IN				= BITS(23:23),
		ADO_GPIO_IN					= BITS(24:24),
		ADIO_GPIO_IN				= BITS(25:25),
		WS_GPIO_IN					= BITS(26:26),
		BITCLK_GPIO_IN				= BITS(27:27),
		HAD_GPIO_IN					= BITS(29:28),

	VIP_GPIO_CNTL					= 0x0034,
		CLKOUT0_GPIO0_OE			= BITS(0:0),
		CLKOUT1_GPIO1_OE			= BITS(1:1),
		CLKOUT2_GPIO2_OE			= BITS(2:2),
		GPIO_6TO3_OE				= BITS(6:3),
		SPDIF_GPIO_OE				= BITS(7:7),
		ADO_GPIO_OE					= BITS(8:8),
		ADIO_GPIO_OE				= BITS(9:9),
		WS_GPIO_OE					= BITS(10:10),
		BITCLK_GPIO_OE				= BITS(11:11),
		HAD_GPIO_OE					= BITS(13:12),
		GPIO_6TO1_STRAPS			= BITS(22:17),

	VIP_RIPINTF_PORT_CNTL			= 0x003c,
		MPP_DATA_DRV				= BITS(2:2),
		HAD_DRV						= BITS(3:3),
		HCTL_DRV					= BITS(4:4),
		SRDY_IRQb_DRV				= BITS(5:5),
		SUB_SYS_ID_EN				= BITS(16:16),

	VIP_DECODER_DEBUG_CNTL			= 0x05d4,
		CHIP_DEBUG_SEL				= BITS(7:0),
		CHIP_DEBUG_EN				= BITS(8:8),
		DECODER_DEBUG_SEL			= BITS(15:12),

	VIP_SINGLE_STEP_DATA			= 0x05d8,
		SS_C						= BITS(7:0),
		SS_Y						= BITS(15:8),

	VIP_I2C_CNTL					= 0x0054,
		I2C_CLK_OE					= BITS(0:0),
		I2C_CLK_OUT					= BITS(1:1),
		I2C_CLK_IN					= BITS(2:2),
		I2C_DAT_OE					= BITS(4:4),
		I2C_DAT_OUT					= BITS(5:5),
		I2C_DAT_IN					= BITS(6:6),
		I2C_CLK_PUB					= BITS(8:8),
		I2C_CLK_PD					= BITS(9:9),
		I2C_CLK_DRV					= BITS(10:10),
		I2C_DAT_PUB					= BITS(12:12),
		I2C_DAT_PD					= BITS(13:13),
		I2C_DAT_DRV					= BITS(14:14),
		I2C_CLK_MX					= BITS(19:16),
		I2C_DAT_MX					= BITS(23:20),
		DELAY_TEST_MODE				= BITS(25:24),

	// Undocumented Registers
	VIP_TV_PLL_FINE_CNTL			= 0x00b8,
	VIP_CRT_PLL_FINE_CNTL			= 0x00bc,
	VIP_MV_MODE_CNTL				= 0x0208,
	VIP_MV_STRIPE_CNTL				= 0x020c,
	VIP_MV_LEVEL_CNTL1				= 0x0210,
	VIP_MV_LEVEL_CNTL2				= 0x0214,
	VIP_MV_STATUS					= 0x0330,
	VIP_TV_DTO_INCREMENTS			= 0x0390,
	VIP_CRT_DTO_INCREMENTS			= 0x0394,
	VIP_VSYNC_DIFF_CNTL				= 0x03a0,
	VIP_VSYNC_DIFF_LIMITS			= 0x03a4,
	VIP_VSYNC_DIFF_RD_DATA			= 0x03a8,

	DSP_OK							= 0x21,
	DSP_INVALID_PARAMETER			= 0x22,
	DSP_MISSING_PARAMETER			= 0x23,
	DSP_UNKNOWN_COMMAND				= 0x24,
	DSP_UNSUCCESS					= 0x25,
	DSP_BUSY						= 0x26,
	DSP_RESET_REQUIRED				= 0x27,
	DSP_UNKNOWN_RESULT				= 0x28,
	DSP_CRC_ERROR					= 0x29,
	DSP_AUDIO_GAIN_ADJ_FAIL			= 0x2a,
	DSP_AUDIO_GAIN_CHK_ERROR		= 0x2b,
	DSP_WARNING						= 0x2c,
	DSP_POWERDOWN_MODE				= 0x2d,

	RT200_NTSC_M					= 0x01,
	RT200_NTSC_433					= 0x03,
	RT200_NTSC_J					= 0x04,
	RT200_PAL_B						= 0x05,
	RT200_PAL_D						= 0x06,
	RT200_PAL_G						= 0x07,
	RT200_PAL_H						= 0x08,
	RT200_PAL_I						= 0x09,
	RT200_PAL_N						= 0x0a,
	RT200_PAL_Ncomb					= 0x0b,
	RT200_PAL_M						= 0x0c,
	RT200_PAL_60					= 0x0d,
	RT200_SECAM						= 0x0e,
	RT200_SECAM_B					= 0x0f,
	RT200_SECAM_D					= 0x10,
	RT200_SECAM_G					= 0x11,
	RT200_SECAM_H					= 0x12,
	RT200_SECAM_K					= 0x13,
	RT200_SECAM_K1					= 0x14,
	RT200_SECAM_L					= 0x15,
	RT200_SECAM_L1					= 0x16,
	RT200_480i						= 0x17,
	RT200_480p						= 0x18,
	RT200_576i						= 0x19,
	RT200_720p						= 0x1a,
	RT200_1080i						= 0x1b

};


/* RT200 stuff there's no way I'm converting these to enums...*/
/* RT200 */
#define VIP_INT_CNTL__FB_INT0                      0x02000000
#define VIP_INT_CNTL__FB_INT0_CLR                  0x02000000
#define VIP_GPIO_INOUT                             0x0030
#define VIP_GPIO_CNTL                              0x0034
#define VIP_CLKOUT_GPIO_CNTL                       0x0038
#define VIP_RIPINTF_PORT_CNTL                      0x003c

/* RT200 */
#define VIP_GPIO_INOUT                             0x0030
#define VIP_GPIO_CNTL                              0x0034
#define VIP_HOSTINTF_PORT_CNTL                     0x003c
#define VIP_HOSTINTF_PORT_CNTL__HAD_HCTL_SDA_SN    0x00000008
#define VIP_HOSTINTF_PORT_CNTL__HAD_HCTL_SDA_SP    0x00000080
#define VIP_HOSTINTF_PORT_CNTL__HAD_HCTL_SDA_SR    0x00000100
#define VIP_HOSTINTF_PORT_CNTL__SUB_SYS_ID_EN      0x00010000
#define VIP_HOSTINTF_PORT_CNTL__FIFO_RW_MODE       0x00300000
#define VIP_HOSTINTF_PORT_CNTL__FIFOA_ENDIAN_SWAP  0x00c00000
#define VIP_HOSTINTF_PORT_CNTL__FIFOB_ENDIAN_SWAP  0x03000000
#define VIP_HOSTINTF_PORT_CNTL__FIFOC_ENDIAN_SWAP  0x0c000000
#define VIP_HOSTINTF_PORT_CNTL__FIFOD_ENDIAN_SWAP  0x30000000
#define VIP_HOSTINTF_PORT_CNTL__FIFOE_ENDIAN_SWAP  0xc0000000

/* RT200 */
#define VIP_DSP_PLL_CNTL                           0x0bc

/* RT200 */
#define VIP_TC_SOURCE                              0x300
#define VIP_TC_DESTINATION                         0x304
#define VIP_TC_COMMAND                             0x308

/* RT200 */
#define VIP_TC_STATUS                              0x030c
#define VIP_TC_STATUS__TC_CHAN_BUSY                0x00007fff
#define VIP_TC_STATUS__TC_WRITE_PENDING            0x00008000
#define VIP_TC_STATUS__TC_FIFO_4_EMPTY             0x00040000
#define VIP_TC_STATUS__TC_FIFO_6_EMPTY             0x00080000
#define VIP_TC_STATUS__TC_FIFO_8_EMPTY             0x00100000
#define VIP_TC_STATUS__TC_FIFO_10_EMPTY            0x00200000
#define VIP_TC_STATUS__TC_FIFO_4_FULL              0x04000000
#define VIP_TC_STATUS__TC_FIFO_6_FULL              0x08080000
#define VIP_TC_STATUS__TC_FIFO_8_FULL              0x10080000
#define VIP_TC_STATUS__TC_FIFO_10_FULL             0x20080000
#define VIP_TC_STATUS__DSP_ILLEGAL_OP              0x80080000

/* RT200 */
#define VIP_TC_DOWNLOAD                            0x0310
#define VIP_TC_DOWNLOAD__TC_DONE_MASK              0x00003fff
#define VIP_TC_DOWNLOAD__TC_RESET_MODE             0x00060000

/* RT200 */
#define VIP_FB_INT                                 0x0314
#define VIP_FB_INT__INT_7                          0x00000080
#define VIP_FB_SCRATCH0                            0x0318 
#define VIP_FB_SCRATCH1                            0x031c 

struct rt200_microc_head
{
	unsigned int device_id;
	unsigned int vendor_id;
	unsigned int revision_id;
	unsigned int num_seg;
};

struct rt200_microc_seg
{
	unsigned int num_bytes;
	unsigned int download_dst;
	unsigned int crc_val;

	unsigned char* data;
	struct rt200_microc_seg* next;
};


struct rt200_microc_data
{
	struct rt200_microc_head		microc_head;
	struct rt200_microc_seg*		microc_seg_list;
};

#endif

/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Overlay unit and Subpicture registers
*/

#ifndef _OVERLAY_REGS_H
#define _OVERLAY_REGS_H


#define RADEON_OV0_Y_X_START                0x0400
#define RADEON_OV0_Y_X_END                  0x0404
#define RADEON_OV0_PIPELINE_CNTL            0x0408 
#define RADEON_OV0_REG_LOAD_CNTL            0x0410
#       define  RADEON_REG_LD_CTL_LOCK                 0x00000001L
#       define  RADEON_REG_LD_CTL_VBLANK_DURING_LOCK   0x00000002L
#       define  RADEON_REG_LD_CTL_STALL_GUI_UNTIL_FLIP 0x00000004L
#       define  RADEON_REG_LD_CTL_LOCK_READBACK        0x00000008L
#define RADEON_OV0_SCALE_CNTL               0x0420
#       define  RADEON_SCALER_HORZ_PICK_NEAREST    0x00000004L
#       define  RADEON_SCALER_VERT_PICK_NEAREST    0x00000008L
#       define  RADEON_SCALER_SIGNED_UV            0x00000010L
#       define  RADEON_SCALER_GAMMA_SEL_MASK       0x00000060L
#       define  RADEON_SCALER_GAMMA_SEL_BRIGHT     0x00000000L
#       define  RADEON_SCALER_GAMMA_SEL_G22        0x00000020L
#       define  RADEON_SCALER_GAMMA_SEL_G18        0x00000040L
#       define  RADEON_SCALER_GAMMA_SEL_G14        0x00000060L
#       define  RADEON_SCALER_COMCORE_SHIFT_UP_ONE 0x00000080L
#       define  RADEON_SCALER_SURFAC_FORMAT        0x00000f00L
#       define  RADEON_SCALER_SOURCE_15BPP         0x00000300L
#       define  RADEON_SCALER_SOURCE_16BPP         0x00000400L
#       define  RADEON_SCALER_SOURCE_32BPP         0x00000600L
#       define  RADEON_SCALER_SOURCE_YUV9          0x00000900L
#       define  RADEON_SCALER_SOURCE_YUV12         0x00000A00L
#       define  RADEON_SCALER_SOURCE_VYUY422       0x00000B00L
#       define  RADEON_SCALER_SOURCE_YVYU422       0x00000C00L
#       define  RADEON_SCALER_ADAPTIVE_DEINT       0x00001000L
#       define  R200_SCALER_TEMPORAL_DEINT         0x00002000L
#       define  RADEON_SCALER_CRTC_SEL             0x00004000L
#       define  RADEON_SCALER_SMART_SWITCH         0x00008000L
#       define  RADEON_SCALER_BURST_PER_PLANE      0x007F0000L
#       define  RADEON_SCALER_DOUBLE_BUFFER        0x01000000L
#       define  RADEON_SCALER_DIS_LIMIT            0x08000000L
#		define  RADEON_SCALER_LIN_TRANS_BYPASS	   0x10000000L
#       define  RADEON_SCALER_INT_EMU              0x20000000L
#       define  RADEON_SCALER_ENABLE               0x40000000L
#       define  RADEON_SCALER_SOFT_RESET           0x80000000L
#define RADEON_OV0_V_INC                    0x0424
#define RADEON_OV0_P1_V_ACCUM_INIT          0x0428
#       define  RADEON_OV0_P1_MAX_LN_IN_PER_LN_OUT 0x00000003L
#       define  RADEON_OV0_P1_V_ACCUM_INIT_MASK    0x01ff8000L
#define RADEON_OV0_P23_V_ACCUM_INIT         0x042C
#       define  RADEON_OV0_P23_MAX_LN_IN_PER_LN_OUT 0x00000003L
#       define  RADEON_OV0_P23_V_ACCUM_INIT_MASK    0x00ff8000L
#define RADEON_OV0_P1_BLANK_LINES_AT_TOP    0x0430
#       define  RADEON_P1_BLNK_LN_AT_TOP_M1_MASK   0x00000fffL
#       define  RADEON_P1_ACTIVE_LINES_M1          0x0fff0000L
#define RADEON_OV0_P23_BLANK_LINES_AT_TOP   0x0434
#       define  RADEON_P23_BLNK_LN_AT_TOP_M1_MASK  0x000007ffL
#       define  RADEON_P23_ACTIVE_LINES_M1         0x07ff0000L
#define RADEON_OV0_VID_BUF0_BASE_ADRS       0x0440
#       define  RADEON_VIF_BUF0_PITCH_SEL          0x00000001L
#       define  RADEON_VIF_BUF0_TILE_ADRS          0x00000002L
#       define  RADEON_VIF_BUF0_BASE_ADRS_MASK     0x03fffff0L
#       define  RADEON_VIF_BUF0_1ST_LINE_LSBS_MASK 0x48000000L
#define RADEON_OV0_VID_BUF1_BASE_ADRS       0x0444
#       define  RADEON_VIF_BUF1_PITCH_SEL          0x00000001L
#       define  RADEON_VIF_BUF1_TILE_ADRS          0x00000002L
#       define  RADEON_VIF_BUF1_BASE_ADRS_MASK     0x03fffff0L
#       define  RADEON_VIF_BUF1_1ST_LINE_LSBS_MASK 0x48000000L
#define RADEON_OV0_VID_BUF2_BASE_ADRS       0x0448
#       define  RADEON_VIF_BUF2_PITCH_SEL          0x00000001L
#       define  RADEON_VIF_BUF2_TILE_ADRS          0x00000002L
#       define  RADEON_VIF_BUF2_BASE_ADRS_MASK     0x03fffff0L
#       define  RADEON_VIF_BUF2_1ST_LINE_LSBS_MASK 0x48000000L
#define RADEON_OV0_VID_BUF3_BASE_ADRS       0x044C
#define RADEON_OV0_VID_BUF4_BASE_ADRS       0x0450
#define RADEON_OV0_VID_BUF5_BASE_ADRS       0x0454
#define RADEON_OV0_VID_BUF_PITCH0_VALUE     0x0460
#define RADEON_OV0_VID_BUF_PITCH1_VALUE     0x0464
#define RADEON_OV0_AUTO_FLIP_CNTRL          0x0470
#       define RADEON_OV0_SOFT_BUF_NUM_MASK         0x00000007
#       define RADEON_OV0_SOFT_REPEAT_FIELD_MASK    0x00000008
#       define RADEON_OV0_SOFT_REPEAT_FIELD         0x00000008
#       define RADEON_OV0_SOFT_BUF_ODD_MASK         0x00000010
#       define RADEON_OV0_SOFT_BUF_ODD              0x00000010
#       define RADEON_OV0_IGNORE_REPEAT_FIELD_MASK  0x00000020
#       define RADEON_OV0_IGNORE_REPEAT_FIELD       0x00000020
#       define RADEON_OV0_SOFT_EOF_TOGGLE_MASK      (1 << 6)
#       define RADEON_OV0_SOFT_EOF_TOGGLE           (1 << 6)
#       define RADEON_OV0_VID_PORT_SELECT_MASK      (3 << 8)
#       define RADEON_OV0_VID_PORT_SELECT_PORT0     (0 << 8)
#       define RADEON_OV0_VID_PORT_SELECT_SOFTWARE  (2 << 8)
#       define RADEON_OV0_P1_FIRST_LINE_EVEN_MASK   0x00010000
#       define RADEON_OV0_P1_FIRST_LINE_EVEN        0x00010000
#       define RADEON_OV0_SHIFT_EVEN_DOWN_MASK      0x00040000
#       define RADEON_OV0_SHIFT_EVEN_DOWN           0x00040000
#       define RADEON_OV0_SHIFT_ODD_DOWN_MASK       0x00080000
#       define RADEON_OV0_SHIFT_ODD_DOWN            0x00080000
#       define RADEON_OV0_FIELD_POL_SOURCE_MASK     0x00800000
#       define RADEON_OV0_FIELD_POL_SOURCE          0x00800000

#define RADEON_OV0_DEINTERLACE_PATTERN      0x0474
#       define RADEON_OV0_DEINT_PAT_SHIFT        0
#       define RADEON_OV0_DEINT_PAT_MASK         (0xfffff << 0)
#       define RADEON_OV0_DEINT_PAT_PNTR_SHIFT   24
#       define RADEON_OV0_DEINT_PAT_PNTR_MASK    (0xf << 24)
#       define RADEON_OV0_DEINT_PAT_LEN_M1_SHIFT 28
#       define RADEON_OV0_DEINT_PAT_LEN_M1_MASK  (0xf << 28)
#define RADEON_OV0_H_INC                    0x0480
#define RADEON_OV0_STEP_BY                  0x0484
#define RADEON_OV0_P1_H_ACCUM_INIT          0x0488
#       define  RADEON_OV0_P1_H_ACCUM_INIT_MASK    0x000f8000L
#       define  RADEON_OV0_P1_PRESHIFT_MASK        0xf0000000L
#define RADEON_OV0_P23_H_ACCUM_INIT         0x048C
#       define  RADEON_OV0_P23_H_ACCUM_INIT_MASK    0x000f8000L
#       define  RADEON_OV0_P23_PRESHIFT_MASK        0x70000000L
#define RADEON_OV0_P1_X_START_END           0x0494
#define RADEON_OV0_P2_X_START_END           0x0498
#define RADEON_OV0_P3_X_START_END           0x049C
#define RADEON_OV0_FILTER_CNTL              0x04A0
#		define RADEON_OV0_HC_COEF_ON_HORZ_Y		0x0001
#		define RADEON_OV0_HC_COEF_ON_HORZ_UV	0x0002
#		define RADEON_OV0_HC_COEF_ON_VERT_Y		0x0004
#		define RADEON_OV0_HC_COEF_ON_VERT_UV	0x0008
#define RADEON_OV0_FOUR_TAP_COEF_0          0x04B0
#define RADEON_OV0_FOUR_TAP_COEF_1          0x04B4
#define RADEON_OV0_FOUR_TAP_COEF_2          0x04B8
#define RADEON_OV0_FOUR_TAP_COEF_3          0x04BC
#define RADEON_OV0_FOUR_TAP_COEF_4          0x04C0
#define RADEON_OV0_COLOUR_CNTL              0x04E0

#define RADEON_OV0_VIDEO_KEY_CLR            0x04E4
#define RADEON_OV0_VIDEO_KEY_MSK            0x04E8
#define RADEON_OV0_GRAPHICS_KEY_CLR         0x04EC
#define RADEON_OV0_GRAPHICS_KEY_MSK         0x04F0

#define RADEON_OV0_VIDEO_KEY_CLR_LOW        0x04E4
#define RADEON_OV0_VIDEO_KEY_CLR_HIGH       0x04E8
#define RADEON_OV0_GRAPHICS_KEY_CLR_LOW     0x04EC
#define RADEON_OV0_GRAPHICS_KEY_CLR_HIGH    0x04F0

#define RADEON_OV0_KEY_CNTL                 0x04F4
#       define  RADEON_VIDEO_KEY_FN_MASK    0x00000003L
#       define  RADEON_VIDEO_KEY_FN_FALSE   0x00000000L
#       define  RADEON_VIDEO_KEY_FN_TRUE    0x00000001L
#       define  RADEON_VIDEO_KEY_FN_EQ      0x00000002L
#       define  RADEON_VIDEO_KEY_FN_NE      0x00000003L
#       define  RADEON_GRAPHIC_KEY_FN_MASK  0x00000030L
#       define  RADEON_GRAPHIC_KEY_FN_FALSE 0x00000000L
#       define  RADEON_GRAPHIC_KEY_FN_TRUE  0x00000010L
#       define  RADEON_GRAPHIC_KEY_FN_EQ    0x00000020L
#       define  RADEON_GRAPHIC_KEY_FN_NE    0x00000030L
#       define  RADEON_CMP_MIX_MASK         0x00000100L
#       define  RADEON_CMP_MIX_OR           0x00000000L
#       define  RADEON_CMP_MIX_AND          0x00000100L
#define RADEON_OV0_TEST                     0x04F8

#define RADEON_OV1_Y_X_START                0x0600
#define RADEON_OV1_Y_X_END                  0x0604
#define RADEON_OV1_PIPELINE_CNTL            0x0608 

#define RADEON_OV0_GAMMA_0_F                0x0d40
#define RADEON_OV0_GAMMA_10_1F              0x0d44
#define RADEON_OV0_GAMMA_20_3F              0x0d48
#define RADEON_OV0_GAMMA_40_7F              0x0d4c
/* the registers that control gamma in the 80-37f range do not
   exist on pre-R200 radeons */
#define RADEON_OV0_GAMMA_80_BF              0x0e00
#define RADEON_OV0_GAMMA_C0_FF              0x0e04
#define RADEON_OV0_GAMMA_100_13F            0x0e08
#define RADEON_OV0_GAMMA_140_17F            0x0e0c
#define RADEON_OV0_GAMMA_180_1BF            0x0e10
#define RADEON_OV0_GAMMA_1C0_1FF            0x0e14
#define RADEON_OV0_GAMMA_200_23F            0x0e18
#define RADEON_OV0_GAMMA_240_27F            0x0e1c
#define RADEON_OV0_GAMMA_280_2BF            0x0e20
#define RADEON_OV0_GAMMA_2C0_2FF            0x0e24
#define RADEON_OV0_GAMMA_300_33F            0x0e28
#define RADEON_OV0_GAMMA_340_37F            0x0e2c
#define RADEON_OV0_GAMMA_380_3BF            0x0d50
#define RADEON_OV0_GAMMA_3C0_3FF            0x0d54
#define RADEON_OV0_LIN_TRANS_A              0x0d20
#define RADEON_OV0_LIN_TRANS_B              0x0d24
#define RADEON_OV0_LIN_TRANS_C              0x0d28
#define RADEON_OV0_LIN_TRANS_D              0x0d2c
#define RADEON_OV0_LIN_TRANS_E              0x0d30
#define RADEON_OV0_LIN_TRANS_F              0x0d34

#define RADEON_SUBPIC_CNTL                  0x0540

#endif

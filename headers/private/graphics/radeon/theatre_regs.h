/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Rage Theatre registers (to be accessed via VIP)
*/

#ifndef _THEATRE_REGS_H
#define _THEATRE_REGS_H

#define THEATRE_ID									0x4D541002

#define THEATRE_VIP_MASTER_CNTL                      0x0040
#define THEATRE_VIP_RGB_CNTL                         0x0048
#define THEATRE_VIP_CLKOUT_CNTL                      0x004c
#define THEATRE_VIP_SYNC_CNTL                        0x0050
#define THEATRE_VIP_I2C_CNTL                         0x0054
#define THEATRE_VIP_HTOTAL                           0x0080
#define THEATRE_VIP_HDISP                            0x0084
#define THEATRE_VIP_HSIZE                            0x0088
#define THEATRE_VIP_HSTART                           0x008c
#define THEATRE_VIP_HCOUNT                           0x0090
#define THEATRE_VIP_VTOTAL                           0x0094
#define THEATRE_VIP_VDISP                            0x0098
#define THEATRE_VIP_VCOUNT                           0x009c
#define THEATRE_VIP_FTOTAL                           0x00a0
#define THEATRE_VIP_FCOUNT                           0x00a4
#define THEATRE_VIP_FRESTART                         0x00a8
#define THEATRE_VIP_HRESTART                         0x00ac
#define THEATRE_VIP_VRESTART	                     0x00b0
#define THEATRE_VIP_SYNC_SIZE                        0x00b4
#define THEATRE_VIP_TV_PLL_CNTL                      0x00c0
#define THEATRE_VIP_CRT_PLL_FINE_CNTL                0x00bc
#define THEATRE_VIP_TV_PLL_FINE_CNTL                 0x00b8
#define THEATRE_VIP_CRT_PLL_CNTL                     0x00c4
#define THEATRE_VIP_PLL_CNTL0                        0x00c8
#define THEATRE_VIP_PLL_TEST_CNTL                    0x00cc
#define THEATRE_VIP_CLOCK_SEL_CNTL                   0x00d0
#define THEATRE_VIP_FRAME_LOCK_CNTL                  0x0100
#define THEATRE_VIP_SYNC_LOCK_CNTL                   0x0104
#define THEATRE_VIP_TVO_SYNC_PAT_ACCUM               0x0108
#define THEATRE_VIP_TVO_SYNC_THRESHOLD               0x010c
#define THEATRE_VIP_TVO_SYNC_PAT_EXPECT              0x0110
#define THEATRE_VIP_DELAY_ONE_MAP_A	                 0x0114
#define THEATRE_VIP_DELAY_ONE_MAP_B                  0x0118
#define THEATRE_VIP_DELAY_ZERO_MAP_A                 0x011c
#define THEATRE_VIP_DELAY_ZERO_MAP_B                 0x0120
#define THEATRE_VIP_TVO_DATA_DELAY_A                 0x0140
#define THEATRE_VIP_TVO_DATA_DELAY_B                 0x0144
#define THEATRE_VIP_HOST_READ_DATA                   0x0180
#define THEATRE_VIP_HOST_WRITE_DATA                  0x0184
#define THEATRE_VIP_HOST_RD_WT_CNTL                  0x0188
#define THEATRE_VIP_VSCALER_CNTL                     0x01c0
#define THEATRE_VIP_TIMING_CNTL                      0x01c4
#define THEATRE_VIP_VSCALER_CNTL2                    0x01c8
#define THEATRE_VIP_Y_FALL_CNTL                      0x01cc
#define THEATRE_VIP_Y_RISE_CNTL                      0x01d0
#define THEATRE_VIP_Y_SAW_TOOTH_CNTL                 0x01d4
#define THEATRE_VIP_UPSAMP_AND_GAIN_CNTL             0x01e0
#define THEATRE_VIP_GAIN_LIMIT_SETTINGS              0x01e4
#define THEATRE_VIP_LINEAR_GAIN_SETTINGS             0x01e8
#define THEATRE_VIP_MODULATOR_CNTL1                  0x0200
#define THEATRE_VIP_MODULATOR_CNTL2                  0x0204
#define THEATRE_VIP_PRE_DAC_MUX_CNTL                 0x0240
#define THEATRE_VIP_TV_DAC_CNTL                      0x0280
#define THEATRE_VIP_CRC_CNTL                         0x02c0
#define THEATRE_VIP_VIDEO_PORT_SIG                   0x02c4
#define THEATRE_VIP_VBI_20BIT_CNTL                   0x02d0
#define THEATRE_VIP_VBI_LEVEL_CNTL                   0x02d8
#define THEATRE_VIP_UV_ADR                           0x0300
#define THEATRE_VIP_UPSAMP_COEFF0_0                  0x0340
#define THEATRE_VIP_UPSAMP_COEFF0_1                  0x0344
#define THEATRE_VIP_UPSAMP_COEFF0_2                  0x0348
#define THEATRE_VIP_UPSAMP_COEFF1_0                  0x034c
#define THEATRE_VIP_UPSAMP_COEFF1_1                  0x0350
#define THEATRE_VIP_UPSAMP_COEFF1_2                  0x0354
#define THEATRE_VIP_UPSAMP_COEFF2_0                  0x0358
#define THEATRE_VIP_UPSAMP_COEFF2_1                  0x035c
#define THEATRE_VIP_UPSAMP_COEFF2_2                  0x0360
#define THEATRE_VIP_UPSAMP_COEFF3_0                  0x0364
#define THEATRE_VIP_UPSAMP_COEFF3_1                  0x0368
#define THEATRE_VIP_UPSAMP_COEFF3_2                  0x036c
#define THEATRE_VIP_UPSAMP_COEFF4_0                  0x0370
#define THEATRE_VIP_UPSAMP_COEFF4_1                  0x0374
#define THEATRE_VIP_UPSAMP_COEFF4_2                  0x0378
#define THEATRE_VIP_HSCALER_CONTROL                  0x0600
#define THEATRE_VIP_VSCALER_CONTROL                  0x0604


#endif

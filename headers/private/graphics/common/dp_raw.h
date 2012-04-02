/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _DP_RAW_H
#define _DP_RAW_H


/* ****************************************************** */
/* *** AUX Channel Communications                     *** */
// Native AUX Communications
#define AUX_NATIVE_WRITE                    0x8
#define AUX_NATIVE_READ                     0x9
#define AUX_NATIVE_REPLY_ACK                (0x0 << 4)
#define AUX_NATIVE_REPLY_NACK               (0x1 << 4)
#define AUX_NATIVE_REPLY_DEFER              (0x2 << 4)
#define AUX_NATIVE_REPLY_MASK               (0x3 << 4)
// AUX i2c Communications
#define AUX_I2C_WRITE                       0x0
#define AUX_I2C_READ                        0x1
#define AUX_I2C_STATUS                      0x2
#define AUX_I2C_MOT                         0x4
#define AUX_I2C_REPLY_ACK                   (0x0 << 6)
#define AUX_I2C_REPLY_NACK                  (0x1 << 6)
#define AUX_I2C_REPLY_DEFER                 (0x2 << 6)
#define AUX_I2C_REPLY_MASK                  (0x3 << 6)


/* ****************************************************** */
/* *** DPCD (DisplayPort Configuration Data)          *** */
/* *** Read / Written over DisplayPort AUX link       *** */

/* *** DPCD Receiver Compatibility Field (0x0000)     *** */
/* *** VESA DisplayPort Standard, rev 1.1, p112       *** */
// DPCD Version (0x0)
#define DP_DPCD_REV							0x0000 // Reg
#define DP_DPCD_REV_MINOR_MASK				0x000F // Int
#define DP_DPCD_REV_MAJOR_MASK				0x00F0 // Int
// DP Maximum Link Rate (0x1)
#define DP_MAX_LINK_RATE					0x0001 // Reg
#define DP_MAX_LINK_RATE_162				0x0006 // 1.62Ghz
#define DP_MAX_LINK_RATE_270				0x000A // 2.70Ghz
#define DP_MAX_LINK_RATE_540				0x0014 // 5.40Ghz
// DP Maximum Lane Count (0x2)
#define DP_MAX_LANE_COUNT					0x0002 // Reg
#define DP_MAX_LANE_COUNT_MASK				0x001F // Count
#define DP_ENHANCED_FRAME_CAP_MASK			0x0080 // Bool, Rev 1.1+
// DP Maximum Downspread (0x3)
#define DP_MAX_DOWNSPREAD					0x0003 // Reg
#define DP_MAX_DOWNSPREAD_EN_MASK			0x0001 // Bool
#define DP_MAX_DOWNSPREAD_EN_AUX_TRAIN_MASK	0x0040 // Bool
// DP Number of Receiver Ports (0x4)
#define DP_NORP								0x0004 // Reg
#define DP_NORP_MASK						0x0001 // Count
// DP Downstream Port Present (0x5)
#define DP_DOWNSTREAMPORT					0x0005 // Reg
#define DP_DOWNSTREAMPORT_EN_MASK			0x0001 // Bool
#define DP_DOWNSTREAMPORT_TYPE_MASK			0x0006 // Type
#define DP_DOWNSTREAMPORT_EN_FORMAT_MASK	0x0008 // Bool
// DP Main Link Channel Coding (0x6)
#define DP_CURR_MAIN_CHAN_CODE				0x0006 // Reg
#define DP_CURR_MAIN_CHAN_CODE_EN_ANSI_MASK	0x0001 // Bool
// DP Downstream Port Count (0x7) (Only 1.1+)
#define DP_DOWNSTREAMPORT_COUNT				0x0007 // Reg
#define DP_DOWNSTREAMPORT_COUNT_MASK		0x000F // Count
#define DP_DOWNSTREAMPORT_COUNT_EN_OUI_MASK	0x0080 // Bool
// DP Port Capability 0
#define DP_PORT0_CAPABILITY0				0x0008 // Reg
#define DP_PORT1_CAPABILITY0				0x000A // Reg
#define DP_PORTX_CAPABILITY0_EN_EDID_MASK	0x0002 // Bool
#define DP_PORTX_CAPABILITY0_EN_SECOND_MASK	0x0004 // Bool
// DP Port Capability 1
#define DP_PORT0_CAPABILITY1				0x0009 // Reg
#define DP_PORT1_CAPABILITY1				0x000B // Reg
#define DP_PORT0_CAPABILITY1_BUF_SIZE_MASK	0x00FF // Size
	// (value + 1) * 32 bytes per lane

/* *** DPCD Link Configuration Field (0x0100)         *** */
/* *** VESA DisplayPort Standard, rev 1.1, p117       *** */

// DP Set Link Rate Per Lane (0x0100)
#define DP_LINK_RATE						0x0100 // Reg
#define DP_LINK_RATE_162					0x0006 // 1.62Ghz
#define DP_LINK_RATE_270					0x000A // 2.70Ghz
#define DP_LINK_RATE_540					0x0014 // 5.40Ghz
// DP Set Lane Count (0x0101)
#define DP_LANE_COUNT						0x0101 // Reg
#define DP_LANE_COUNT_MASK					0x001F // Count
#define DP_ENHANCED_FRAME_EN_MASK			0x0080 // Bool, Rev 1.1+
// DP Training Pattern (0x0102)
#define DP_LINK_TRAIN						0x0102 // Reg
#define DP_LINK_TRAIN_PATTERN_MASK			0x0003 // Mask
#define DP_LINK_TRAIN_PATTERN_DISABLED		0x0000 // Value
#define DP_LINK_TRAIN_PATTERN_1				0x0001 // Value
#define DP_LINK_TRAIN_PATTERN_2				0x0002 // Value
#define DP_LINK_TRAIN_PATTERN_3				0x0003 // Value
#define DP_LINK_TRAIN_QUAL_MASK				0x000C // Mask
#define DP_LINK_TRAIN_QUAL_NONE				0x0000 // Value
#define DP_LINK_TRAIN_QUAL_D102				0x0004 // Value
#define DP_LINK_TRAIN_QUAL_SYMB_ERR			0x0008 // Value
#define DP_LINK_TRAIN_QUAL_PRBS7			0x000C // Value
#define DP_LINK_TRAIN_CLOCK_RECOVER_EN_MASK 0x0010 // Bool
#define DP_LINK_TRAIN_SCRAMBLE_DI_MASK		0x0020 // Bool (rev)
#define DP_LINK_TRAIN_SYMBL_ERR_SEL_MASK	0x00C0 // Mask
// DP Training Lane n (0x0103 - 0x0106)
#define DP_LINK_TRAIN_LANE0					0x0103 // Reg
#define DP_LINK_TRAIN_LANE1					0x0104 // Reg
#define DP_LINK_TRAIN_LANE2					0x0105 // Reg
#define DP_LINK_TRAIN_LANE3					0x0106 // Reg
#define DP_LINK_TRAIN_LANEn_VCCSWING_MASK	0x0003 // Mask
#define DP_LINK_TRAIN_LANEn_MAXSWING_MASK	0x0004 // Mask
#define DP_LINK_TRAIN_LANEn_PREE_MASK		0x0018 // Mask
#define DP_LINK_TRAIN_LANEn_MAXPREE_MASK	0x0020 // Mask
// DP Down-spread Control (0x0107)
#define DP_DOWNSPREAD_CTL					0x0107 // Reg
#define DP_DOWNSPREAD_CTL_FREQ_MASK			0x0001 // Int
#define DP_DOWNSPREAD_CTL_AMP_MASK			0x0010 // Int
// DP Main Link Channel Coding (0x0108)
#define DP_MAIN_CHAN_CODE					0x0108 // Reg
#define DP_MAIN_CHAN_CODE_EN_ANSI_MASK		0x0001 // Bool

/* *** DPCD Link / Sink Status Field (0x0200)         *** */
/* *** VESA DisplayPort Standard, rev 1.1, p120       *** */

// TODO

/* *** DPCD Automated Self-testing Field (0x0218)     *** */
/* *** VESA DisplayPort Standard, rev 1.1, p123       *** */

// TODO: Optional Field

/* *** DPCD Source Device Specific Field (0x0300)     *** */
/* *** VESA DisplayPort Standard, rev 1.1, p127       *** */

// TODO

/* *** DPCD Sink Device Specific Field (0x0400)       *** */
/* *** VESA DisplayPort Standard, rev 1.1, p127       *** */

// TODO

/* *** DPCD Branch Device Specific Field (0x0500)     *** */
/* *** VESA DisplayPort Standard, rev 1.1, p127       *** */

// TODO

/* *** DPCD Sink Control Field (0x0600)               *** */
/* *** VESA DisplayPort Standard, rev 1.1, p128       *** */

// TODO

/* *** DPCD Reserved (0x0700+)                        *** */
/* ****************************************************** */


#endif /* _DP_RAW_H */

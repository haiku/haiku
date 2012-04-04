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
#define AUX_NATIVE_WRITE                    (8 << 0)
#define AUX_NATIVE_READ                     (9 << 0)
#define AUX_NATIVE_REPLY_ACK                (0 << 4)
#define AUX_NATIVE_REPLY_NACK               (1 << 4)
#define AUX_NATIVE_REPLY_DEFER              (2 << 4)
#define AUX_NATIVE_REPLY_MASK               (3 << 4)
// AUX i2c Communications
#define AUX_I2C_WRITE                       (0 << 0)
#define AUX_I2C_READ                        (1 << 0)
#define AUX_I2C_STATUS                      (2 << 0)
#define AUX_I2C_MOT                         (4 << 0)
#define AUX_I2C_REPLY_ACK                   (0 << 6)
#define AUX_I2C_REPLY_NACK                  (1 << 6)
#define AUX_I2C_REPLY_DEFER                 (2 << 6)
#define AUX_I2C_REPLY_MASK                  (3 << 6)


/* ****************************************************** */
/* *** DPCD (DisplayPort Configuration Data)          *** */
/* *** Read / Written over DisplayPort AUX link       *** */

/* *** DPCD Receiver Compatibility Field (0x0000)     *** */
/* *** VESA DisplayPort Standard, rev 1.1, p112       *** */
// DPCD Version (0x0)
#define DP_DPCD_REV							0x0000		// Reg
#define DP_DPCD_REV_MINOR_MASK				(15 << 0)	// Int
#define DP_DPCD_REV_MAJOR_MASK				(15 << 4)	// Int
#define DP_DPCD_REV_10						0x0010		// Value
#define DP_DPCD_REV_11						0x0011		// Value
// DP Maximum Link Rate (0x1)
#define DP_MAX_LINK_RATE					0x0001		// Reg
// Use DP_LINK_RATE_* for speed.
// DP Maximum Lane Count (0x2)
#define DP_MAX_LANE_COUNT					0x0002		// Reg
#define DP_MAX_LANE_COUNT_MASK				(31 << 0)	// Count
#define DP_MAX_LANE_COUNT_1					(1 << 0)	// Value
#define DP_MAX_LANE_COUNT_2					(2 << 0)	// Value
#define DP_MAX_LANE_COUNT_4					(4 << 0)	// Value
#define DP_ENHANCED_FRAME_CAP_EN			(1 << 7)	// Bool, Rev 1.1
// DP Maximum Downspread (0x3)
#define DP_MAX_DOWNSPREAD					0x0003		// Reg
#define DP_MAX_DOWNSPREAD_EN				(1 << 0)	// Bool
#define DP_MAX_DOWNSPREAD_REQ_NO_HANDSHAKE	(1 << 6)	// Bool
// DP Number of Receiver Ports (0x4)
#define DP_NORP								0x0004		// Reg
#define DP_NORP_MASK						(1 << 0)	// Count
// DP Downstream Port Present (0x5)
#define DP_DOWNSTREAMPORT					0x0005		// Reg
#define DP_DOWNSTREAMPORT_EN				(1 << 0)	// Bool
#define DP_DOWNSTREAMPORT_TYPE_MASK			(3 << 1)	// Mask
#define DP_DOWNSTREAMPORT_TYPE_DP			(0 << 1)	// Value
#define DP_DOWNSTREAMPORT_TYPE_ANALOG		(1 << 1)	// Value
#define DP_DOWNSTREAMPORT_TYPE_DIGITAL		(2 << 1)	// Value
#define DP_DOWNSTREAMPORT_TYPE_OTHER		(3 << 1)	// Value
#define DP_DOWNSTREAMPORT_FORMAT_EN			(1 << 3)	// Bool
// DP Main Link Channel Coding (0x6)
#define DP_CURR_MAIN_CHAN_CODE				0x0006		// Reg
#define DP_CURR_MAIN_CHAN_CODE_ANSIX3_EN	(1 << 0)	// Bool
// DP Downstream Port Count (0x7) (Only 1.1+)
#define DP_DOWNSTREAMPORT_COUNT				0x0007		// Reg
#define DP_DOWNSTREAMPORT_COUNT_MASK		(15 << 0)	// Count
#define DP_DOWNSTREAMPORT_COUNT_OUI_EN		(1 << 7)	// Bool
// DP Port Capability 0
#define DP_PORT0_CAPABILITY0				0x0008		// Reg
#define DP_PORT1_CAPABILITY0				0x000A		// Reg
#define DP_PORT_CAPABILITY0_EDID_EN			(1 << 1)	// Bool
#define DP_PORT_CAPABILITY0_SECOND_EN		(1 << 2)	// Bool
// DP Port Capability 1
#define DP_PORT0_CAPABILITY1				0x0009		// Reg
#define DP_PORT1_CAPABILITY1				0x000B		// Reg
#define DP_PORT_CAPABILITY1_BUF_SIZE_MASK	(255 << 0)	// Size
	// (value + 1) * 32 bytes per lane

/* *** DPCD Link Configuration Field (0x0100)         *** */
/* *** VESA DisplayPort Standard, rev 1.1, p117       *** */

// DP Set Link Rate Per Lane (0x0100)
#define DP_LINK_RATE						0x0100		// Reg
#define DP_LINK_RATE_162					0x0006		// 1.62Ghz
#define DP_LINK_RATE_270					0x000A		// 2.70Ghz
#define DP_LINK_RATE_540					0x0014		// 5.40Ghz
// DP Set Lane Count (0x0101)
#define DP_LANE_COUNT						0x0101		// Reg
#define DP_LANE_COUNT_MASK					(31 << 0)	// Count
#define DP_ENHANCED_FRAME_EN				(1 << 7)	// Bool, Rev 1.1
// DP Training Pattern (0x0102)
#define DP_TRAIN							0x0102		// Reg
#define DP_TRAIN_PATTERN_MASK				(3 << 0)	// Mask
#define DP_TRAIN_PATTERN_DISABLED			(0 << 0)	// Value
#define DP_TRAIN_PATTERN_1					(1 << 0)	// Value
#define DP_TRAIN_PATTERN_2					(2 << 0)	// Value
#define DP_TRAIN_PATTERN_3					(3 << 0)	// Value

#define DP_TRAIN_QUAL_MASK					(3 << 2)	// Mask
#define DP_TRAIN_QUAL_NONE					(0 << 2)	// Value
#define DP_TRAIN_QUAL_D102					(1 << 2)	// Value
#define DP_TRAIN_QUAL_SYMB_ERR				(2 << 2)	// Value
#define DP_TRAIN_QUAL_PRBS7					(3 << 2)	// Value

#define DP_TRAIN_CLOCK_RECOVER_EN			(1 << 4)	// Bool
#define DP_TRAIN_SCRAMBLE_DI				(1 << 5)	// Bool (rev)
#define DP_TRAIN_SYMBL_ERR_SEL_MASK			(3 << 6)	// Mask
#define DP_TRAIN_SYMBL_ERR_SEL_BOTH			(0 << 6)	// Value
#define DP_TRAIN_SYMBL_ERR_SEL_DISPARITY	(1 << 6)	// Value
#define DP_TRAIN_SYMBL_ERR_SEL_SYMBOL		(2 << 6)	// Value
// DP Training Lane n (0x0103 - 0x0106)
#define DP_TRAIN_LANE0						0x0103		// Reg
#define DP_TRAIN_LANE1						0x0104		// Reg
#define DP_TRAIN_LANE2						0x0105		// Reg
#define DP_TRAIN_LANE3						0x0106		// Reg

#define DP_TRAIN_VCC_SWING_SHIFT			(0 << 0)	// Shift
#define DP_TRAIN_VCC_SWING_MASK				(3 << 0)	// Mask
#define DP_TRAIN_VCC_SWING_400				(0 << 0)	// Value
#define DP_TRAIN_VCC_SWING_600				(1 << 0)	// Value
#define DP_TRAIN_VCC_SWING_800				(2 << 0)	// Value
#define DP_TRAIN_VCC_SWING_1200				(3 << 0)	// Value
#define DP_TRAIN_MAX_SWING_EN				(1 << 2)	// Bool

#define DP_TRAIN_PRE_EMPHASIS_SHIFT			(3 << 0)	// Shift
#define DP_TRAIN_PRE_EMPHASIS_MASK			(3 << 3)	// Mask
#define DP_TRAIN_PRE_EMPHASIS_0				(0 << 3)	// Value
#define DP_TRAIN_PRE_EMPHASIS_3_5			(1 << 3)	// Value
#define DP_TRAIN_PRE_EMPHASIS_6				(2 << 3)	// Value
#define DP_TRAIN_PRE_EMPHASIS_9_5			(3 << 3)	// Value
#define DP_TRAIN_MAX_EMPHASIS_EN			(1 << 5)	// Bool
// DP Down-spread Control (0x0107)
#define DP_DOWNSPREAD_CTRL					0x0107		// Reg
#define DP_DOWNSPREAD_CTRL_FREQ_MASK		(1 << 0)	// Int
#define DP_DOWNSPREAD_CTRL_AMP_EN			(1 << 4)	// Int
// DP Main Link Channel Coding (0x0108)
#define DP_MAIN_CHAN_CODE					0x0108		// Reg
#define DP_MAIN_CHAN_CODE_ANSIX3_EN			(1 << 0)	// Bool

/* *** DPCD Link / Sink Status Field (0x0200)         *** */
/* *** VESA DisplayPort Standard, rev 1.1, p120       *** */

// DP Sink Count (0x0200)
#define DP_SINK_COUNT						0x0200		// Reg
#define DP_SINK_COUNT_MASK					(63 << 0)	// Mask
#define DP_SINK_COUNT_CP_READY				(1 << 6)	// Bool
// DP Service IRQ Vector (0x0201)
#define DP_SINK_IRQ_VECTOR					0x0201		// Reg
#define DP_SINK_IRQ_TEST_REQ				(1 << 1)	// Bool
#define DP_SINK_IRQ_CP_IRQ					(1 << 2)	// Bool
#define DP_SINK_IRQ_VENDOR					(1 << 6)	// Bool
// DP Lane Status      A B
#define DP_LANE_STATUS_0_1					0x0202		// Reg
#define DP_LANE_STATUS_2_3					0x0203		// Reg
#define DP_LINK_STATUS_SIZE					6			// Size
#define DP_LANE_STATUS_CR_DONE_A			(1 << 0)	// Bool
#define DP_LANE_STATUS_CHEQ_DONE_A			(1 << 1)	// Bool
#define DP_LANE_STATUS_SYMB_LOCK_A			(1 << 2)	// Bool
#define DP_LANE_STATUS_CR_DONE_B			(1 << 4)	// Bool
#define DP_LANE_STATUS_CHEQ_DONE_B			(1 << 5)	// Bool
#define DP_LANE_STATUS_SYMB_LOCK_B			(1 << 6)	// Bool
// DP Lane Align Status (0x0204)
#define DP_LANE_ALIGN						0x0204		// Reg
#define DP_LANE_ALIGN_DONE					(1 << 0)	// Bool
#define DP_LANE_ALIGN_PORT_STATUS_CHANGE	(1 << 6)	// Bool
#define DP_LANE_ALIGN_LINK_STATUS_UPDATE	(1 << 7)	// Bool
// DP Sink Status (0x0205)
#define DP_SINK_STATUS						0x0205		// Reg
#define DP_SINK_STATUS_IN_SYNC_0			(1 << 0)	// Bool
#define DP_SINK_STATUS_IN_SYNC_1			(1 << 1)	// Bool
// DP Adjust Request   A B
#define DP_ADJ_REQUEST_0_1					0x0206		// Reg
#define DP_ADJ_REQUEST_2_3					0x0207		// Reg
#define DP_ADJ_VCC_SWING_LANEA_SHIFT		0			// Shift
#define DP_ADJ_VCC_SWING_LANEA_MASK			(3 << 0)	// Mask
#define DP_ADJ_PRE_EMPHASIS_LANEA_SHIFT		2			// Shift
#define DP_ADJ_PRE_EMPHASIS_LANEA_MASK		(3 << 2)	// Mask
#define DP_ADJ_VCC_SWING_LANEB_SHIFT		4			// Shift
#define DP_ADJ_VCC_SRING_LANEB_MASK			(3 << 4)	// Mask
#define DP_ADJ_PRE_EMPHASIS_LANEB_SHIFT		6			// Shift
#define DP_ADJ_PRE_EMPHASIS_LANEB_MASK		(3 << 6)	// Mask

// TODO: 0x0210 - 0x0217

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

#define DP_SET_POWER						0x0600		// Reg
#define DP_SET_POWER_D0						(1 << 0)	// Value
#define DP_SET_POWER_D3						(1 << 1)	// Value

/* *** DPCD Reserved (0x0700+)                        *** */
/* ****************************************************** */


#endif /* _DP_RAW_H */

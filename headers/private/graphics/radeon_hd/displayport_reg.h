/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 *
 * DisplayPort DRM Specifications:
 *		 Copyright © 2008 Keith Packard
 */
#ifndef __DISPLAYPORT_REG_H__
#define __DISPLAYPORT_REG_H__


/* TODO: get access to DisplayPort specifications and
 * place this into graphic private common code
 */

#define DP_TPS3_SUPPORTED					(1 << 6)

#define DP_LANE0_1_STATUS					0x202
#define DP_LANE2_3_STATUS					0x203
#define DP_LANE_CR_DONE						(1 << 0)
#define DP_LANE_CHANNEL_EQ_DONE				(1 << 1)
#define DP_LANE_SYMBOL_LOCKED				(1 << 2)

#define DP_CHANNEL_EQ_BITS (DP_LANE_CR_DONE \
	| DP_LANE_CHANNEL_EQ_DONE \
	| DP_LANE_SYMBOL_LOCKED)

#define DP_LANE_ALIGN_STATUS_UPDATED		0x204

#define DP_INTERLANE_ALIGN_DONE				(1 << 0)
#define DP_DOWNSTREAM_PORT_STATUS_CHANGED	(1 << 6)
#define DP_LINK_STATUS_UPDATED				(1 << 7)
#define DP_LINK_STATUS_SIZE					6

#define DP_SINK_STATUS						0x205

#define DP_RECEIVE_PORT_0_STATUS			(1 << 0)
#define DP_RECEIVE_PORT_1_STATUS			(1 << 1)

#define DP_ADJUST_REQUEST_LANE0_1				0x206
#define DP_ADJUST_REQUEST_LANE2_3				0x207
#define DP_ADJUST_VOLTAGE_SWING_LANE0_MASK		0x03
#define DP_ADJUST_VOLTAGE_SWING_LANE0_SHIFT 	0
#define DP_ADJUST_PRE_EMPHASIS_LANE0_MASK		0x0c
#define DP_ADJUST_PRE_EMPHASIS_LANE0_SHIFT		2
#define DP_ADJUST_VOLTAGE_SWING_LANE1_MASK		0x30
#define DP_ADJUST_VOLTAGE_SWING_LANE1_SHIFT		4
#define DP_ADJUST_PRE_EMPHASIS_LANE1_MASK		0xc0
#define DP_ADJUST_PRE_EMPHASIS_LANE1_SHIFT		6

#define DP_SET_POWER						0x600
#define DP_SET_POWER_D0						0x1


#endif /*__DISPLAYPORT_REG_H__*/

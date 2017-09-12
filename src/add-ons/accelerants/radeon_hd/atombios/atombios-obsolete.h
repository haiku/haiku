/*
 * Copyright 2016-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _ATOMBIOS_OBSOLETE_H
#define _ATOMBIOS_OBSOLETE_H


// atombios.h in radeon linux has some older obsolete things.
// atombios.h in amdgpu has these obsolete things removed.
// This file contains things removed from amdgpu that we still need.


#define ATOM_S2_CV_DPMS_STATE			0x01000000L
#define ATOM_S2_DFP2_DPMS_STATE			0x00800000L
#define ATOM_S2_DFP3_DPMS_STATE			0x02000000L
#define ATOM_S2_DFP4_DPMS_STATE			0x04000000L
#define ATOM_S2_DFP5_DPMS_STATE			0x08000000L


// ucAction
#define ATOM_DP_ACTION_TRAINING_START           0x02
#define ATOM_DP_ACTION_TRAINING_COMPLETE        0x03
#define ATOM_DP_ACTION_TRAINING_PATTERN_SEL     0x04
#define ATOM_DP_ACTION_SET_VSWING_PREEMP        0x05
#define ATOM_DP_ACTION_GET_VSWING_PREEMP        0x06
#define ATOM_DP_ACTION_BLANKING                 0x07

// ucConfig
#define ATOM_DP_CONFIG_ENCODER_SEL_MASK         0x03
#define ATOM_DP_CONFIG_DIG1_ENCODER             0x00
#define ATOM_DP_CONFIG_DIG2_ENCODER             0x01
#define ATOM_DP_CONFIG_EXTERNAL_ENCODER         0x02
#define ATOM_DP_CONFIG_LINK_SEL_MASK            0x04
#define ATOM_DP_CONFIG_LINK_A                   0x00
#define ATOM_DP_CONFIG_LINK_B                   0x04


#endif /* _ATOMBIOS_OBSOLETE_H */

/*
 * Copyright 2016, Haiku, Inc. All Rights Reserved.
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


#endif /* _ATOMBIOS_OBSOLETE_H */

/*
 * Copyright (c) 2003, Thomas Kurschel
 * Distributed under the terms of the MIT License.
*/
#ifndef _DDC_H
#define _DDC_H


#include "i2c.h"
#include "edid.h"


#ifdef __cplusplus
extern "C" {
#endif

void ddc2_init_timing(i2c_bus *bus);

// read EDID and VDIF from monitor via ddc2
// (currently, *vdif and *vdif_len is always set to null)
status_t ddc2_read_edid1(const i2c_bus *bus, edid1_info *edid, 
	void **vdif, size_t *vdifLength);

#ifdef __cplusplus
}
#endif

#endif	/* _DDC_H */

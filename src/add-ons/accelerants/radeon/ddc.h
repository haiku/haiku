/*
	Copyright (c) 2003, Thomas Kurschel


	Part of DDC driver
		
	Main DDC communication
*/

#ifndef _DDC_H
#define _DDC_H

#include "i2c.h"
#include "edid.h"

// read EDID and VDIF from monitor via ddc2
// (currently, *vdif and *vdif_len is always set to null)
status_t ddc2_read_edid1( const i2c_bus *bus, edid1_info *edid, 
	void **vdif, size_t *vdif_len );

#endif

/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_BIOS_H
#define RADEON_HD_BIOS_H


#include <stdint.h>

#include "atom.h"


void atombios_crtc_power(uint8 crt_id, int state);
status_t radeon_init_bios(uint8* bios);


#endif /* RADEON_HD_BIOS_H */

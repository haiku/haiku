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


status_t radeon_init_bios(uint8* bios);
bool radeon_bios_isposted();
status_t radeon_dump_bios();


#endif /* RADEON_HD_BIOS_H */

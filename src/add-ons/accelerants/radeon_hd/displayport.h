/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_DISPLAYPORT_H
#define RADEON_HD_DISPLAYPORT_H


#include <create_display_modes.h>
#include <stdint.h>
#include <SupportDefs.h>

#include "displayport_reg.h"


int dp_aux_write(uint32 hwPin, uint16 address, uint8* send,
	uint8 sendBytes, uint8 delay);
int dp_aux_read(uint32 hwPin, uint16 address, uint8* recv,
	int recvBytes, uint8 delay);
status_t dp_aux_set_i2c_byte(uint32 hwPin, uint16 address,
	uint8* data, bool end);
status_t dp_aux_get_i2c_byte(uint32 hwPin, uint16 address,
	uint8* data, bool end);

uint32 dp_get_link_clock(uint32 connectorIndex);
uint32 dp_get_link_clock_encode(uint32 dpLinkClock);
uint32 dp_get_link_clock_decode(uint32 dpLinkClock);

void dp_setup_connectors();
status_t dp_link_train(uint8 crtcID, display_mode* mode);


#endif /* RADEON_HD_DISPLAYPORT_H */

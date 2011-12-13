/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_DISPLAYPORT_H
#define RADEON_HD_DISPLAYPORT_H


#include <stdint.h>
#include <SupportDefs.h>


// From the VESA DisplayPort spec
// TODO: may want to move these into common code
#define AUX_NATIVE_WRITE        0x8
#define AUX_NATIVE_READ         0x9
#define AUX_I2C_WRITE           0x0
#define AUX_I2C_READ            0x1
#define AUX_I2C_STATUS          0x2
#define AUX_I2C_MOT             0x4

#define AUX_NATIVE_REPLY_ACK    (0x0 << 4)
#define AUX_NATIVE_REPLY_NACK   (0x1 << 4)
#define AUX_NATIVE_REPLY_DEFER  (0x2 << 4)
#define AUX_NATIVE_REPLY_MASK   (0x3 << 4)

#define AUX_I2C_REPLY_ACK       (0x0 << 6)
#define AUX_I2C_REPLY_NACK      (0x1 << 6)
#define AUX_I2C_REPLY_DEFER     (0x2 << 6)
#define AUX_I2C_REPLY_MASK      (0x3 << 6)


int dp_aux_write(uint32 hwLine, uint16 address, uint8* send,
	uint8 sendBytes, uint8 delay);
int dp_aux_read(uint32 hwLine, uint16 address, uint8* recv,
	int recvBytes, uint8 delay);
status_t dp_aux_set_i2c_byte(uint32 hwLine, uint16 address,
	uint8* data, bool end);
status_t dp_aux_get_i2c_byte(uint32 hwLine, uint16 address,
	uint8* data, bool end);

uint32 dp_get_link_clock(uint32 connectorIndex);
status_t dp_link_train(uint32 connectorIndex);


#endif /* RADEON_HD_DISPLAYPORT_H */

/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_CONNECTOR_H
#define RADEON_HD_CONNECTOR_H


#include <video_configuration.h>

#include "accelerant.h"


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


// convert radeon connector to common connector type
const int connector_convert_legacy[] = {
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_VGA,
	VIDEO_CONNECTOR_DVII,
	VIDEO_CONNECTOR_DVID,
	VIDEO_CONNECTOR_DVIA,
	VIDEO_CONNECTOR_SVIDEO,
	VIDEO_CONNECTOR_COMPOSITE,
	VIDEO_CONNECTOR_LVDS,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_HDMIA,
	VIDEO_CONNECTOR_HDMIB,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_9DIN,
	VIDEO_CONNECTOR_DP
};

const int connector_convert[] = {
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_DVII,
	VIDEO_CONNECTOR_DVII,
	VIDEO_CONNECTOR_DVID,
	VIDEO_CONNECTOR_DVID,
	VIDEO_CONNECTOR_VGA,
	VIDEO_CONNECTOR_COMPOSITE,
	VIDEO_CONNECTOR_SVIDEO,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_9DIN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_HDMIA,
	VIDEO_CONNECTOR_HDMIB,
	VIDEO_CONNECTOR_LVDS,
	VIDEO_CONNECTOR_9DIN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_DP,
	VIDEO_CONNECTOR_EDP,
	VIDEO_CONNECTOR_UNKNOWN
};


int dp_aux_write(uint32 hwLine, uint16 address, uint8 *send,
	uint8 sendBytes, uint8 delay);
int dp_aux_read(uint32 hwLine, uint16 address, uint8 *recv,
	int recvBytes, uint8 delay);
status_t dp_aux_set_i2c_byte(uint32 hwLine, uint16 address,
	uint8* data, bool end);
status_t dp_aux_get_i2c_byte(uint32 hwLine, uint16 address,
	uint8* data, bool end);


status_t gpio_probe();
status_t connector_attach_gpio(uint32 id, uint8 hw_line);
bool connector_read_edid(uint32 connector, edid1_info *edid);
status_t connector_probe();
status_t connector_probe_legacy();
void debug_connectors();


#endif /* RADEON_HD_CONNECTOR_H */

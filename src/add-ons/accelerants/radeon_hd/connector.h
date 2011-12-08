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


int dp_aux_speak(uint8 connectorIndex, uint8 *send, int sendBytes,
	uint8 *recv, int recvBytes, uint8 delay, uint8 *ack);
status_t gpio_probe();
status_t connector_attach_gpio(uint32 id, uint8 hw_line);
bool connector_read_edid(uint32 connector, edid1_info *edid);
status_t connector_probe();
status_t connector_probe_legacy();
void debug_connectors();


#endif /* RADEON_HD_CONNECTOR_H */

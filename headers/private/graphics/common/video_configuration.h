/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _VIDEO_CONFIGURATION_H
#define _VIDEO_CONFIGURATION_H


// Video connector types
#define VIDEO_CONNECTOR_UNKNOWN 	0x00
#define VIDEO_CONNECTOR_VGA			0x01
#define VIDEO_CONNECTOR_DVII		0x02
#define VIDEO_CONNECTOR_DVID		0x03
#define VIDEO_CONNECTOR_DVIA		0x04
#define VIDEO_CONNECTOR_COMPOSITE	0x05
#define VIDEO_CONNECTOR_SVIDEO		0x06
#define VIDEO_CONNECTOR_LVDS		0x07
#define VIDEO_CONNECTOR_COMPONENT	0x08
#define VIDEO_CONNECTOR_9DIN		0x09
#define VIDEO_CONNECTOR_DP			0x0A
#define VIDEO_CONNECTOR_EDP			0x0B
#define VIDEO_CONNECTOR_HDMIA		0x0C
#define VIDEO_CONNECTOR_HDMIB		0x0D
#define VIDEO_CONNECTOR_TV			0x0E


// Video encoder types
#define VIDEO_ENCODER_NONE		0x00
#define VIDEO_ENCODER_DAC		0x01
#define VIDEO_ENCODER_TMDS		0x02
#define VIDEO_ENCODER_LVDS		0x03
#define VIDEO_ENCODER_TVDAC		0x04


// to ensure compatibility with C accelerants
#ifdef __cplusplus
extern "C" {
#endif


// mostly for debugging detected monitors
const char* get_connector_name(uint32 connector);
const char* get_encoder_name(uint32 encoder);


#ifdef __cplusplus
}
#endif


#endif /* _VIDEO_CONFIGURATION_H */

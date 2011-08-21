/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _VIDEO_ELECTRONICS_H
#define _VIDEO_ELECTRONICS_H


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


const struct video_connectors {
	uint32      type;
	const char* name;
} kVideoConnector[] = {
	{VIDEO_CONNECTOR_UNKNOWN, "Unknown"},
	{VIDEO_CONNECTOR_VGA, "VGA" },
	{VIDEO_CONNECTOR_DVII, "DVI-I"},
	{VIDEO_CONNECTOR_DVID, "DVI-D"},
	{VIDEO_CONNECTOR_DVIA, "DVI-A"},
	{VIDEO_CONNECTOR_COMPOSITE, "Composite"},
	{VIDEO_CONNECTOR_SVIDEO, "S-Video"},
	{VIDEO_CONNECTOR_LVDS, "LVDS"},
	{VIDEO_CONNECTOR_COMPONENT, "Component"},
	{VIDEO_CONNECTOR_9DIN, "DIN"},
	{VIDEO_CONNECTOR_DP, "DisplayPort"},
	{VIDEO_CONNECTOR_EDP, "Embedded DisplayPort"},
	{VIDEO_CONNECTOR_HDMIA, "HDMI A"},
	{VIDEO_CONNECTOR_HDMIB, "HDMI B"},
	{VIDEO_CONNECTOR_TV, "TV"},
};


// Video encoder types
#define VIDEO_ENCODER_NONE		0x00
#define VIDEO_ENCODER_DAC		0x01
#define VIDEO_ENCODER_TMDS		0x02
#define VIDEO_ENCODER_LVDS		0x03
#define VIDEO_ENCODER_TVDAC		0x04


const struct video_encoders {
	uint32      type;
	const char* name;
} kVideoEncoder[] = {
	{VIDEO_ENCODER_NONE, "None"},
	{VIDEO_ENCODER_DAC, "DAC"},
	{VIDEO_ENCODER_TMDS, "TMDS"},
	{VIDEO_ENCODER_LVDS, "LVDS"},
	{VIDEO_ENCODER_TVDAC, "TV"},
};


#endif /* _VIDEO_ELECTRONICS_H */

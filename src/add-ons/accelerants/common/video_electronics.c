/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include <KernelExport.h>
#include <stdio.h>

#include "video_electronics.h"


const char*
decode_connector_name(uint32 connector)
{
	switch (connector) {
		case VIDEO_CONNECTOR_VGA:
			return "VGA";
		case VIDEO_CONNECTOR_DVII:
			return "DVI-I (Digital and Analog)";
		case VIDEO_CONNECTOR_DVID:
			return "DVI-D (Digital Only)";
		case VIDEO_CONNECTOR_DVIA:
			return "DVI-A (Analog Only)";
		case VIDEO_CONNECTOR_COMPOSITE:
			return "Composite";
		case VIDEO_CONNECTOR_SVIDEO:
			return "S-Video";
		case VIDEO_CONNECTOR_LVDS:
			return "LVDS Panel";
		case VIDEO_CONNECTOR_COMPONENT:
			return "Component";
		case VIDEO_CONNECTOR_9DIN:
			return "9-Pin DIN";
		case VIDEO_CONNECTOR_DP:
			return "DisplayPort";
		case VIDEO_CONNECTOR_EDP:
			return "Embedded DisplayPort";
		case VIDEO_CONNECTOR_HDMIA:
			return "HDMI A";
		case VIDEO_CONNECTOR_HDMIB:
			return "HDMI B";
		case VIDEO_CONNECTOR_TV:
			return "TV";
		case VIDEO_CONNECTOR_UNKNOWN:
			return "Unknown";
	}
	return "Connector Undefined";
}


const char*
decode_encoder_name(uint32 encoder)
{
	switch (encoder) {
		case VIDEO_ENCODER_NONE:
			return "None";
		case VIDEO_ENCODER_DAC:
			return "DAC";
		case VIDEO_ENCODER_TMDS:
			return "TMDS";
		case VIDEO_ENCODER_LVDS:
			return "LVDS";
		case VIDEO_ENCODER_TVDAC:
			return "TV DAC";
	}
	return "Encoder Undefined";
}

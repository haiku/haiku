/*
 * Copyright 2016-2019, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */
#ifndef _RTSP_STREAMER_PLUGIN_H
#define _RTSP_STREAMER_PLUGIN_H

#include <Streamer.h>

using BCodecKit::BStreamer;
using BCodecKit::BStreamerPlugin;


class RTSPStreamer : public BStreamer
{
public:
								RTSPStreamer();
	virtual						~RTSPStreamer();

	virtual status_t			Sniff(const BUrl& url, BDataIO** source);
};


class RTSPStreamerPlugin : public BStreamerPlugin {
public:
	virtual	BStreamer*			NewStreamer();
};

#endif // _RTSP_STREAMER_PLUGIN_H

/*
 * Copyright 2016-2019, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */

#include "RTSPStreamerPlugin.h"

#include "RTSPMediaIO.h"


B_DECLARE_CODEC_KIT_PLUGIN(
	RTSPStreamerPlugin,
	"rtsp_streamer",
	B_CODEC_KIT_PLUGIN_VERSION
);


RTSPStreamer::RTSPStreamer()
	:
	BStreamer()
{
}


RTSPStreamer::~RTSPStreamer()
{
}


status_t
RTSPStreamer::Sniff(const BUrl& url, BDataIO** source)
{
	RTSPMediaIO* outSource = new RTSPMediaIO(url);
	status_t ret = outSource->Open();
	if (ret == B_OK) {
		*source = outSource;
		return B_OK;
	}
	delete outSource;
	return ret;
}


BStreamer*
RTSPStreamerPlugin::NewStreamer()
{
	return new RTSPStreamer();
}

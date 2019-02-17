/*
 * Copyright 2016-2019, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */

#include "RTSPStreamerPlugin.h"

#include "RTSPMediaIO.h"


RTSPStreamer::RTSPStreamer()
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


Streamer*
RTSPStreamerPlugin::NewStreamer()
{
	return new RTSPStreamer();
}


MediaPlugin *instantiate_plugin()
{
	return new RTSPStreamerPlugin();
}

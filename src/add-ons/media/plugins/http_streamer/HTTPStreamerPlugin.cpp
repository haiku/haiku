/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */


#include "HTTPStreamerPlugin.h"

#include "HTTPMediaIO.h"


HTTPStreamer::HTTPStreamer()
{
}


HTTPStreamer::~HTTPStreamer()
{
}


status_t
HTTPStreamer::Sniff(const BUrl& url, BDataIO** source)
{
	HTTPMediaIO* outSource = new HTTPMediaIO(url);
	status_t ret = outSource->Open();
	if (ret == B_OK) {
		*source = outSource;
		return B_OK;
	}
	delete outSource;
	return ret;
}


Streamer*
HTTPStreamerPlugin::NewStreamer()
{
	return new HTTPStreamer();
}


MediaPlugin *instantiate_plugin()
{
	return new HTTPStreamerPlugin();
}

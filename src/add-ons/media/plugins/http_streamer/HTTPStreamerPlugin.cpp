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
HTTPStreamer::Sniff(BUrl* url, BMediaIO** source)
{
	HTTPMediaIO* ret = new HTTPMediaIO(url);
	if (ret->InitCheck() == B_OK) {
		*source = ret;
		return B_OK;
	}
	delete ret;
	return B_ERROR;
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

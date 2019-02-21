/*
 * Copyright 2016-2018, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */


#include "HTTPStreamerPlugin.h"

#include "MediaDebug.h"


B_DECLARE_CODEC_KIT_PLUGIN(
	HTTPStreamerPlugin,
	"http_streamer",
	B_CODEC_KIT_PLUGIN_VERSION
);


HTTPStreamer::HTTPStreamer()
{
	CALLED();
}


HTTPStreamer::~HTTPStreamer()
{
	CALLED();
}


status_t
HTTPStreamer::Sniff(const BUrl& url)
{
	CALLED();

	HTTPMediaIO* adapter = new HTTPMediaIO(url);

	status_t ret = adapter->Open();
	if (ret == B_OK) {
		fAdapter = adapter;
		return B_OK;
	}
	delete adapter;
	return ret;
}


BMediaIO*
HTTPStreamer::Adapter() const
{
	return fAdapter;
}


BStreamer*
HTTPStreamerPlugin::NewStreamer()
{
	return new HTTPStreamer();
}

/*
 * Copyright 2016-2018, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */


#include "HTTPStreamerPlugin.h"

#include "HTTPMediaIO.h"

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
HTTPStreamer::Sniff(const BUrl& url, BDataIO** source)
{
	CALLED();

	HTTPMediaIO* outSource = new HTTPMediaIO(url);

	status_t ret = outSource->Open();
	if (ret == B_OK) {
		*source = outSource;
		return B_OK;
	}
	delete outSource;
	return ret;
}


BStreamer*
HTTPStreamerPlugin::NewStreamer()
{
	return new HTTPStreamer();
}

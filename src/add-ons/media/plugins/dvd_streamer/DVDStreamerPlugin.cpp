/*
 * Copyright 2019, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "DVDStreamerPlugin.h"

#include "DVDMediaIO.h"


B_DECLARE_CODEC_KIT_PLUGIN(
	DVDStreamerPlugin,
	"dvd_streamer",
	B_CODEC_KIT_PLUGIN_VERSION
);


DVDStreamer::DVDStreamer()
	:
	BStreamer()
{
}


DVDStreamer::~DVDStreamer()
{
}


status_t
DVDStreamer::Sniff(const BUrl& url, BDataIO** source)
{
	BString path = url.UrlString();
	BString protocol = url.Protocol();
	if (protocol == "dvd") {
		path = path.RemoveFirst("dvd://");
	} else if(protocol == "file") {
		path = path.RemoveFirst("file://");	
	} else
		return B_UNSUPPORTED;

	DVDMediaIO* outSource = new DVDMediaIO(path);
	status_t ret = outSource->Open();
	if (ret == B_OK) {
		*source = outSource;
		return B_OK;
	}
	delete outSource;
	return ret;	
}


BStreamer*
DVDStreamerPlugin::NewStreamer()
{
	return new DVDStreamer();
}

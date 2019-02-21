/*
 * Copyright 2019, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "DVDStreamerPlugin.h"


B_DECLARE_CODEC_KIT_PLUGIN(
	DVDStreamerPlugin,
	"dvd_streamer",
	B_CODEC_KIT_PLUGIN_VERSION
);


DVDStreamer::DVDStreamer()
	:
	BStreamer(),
	fAdapter(NULL)
{
}


DVDStreamer::~DVDStreamer()
{
}


status_t
DVDStreamer::Sniff(const BUrl& url)
{
	BString path = url.UrlString();
	BString protocol = url.Protocol();
	if (protocol == "dvd") {
		path = path.RemoveFirst("dvd://");
	} else if (protocol == "file") {
		path = path.RemoveFirst("file://");	
	} else
		return B_UNSUPPORTED;

	DVDMediaIO* adapter = new DVDMediaIO(path);
	status_t ret = adapter->Open();
	if (ret == B_OK) {
		fAdapter = adapter;
		return B_OK;
	}
	delete adapter;
	return ret;	
}


BMediaIO*
DVDStreamer::Adapter() const
{
	return fAdapter;
}


void
DVDStreamer::MouseMoved(uint32 x, uint32 y)
{
	fAdapter->MouseMoved(x, y);
}


void
DVDStreamer::MouseDown(uint32 x, uint32 y)
{
	fAdapter->MouseDown(x, y);
}


BStreamer*
DVDStreamerPlugin::NewStreamer()
{
	return new DVDStreamer();
}

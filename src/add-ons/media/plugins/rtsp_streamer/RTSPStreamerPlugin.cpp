
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
	RTSPMediaIO* ret = new RTSPMediaIO(url);
	if (ret->InitCheck() == B_OK) {
		*source = ret;
		return B_OK;
	}
	delete ret;
	return B_ERROR;
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

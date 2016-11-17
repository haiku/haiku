
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

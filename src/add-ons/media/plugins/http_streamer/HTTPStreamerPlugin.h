/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */
#ifndef _HTTP_STREAMER_PLUGIN_H
#define _HTTP_STREAMER_PLUGIN_H


#include "StreamerPlugin.h"

class HTTPStreamer : public Streamer {
public:
								HTTPStreamer();
	virtual						~HTTPStreamer();

			status_t			Sniff(BUrl* url, BMediaIO** source);
};


class HTTPStreamerPlugin : public StreamerPlugin {
public:
	virtual	Streamer*			NewStreamer();
};

#endif // _HTTP_STREAMER_PLUGIN_H

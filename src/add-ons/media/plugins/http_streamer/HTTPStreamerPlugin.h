/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */
#ifndef _HTTP_STREAMER_PLUGIN_H
#define _HTTP_STREAMER_PLUGIN_H


#include <Streamer.h>


class HTTPStreamer : public BStreamer
{
public:
								HTTPStreamer();
	virtual						~HTTPStreamer();

	virtual status_t			Sniff(const BUrl& url, BDataIO** source);
};


class HTTPStreamerPlugin : public BStreamerPlugin
{
public:
	virtual	BStreamer*			NewStreamer();
};

#endif // _HTTP_STREAMER_PLUGIN_H

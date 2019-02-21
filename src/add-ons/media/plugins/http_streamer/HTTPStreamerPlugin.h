/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */
#ifndef _HTTP_STREAMER_PLUGIN_H
#define _HTTP_STREAMER_PLUGIN_H


#include <Streamer.h>

#include "HTTPMediaIO.h"

using BCodecKit::BMediaIO;
using BCodecKit::BMediaPlugin;
using BCodecKit::BStreamer;
using BCodecKit::BStreamerPlugin;


class HTTPStreamer : public BStreamer
{
public:
								HTTPStreamer();
	virtual						~HTTPStreamer();

	virtual status_t			Sniff(const BUrl& url);
	virtual BMediaIO*			Adapter() const;

private:
			HTTPMediaIO*		fAdapter;
};


class HTTPStreamerPlugin : public BStreamerPlugin
{
public:
	virtual	BStreamer*			NewStreamer();
};

#endif // _HTTP_STREAMER_PLUGIN_H

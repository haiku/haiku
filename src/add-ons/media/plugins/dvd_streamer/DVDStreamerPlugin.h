/*
 * Copyright 2019, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _DVD_STREAMER_PLUGIN_H
#define _DVD_STREAMER_PLUGIN_H


#include <Streamer.h>

using BCodecKit::BStreamer;
using BCodecKit::BStreamerPlugin;


class DVDStreamer : public BStreamer
{
public:
								DVDStreamer();
	virtual						~DVDStreamer();

	virtual status_t			Sniff(const BUrl& url, BDataIO** source);
};


class DVDStreamerPlugin : public BStreamerPlugin {
public:
	virtual	BStreamer*			NewStreamer();
};


#endif // _DVD_STREAMER_PLUGIN_H

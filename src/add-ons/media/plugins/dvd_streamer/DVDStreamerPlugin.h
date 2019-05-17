/*
 * Copyright 2019, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DVD_STREAMER_PLUGIN_H
#define _DVD_STREAMER_PLUGIN_H


#include "StreamerPlugin.h"

class DVDStreamer : public Streamer
{
public:
								DVDStreamer();
	virtual						~DVDStreamer();

	virtual status_t			Sniff(const BUrl& url, BDataIO**);

#if 0
	virtual void				MouseMoved(uint32 x, uint32 y);
	virtual void				MouseDown(uint32 x, uint32 y);
#endif
};


class DVDStreamerPlugin : public StreamerPlugin
{
public:
	virtual	Streamer*			NewStreamer();
};


#endif // _DVD_STREAMER_PLUGIN_H

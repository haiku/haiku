/*
 * Copyright 2019, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DVD_STREAMER_PLUGIN_H
#define _DVD_STREAMER_PLUGIN_H


#include <Streamer.h>

#include "DVDMediaIO.h"

using BCodecKit::BMediaIO;
using BCodecKit::BStreamer;
using BCodecKit::BStreamerPlugin;


class DVDStreamer : public BStreamer
{
public:
								DVDStreamer();
	virtual						~DVDStreamer();

	virtual status_t			Sniff(const BUrl& url);
	virtual BMediaIO*			Adapter() const;

	virtual void				MouseMoved(uint32 x, uint32 y);
	virtual void				MouseDown(uint32 x, uint32 y);

private:
			DVDMediaIO*			fAdapter;
};


class DVDStreamerPlugin : public BStreamerPlugin {
public:
	virtual	BStreamer*			NewStreamer();
};


#endif // _DVD_STREAMER_PLUGIN_H

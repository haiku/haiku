/*
 * Copyright 2017, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STREAMER_PLUGIN_H
#define _STREAMER_PLUGIN_H


#include <MediaIO.h>
#include <Url.h>

#include "MediaPlugin.h"


namespace BPrivate { namespace media {

class PluginManager;


class Streamer {
public:
	virtual status_t			Sniff(const BUrl& url, BDataIO** source) = 0;

protected:
								Streamer();
	virtual						~Streamer();

private:
			MediaPlugin*		fMediaPlugin;

	friend class PluginManager;
	friend class MediaStreamer;

	virtual void				_ReservedStreamer1();
	virtual void				_ReservedStreamer2();
	virtual void				_ReservedStreamer3();
	virtual void				_ReservedStreamer4();
	virtual void				_ReservedStreamer5();

			uint32				fReserved[5];
};


class StreamerPlugin : public virtual MediaPlugin {
public:
								StreamerPlugin();

	virtual	Streamer*			NewStreamer() = 0;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;


#endif // _STREAMER_PLUGIN_H

/*
 * Copyright 2017, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STREAMER_PLUGIN_H
#define _STREAMER_PLUGIN_H


#include <MediaIO.h>
#include <MediaPlugin.h>
#include <Url.h>


namespace BCodecKit {

namespace BPrivate {
	class PluginManager;
}


class BStreamer {
public:
	virtual status_t			Sniff(const BUrl& url) = 0;
	virtual BMediaIO*			Adapter() const = 0;

	// Base impl does nothing
	virtual void				MouseMoved(uint32 x, uint32 y);
	virtual void				MouseDown(uint32 x, uint32 y);

protected:
								BStreamer();
	virtual						~BStreamer();

private:
			BMediaPlugin*		fMediaPlugin;

	friend class BCodecKit::BPrivate::PluginManager;

	virtual void				_ReservedStreamer1();
	virtual void				_ReservedStreamer2();
	virtual void				_ReservedStreamer3();
	virtual void				_ReservedStreamer4();
	virtual void				_ReservedStreamer5();

			uint32				fReserved[5];
};


class BStreamerPlugin : public virtual BMediaPlugin {
public:
								BStreamerPlugin();

	virtual	BStreamer*			NewStreamer() = 0;
};


} // namespace BCodecKit


#endif // _STREAMER_PLUGIN_H

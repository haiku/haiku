#ifndef _STREAMER_PLUGIN_H
#define _STREAMER_PLUGIN_H


#include <MediaIO.h>
#include <MediaTrack.h>
#include <Url.h>

#include "MediaPlugin.h"


namespace BPrivate { namespace media {

class PluginManager;

class Streamer {
public:
								Streamer();
	virtual						~Streamer();

	virtual status_t			Sniff(const BUrl& url, BDataIO** source) = 0;
private:
	virtual void				_ReservedStreamer1();
	virtual void				_ReservedStreamer2();
	virtual void				_ReservedStreamer3();
	virtual void				_ReservedStreamer4();
	virtual void				_ReservedStreamer5();

			MediaPlugin*		fMediaPlugin;
			uint32				fReserved[5];

	friend class PluginManager;
};


class StreamerPlugin : public virtual MediaPlugin {
public:
	virtual	Streamer*			NewStreamer() = 0;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;


#endif // _STREAMER_PLUGIN_H

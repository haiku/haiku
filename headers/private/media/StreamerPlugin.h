#ifndef _STREAMER_PLUGIN_H
#define _STREAMER_PLUGIN_H

#include <MediaTrack.h>
#include <Url.h>

#include "MediaPlugin.h"

namespace BPrivate { namespace media {

class PluginManager;

class Streamer {
public:
								Streamer();
	virtual						~Streamer();

			status_t			Sniff(BUrl* url, BMediaIO** source) = 0;
private:
	virtual void				_ReservedStreamer1();
	virtual void				_ReservedStreamer2();
	virtual void				_ReservedStreamer3();
	virtual void				_ReservedStreamer4();
	virtual void				_ReservedStreamer5();

			MediaPlugin*		fMediaPlugin;
			uint32				fReserved[5];

	// needed for plug-in reference count management
	friend class PluginManager;
};


class StreamerPlugin : public virtual MediaPlugin {
public:
	virtual	Streamer*			NewStreamer() = 0;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _STREAMER_PLUGIN_H

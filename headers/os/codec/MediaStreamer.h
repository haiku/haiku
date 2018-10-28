/*
 * Copyright 2017, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_STREAMER_H
#define _MEDIA_STREAMER_H


#include <Url.h>

#include "StreamerPlugin.h"


namespace BPrivate {
namespace media {


class MediaStreamer {
public:
								MediaStreamer(BUrl url);
								~MediaStreamer();

			status_t			CreateAdapter(BDataIO** adapter);

private:
			BUrl				fUrl;
			Streamer*			fStreamer;

			// No virtual padding needed. Looks like a design decision.
			// Let's respect that, for now.
			// Same apply to MediaWriter and MediaExtractor.
			uint32				fReserved[5];
};


}; // namespace media
}; // namespace BPrivate

using namespace BPrivate::media;

#endif

/*
 * Copyright 2017, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_STREAMER_H
#define _MEDIA_STREAMER_H


#include <Streamer.h>
#include <Url.h>


namespace BCodecKit {


class BMediaStreamer {
public:
								BMediaStreamer(BUrl url);
								~BMediaStreamer();

			status_t			CreateAdapter(BDataIO** adapter);

private:
			BUrl				fUrl;
			BStreamer*			fStreamer;

			// No virtual padding needed. Looks like a design decision.
			// Let's respect that, for now.
			// Same apply to MediaWriter and MediaExtractor.
			uint32				fReserved[5];
};


} // namespace BCodecKit


#endif

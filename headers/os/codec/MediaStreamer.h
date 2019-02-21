/*
 * Copyright 2017, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_STREAMER_H
#define _MEDIA_STREAMER_H


#include <MediaIO.h>
#include <Streamer.h>
#include <Url.h>


namespace BCodecKit {


class BMediaStreamer {
public:
								BMediaStreamer(BUrl url);
								~BMediaStreamer();

			status_t			InitCheck() const;

			// TODO: So, it seems since this API was not public,
			// the memory ownership is leaved to class users.
			// See if this fits our plans, and eventually, move
			// this memory management from BMediaFile to BMediaExtractor,
			// BMediaWriter and BMediaStreamer.
			BMediaIO*			Adapter() const;

			// TODO: Don't we need an open in the extractor and writer?
			status_t			Open();
			void				Close();
			bool				IsOpened() const;

			//uint32			Capabilities() const;
			//status_t			GetMetaData(BMetaData* data) const;
			//status_t			SetHandler(BHandler* handler);
			//BHandler*			Handler() const;

			void				MouseMoved(uint32 x, uint32 y);
			void				MouseDown(uint32 x, uint32 y);

private:
			BUrl				fUrl;
			BStreamer*			fStreamer;
			status_t			fInitCheck;

			bool				fOpened;

			// No virtual padding needed. Looks like a design decision.
			// Let's respect that, for now.
			// Same apply to MediaWriter and MediaExtractor.
			uint32				fReserved[5];
};


} // namespace BCodecKit


#endif

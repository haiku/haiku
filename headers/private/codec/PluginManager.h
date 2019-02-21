/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Copyright 2012, Fredrik Modéen. All rights reserved.
 * Copyright 2004-2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H


#include <string.h>

#include <Decoder.h>
#include <Encoder.h>
#include <Locker.h>
#include <Reader.h>
#include <Streamer.h>
#include <Writer.h>

#include "TList.h"


namespace BCodecKit {

namespace BPrivate {


class PluginManager {
public:
								PluginManager();
								~PluginManager();
	
			BMediaPlugin*		GetPlugin(const entry_ref& ref);
			void				PutPlugin(BMediaPlugin* plugin);

	// Readers and Decoders
			status_t			CreateReader(BReader** reader,
									int32* streamCount, media_file_format* mff,
									BDataIO* source);
			void				DestroyReader(BReader* reader);

			status_t			CreateDecoder(BDecoder** decoder,
									const media_format& format);
			status_t			CreateDecoder(BDecoder** decoder,
									const media_codec_info& mci);
			status_t			GetDecoderInfo(BDecoder* decoder,
									media_codec_info* _info) const;
			void				DestroyDecoder(BDecoder* decoder);

	// Writers and Encoders	
			status_t			CreateWriter(BWriter** writer,
									const media_file_format& mff,
									BDataIO* target);
			void				DestroyWriter(BWriter* writer);

			status_t			CreateEncoder(BEncoder** encoder,
									const media_codec_info* codecInfo,
									uint32 flags);

			status_t			CreateEncoder(BEncoder** encoder,
									const media_format& format);

			void				DestroyEncoder(BEncoder* encoder);

			status_t			CreateStreamer(BStreamer** streamer,
									BUrl url);
			void				DestroyStreamer(BStreamer* streamer);

private:
			status_t			_LoadPlugin(const entry_ref& ref,
									BMediaPlugin** plugin, image_id* image);

			struct plugin_info {
				char			name[260];
				int				usecount;
				BMediaPlugin*	plugin;
				image_id		image;
		
				plugin_info& operator=(const plugin_info& other)
				{
					strcpy(name, other.name);
					usecount = other.usecount;
					plugin = other.plugin;
					image = other.image;
					return *this;
				}
			};

			List<plugin_info>	fPluginList;
			BLocker				fLocker;
};


} } // namespace BCodecKit::BPrivate


extern BCodecKit::BPrivate::PluginManager gPluginManager;


#endif // _PLUGIN_MANAGER_H

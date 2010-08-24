/* 
 * Copyright 2004-2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the OpenBeOS License.
 */
#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H


#include <string.h>


#include "DecoderPlugin.h"
#include "EncoderPlugin.h"
#include "ReaderPlugin.h"
#include "WriterPlugin.h"

#include <TList.h>
#include <Locker.h>


namespace BPrivate { namespace media {

class PluginManager {
public:
								PluginManager();
								~PluginManager();
	
			MediaPlugin*		GetPlugin(const entry_ref& ref);
			void				PutPlugin(MediaPlugin* plugin);

	// Readers and Decoders
			status_t			CreateReader(Reader** reader,
									int32* streamCount, media_file_format* mff,
									BDataIO* source);
			void				DestroyReader(Reader* reader);

			status_t			CreateDecoder(Decoder** decoder,
									const media_format& format);
			status_t			CreateDecoder(Decoder** decoder,
									const media_codec_info& mci);
			status_t			GetDecoderInfo(Decoder* decoder,
									media_codec_info* _info) const;
			void				DestroyDecoder(Decoder* decoder);

	// Writers and Encoders	
			status_t			CreateWriter(Writer** writer,
									const media_file_format& mff,
									BDataIO* target);
			void				DestroyWriter(Writer* writer);

			status_t			CreateEncoder(Encoder** encoder,
									const media_codec_info* codecInfo,
									uint32 flags);
			void				DestroyEncoder(Encoder* encoder);
	
private:
			status_t			_LoadPlugin(const entry_ref& ref,
									MediaPlugin** plugin, image_id* image);

			struct plugin_info {
				char			name[260];
				int				usecount;
				MediaPlugin*	plugin;
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

} } // namespace BPrivate::media

using namespace BPrivate::media;

extern PluginManager gPluginManager;

#endif // _PLUGIN_MANAGER_H

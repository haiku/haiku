#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H

#include "ReaderPlugin.h"
#include "DecoderPlugin.h"
#include <TList.h>
#include <Locker.h>

status_t _CreateReader(Reader **reader, int32 *streamCount, media_file_format *mff, BDataIO *source);
status_t _CreateDecoder(Decoder **decoder, media_codec_info *mci, const media_format *format);

void _DestroyReader(Reader *reader);
void _DestroyDecoder(Decoder *decoder);

status_t _PublishDecoder(DecoderPlugin *decoderplugin,
						 const char *meta_description,
						 const char *short_name,
						 const char *pretty_name,
						 const char *default_mapping /* = 0 */);

class PluginManager
{
public:
					PluginManager();
					~PluginManager();
	
	MediaPlugin *	GetPlugin(const char *name);
	void			PutPlugin(MediaPlugin *plugin);
	
private:
	bool			LoadPlugin(const char *name, MediaPlugin **plugin, image_id *image);

	struct plugin_info
	{
		char		name[260];
		int			usecount;
		MediaPlugin *plugin;
		image_id	image;
	};

	List<plugin_info> *fPluginList;
	BLocker 		*fLocker;
};

#endif

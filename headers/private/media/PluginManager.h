#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H

#include "ReaderPlugin.h"
#include "DecoderPlugin.h"
#include <TList.h>
#include <Locker.h>

status_t _CreateReader(Reader **reader, int32 *streamCount, media_file_format *mff, BDataIO *source);
status_t _CreateDecoder(Decoder **decoder, const media_format *format);

void _DestroyReader(Reader *reader);
void _DestroyDecoder(Decoder *decoder);


class PluginManager
{
public:
					PluginManager();
					~PluginManager();
	
	MediaPlugin *	GetPlugin(const entry_ref &ref);
	void			PutPlugin(MediaPlugin *plugin);
	
private:
	bool			LoadPlugin(const entry_ref &ref, MediaPlugin **plugin, image_id *image);

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

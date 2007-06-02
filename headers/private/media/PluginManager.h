#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H

#include "ReaderPlugin.h"
#include "DecoderPlugin.h"
#include <TList.h>
#include <Locker.h>


class PluginManager
{
public:
					PluginManager();
					~PluginManager();
	
	MediaPlugin *	GetPlugin(const entry_ref &ref);
	void			PutPlugin(MediaPlugin *plugin);


	status_t		CreateReader(Reader **reader, int32 *streamCount, media_file_format *mff, BDataIO *source);
	void			DestroyReader(Reader *reader);

	status_t		CreateDecoder(Decoder **decoder, const media_format &format);
	status_t		CreateDecoder(Decoder **decoder, const media_codec_info &mci);
	status_t		GetDecoderInfo(Decoder *decoder, media_codec_info *out_info) const;
	void			DestroyDecoder(Decoder *decoder);
	
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

extern PluginManager _plugin_manager;

#endif

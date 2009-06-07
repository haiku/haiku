#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H

#include "ReaderPlugin.h"
#include "DecoderPlugin.h"
#include <TList.h>
#include <Locker.h>


namespace BPrivate { namespace media {

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
	status_t		LoadPlugin(const entry_ref &ref, MediaPlugin **plugin, image_id *image);

	struct plugin_info
	{
		char		name[260];
		int			usecount;
		MediaPlugin *plugin;
		image_id	image;

		plugin_info& operator=(const plugin_info& other)
		{
			strcpy(name, other.name);
			usecount = other.usecount;
			plugin = other.plugin;
			image = other.image;
			return *this;
		}
	};

	List<plugin_info> *fPluginList;
	BLocker 		*fLocker;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

extern PluginManager _plugin_manager;

#endif

#ifndef _MEDIA_PLUGIN_H
#define _MEDIA_PLUGIN_H

#include <BeBuild.h>
#include <SupportDefs.h>

namespace BPrivate { namespace media {

class MediaPlugin
{
public:
						MediaPlugin();
	virtual				~MediaPlugin();
	
	virtual status_t	RegisterPlugin();
};

class Decoder;
class Reader;

} } // namespace BPrivate::media

using namespace BPrivate::media;

extern "C" MediaPlugin *instantiate_plugin();

#endif

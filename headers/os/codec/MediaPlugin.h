/* 
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _MEDIA_PLUGIN_H
#define _MEDIA_PLUGIN_H

#include <SupportDefs.h>

namespace BPrivate { namespace media {

class MediaPlugin {
public:
								MediaPlugin();
	virtual						~MediaPlugin();

private:
	// needed for plug-in reference count management
	friend class PluginManager;

			int32				fRefCount;
};

class Decoder;
class Reader;

} } // namespace BPrivate::media

using namespace BPrivate::media;

extern "C" MediaPlugin* instantiate_plugin();

#endif // _MEDIA_PLUGIN_H

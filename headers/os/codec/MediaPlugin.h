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
			int32				fRefCount;

	// needed for plug-in reference count management
	friend class PluginManager;

	virtual void				_ReservedMediaPlugin1();
	virtual void				_ReservedMediaPlugin2();

			uint32				fReserved[5];
};

class Decoder;
class Reader;

} } // namespace BPrivate::media

using namespace BPrivate::media;

extern "C" MediaPlugin* instantiate_plugin();

#endif // _MEDIA_PLUGIN_H

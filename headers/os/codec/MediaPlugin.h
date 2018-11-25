/* 
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _MEDIA_PLUGIN_H
#define _MEDIA_PLUGIN_H

#include <SupportDefs.h>

namespace BPrivate { namespace media {


// TODO: Shouldn't this be a BReferenceable?
// TODO: This will replace BMediaAddOn in media2,
// see if we need some more accessor method and
// add the needed padding.
class BMediaPlugin {
public:
								BMediaPlugin();
	virtual						~BMediaPlugin();

private:
			int32				fRefCount;

	// needed for plug-in reference count management
	friend class PluginManager;

	virtual void				_ReservedMediaPlugin1();
	virtual void				_ReservedMediaPlugin2();

			uint32				fReserved[5];
};

class BDecoder;
class BReader;

} } // namespace BPrivate::media

using namespace BPrivate::media;

extern "C" BMediaPlugin* instantiate_plugin();

#endif // _MEDIA_PLUGIN_H

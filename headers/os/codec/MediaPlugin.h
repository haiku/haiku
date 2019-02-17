/* 
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 * Copyright 2018, Dario Casalinuovo. All rights reserverd.
 * Distributed under the terms of the MIT license.
 */
#ifndef _MEDIA_PLUGIN_H
#define _MEDIA_PLUGIN_H


#include <SupportDefs.h>


namespace BCodecKit {

namespace BPrivate {
	class PluginManager;
}


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
	friend class BCodecKit::BPrivate::PluginManager;

	virtual void				_ReservedMediaPlugin1();
	virtual void				_ReservedMediaPlugin2();

			uint32				fReserved[5];
};


extern "C" BMediaPlugin* instantiate_plugin();
extern "C" uint32 get_plugin_version();
extern "C" const char* get_plugin_name();


#define B_CODEC_KIT_PLUGIN_VERSION 100

#define B_DECLARE_CODEC_KIT_PLUGIN(className, name, version)	\
extern "C" {													\
	BCodecKit::BMediaPlugin* instantiate_plugin()				\
	{															\
		return new(std::nothrow) className();					\
	}															\
																\
	uint32 get_plugin_version()									\
	{															\
		return version;											\
	}															\
																\
	const char* get_plugin_name()								\
	{															\
		return name;											\
	}															\
}


} // namespace BCodecKit


#endif // _MEDIA_PLUGIN_H

/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__BUFFER_CACHE_H_
#define _PACKAGE__HPKG__BUFFER_CACHE_H_


#include <stddef.h>


namespace BPackageKit {

namespace BHPKG {


namespace BPrivate {
	class CachedBuffer;
}
using BPrivate::CachedBuffer;


class BBufferCache {
public:
	virtual						~BBufferCache();

	virtual	CachedBuffer*		GetBuffer(size_t size,
									CachedBuffer** owner = NULL,
									bool* _newBuffer = NULL) = 0;
	virtual	void				PutBufferAndCache(CachedBuffer** owner) = 0;
									// caller is buffer owner and wants the
									// buffer cached, if possible
	virtual	void				PutBuffer(CachedBuffer** owner) = 0;
									// puts the buffer for good, owner might
									// have called PutBufferAndCache() before
									// and might not own a buffer anymore
};


class BBufferCacheLockable {
public:
	virtual						~BBufferCacheLockable();

	virtual	bool				Lock() = 0;
	virtual	void				Unlock() = 0;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__BUFFER_CACHE_H_
